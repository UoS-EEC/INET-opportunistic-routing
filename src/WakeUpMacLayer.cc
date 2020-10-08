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
        wuPacketPrototype = new cMessage("Wake-up");
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        ackWaitDuration = par("ackWaitDuration");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("dataRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        dataRadio = check_and_cast<IRadio *>(radioModule);

        radioModule = getModuleFromPar<cModule>(par("wakeUpRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

        updateMacState(S_IDLE);
        activeRadio = wakeUpRadio;
        transmissionState = activeRadio->getTransmissionState();
    }
}
void WakeUpMacLayer::changeActiveRadio(physicallayer::IRadio* newActiveRadio) {
    if(activeRadio->getId() != newActiveRadio->getId()){ // TODO: Check ID?
        activeRadio->setRadioMode(IRadio::RADIO_MODE_OFF);
        activeRadio = newActiveRadio;
        transmissionState = activeRadio->getTransmissionState();
    }
}


void WakeUpMacLayer::handleLowerPacket(Packet *packet) {
    // Process packet from the wake-up radio or delegate handler
    if (packet->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up packet" << endl;
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

void WakeUpMacLayer::receiveSignal(cComponent *source, simsignal_t signalID,
        intval_t value, cObject *details) {
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        // Check it is for the active radio
        cComponent* activeRadioComponent = check_and_cast_nullable<cComponent*>(activeRadio);
        if(activeRadioComponent && activeRadioComponent == source){
            // Handle both the data transmission ending and the wake-up transmission ending.
            // They should never happen at the same time, so one variable is enough
            // to manage both
            IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
            if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
                stepMacSM(EV_TX_END, new cMessage("Transmission over"));
            }
            if (transmissionState == IRadio::TRANSMISSION_STATE_UNDEFINED && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // radio has finished switching to startup
                stepMacSM(EV_TX_READY, new cMessage("Transmitter Started"));
            }
            else {
                EV_DEBUG << "Unhandled transmitter state transition" << endl;
            }
            transmissionState = newRadioTransmissionState;
        }
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

void WakeUpMacLayer::handleSelfMessage(cMessage *msg) {
    if(msg == wakeUpBackoffTimer){
        stepMacSM(EV_WAKEUP_BACKOFF, msg);
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
        else{
            EV_DEBUG << "Wake-up MAC received unhandled event";
        }
        break;
    case S_TRANSMIT:
        stepTxSM(event, msg);
        // TODO: Add transmission timeout to catch state locks
        break;
    default:
        EV_DEBUG << "Wake-up MAC in unhandled state";
    }
}
void WakeUpMacLayer::updateMacState(t_mac_state newMacState)
{
    macState = newMacState;
}

void WakeUpMacLayer::stepTxSM(t_mac_event event, cMessage *msg) {
    if(event == EV_TX_START){
        txState = TX_IDLE;
    }
    switch (txState){
    case TX_IDLE:
        if(event == EV_TX_START){
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            txPacketInProgress = msg;
            auto wuHeader = makeShared<WakeUpGram>();
            wuHeader->setChunkLength(B(wuLength));
            wuHeader->setHeaderLengthField(B(wuLength).get());
            wuHeader->setType(WU_BEACON);
//            wuHeader->setProgress(0xFF);
            wuHeader->setTransmitterAddress(MacAddress("aaaaaaaaaaaa"));
            wuHeader->setReceiverAddress(MacAddress("aaaaaaaaaaaa"));
            auto frame = new Packet("wake-up");
            frame->insertAtFront(wuHeader);
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
        if(event == EV_TX_END){
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
        if(event == EV_TX_END){
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
        EV_DEBUG << "Unhandled TX State" << endl;
    }
}
void WakeUpMacLayer::updateTxState(t_tx_state newTxState)
{
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
}

WakeUpMacLayer::~WakeUpMacLayer() {
    // TODO: Cleanup allocated shared packets
//    delete wuPacketInProgress;
//    delete txPacketInProgress;
    cancelAndDelete(wakeUpBackoffTimer);
    cancelAndDelete(ackBackoffTimer);
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

void WakeUpMacLayer::handleCrashOperation(LifecycleOperation *operation) {
}
