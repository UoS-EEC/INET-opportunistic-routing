/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "WakeUpMacLayer.h"
#include <omnetpp.h>
#include <algorithm> // for min max
#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/common/packet/chunk/Chunk.h>
#include <inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h>

#include "common/EqDCTag_m.h"
#include "common/EncounterDetails_m.h"
#include "MacEnergyMonitor.h"
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
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        wuApproveResponseLimit = par("wuApproveResponseLimit");

        maxTxTries = par("maxTxTries");

        const char* wakeUpRadioModulePath = par("wakeUpRadioModule");
        cModule *radioModule = getModuleByPath(wakeUpRadioModulePath);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);

        // Retransmission reduction through data packet updating
        checkDataPacketEqDC = par("checkDataPacketEqDC");
        skipDirectTxFinalAck = checkDataPacketEqDC && par("skipDirectTxFinalAck");

        dyanmicWakeUpChecking = par("dynamicWakeUpChecking");

        // Validation
        if(dyanmicWakeUpChecking && not checkDataPacketEqDC)
            throw cRuntimeError("Data packet checking must be enabled if dynamic wake up checking is not enabled");

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

void WakeUpMacLayer::receiveSignal(cComponent* const source, simsignal_t const signalID,
        intval_t const value, cObject* const details) {
    Enter_Method_Silent();
    // Check it is for the active radio
    cComponent* activeRadioComponent = check_and_cast_nullable<cComponent*>(activeRadio);
    if(operationalState == OPERATING && activeRadioComponent && activeRadioComponent == source){
        handleRadioSignal(signalID, value);
    }
}

void WakeUpMacLayer::handleSelfMessage(cMessage* const msg) {
    if(msg->getKind() == WAKEUP_APPROVE){
        stateProcess(MacEvent::WU_APPROVE, msg);
        delete msg;
    }
    else if(msg->getKind() == WAKEUP_REJECT){
        stateProcess(MacEvent::WU_REJECT, msg);
        delete msg;
    }
    else{
        ORWMac::handleSelfMessage(msg);
    }
}

void WakeUpMacLayer::stateProcess(const MacEvent& event, cMessage * const msg) {
    // Operate State machine based on current state and event
    switch (macState){
    case State::WAKE_UP_IDLE:
        macState = stateWakeUpIdleProcess(event, msg);
        break;
    case State::WAKE_UP_WAIT:
        // Process wake-up and wait for listening to start
        macState = stateWakeUpProcess(event, msg);
        break;
    case State::DATA_IDLE:
        EV_WARN << "Wake-up MAC in unhandled state. Return to idle" << endl;
        macState = State::WAKE_UP_IDLE;
        break;
    default:
        ORWMac::stateProcess(event, msg);
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
    else {
        return ORWMac::stateAwaitTransmitProcess(event, msg);
    }
}

WakeUpMacLayer::State WakeUpMacLayer::stateReceiveEnter()
{
    rxAckRound = 0;
    stateReceiveEnterDataWait();
    return State::RECEIVE;
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
    default:
        return ORWMac::stateTxProcess(event, msg);
    }
    return false;
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
    if(not dyanmicWakeUpChecking){
        // Must wake and wait for data
        acceptDataEqDCThreshold = EqDC(25.5);
        // Approve wake-up request
        cMessage* msg = new cMessage("approve");
        msg->setKind(WAKEUP_APPROVE);
        scheduleAt(simTime(), msg);
    }
    else if(datagramPreRoutingHook(wakeUp)==HookBase::Result::ACCEPT){
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
