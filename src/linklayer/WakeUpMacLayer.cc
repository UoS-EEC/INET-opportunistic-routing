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

#include "WakeUpMacLayer.h"
#include "WuMacEnergyMonitor.h"

#include <omnetpp.h>
#include <algorithm> // for min max
#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/common/packet/chunk/Chunk.h>
#include <inet/physicallayer/base/packetlevel/FlatReceiverBase.h>

#include "../networklayer/ORWRouting.h"
#include "common/EqDCTag_m.h"
#include "common/EncounterDetails_m.h"
#include "ORWGram_m.h"

using namespace inet;
using physicallayer::IRadio;
using namespace oppostack;

Define_Module(WakeUpMacLayer);

void WakeUpMacLayer::initialize(int const stage) {
    ORWMac::initialize(stage);
//    // Allow serialization to better represent conflicting radio protocols
//    Chunk::enableImplicitChunkSerialization = true;
    if (stage == INITSTAGE_LOCAL) {
        //Register the Wake-up radio gates
        wakeUpRadioInGateId = findGate("wakeUpRadioIn");
        wakeUpRadioOutGateId = findGate("wakeUpRadioOut");

        //Create timer messages
        transmitStartDelay = new cMessage("transmit backoff");
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        wuApproveResponseLimit = par("wuApproveResponseLimit");

        maxTxTries = par("maxTxTries");

        const char* wakeUpRadioModulePath = par("wakeUpRadioModule");
        cModule *radioModule = getModuleByPath(wakeUpRadioModulePath);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);

        const char* networkNodePath = par("networkNode");
        networkNode = getModuleByPath(networkNodePath);

        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));

        // Retransmission reduction through data packet updating
        recheckDataPacketEqDC = par("recheckDataPacketEqDC");
        skipDirectTxFinalAck = recheckDataPacketEqDC && par("skipDirectTxFinalAck");

        // Validation
        auto dataReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(dataRadio->getReceiver());
        auto wakeUpReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(wakeUpRadio->getReceiver());
        if(dataReceiverModel && wakeUpReceiverModel)
            if(wakeUpReceiverModel->getEnergyDetection() > wakeUpReceiverModel->getEnergyDetection())
                throw cRuntimeError("Wake-up radio is more sensitive than the data radio. There may be background noise issues");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // Register signals for handling radio mode changes
        cModule* const wakeUpCMod = check_and_cast<cModule*>(wakeUpRadio);
        wakeUpCMod->subscribe(IRadio::radioModeChangedSignal, this);
        wakeUpCMod->subscribe(IRadio::transmissionStateChangedSignal, this);
        wakeUpCMod->subscribe(IRadio::receptionStateChangedSignal, this);
    }
}

void WakeUpMacLayer::handleLowerPacket(Packet* const packet) {
    // Process packet from the wake-up radio or delegate handler
    if (packet->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up packet" << endl;
        stateProcess(MacEvent::WU_START, packet);
        delete packet;
    }
    else if(packet->getArrivalGateId() == lowerLayerInGateId){
        EV_DEBUG << "Received  main packet" << endl;
        stateProcess(MacEvent::DATA_RECEIVED, packet);
    }
    else{
        ORWMac::handleLowerCommand(packet);
    }
}

void WakeUpMacLayer::handleUpperPacket(Packet* const packet) {
    // step Mac state machine
    // Make Mac owned copy of message
    auto addressRequest =packet->addTagIfAbsent<MacAddressReq>();
    if(addressRequest->getDestAddress() == MacAddress::UNSPECIFIED_ADDRESS){
        addressRequest->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    }
    txQueue->pushPacket(packet);
    stateProcess(MacEvent::QUEUE_SEND, packet);
}

