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
#include <algorithm> // for min max
#include "inet/common/ModuleAccess.h"
#include "WakeUpMacLayer.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/packet/chunk/Chunk.h"
#include "OpportunisticRpl.h"
#include "WakeUpGram_m.h"
#include "ExpectedCostTag_m.h"
using namespace inet;
using namespace physicallayer;

Define_Module(WakeUpMacLayer);


void WakeUpMacLayer::initialize(int const stage) {
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
        dataListeningDuration = par("dataListeningDuration");
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        ackWaitDuration = par("ackWaitDuration");
        wuApproveResponseLimit = par("wuApproveResponseLimit");
        candiateRelayContentionProbability = par("candiateRelayContentionProbability");
        expectedCostJump = 2;

        cModule *radioModule = getModuleFromPar<cModule>(par("dataRadioModule"), this);
        dataRadio = check_and_cast<IRadio *>(radioModule);
        radioModule = getModuleFromPar<cModule>(par("wakeUpRadioModule"), this);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);
        cModule *module = getModuleFromPar<cModule>(par("routingModule"), this, false);
        routingModule = check_and_cast_nullable<OpportunisticRpl*>(module);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule* const dataCMod = check_and_cast<cModule*>(dataRadio);
        dataCMod->subscribe(IRadio::radioModeChangedSignal, this);
        dataCMod->subscribe(IRadio::transmissionStateChangedSignal, this);
        dataCMod->subscribe(IRadio::receptionStateChangedSignal, this);

        cModule* const wakeUpCMod = check_and_cast<cModule*>(wakeUpRadio);
        wakeUpCMod->subscribe(IRadio::radioModeChangedSignal, this);
        wakeUpCMod->subscribe(IRadio::transmissionStateChangedSignal, this);
        wakeUpCMod->subscribe(IRadio::receptionStateChangedSignal, this);

        // Initial state handled by handleStartOperation()

    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        // Find network layer with query wake-up request
        if (routingModule != nullptr){
            // Found routing module that implements OpportunisticRpl::queryAcceptPacket();

        }
        else{
            EV_WARN << "WuMac initialize(): No routing module with wake-up discernment provided" << endl;
        }
    }
}
void WakeUpMacLayer::changeActiveRadio(physicallayer::IRadio* const newActiveRadio) {
    if(activeRadio->getId() != newActiveRadio->getId()){ // TODO: Check ID?
        activeRadio->setRadioMode(IRadio::RADIO_MODE_OFF);
        activeRadio = newActiveRadio;
        transmissionState = activeRadio->getTransmissionState();
        receptionState = activeRadio->getReceptionState();
    }
}


void WakeUpMacLayer::handleLowerPacket(Packet* const packet) {
    // Process packet from the wake-up radio or delegate handler
    if (packet->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up packet" << endl;
        stepMacSM(EV_WU_START, packet);
        delete packet;
    }
    else if(packet->getArrivalGateId() == lowerLayerInGateId){
        EV_DEBUG << "Received  main packet" << endl;
        stepMacSM(EV_DATA_RECEIVED, packet);
    }
    else{
        MacProtocolBase::handleLowerCommand(packet);
    }
}

void WakeUpMacLayer::handleLowerCommand(cMessage* const msg) {
    // Process command from the wake-up radio or delegate handler
    if (msg->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up command" << endl;
    }
    else{
        MacProtocolBase::handleLowerCommand(msg);
    }
}

void WakeUpMacLayer::handleUpperPacket(Packet* const packet) {
    // step Mac state machine
    // Make Mac owned copy of message
    auto addressRequest =packet->addTagIfAbsent<MacAddressReq>();
    if(addressRequest->getDestAddress() == MacAddress::UNSPECIFIED_ADDRESS){
        addressRequest->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    }
    auto macPkt = check_and_cast<Packet*>(packet);
    encapsulate(macPkt);
    stepMacSM(EV_QUEUE_SEND, macPkt);
}
void WakeUpMacLayer::handleUpperCommand(cMessage* const msg) {
    // TODO: Check for approval messages
    EV_WARN << "Unhandled Upper Command" << endl;
}

