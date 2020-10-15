//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <omnetpp.h>
#include "inet/common/ModuleAccess.h"
#include "WakeUpMacLayer.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/csmaca/CsmaCaMacHeader_m.h"
#include "inet/common/packet/chunk/Chunk.h"
#include "WakeUpGram_m.h"
using namespace inet;
using namespace physicallayer;

Define_Module(WakeUpMacLayer);


void WakeUpMacLayer::initialize(int stage) {
    MacProtocolBase::initialize(stage);
//    // Allow serialization to better respresent conflicting radio protocols
//    Chunk::enableImplicitChunkSerialization = true;
    if (stage == INITSTAGE_LOCAL) {
        //Register the Wake-up radio gates
        wakeUpRadioInGateId = findGate("wakeUpRadioIn");
        wakeUpRadioOutGateId = findGate("wakeUpRadioOut");

        //Create timer messages
        wakeUpBackoffTimer = new cMessage("wake-up backoff");
        ackBackoffTimer = new cMessage("ack backoff");
        wuTimeout = new cMessage("wake-up accept timeout");
        wuPacketPrototype = new cMessage("Wake-up");
        dataListeningDuration = par("dataListeningDuration");
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        ackWaitDuration = par("ackWaitDuration");
        wuApproveResponseLimit = par("wuApproveResponseLimit");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("dataRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        dataRadio = check_and_cast<IRadio *>(radioModule);

        radioModule = getModuleFromPar<cModule>(par("wakeUpRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

        updateMacState(S_IDLE);
        updateWuState(WU_IDLE);
        updateTxState(TX_IDLE);
        activeRadio = wakeUpRadio;
        transmissionState = activeRadio->getTransmissionState();
    }
}
void WakeUpMacLayer::changeActiveRadio(physicallayer::IRadio* newActiveRadio) {
    if(activeRadio->getId() != newActiveRadio->getId()){ // TODO: Check ID?
        activeRadio->setRadioMode(IRadio::RADIO_MODE_OFF);
        activeRadio = newActiveRadio;
        transmissionState = activeRadio->getTransmissionState();
        receptionState = activeRadio->getReceptionState();
    }
}


void WakeUpMacLayer::handleLowerPacket(Packet *packet) {
    // Process packet from the wake-up radio or delegate handler
    if (packet->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up packet" << endl;
        stepMacSM(EV_WU_START, packet);
    }
    else if(packet->getArrivalGateId() == lowerLayerInGateId){
        EV_DEBUG << "Received  main packet" << endl;
        stepMacSM(EV_DATA_RECEIVED, packet);
    }
    else{
        MacProtocolBase::handleLowerCommand(packet);
    }
}

void WakeUpMacLayer::handleLowerCommand(cMessage *msg) {
    // Process command from the wake-up radio or delegate handler
    if (msg->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up command" << endl;
    }
    else{
        MacProtocolBase::handleLowerCommand(msg);
    }
}

void WakeUpMacLayer::handleUpperPacket(Packet *packet) {
    // step Mac state machine
    // Make Mac owned copy of message
    packet->addTagIfAbsent<MacAddressReq>()->setDestAddress(MacAddress("aaaaaaaaaaaa"));
    auto macPkt = check_and_cast<Packet*>(packet);
    encapsulate(macPkt);
    stepMacSM(EV_QUEUE_SEND, macPkt);
}
void WakeUpMacLayer::handleUpperCommand(cMessage *msg) {
    // TODO: Check for approval messages
}

void WakeUpMacLayer::receiveSignal(cComponent *source, simsignal_t signalID,
        intval_t value, cObject *details) {
    Enter_Method_Silent();
    // Check it is for the active radio
    cComponent* activeRadioComponent = check_and_cast_nullable<cComponent*>(activeRadio);
    if (signalID == IRadio::transmissionStateChangedSignal) {
        if(activeRadioComponent && activeRadioComponent == source){
            // Handle both the data transmission ending and the wake-up transmission ending.
            // They should never happen at the same time, so one variable is enough
            // to manage both
            IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
            if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
                stepMacSM(EV_TX_END, new cMessage("Transmission over"));
            }
            else if (transmissionState == IRadio::TRANSMISSION_STATE_UNDEFINED && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // radio has finished switching to startup
                stepMacSM(EV_TX_READY, new cMessage("Transmitter Started"));
            }
            else {
                EV_DEBUG << "Unhandled transmitter state transition" << endl;
            }
            transmissionState = newRadioTransmissionState;
        }
    }
    else if (signalID == IRadio::receptionStateChangedSignal) {
        if(activeRadioComponent && activeRadioComponent == source){
            // Handle radio moving to receive mode
            IRadio::ReceptionState newRadioReceptionState = static_cast<IRadio::ReceptionState>(value);
            if (receptionState == IRadio::RECEPTION_STATE_UNDEFINED &&
                    (newRadioReceptionState == IRadio::RECEPTION_STATE_IDLE
                      || newRadioReceptionState == IRadio::RECEPTION_STATE_BUSY) ) {
                // radio has finished switching to listening
                stepMacSM(EV_DATA_RX_READY, new cMessage("Reception ready"));
            }
            else {
                EV_DEBUG << "Unhandled reception state transition" << endl;
            }
            receptionState = newRadioReceptionState;
        }
    }
    else if(signalID == IRadio::radioModeChangedSignal){
        if(activeRadioComponent && activeRadioComponent == source){
            // Handle radio switching into sleep mode.
            IRadio::RadioMode newRadioMode = static_cast<IRadio::RadioMode>(value);
            if (newRadioMode == IRadio::RADIO_MODE_SLEEP){
                stepMacSM(EV_DATA_RX_IDLE, new cMessage("Radio switched to sleep"));
            }
        }
    }
}

void WakeUpMacLayer::handleSelfMessage(cMessage *msg) {
    if(msg == wakeUpBackoffTimer){
        stepMacSM(EV_WAKEUP_BACKOFF, msg);
    }
    else if(msg == wuTimeout){
        stepMacSM(EV_WU_TIMEOUT, msg);
    }
    else if(msg->getKind() == WAKEUP_APPROVE){
        stepMacSM(EV_WU_APPROVE, msg);
    }
    else if(msg->getKind() == WAKEUP_REJECT){

        stepMacSM(EV_WU_REJECT, msg);
    }
    else{
        EV_DEBUG << "Unhandled self message" << endl;
    }
}

bool WakeUpMacLayer::isLowerMessage(cMessage *msg) {
    // Check if message comes from lower gate or wake-up radio
    return MacProtocolBase::isLowerMessage(msg)
              || msg->getArrivalGateId() == wakeUpRadioInGateId;
}

void WakeUpMacLayer::configureInterfaceEntry() {
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setPointToPoint(false);
}
void WakeUpMacLayer::queryWakeupRequest(cMessage *wakeUp) {
    // TODO: Query Opportunistic layer for permission to wake-up
    // For now just send immediate acceptance
    cMessage* msg = new cMessage("approve");
    msg->setKind(WAKEUP_APPROVE);
    scheduleAt(simTime(), msg);
}


void WakeUpMacLayer::stepMacSM(t_mac_event event, cMessage *msg) {
    switch (macState){
    case S_IDLE:
        if(event == EV_QUEUE_SEND){
            // Push into Queue
            // Check power level
            // Turn on wake up transmitter
            startImmediateTransmission(msg);
        }
        else if(event == EV_WU_START){
            // Start the wake-up state machine
            stepWuSM(event, msg);
            updateMacState(S_WAKEUP_LSN);
        }
        else{
            EV_WARN << "Wake-up MAC received unhandled event" << msg << endl;
        }
        break;
    case S_TRANSMIT:
        stepTxSM(event, msg);
        // TODO: Add transmission timeout to catch state locks
        break;
    case S_WAKEUP_LSN:
        // Process wake-up and wait for listening to start
        stepWuSM(event, msg);
        break;
    case S_RECEIVE:
        // Listen for a data packet after a wake-up and start timeout for ack
        if(event == EV_WU_TIMEOUT){
            // There was no subsequent data transmission
            EV_DEBUG << "Cancelled listening, no packet received" << endl;
            changeActiveRadio(wakeUpRadio);
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateMacState(S_IDLE);
        }
        else if(event == EV_DATA_RECEIVED){
            // TODO: Correctly received. Do Acknowledgements need to be made?
            Packet *pkt = dynamic_cast<Packet *>(msg);
            decapsulate(pkt);
            sendUp(pkt);
            changeActiveRadio(wakeUpRadio);
            cancelEvent(wuTimeout);
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateMacState(S_IDLE);
        }
        else{
            EV_WARN << "Unhandled event in receive state: " << msg << endl;
        }
        break;
    default:
        EV_WARN << "Wake-up MAC in unhandled state. Return to idle" << endl;
        updateMacState(S_IDLE);
    }
}
void WakeUpMacLayer::updateMacState(t_mac_state newMacState)
{
    macState = newMacState;
}

void WakeUpMacLayer::stepTxSM(t_mac_event event, cMessage *msg) {
    txStateChange = false;
    if(event == EV_TX_START){
        // Force state machine to start
        txState = TX_IDLE;
    }
    switch (txState){
    case TX_IDLE:
        if(event == EV_TX_START){
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            txPacketInProgress = msg;
            auto wuHeader = makeShared<WakeUpBeacon>();
            wuHeader->setType(WU_BEACON);
            wuHeader->setMinProgress(0xFFFF);
            wuHeader->setTransmitterAddress(MacAddress("aaaaaaaaaaaa"));
            wuHeader->setReceiverAddress(MacAddress("aaaaaaaaaaaa"));
            auto frame = new Packet("wake-up", wuHeader);
            frame->addTag<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
            wuPacketInProgress = check_and_cast<cMessage*>(frame);
            changeActiveRadio(wakeUpRadio);
            updateTxState(TX_WAKEUP_WAIT);
            EV_DEBUG << "TX SM: EV_TX_START --> TX_WAKEUP_WAIT";
        }
        break;
    case TX_WAKEUP_WAIT:
        if(event == EV_TX_READY){
            // TODO: Change this to a short WU packet
            send(wuPacketInProgress, wakeUpRadioOutGateId);
            updateTxState(TX_WAKEUP_WAIT);
            EV_DEBUG << "TX SM in TX_WAKEUP_WAIT";
        }
        else if(event == EV_TX_END){
            // Wake-up transmission has ended, start wait backoff for neighbors to wake-up
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateTxState(TX_DATA_WAIT);
            scheduleAt(simTime() + txWakeUpWaitDuration, wakeUpBackoffTimer);
            EV_DEBUG << "TX SM: TX_WAKEUP_WAIT --> TX_DATA_WAIT";
        }
        break;
    case TX_DATA_WAIT:
        if(event==EV_WAKEUP_BACKOFF){
            dataRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            updateTxState(TX_DATA);
        }
        break;
    case TX_DATA:
        if(event == EV_TX_READY){
            sendDown(txPacketInProgress);
        }
        else if(event == EV_TX_END){
            updateTxState(TX_END);
            scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
        }
        break;
    case TX_END:
        // End transmission by turning the radio off and start listening on wake-up radio
        changeActiveRadio(wakeUpRadio);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        updateMacState(S_IDLE);
        updateTxState(TX_IDLE);
        break;
    default:
        EV_DEBUG << "Unhandled TX State. Return to idle" << endl;
        updateTxState(TX_IDLE);
    }
    if(txStateChange == false){
        EV_WARN << "Unhandled event in tx state machine: " << msg << endl;
    }
}
void WakeUpMacLayer::updateTxState(t_tx_state newTxState)
{
    txStateChange = true;
    txState = newTxState;
}
bool WakeUpMacLayer::startImmediateTransmission(cMessage *msg) {
    // TODO: Check stored energy level, return false if too little energy
    //Start the transmission state machine
    updateMacState(S_TRANSMIT);
    stepTxSM(EV_TX_START, msg);
    return true;
}

void WakeUpMacLayer::handleStartOperation(LifecycleOperation *operation) {
}

void WakeUpMacLayer::handleStopOperation(LifecycleOperation *operation) {
    handleCrashOperation(operation);
}

WakeUpMacLayer::~WakeUpMacLayer() {
    // TODO: Cleanup allocated shared packets
//    delete wuPacketInProgress;
//    delete txPacketInProgress;
    cancelAndDelete(wakeUpBackoffTimer);
    cancelAndDelete(ackBackoffTimer);
    cancelAndDelete(wuTimeout);
}

void WakeUpMacLayer::encapsulate(Packet *pkt) { // From CsmaCaMac
    auto macHeader = makeShared<CsmaCaMacDataHeader>();
    macHeader->setChunkLength(B(headerLength));
    macHeader->setHeaderLengthField(B(headerLength).get());
    macHeader->setTransmitterAddress(interfaceEntry->getMacAddress());
    // TODO: Make Receiver a multicast address for progress
    macHeader->setReceiverAddress(pkt->getTag<MacAddressReq>()->getDestAddress());;
    pkt->insertAtFront(macHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
}

void WakeUpMacLayer::decapsulate(Packet *pkt) { // From CsmaCaMac
    auto macHeader = pkt->popAtFront<CsmaCaMacDataHeader>();
    auto addressInd = pkt->addTagIfAbsent<MacAddressInd>();
    addressInd->setSrcAddress(macHeader->getTransmitterAddress());
    addressInd->setDestAddress(macHeader->getReceiverAddress());
    pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
}

void WakeUpMacLayer::stepWuSM(t_mac_event event, cMessage *msg) {
    wuStateChange = false;
    switch(wuState){
    case WU_IDLE:
        if(event==EV_WU_START){
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateWuState(WU_APPROVE_WAIT);
            scheduleAt(simTime() + wuApproveResponseLimit, wuTimeout);
            queryWakeupRequest(msg);
        }
        break;
    case WU_APPROVE_WAIT:
        if(event==EV_WU_APPROVE){
            updateWuState(WU_WAKEUP_WAIT);
            cancelEvent(wuTimeout);
        }
        else if(event==EV_WU_TIMEOUT){
            // Upper layer did not approve wake-up in time.
            // Will abort the wake-up when radio mode switches
            updateWuState(WU_ABORT);
        }
        else if(event==EV_DATA_RX_IDLE||event==EV_DATA_RX_READY){
            //Stop the timer if other event called it first
            changeActiveRadio(wakeUpRadio);
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateWuState(WU_IDLE);
            updateMacState(S_IDLE);
        }
        break;
    case WU_WAKEUP_WAIT:
        if(event==EV_DATA_RX_IDLE){
            dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateWuState(WU_WAKEUP_WAIT);
        }
        if(event==EV_DATA_RX_READY){
            // data radio now listening
            scheduleAt(simTime() + dataListeningDuration, wuTimeout);
            updateWuState(WU_IDLE);
            updateMacState(S_RECEIVE);
        }
        break;
    case WU_ABORT:
        if(event==EV_DATA_RX_IDLE||event==EV_DATA_RX_READY){
            changeActiveRadio(wakeUpRadio);
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateWuState(WU_IDLE);
            updateMacState(S_IDLE);
        }
        break;
    default:
        EV_WARN << "Unhandled wake-up State. Return to Idle" << endl;
        updateWuState(WU_IDLE);
    }
    if(wuStateChange == false){
        EV_WARN << "Unhandled event in wake-up state machine: " << msg << endl;
    }
}

void WakeUpMacLayer::updateWuState(t_wu_state newWuState) {
    wuStateChange = true;
    wuState = newWuState;
}


void WakeUpMacLayer::handleCrashOperation(LifecycleOperation *operation) {
    EV_DEBUG << "Unimplemented crash" << endl;
}