void WakeUpMacLayer::receiveSignal(cComponent* const source, simsignal_t const signalID,
        intval_t const value, cObject* const details) {
    Enter_Method_Silent();
    // Check it is for the active radio
    cComponent* activeRadioComponent = check_and_cast_nullable<cComponent*>(activeRadio);
    if(operationalState == OPERATING && activeRadioComponent && activeRadioComponent == source){
        if (signalID == IRadio::transmissionStateChangedSignal) {
            // Handle both the data transmission ending and the wake-up transmission ending.
            // They should never happen at the same time, so one variable is enough
            // to manage both
            IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
            if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
                // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
                auto msg = new cMessage("Transmission over");
                stateProcess(MacEvent::TX_END, msg);
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
                stateProcess(MacEvent::DATA_RX_READY, msg);
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
                stateProcess(MacEvent::DATA_RX_IDLE, msg);
                delete msg;
            }
            else if(newRadioMode == IRadio::RADIO_MODE_TRANSMITTER){
                auto msg = new cMessage("Transmitter Started");
                stateProcess(MacEvent::TX_READY, msg);
                delete msg;
            }
            receptionState = IRadio::RECEPTION_STATE_UNDEFINED;
            transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;
        }
    }
}

void WakeUpMacLayer::handleSelfMessage(cMessage* const msg) {
    if(msg == transmitStartDelay){
        stateProcess(MacEvent::TX_START, msg);
    }
    else if(msg == receiveTimeout){
        stateProcess(MacEvent::DATA_TIMEOUT, msg);
    }
    else if(msg == ackBackoffTimer){
        stateProcess(MacEvent::ACK_TIMEOUT, msg);
    }
    else if(msg == replenishmentTimer){
        stateProcess(MacEvent::REPLENISH_TIMEOUT, msg);
    }
    else if( activeBackoff && activeBackoff->isBackoffTimer(msg) ){
        stateProcess(MacEvent::CSMA_BACKOFF, msg);
    }
    else if(msg->getKind() == WAKEUP_APPROVE){
        stateProcess(MacEvent::WU_APPROVE, msg);
        delete msg;
    }
    else if(msg->getKind() == WAKEUP_REJECT){
        stateProcess(MacEvent::WU_REJECT, msg);
        delete msg;
    }
    else{
        EV_DEBUG << "Unhandled self message" << endl;
    }
}

void WakeUpMacLayer::stateProcess(const MacEvent& event, cMessage * const msg) {
    // Operate State machine based on current state and event
    switch (macState){
    case State::WAKE_UP_IDLE:
        macState = stateWakeUpIdleProcess(event, msg);
        break;
    case State::AWAIT_TRANSMIT:
        ASSERT(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER
                || activeRadio->getRadioMode() == IRadio::RADIO_MODE_SWITCHING);
        macState = stateAwaitTransmitProcess(event, msg);
        break;
    case State::TRANSMIT:
        if( stateTxProcess(event, msg) ){
            // State transmit is therefore finished, enter listening
            macState = stateListeningEnter();
        }
        break;
    case State::WAKE_UP_WAIT:
        // Process wake-up and wait for listening to start
        macState = stateWakeUpProcess(event, msg);
        break;
    case State::RECEIVE:
        // Listen for a data packet after a wake-up and start timeout for ack
        if( stateReceiveProcess(event, msg) ){
            // State receive is therefore finished, enter listening
            macState = stateListeningEnter();
        }
        break;
    default:
        EV_WARN << "Wake-up MAC in unhandled state. Return to idle" << endl;
        macState = State::WAKE_UP_IDLE;
    }
}

WakeUpMacLayer::State WakeUpMacLayer::stateListeningEnter(){
    wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    changeActiveRadio(wakeUpRadio);
    State s;
    // Override transition to data idle with transitio to wake-up idle
    if( (s = stateListeningEnterAlreadyListening()) != State::DATA_IDLE){
        return s;
    }
    return State::WAKE_UP_IDLE;
}

WakeUpMacLayer::State WakeUpMacLayer::stateTxEnter()
{
    dataMinExpectedCost = EqDC(25.5);
    txDataState = TxDataState::WAKE_UP_WAIT;
    return State::TRANSMIT;
}