void WakeUpMacLayer::receiveSignal(cComponent* const source, simsignal_t const signalID,
        intval_t const value, cObject* const details) {
    Enter_Method_Silent();
    // Check it is for the active radio
    cComponent* activeRadioComponent = check_and_cast_nullable<cComponent*>(activeRadio);
    if(activeRadioComponent && activeRadioComponent == source){
        if (signalID == IRadio::transmissionStateChangedSignal) {
            // Handle both the data transmission ending and the wake-up transmission ending.
            // They should never happen at the same time, so one variable is enough
            // to manage both
            IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
            if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
                auto msg = new cMessage("Transmission over");
                stepMacSM(EV_TX_END, msg);
                delete msg;
            }
            else if (transmissionState == IRadio::TRANSMISSION_STATE_UNDEFINED && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // radio has finished switching to startup
                // But handled by radioModeChanged due to signal order bugfix
            }
            else {
                EV_DEBUG << "Unhandled transmitter state transition" << endl;
            }
            transmissionState = newRadioTransmissionState;
        }
        else if (signalID == IRadio::receptionStateChangedSignal) {
            // Handle radio moving to receive mode
            IRadio::ReceptionState newRadioReceptionState = static_cast<IRadio::ReceptionState>(value);
            if (receptionState == IRadio::RECEPTION_STATE_UNDEFINED &&
                    (newRadioReceptionState == IRadio::RECEPTION_STATE_IDLE
                      || newRadioReceptionState == IRadio::RECEPTION_STATE_BUSY) ) {
                // radio has finished switching to listening
                auto msg = new cMessage("Reception ready");
                stepMacSM(EV_DATA_RX_READY, msg);
                delete msg;
            }
            else {
                EV_DEBUG << "Unhandled reception state transition" << endl;
            }
            receptionState = newRadioReceptionState;
        }
        else if(signalID == IRadio::radioModeChangedSignal){
            // Handle radio switching into sleep mode and into transmitter mode, since radio mode fired last for transmitter mode
            IRadio::RadioMode newRadioMode = static_cast<IRadio::RadioMode>(value);
            if (newRadioMode == IRadio::RADIO_MODE_SLEEP){
                auto msg = new cMessage("Radio switched to sleep");
                stepMacSM(EV_DATA_RX_IDLE, msg);
                delete msg;
            }
            else if(newRadioMode == IRadio::RADIO_MODE_TRANSMITTER){
                auto msg = new cMessage("Transmitter Started");
                stepMacSM(EV_TX_READY, msg);
                delete msg;
            }
            receptionState = IRadio::RECEPTION_STATE_UNDEFINED;
            transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;
        }
    }
}

void WakeUpMacLayer::handleSelfMessage(cMessage* const msg) {
    if(msg == wakeUpBackoffTimer){
        stepMacSM(EV_WAKEUP_BACKOFF, msg);
    }
    else if(msg == wuTimeout){
        stepMacSM(EV_WU_TIMEOUT, msg);
    }
    else if(msg == ackBackoffTimer){
        stepMacSM(EV_ACK_TIMEOUT, msg);
    }
    else if(msg->getKind() == WAKEUP_APPROVE){
        stepMacSM(EV_WU_APPROVE, msg);
        delete msg;
    }
    else if(msg->getKind() == WAKEUP_REJECT){
        stepMacSM(EV_WU_REJECT, msg);
        delete msg;
    }
    else{
        EV_DEBUG << "Unhandled self message" << endl;
    }
}

bool WakeUpMacLayer::isLowerMessage(cMessage* const msg) {
    // Check if message comes from lower gate or wake-up radio
    return MacProtocolBase::isLowerMessage(msg)
              || msg->getArrivalGateId() == wakeUpRadioInGateId;
}

void WakeUpMacLayer::configureInterfaceEntry() {
    // generate a link-layer address to be used as interface token for IPv6
    interfaceEntry->setMtu(255-16);
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setPointToPoint(false);
}
void WakeUpMacLayer::queryWakeupRequest(const Packet* wakeUp) {
    // For now just send immediate acceptance
    // TODO: Check if receiver mac address is this node
    auto header = wakeUp->peekAtFront<WakeUpGram>();
    bool approve = false;
    if(header->getType()!=WakeUpGramType::WU_BEACON){
        approve = false;
    }
    else if(header->getReceiverAddress() == interfaceEntry->getMacAddress()){
        approve = true;
    }
    else if(header->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS){
        approve = true;
    }
    else if(routingModule != nullptr && header->getType()==WakeUpGramType::WU_BEACON){
        auto costHeader = wakeUp->peekAtFront<WakeUpBeacon>();
        // TODO: Query Opportunistic layer for permission to wake-up
        if(routingModule->queryAcceptPacket(header->getReceiverAddress(), costHeader->getMinExpectedCost())){
            approve = true;
        }
    }

    if(approve == true){
        // Approve wake-up request
        cMessage* msg = new cMessage("approve");
        msg->setKind(WAKEUP_APPROVE);
        scheduleAt(simTime(), msg);
    }
}


void WakeUpMacLayer::stepMacSM(const t_mac_event& event, cMessage * const msg) {
    if(event == EV_DATA_RECEIVED){
        // TODO: Update neighbor tables
    }
    // Operate State machine based on current state and event
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
        if(event == EV_WU_TIMEOUT||event == EV_DATA_RECEIVED){
            stepRxAckProcess(event, msg);
        }
        else{
            EV_WARN << "Unhandled event in receive state: " << msg << endl;
        }
        break;
    case S_ACK:
        stepRxAckProcess(event, msg);
        break;
    default:
        EV_WARN << "Wake-up MAC in unhandled state. Return to idle" << endl;
        updateMacState(S_IDLE);
    }
}

void WakeUpMacLayer::completePacketReception()
{
    // The receiving has timed out, if packet is received process
    if (currentRxFrame != nullptr) {
        Packet* pkt = dynamic_cast<Packet*>(currentRxFrame);
        decapsulate(pkt);
        sendUp(pkt);
        currentRxFrame = nullptr;
    }
}

// Receive acknowledgement process that can be overridden
void WakeUpMacLayer::stepRxAckProcess(const t_mac_event& event, cMessage * const msg) {
    if(event == EV_DATA_RECEIVED){
        handleDataReceivedInAckState(msg);
    }
    else if(event == EV_TX_READY){
        // send acknowledgement packet when radio is ready
        sendDown(buildAck(check_and_cast<Packet*>(currentRxFrame)));
        updateMacState(S_ACK);
    }
    else if(event == EV_TX_END){
        // return to receive mode (via receive wait) when ack transmitted
        // For follow up packet
        updateMacState(S_WAKEUP_LSN);
        updateWuState(WU_WAKEUP_WAIT);
        cancelEvent(wuTimeout);
        dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    }
    else if(event == EV_ACK_TIMEOUT){
        // Calculate backoff from remaining time
        simtime_t maxDelay = (ackWaitDuration - cumulativeAckBackoff)/2;
        if(maxDelay > simtime_t::ZERO){
            // Add expected random backoff to cumulative total
            cumulativeAckBackoff += maxDelay;
            setRadioToTransmitIfFreeOrDelay(ackBackoffTimer, maxDelay);
            updateMacState(S_ACK);
        }
        else{
            // Give up because ack period too long
            // Drop packet
            // Return to idle listening
            changeActiveRadio(wakeUpRadio);
            cancelEvent(wuTimeout);
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateMacState(S_IDLE);
            PacketDropDetails details;
            EV_WARN << "Dropping packet because of channel congestion";
            details.setReason(PacketDropReason::INCORRECTLY_RECEIVED);
            dropCurrentRxFrame(details);
        }
    }
    else if(event == EV_WU_TIMEOUT){
        // The receiving has timed out, if packet is received process
        completePacketReception();
        // Return to idle listening
        changeActiveRadio(wakeUpRadio);
        cancelEvent(wuTimeout);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        updateMacState(S_IDLE);
    }
}