WakeUpMacLayer::State WakeUpMacLayer::stateWakeUpIdleProcess(const MacEvent& event, cMessage* const msg)
{
    const IRadio::RadioMode wuRadioMode = wakeUpRadio->getRadioMode();
    if (event == MacEvent::WU_START) {
        // Start the wake-up state machine
        handleCoincidentalOverheardData(check_and_cast<Packet*>(msg));
        return stateWakeUpWaitApproveWaitEnter(msg);
    }
    else if (event == MacEvent::QUEUE_SEND) {
        ASSERT(currentTxFrame == nullptr);
        setupTransmission();
        if (wuRadioMode == IRadio::RADIO_MODE_SWITCHING || !transmissionStartEnergyCheck()) {
            return stateListeningEnterAlreadyListening();
        }
        else {
            activeBackoff = new CSMATxUniformBackoff(this, activeRadio, 0, txWakeUpWaitDuration);
            const simtime_t delayIfBusy = txWakeUpWaitDuration + dataListeningDuration;
            activeBackoff->startTxOrDelay(delayIfBusy);
            return stateTxEnter();
        }
    }
    return macState;
}

WakeUpMacLayer::State WakeUpMacLayer::stateAwaitTransmitProcess(const MacEvent& event, cMessage* const msg)
{
    if (event == MacEvent::WU_START) {
        cancelEvent(transmitStartDelay);
        cancelEvent(replenishmentTimer);
        // Start the wake-up state machine
        handleCoincidentalOverheardData(check_and_cast<Packet*>(msg));
        return stateWakeUpWaitApproveWaitEnter(msg);
    }
    else if (event == MacEvent::DATA_RX_READY && !transmitStartDelay->isScheduled() && !replenishmentTimer->isScheduled()) {
        // MacEvent::DATA_RX_READY triggered by transmitting ending but with packet ready
        // Perform carrier sense if there is a currentTxFrame
        if (!txQueue->isEmpty()) {
            ASSERT(not txQueue->isEmpty());
            setupTransmission();
        }
        auto minimumBackoff = txWakeUpWaitDuration;
        auto maximumBackoff = txWakeUpWaitDuration + dataListeningDuration;
        activeBackoff = new CSMATxUniformBackoff(this, activeRadio, minimumBackoff, maximumBackoff);
        activeBackoff->startTxOrDelay(minimumBackoff, maximumBackoff);
        return stateTxEnter();
    }
    else if (event == MacEvent::TX_START && !replenishmentTimer->isScheduled()) {
        // MacEvent::TX_START triggered by wait before transmit after rxing or txing
        // Check if there are packets to send and if so, send them
        if (currentTxFrame == nullptr) {
            ASSERT(not txQueue->isEmpty());
            setupTransmission();
        }
        activeBackoff = new CSMATxUniformBackoff(this, activeRadio, 0, txWakeUpWaitDuration);
        const simtime_t delayIfBusy = txWakeUpWaitDuration + dataListeningDuration;
        activeBackoff->startTxOrDelay(delayIfBusy);
        return stateTxEnter();
    }
    else if (event == MacEvent::REPLENISH_TIMEOUT) {
        // Check if there is enough energy. If not, replenish to maintain above tx threshold
        if (!transmissionStartEnergyCheck()) {
            // Turn off and let the SimpleEpEnergyManager turn back on at the on threshold
            LifecycleOperation::StringMap params;
            auto* operation = new ModuleStopOperation();
            operation->initialize(networkNode, params);
            lifecycleController.initiateOperation(operation);
        }
        else {
            // Now got enough energy so transmit by triggering ackBackoff
            if (!transmitStartDelay->isScheduled())
                scheduleAt(simTime(), transmitStartDelay);
        }
    }
    return macState;
}

WakeUpMacLayer::State WakeUpMacLayer::stateReceiveEnter()
{
    rxAckRound = 0;
    stateReceiveEnterDataWait();
    return State::RECEIVE;
}

void WakeUpMacLayer::stateTxEnterEnd()
{
    //The Radio Receive->Sleep triggers next SM transition
    txDataState = TxDataState::END;
    dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
}

void WakeUpMacLayer::stateTxDataWaitExitEnterAckWait()
{
    delete activeBackoff;
    activeBackoff = nullptr;
    Packet* dataFrame = currentTxFrame->dup();
    if(datagramPostRoutingHook(dataFrame)!=INetfilter::IHook::Result::ACCEPT){
        EV_ERROR << "Aborted transmission of data is unimplemented." << endl;
    }
    encapsulate(dataFrame);
    sendDown(dataFrame);
    txDataState = TxDataState::DATA;
}