void WakeUpMacLayer::handleDataReceivedInAckState(cMessage * const msg) {
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<WakeUpGram>();
    // TODO: Update neighbor table
    if(incomingMacData->getType()==WU_DATA && currentRxFrame == nullptr){
        updateMacState(S_ACK);
        // Store the new received packet
        currentRxFrame = incomingFrame;
        // Start ack backoff
        cancelEvent(ackBackoffTimer);
        // Reset cumulative ack backoff
        cumulativeAckBackoff = uniform(0,ackWaitDuration/3);
        scheduleAt(simTime() + cumulativeAckBackoff, ackBackoffTimer);
    }
    else if(incomingMacData->getType()==WU_DATA/* && rxPacketInProgress != nullptr*/){
        updateMacState(S_ACK);
        // Compare the received data to stored data
        Packet* storedFrame = check_and_cast<Packet*>(currentRxFrame);
        auto storedMacData = storedFrame->peekAtFront<WakeUpGram>();
        if(storedMacData->getTransmitterAddress() == incomingMacData->getTransmitterAddress()){
            // Enough to say packet matches, meaning retransmission due to forwarder contention
            // TODO: Cancel own ack if new data has expected cost of zero
            // Begin random relay contention

            // Cancel delivery timer until next round
            cancelEvent(wuTimeout);
            // Cancel queued ack if existing
            cancelEvent(ackBackoffTimer);
            // Reset cumulative ack backoff
            cumulativeAckBackoff = uniform(0,ackWaitDuration);

            // Start CCA Timer to send Ack
            double relayDiceRoll = uniform(0,1);
            if(relayDiceRoll<candiateRelayContentionProbability){
                scheduleAt(simTime() + cumulativeAckBackoff, ackBackoffTimer);
            }
            else{
                PacketDropDetails details;
                details.setReason(PacketDropReason::DUPLICATE_DETECTED);
                dropCurrentRxFrame(details);
                // Send immediate wuTimeout to trigger EV_WU_TIMEOUT
                scheduleAt(simTime(), wuTimeout);
                EV_DEBUG  << "Detected other relay so discarding packet" << endl;
            }
            // Delete retransmitted message
            delete incomingFrame;
        }
        else{
            EV_DEBUG << "Discard interfering data transmission" << endl;
            // Delete unknown message
            delete incomingFrame;
        }
    }
    else if(incomingMacData->getType()==WU_ACK){
        // Overheard Ack from neighbor
        EV_WARN << "Overheard Ack from neighbor is it worth sending own ACK?" << endl;
        // Leave in current mac state which could be S_RECEIVE or S_ACK
        delete incomingFrame;
    }
    else{
        updateMacState(S_RECEIVE);
    }
}

void WakeUpMacLayer::setRadioToTransmitIfFreeOrDelay(cMessage* const timer,
        const simtime_t& maxDelay) {
    // Check medium is free, or schedule tiny timer
    IRadio::ReceptionState receptionState = dataRadio->getReceptionState();
    bool isIdle = receptionState == IRadio::RECEPTION_STATE_IDLE
            || receptionState == IRadio::RECEPTION_STATE_BUSY;
    if (isIdle) {
        // Switch to transmit, send ack then
        dataRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    }
    else{
        // Reschedule backoff timer with shorter backoff
        cancelEvent(timer);
        scheduleAt(simTime() + uniform(0,maxDelay), timer);
    }
}
Packet* WakeUpMacLayer::buildAck(const Packet* receivedFrame) const{
    auto receivedMacData = receivedFrame->peekAtFront<WakeUpGram>();
    auto ackPacket = makeShared<WakeUpAck>();
    ackPacket->setTransmitterAddress(interfaceEntry->getMacAddress());
    ackPacket->setReceiverAddress(receivedMacData->getTransmitterAddress());
    auto frame = new Packet("CsmaAck");
    frame->insertAtFront(ackPacket);
    frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
    return frame;
}
void WakeUpMacLayer::updateMacState(const t_mac_state& newMacState)
{
    macState = newMacState;
}