void WakeUpMacLayer::stateTxWakeUpWaitExit()
{
    delete activeBackoff;
    activeBackoff = nullptr;
}

bool WakeUpMacLayer::stateTxProcess(const MacEvent& event, cMessage* const msg) {
    switch (txDataState){
    case TxDataState::WAKE_UP_WAIT:
        stepBackoffSM(event);
        if(event==MacEvent::TX_READY){
            // TODO: Change this to a short WU packet
            cMessage* const currentTxWakeUp = check_and_cast<cMessage*>(buildWakeUp(currentTxFrame, txInProgressTries));
            send(currentTxWakeUp, wakeUpRadioOutGateId);
            txInProgressTries++;
            txDataState = TxDataState::WAKE_UP;
            stateTxWakeUpWaitExit();
        }
        break;
    case TxDataState::WAKE_UP:
        if(event == MacEvent::TX_END){
            // Wake-up transmission has ended, start wait backoff for neighbors to wake-up
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            scheduleAt(simTime() + txWakeUpWaitDuration, receiveTimeout);
            EV_DEBUG << "TX SM: WAKE_UP_WAIT --> DATA_WAIT";
            // Reset statistic variable counting ack rounds (from transmitter perspective)
            acknowledgmentRound = 0;
        }
        else if(event==MacEvent::DATA_TIMEOUT){
            stateTxEnterDataWait();
        }
        break;
    case TxDataState::DATA_WAIT:
        stepBackoffSM(event);
        if(event==MacEvent::TX_READY){
            stateTxDataWaitExitEnterAckWait();
        }
        break;
    case TxDataState::DATA:
        if(event == MacEvent::TX_END){
            if (skipDirectTxFinalAck && dataMinExpectedCost == EqDC(0)) {
                // Only the final dest will Ack when expectedCost=0 (A DirectTx)
                // therefore do not retransmit dataPacketAgain go straight to end
                // Even if destination does not Ack again.
                currentTxFrame->addTagIfAbsent<EqDCBroadcast>();
                completePacketTransmission();
                stateTxEnterEnd();
            }
            else{//Ack required
                //reset confirmed forwarders count
                acknowledgedForwarders = 0;
                // Increase acknowledgment round value
                acknowledgmentRound++;
                // Schedule acknowledgement wait timeout
                scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
                txDataState = TxDataState::ACK_WAIT;
                dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            }
        }
        break;
    case TxDataState::ACK_WAIT:
        stateTxAckWaitProcess(event, msg);
        break;
    case TxDataState::END:
        if(event == MacEvent::TX_START){
            // Reschedule, because radio transition not finished
            scheduleAt(simTime() + ackWaitDuration, transmitStartDelay);
        }
        else if(event == MacEvent::DATA_RX_IDLE){
            return true;
        }
        break;
    default:
        cRuntimeError("Unhandled State");
    }
    return false;
}

void WakeUpMacLayer::stateTxEnterDataWait()
{
    txDataState = TxDataState::DATA_WAIT;
    // use activeBackoff for backoff state machine
    ASSERT(activeBackoff == nullptr);
    activeBackoff = new CSMATxUniformBackoff(this, activeRadio,
            0.0, ackWaitDuration/3);
    if(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER){
        activeBackoff->startTxOrBackoff();
    }
    else{
        activeBackoff->startCold();
    }
}

void WakeUpMacLayer::stateTxAckWaitProcess(const MacEvent& event, cMessage * const msg) {
    if(event == MacEvent::DATA_RECEIVED){
        EV_DEBUG << "Data Ack Received";
        auto receivedData = check_and_cast<Packet* >(msg);
        auto receivedAck = receivedData->peekAtFront<ORWGram>();
        if(receivedAck->getType() == ORW_ACK &&
                receivedAck->getReceiverAddress() == interfaceEntry->getMacAddress() ){
            EncounterDetails details;
            details.setEncountered(receivedAck->getTransmitterAddress());
            details.setCurrentEqDC(receivedAck->getExpectedCostInd());
            emit(expectedEncounterSignal, 0.8/acknowledgmentRound, &details);

            acknowledgedForwarders++;
            // If acknowledging node is packet destination
            // Set MinExpectedCost to 0 for the next data packet
            // This stops nodes other than the destination participating
            // if recheckDataPacketEqDC is enabled
            const inet::MacAddress ackSender = receivedAck->getTransmitterAddress();
            const inet::MacAddress packetDestination = currentTxFrame->getTag<MacAddressReq>()->getDestAddress();
            if (ackSender == packetDestination) {
                // Update value of EqDC on Tag
                dataMinExpectedCost = EqDC(0.0);
            }
            delete receivedData;
        }
        else{
            handleCoincidentalOverheardData(receivedData);
            EV_DEBUG <<  "Discarding overheard data as busy transmitting" << endl;
            delete receivedData;
        }
    }
    else if(event == MacEvent::ACK_TIMEOUT){
        if(acknowledgmentRound <= 1){
            // At the end of the first ack round, notify of expecting encounters
            emit(listenForEncountersEndedSignal, (double)acknowledgedForwarders);
        }
        auto broadcastTag = currentTxFrame->findTag<EqDCBroadcast>();

        // TODO: Get required forwarders count from packetTag from n/w layer
        // TODO: Test this with more nodes should this include forwarders from prev timeslot?
        const int supplementaryForwarders = acknowledgedForwarders - requiredForwarders;
        if(broadcastTag != nullptr){
            // Don't resend data, broadcasts only get sent once
            completePacketTransmission();
            stateTxEnterEnd();
        }
        else if( activeRadio->getReceptionState() != IRadio::RECEPTION_STATE_IDLE ){
            // Data, possibly ACK or contending wake-up still in progress
            scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
        }
        else if(supplementaryForwarders > 0){
            // Go straight to immediate data retransmission to reduce forwarders
            stateTxEnterDataWait();
        }
        else{
            txInProgressForwarders = txInProgressForwarders+acknowledgedForwarders; // TODO: Check forwarders uniqueness
            completePacketTransmission();
            if(currentTxFrame){//Not complete yet
                // Try transmitting again after standard ack backoff
                scheduleAt(simTime() + ackWaitDuration, transmitStartDelay);
            }
            stateTxEnterEnd();
        }
    }
}

void WakeUpMacLayer::stateWakeUpWaitEnter()
{
    changeActiveRadio(dataRadio);
    dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
    emit(receptionStartedSignal, true);
}

WakeUpMacLayer::State WakeUpMacLayer::stateWakeUpWaitApproveWaitEnter(cMessage* const msg)
{
    wuState = WuWaitState::APPROVE_WAIT;
    scheduleAt(simTime() + wuApproveResponseLimit, receiveTimeout);
    Packet* receivedData = check_and_cast<Packet*>(msg);
    queryWakeupRequest(receivedData);

    stateWakeUpWaitEnter();
    return State::WAKE_UP_WAIT;
}

WakeUpMacLayer::State WakeUpMacLayer::stateWakeUpWaitExitToListening()
{
    //Stop the timer if other event called it first
    emit(receptionDroppedSignal, true);
    return stateListeningEnter();
}

WakeUpMacLayer::State WakeUpMacLayer::stateWakeUpProcess(const MacEvent& event, cMessage * const msg) {
    switch(wuState){
    case WuWaitState::APPROVE_WAIT:
        if(event==MacEvent::WU_APPROVE){
            wuState = WuWaitState::DATA_RADIO_WAIT;
            cancelEvent(receiveTimeout);
            // Cancel transmit packet backoff till receive is done
            cancelEvent(transmitStartDelay); // TODO: What problem does this solve?
        }
        else if(event==MacEvent::DATA_TIMEOUT||event==MacEvent::WU_REJECT){
            // Upper layer did not approve wake-up in time.
            // Will abort the wake-up when radio mode switches
            wuState = WuWaitState::ABORT;
        }
        else if(event==MacEvent::DATA_RX_IDLE||event==MacEvent::DATA_RX_READY){
            //Stop the timer if other event called it first
            return stateWakeUpWaitExitToListening();
        }
        break;
    case WuWaitState::DATA_RADIO_WAIT:
        if(event==MacEvent::DATA_RX_IDLE){
            dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        }
        else if(event==MacEvent::DATA_RX_READY){
            // data radio now listening
            wuState = WuWaitState::IDLE;
            return stateReceiveEnter();
        }
        break;
    case WuWaitState::ABORT:
        if(event==MacEvent::DATA_RX_IDLE||event==MacEvent::DATA_RX_READY){
            return stateWakeUpWaitExitToListening();
        }
        break;
    default:
        cRuntimeError("unhandled state");
    }
    return macState;
}