void WakeUpMacLayer::stepTxSM(const t_mac_event& event, cMessage* const msg) {
    txStateChange = false;
    if(event == EV_TX_START){
        // Force state machine to start
        txState = TX_IDLE;
        if(currentTxFrame!=nullptr){
            PacketDropDetails details;
            details.setReason(PacketDropReason::QUEUE_OVERFLOW);
            dropCurrentTxFrame(details);
        }
        // TODO: Replace with popTxQueue()
        currentTxFrame = check_and_cast<Packet*>(msg);
    }
    switch (txState){
    case TX_IDLE:
        if(event == EV_TX_START || event == EV_ACK_TIMEOUT){
            wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            txInProgressRetries++;
            changeActiveRadio(wakeUpRadio);
            updateTxState(TX_WAKEUP_WAIT);
            EV_DEBUG << "TX SM: EV_TX_START --> TX_WAKEUP_WAIT";
        }
        break;
    case TX_WAKEUP_WAIT:
        if(event == EV_TX_READY){
            // TODO: Change this to a short WU packet
            cMessage* const currentTxWakeUp = check_and_cast<cMessage*>(buildWakeUp(currentTxFrame, txInProgressRetries));
            send(currentTxWakeUp, wakeUpRadioOutGateId);
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
            // Reuse wakeup backoff for carrier sense backoff
            dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateTxState(TX_DATA);
        }
        break;
    case TX_DATA:
        if(event == EV_DATA_RX_READY || event == EV_WAKEUP_BACKOFF){
            setRadioToTransmitIfFreeOrDelay(wakeUpBackoffTimer, ackWaitDuration/5);
            updateTxState(TX_DATA);
        }
        else if(event == EV_TX_READY){
            sendDown(currentTxFrame->dup());
            updateTxState(TX_DATA);
        }
        else if(event == EV_TX_END){
            stepTxAckProcess(EV_TX_END, msg);
        }
        break;
    case TX_ACK_WAIT:
        stepTxAckProcess(event, msg);
        break;
    case TX_END:
        // End transmission by turning the radio off and start listening on wake-up radio
        changeActiveRadio(wakeUpRadio);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        updateMacState(S_IDLE);
        updateTxState(TX_IDLE);
        if(txInProgressForwarders<1){
            PacketDropDetails details;
            // This reason could also justifiably be LIFETIME_EXPIRED
            details.setReason(PacketDropReason::NO_ROUTE_FOUND);
            dropCurrentTxFrame(details);
        }
        else deleteCurrentTxFrame();
        break;
    default:
        EV_DEBUG << "Unhandled TX State. Return to idle" << endl;
        updateTxState(TX_IDLE);
    }
    if(txStateChange == false){
        EV_WARN << "Unhandled event in tx state machine: " << msg << endl;
    }
}
Packet* WakeUpMacLayer::buildWakeUp(const Packet *subject, const int retryCount) const{
    auto expectedCostTag = subject->findTag<ExpectedCostReq>();
    int minExpectedCost = 0xFFFF;
    const int expJumpFloor = std::floor(expectedCostJump);
    const int expJumpBern = expJumpFloor + bernoulli(expectedCostJump - expJumpFloor);
    if(expectedCostTag != nullptr){
        minExpectedCost = expectedCostTag->getExpectedCost();
        minExpectedCost = minExpectedCost - std::max(0, expJumpBern - retryCount);
        minExpectedCost = std::max(0, minExpectedCost);
    }
    auto wuHeader = makeShared<WakeUpBeacon>();
    wuHeader->setType(WU_BEACON);
    wuHeader->setMinExpectedCost(minExpectedCost);
    wuHeader->setTransmitterAddress(interfaceEntry->getMacAddress());
    wuHeader->setReceiverAddress(subject->peekAtFront<WakeUpDatagram>()->getReceiverAddress());
    auto frame = new Packet("wake-up", wuHeader);
    frame->addTag<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
    return frame;
}
void WakeUpMacLayer::stepTxAckProcess(const t_mac_event& event, cMessage * const msg) {
    if(event == EV_TX_END){
        //reset confirmed forwarders count
        acknowledgedForwarders = 0;
        // Schedule acknowledgement timeout
        updateMacState(S_TRANSMIT);
        updateTxState(TX_ACK_WAIT);
        dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        //TODO: Better calculation of wait time including ack length
        scheduleAt(simTime() + ackWaitDuration*2, ackBackoffTimer);
    }
    else if(event == EV_DATA_RECEIVED){
        EV_DEBUG << "Data Ack Received";
        auto receivedData = check_and_cast<Packet* >(msg);
        auto receivedAck = receivedData->popAtFront<WakeUpGram>();
        updateMacState(S_TRANSMIT);
        if(receivedAck->getType() == WU_ACK){
            // TODO: Update neighbors and check source and dest address match
            // Reset ackBackoffTimer
            cancelEvent(ackBackoffTimer);
            // count first few ack
            acknowledgedForwarders++;
            if(acknowledgedForwarders>4){
                // Skip listening for any more and send data again to reduce forwarders
                updateTxState(TX_DATA);
                scheduleAt(simTime(), wakeUpBackoffTimer);
            }
            else{
                updateTxState(TX_ACK_WAIT);
                //TODO: Better calculation of wait time including ack length
                scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
            }
            delete receivedData;
        }
        else{
            updateTxState(TX_ACK_WAIT);
            EV_DEBUG <<  "Discarding overheard data as busy transmitting" << endl;
            delete receivedData;
        }
    }
    else if(event == EV_ACK_TIMEOUT){
        // TODO: Get required forwarders count from packetTag from n/w layer
        int requiredForwarders = 1;
        updateMacState(S_TRANSMIT);
        // TODO: Test this with more nodes should this include forwarders from prev timeslot?
        updateMacState(S_TRANSMIT);
        const int supplementaryForwarders = acknowledgedForwarders - requiredForwarders;
        if(supplementaryForwarders>0){
            // Go straight to immediate data retransmission to reduce forwarders
            updateTxState(TX_DATA);
            scheduleAt(simTime(), wakeUpBackoffTimer);
            //TODO: stabilize and parameterize this control of expected cost jump
            expectedCostJump += supplementaryForwarders*0.2; // Grow slower
            expectedCostJump = std::max(0.0, expectedCostJump);
        }
        else{
            txInProgressForwarders = txInProgressForwarders+acknowledgedForwarders; // TODO: Check forwarders uniqueness
            if(txInProgressForwarders<requiredForwarders && txInProgressRetries<maxWakeUpRetries){
                // Reduce expected cost jump to find more forwarders
                const int retriesOver1 = std::max(1, txInProgressRetries) - 1;
                expectedCostJump -= retriesOver1 * 0.3; // Shrink faster
                expectedCostJump = std::max(0.0, expectedCostJump);
                // Try transmitting wake-up again after standard ack backoff
                scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
                updateTxState(TX_IDLE);
            }
            else{
                updateTxState(TX_END);
                //The Radio Receive->Sleep triggers next SM transition
                dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            }
        }
    }
}

void WakeUpMacLayer::updateTxState(const t_tx_state& newTxState)
{
    txStateChange = true;
    txState = newTxState;
}
bool WakeUpMacLayer::startImmediateTransmission(cMessage* const msg) {
    // TODO: Check stored energy level, return false if too little energy
    //Cancel transmission timers
    cancelEvent(wakeUpBackoffTimer);
    cancelEvent(ackBackoffTimer);
    cancelEvent(wuTimeout);
    //Reset progress counters
    txInProgressForwarders = 0;
    txInProgressRetries = 0;
    //Start the transmission state machine
    updateMacState(S_TRANSMIT);
    stepTxSM(EV_TX_START, msg);
    return true;
}

void WakeUpMacLayer::handleStartOperation(LifecycleOperation *operation) {
    // complete unfinished reception
    completePacketReception();
    // Unfinished transmission so restart that transmission
    if(currentTxFrame){
        updateMacState(S_TRANSMIT);
        updateTxState(TX_IDLE);
        auto msg = new cMessage("Ack timeout from restart");
        stepMacSM(EV_ACK_TIMEOUT, msg);
        delete msg;
    }
    else{
        // TODO: Move to function shared with initialize
        updateMacState(S_IDLE);
        updateWuState(WU_IDLE);
        updateTxState(TX_IDLE);
        activeRadio = wakeUpRadio;
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        transmissionState = activeRadio->getTransmissionState();
        receptionState = activeRadio->getReceptionState();
    }
    interfaceEntry->setState(InterfaceEntry::State::UP);
    interfaceEntry->setCarrier(true);
}

void WakeUpMacLayer::handleStopOperation(LifecycleOperation *operation) {
    handleCrashOperation(operation);
}