void WakeUpMacLayer::cancelAllTimers()
{
    ORWMac::cancelAllTimers();
    cancelEvent(transmitStartDelay);
}

void WakeUpMacLayer::deleteAllTimers(){
    delete transmitStartDelay;
}

WakeUpMacLayer::~WakeUpMacLayer() {
    cancelAllTimers();
    deleteAllTimers();
}

void WakeUpMacLayer::changeActiveRadio(physicallayer::IRadio* const newActiveRadio) {
    if(activeRadio != newActiveRadio){
        if(activeRadio)
            activeRadio->setRadioMode(IRadio::RADIO_MODE_OFF);
        activeRadio = newActiveRadio;
        transmissionState = activeRadio->getTransmissionState();
        receptionState = activeRadio->getReceptionState();
    }
}

void WakeUpMacLayer::queryWakeupRequest(Packet* wakeUp) {
    const auto header = wakeUp->peekAtFront<ORWGram>();
    if(header->getType()!=ORWGramType::ORW_BEACON){
        return;
    }
    if(datagramPreRoutingHook(wakeUp)==HookBase::Result::ACCEPT){
        acceptDataEqDCThreshold = wakeUp->getTag<EqDCReq>()->getEqDC();
        // Approve wake-up request
        cMessage* msg = new cMessage("approve");
        msg->setKind(WAKEUP_APPROVE);
        scheduleAt(simTime(), msg);
    }
}

Packet* WakeUpMacLayer::buildWakeUp(const Packet *subject, const int retryCount) const{
    auto wuHeader = makeShared<ORWBeacon>();
    setBeaconFieldsFromTags(subject, wuHeader);
    auto frame = new Packet("wake-up", wuHeader);
    frame->addTag<PacketProtocolTag>()->setProtocol(&ORWProtocol);
    return frame;
}

void WakeUpMacLayer::handleStartOperation(LifecycleOperation *operation) {
    // complete unfinished reception
    completePacketReception();
    macState = stateListeningEnter();
    interfaceEntry->setState(InterfaceEntry::State::UP);
    interfaceEntry->setCarrier(true);
}

void WakeUpMacLayer::handleStopOperation(LifecycleOperation *operation) {
    handleCrashOperation(operation);
}

void WakeUpMacLayer::handleCrashOperation(LifecycleOperation* const operation) {
    if(currentTxFrame != nullptr){
        emit(ackContentionRoundsSignal, acknowledgmentRound);
        if(txInProgressForwarders>0){
            PacketDropDetails details;
            details.setReason(INTERFACE_DOWN);
            // Packet has been received by a forwarder
            dropCurrentTxFrame(details);
        }
    }
    else if(currentRxFrame != nullptr){
        // Check if in ack backoff period and if waiting to send ack
        if(receiveTimeout->isScheduled() && ackBackoffTimer->isScheduled()){
            // Ack not sent yet so just bow out
            delete currentRxFrame;
            currentRxFrame = nullptr;
            emit(receptionDroppedSignal, true);
        }// TODO: check how many contending acks there were
        else{
            // Send packet up upon restart by leaving in memory
        }
    }

    cancelAllTimers();
    if(activeBackoff != nullptr){
        delete activeBackoff;
        activeBackoff = nullptr;
    }
    // Stop all signals from being interpreted
    interfaceEntry->setCarrier(false);
    interfaceEntry->setState(InterfaceEntry::State::DOWN);
}