WakeUpMacLayer::~WakeUpMacLayer() {
    // TODO: Move this to the finish function
    // TODO: Cleanup allocated shared packets
    cancelAllTimers();
    deleteAllTimers();
}

void WakeUpMacLayer::encapsulate(Packet* const pkt) { // From CsmaCaMac
    auto macHeader = makeShared<WakeUpDatagram>();
    macHeader->setTransmitterAddress(interfaceEntry->getMacAddress());
    // TODO: Make Receiver a multicast address for progress
    macHeader->setReceiverAddress(pkt->getTag<MacAddressReq>()->getDestAddress());;
    pkt->insertAtFront(macHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
}

void WakeUpMacLayer::decapsulate(Packet* const pkt) { // From CsmaCaMac
    auto macHeader = pkt->popAtFront<WakeUpDatagram>();
    auto addressInd = pkt->addTagIfAbsent<MacAddressInd>();
    addressInd->setSrcAddress(macHeader->getTransmitterAddress());
    addressInd->setDestAddress(macHeader->getReceiverAddress());
    auto payloadProtocol = ProtocolGroup::ipprotocol.getProtocol(245);
    pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
}

void WakeUpMacLayer::stepWuSM(const t_mac_event& event, cMessage * const msg) {
    wuStateChange = false;
    switch(wuState){
    case WU_IDLE:
        if(event==EV_WU_START){
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateWuState(WU_APPROVE_WAIT);
            scheduleAt(simTime() + wuApproveResponseLimit, wuTimeout);
            queryWakeupRequest(check_and_cast<Packet*>(msg));
        }
        break;
    case WU_APPROVE_WAIT:
        if(event==EV_WU_APPROVE){
            updateMacState(S_WAKEUP_LSN);
            updateWuState(WU_WAKEUP_WAIT);
            cancelEvent(wuTimeout);
        }
        else if(event==EV_WU_TIMEOUT||event==EV_WU_REJECT){
            // Upper layer did not approve wake-up in time.
            // Will abort the wake-up when radio mode switches
            updateMacState(S_WAKEUP_LSN);
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
            updateMacState(S_WAKEUP_LSN);
            updateWuState(WU_WAKEUP_WAIT);
        }
        else if(event==EV_DATA_RX_READY){
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
        else if(event==EV_WU_TIMEOUT||event==EV_WU_REJECT){
            // Already canceled the wake-up still waiting for radio mode switch
            updateMacState(S_WAKEUP_LSN);
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

void WakeUpMacLayer::updateWuState(const t_wu_state& newWuState) {
    wuStateChange = true;
    wuState = newWuState;
}

void WakeUpMacLayer::cancelAllTimers()
{
    cancelEvent(wakeUpBackoffTimer);
    cancelEvent(ackBackoffTimer);
    cancelEvent(wuTimeout);
}

void WakeUpMacLayer::deleteAllTimers(){
    delete wakeUpBackoffTimer;
    delete ackBackoffTimer;
    delete wuTimeout;
}

void WakeUpMacLayer::dropCurrentRxFrame(PacketDropDetails& details)
{
    emit(packetDroppedSignal, currentRxFrame, &details);
    delete currentRxFrame;
    currentRxFrame = nullptr;
}

void WakeUpMacLayer::handleCrashOperation(LifecycleOperation* const operation) {
    if(currentTxFrame != nullptr){
        if(txInProgressForwarders>0){
            PacketDropDetails details;
            details.setReason(INTERFACE_DOWN);
            // Packet has been received by a forwarder
            dropCurrentTxFrame(details);
        }
    }
    else if(currentRxFrame != nullptr){
        // Check if in ack backoff period and if waiting to send ack
        if(wuTimeout->isScheduled() && ackBackoffTimer->isScheduled()){
            // Ack not sent yet so just bow out
            delete currentRxFrame;
            currentRxFrame = nullptr;
        }// TODO: check how many contending acks there were
        else{
            // Send packet up upon restart by leaving in memory
        }
    }
    cancelAllTimers();
    // Stop all signals from being interpreted
    updateMacState(S_IDLE);
    interfaceEntry->setCarrier(false);
    interfaceEntry->setState(InterfaceEntry::State::DOWN);
}
