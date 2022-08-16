/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ORWMac.h"
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include "common/EqDCTag_m.h"
#include "common/EncounterDetails_m.h"

using namespace inet;
using physicallayer::IRadio;
using namespace oppostack;

void ORWMac::stateProcess(const MacEvent& event, cMessage * const msg) {
    // Operate State machine based on current state and event
    auto ret = macState;
    switch (macState){
    case State::DATA_IDLE:
        if(event == MacEvent::DATA_RECEIVED){
            handleCoincidentalOverheardData(check_and_cast<Packet*>(msg)); // TODO: Should this be here, see WuMac::stateWakeUpIdleProcess()
            emit(receptionStartedSignal, true);
            ret = stateReceiveEnter(); // TODO: Replace with startReception function (see ORWMac::stateAwaitTransmitProcess() )
            stateReceiveDataWaitProcessDataReceived(msg);// TODO: This may not work
        }
        else if (event == MacEvent::QUEUE_SEND) {
            ASSERT(currentTxFrame == nullptr);
            setupTransmission();
            if (dataRadio->getRadioMode() == IRadio::RADIO_MODE_SWITCHING || !transmissionStartEnergyCheck()) {
                ret = stateListeningEnterAlreadyListening();
            }
            else {
                ret = stateTxEnter();
            }
        }
        macState = ret;
        break;
    case State::AWAIT_TRANSMIT:
//        ASSERT(dataRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER
//                || dataRadio->getRadioMode() == IRadio::RADIO_MODE_SWITCHING);
        macState = stateAwaitTransmitProcess(event, msg);
        break;
    case State::TRANSMIT:
        if( stateTxProcess(event, msg) ){
            // State transmit is therefore finished, enter listening
            macState = stateListeningEnter();
        }
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

ORWMac::State ORWMac::stateListeningEnterAlreadyListening(){
    if(currentTxFrame || not txQueue->isEmpty()){
        // Data is waiting in the tx queue
        // Schedule replenishment timer if insufficient stored energy
        if(!transmissionStartEnergyCheck())
            scheduleAt(simTime() + replenishmentCheckRate, replenishmentTimer);
        else if (dataRadio->getRadioMode() != IRadio::RADIO_MODE_SWITCHING )
            scheduleAt(simTime(), transmitStartDelay);
        return State::AWAIT_TRANSMIT;
    }
    else{
        return State::DATA_IDLE;
    }
}


ORWMac::State ORWMac::stateListeningEnter(){
    dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    return stateListeningEnterAlreadyListening();
}

// Receive acknowledgement process that can be overridden
bool ORWMac::stateReceiveProcess(const MacEvent& event, cMessage * const msg) {
    if(event == MacEvent::DATA_RECEIVED){
        auto packet = check_and_cast<const Packet*>(msg);
        if(packet->peekAtFront<ORWGram>()->getType() == ORW_ACK){
            handleOverheardAckInDataReceiveState(packet);
            delete packet;
            return false;
        }
    }

    switch(rxState){
        case RxState::DATA_WAIT:
            if(event == MacEvent::DATA_TIMEOUT){
                // The receiving has timed out, optionally process received packet
                stateReceiveProcessDataTimeout();
                return true;
            }
            else if(event == MacEvent::DATA_RECEIVED){
                stateReceiveDataWaitProcessDataReceived(msg);
            }
            // Transition from ACK after TX_END
            else if(event == MacEvent::DATA_RX_READY){
                stateReceiveExitDataWait();
                stateReceiveEnterDataWait();
            }
            break;
        case RxState::ACK:
            stateReceiveAckProcessBackoff(event);
            if(event == MacEvent::TX_END){
                stateReceiveExitAck();
                stateReceiveAckEnterReceiveDataWait();
            }
            else if(event == MacEvent::DATA_RECEIVED){
                stateReceiveDataWaitProcessDataReceived(msg);
            }
            break;
        case RxState::FINISH:
            if(event == MacEvent::DATA_RECEIVED){
                Packet* incomingFrame = check_and_cast<Packet*>(msg);
                auto incomingMacData = incomingFrame->peekAtFront<ORWGram>();
                Packet* storedFrame = check_and_cast<Packet*>(currentRxFrame);
                if(incomingMacData->getType()==ORW_DATA
                            && storedFrame->peekAtFront<ORWGram>()->getTransmitterAddress() == incomingMacData->getTransmitterAddress() ){
                    // Contention for the data packet is still going on
                    // Reset the timer
                    cancelEvent(receiveTimeout);
                    stateReceiveEnterFinish();
                }
            }
            if(event == MacEvent::DATA_TIMEOUT){
                if(deferredDuplicateDrop){
                    // Complete defer packet drop
                    PacketDropDetails details;
                    details.setReason(PacketDropReason::DUPLICATE_DETECTED);
                    dropCurrentRxFrame(details);
                    deferredDuplicateDrop = false;
                }
                // The receiving has timed out, optionally process received packet
                stateReceiveProcessDataTimeout();
                return true;
            }
            break;
        default: cRuntimeError("Unknown state");
    }
    return false;
}

ORWMac::State ORWMac::stateReceiveEnter()
{
    rxAckRound = 0;
    stateReceiveEnterDataWait();
    return State::RECEIVE;
}

void ORWMac::stateReceiveEnterDataWait()
{
    rxState = RxState::DATA_WAIT;
    scheduleAt(simTime() + dataListeningDuration, receiveTimeout);
}

void ORWMac::stateReceiveExitDataWait()
{
    // Cancel delivery timer until next round
    cancelEvent(receiveTimeout);
}

void ORWMac::stateReceiveAckProcessBackoff(const MacEvent& event)
{
    const auto backoffResult = stepBackoffSM(event);
    if (event == MacEvent::TX_READY) {
        // send acknowledgement packet when radio is ready
        sendDown(buildAck(check_and_cast<Packet*>(currentRxFrame)));
    }
    else if (backoffResult == CSMATxBackoffBase::State::OFF) {
        // Backoff has been aborted
        // Drop packet and schedule immediate timeout
        EV_WARN << "Dropping packet because of channel congestion";
        stateReceiveExitAck();
        stateReceiveEnterFinishDropReceived(PacketDropReason::DUPLICATE_DETECTED);
    }
}

void ORWMac::stateReceiveEnterAck()
{
    // Continue to contend for packet
    rxAckRound++;
    activeBackoff = new CSMATxRemainderReciprocalBackoff(this, dataRadio,
            ackTxWaitDuration, minimumContentionWindow);
    activeBackoff->delayCarrierSense(uniform(0, initialContentionDuration));
    rxState = RxState::ACK;
}

void ORWMac::stateReceiveExitAck()
{
    delete activeBackoff;
    activeBackoff = nullptr;
    rxState = RxState::IDLE;
}

void ORWMac::stateReceiveEnterFinishDropReceived(const inet::PacketDropReason reason)
{
    if(reason == PacketDropReason::DUPLICATE_DETECTED){
        //defer packet drop till RxState::Finish DATA_TIMEOUT
        deferredDuplicateDrop = true;
    }
    else{
        PacketDropDetails details;
        details.setReason(reason);
        dropCurrentRxFrame(details);
        deferredDuplicateDrop = false;
    }
    stateReceiveEnterFinish();
}

void ORWMac::stateReceiveEnterFinish()
{
    // return to receive mode (via receive finish) when ack transmitted
    // Wait for retransmissions to avoid contention again
    rxState = RxState::FINISH;
    if(currentRxFrame == nullptr)
        scheduleAt(simTime(), receiveTimeout);
    else
        scheduleAt(simTime() + dataListeningDuration + ackWaitDuration, receiveTimeout);
}


void ORWMac::stateReceiveDataWaitProcessDataReceived(cMessage * const msg) {
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<ORWGram>();
    Packet* storedFrame = check_and_cast_nullable<Packet*>(currentRxFrame);
    if(incomingMacData->getType()==ORW_DATA && currentRxFrame == nullptr){
        stateReceiveExitDataWait();
        // Store the new received packet
        currentRxFrame = incomingFrame;

        // Check using packet data that accepting wake-up is still correct
        INetfilter::IHook::Result preRoutingResponse = datagramPreRoutingHook(incomingFrame);
        if(preRoutingResponse != IHook::ACCEPT){
            // New information in the data packet means do not accept data packet
            stateReceiveEnterFinishDropReceived(PacketDropReason::OTHER_PACKET_DROP);
        }
        else{
            //TODO: Make Opportunistic Acceptance Decision
            auto acceptThresholdTag = incomingFrame->findTag<EqDCReq>();
            EqDC newAcceptThreshold = acceptThresholdTag!=nullptr ? acceptThresholdTag->getEqDC() : EqDC(25.5);
            // If better EqDC threshold, update
            if(newAcceptThreshold < acceptDataEqDCThreshold){
                acceptDataEqDCThreshold = newAcceptThreshold;
            }

            // Limit collisions with exponentially decreasing backoff, ack takes about 1ms anyway,
            // with minimum contention window derived from the Rx->tx turnaround time
            stateReceiveEnterAck();
        }
    }
    // Compare the received data to stored data, discard it new data
    else if(incomingMacData->getType()==ORW_DATA/* && currentRxFrame != nullptr*/
            && storedFrame->peekAtFront<ORWGram>()->getTransmitterAddress() == incomingMacData->getTransmitterAddress() ){
        // Delete the existing currentRxFrame
        delete currentRxFrame;

        stateReceiveExitDataWait();
        // Store the new received packet
        currentRxFrame = incomingFrame;


        // TODO: Make Opportunistic Contention Decision
        //        Containing Make Opportunistic Acceptance Decision
        // Check if the retransmitted packet still accepted
        // Packet will change if transmitter receives ack from the final destination
        // to improve the chances that only the data destination responds.
        if(datagramPreRoutingHook(incomingFrame) != INetfilter::IHook::ACCEPT
                && checkDataPacketEqDC){
            // New information in the data packet means do not accept data packet
            stateReceiveEnterFinishDropReceived(PacketDropReason::OTHER_PACKET_DROP);
        }
        else{
            // Begin random relay contention
            double relayDiceRoll = uniform(0,1);
            bool destinationAckPersistance = checkDataPacketEqDC && (acceptDataEqDCThreshold == EqDC(0.0) );
            if(destinationAckPersistance && skipDirectTxFinalAck){
                // Received Direct Tx and so stop extra ack
                // Send immediate wuTimeout to trigger MacEvent::WU_TIMEOUT
                stateReceiveEnterFinish();
            }
            else if(destinationAckPersistance ||
                    relayDiceRoll<candiateRelayContentionProbability){
                // Continue to contend for packet
                stateReceiveEnterAck();
            }
            else{
                // Drop out of forwarder contention
                stateReceiveEnterFinishDropReceived(PacketDropReason::DUPLICATE_DETECTED);
                EV_DEBUG  << "Detected other relay so discarding packet" << endl;
            }
        }
    }
    else{
        // Delete unknown message
        EV_DEBUG << "Discard interfering data transmission" << endl;
        delete incomingFrame;
    }
}

void ORWMac::stateReceiveAckProcessDataReceived(cMessage* msg)
{
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<ORWGram>();
    Packet* storedFrame = check_and_cast_nullable<Packet*>(currentRxFrame);
    if(incomingMacData->getType()==ORW_DATA/* && currentRxFrame != nullptr*/
            && storedFrame->peekAtFront<ORWGram>()->getTransmitterAddress() == incomingMacData->getTransmitterAddress() ){
        stateReceiveExitAck();
        stateReceiveEnterFinishDropReceived(PacketDropReason::DUPLICATE_DETECTED);
    }
    delete msg;
}

void ORWMac::stateReceiveAckEnterReceiveDataWait()
{
    // return to receive mode (via receive wait) when ack transmitted
    // For follow up packet
    cancelEvent(receiveTimeout);
    stateReceiveEnterDataWait();

    // From transmit mode
    dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
}

void ORWMac::stateReceiveProcessDataTimeout()
{
    // The receiving has timed out, if packet is received process
    completePacketReception();
}

ORWMac::State ORWMac::stateAwaitTransmitProcess(const MacEvent& event, cMessage* const msg)
{
    if(event == MacEvent::DATA_RECEIVED){
        cancelEvent(transmitStartDelay);
        cancelEvent(replenishmentTimer);
        handleCoincidentalOverheardData(check_and_cast<Packet*>(msg)); // TODO: Should this be here, see WuMac::stateAwaitTransmitProcess()
        auto ret = stateReceiveEnter(); // TODO: Replace with startReception function (see WuMac::stateAwaitTransmitProcess() )
        stateReceiveDataWaitProcessDataReceived(msg);// TODO: This may not work
        return ret;
    }
    else if (event == MacEvent::DATA_RX_READY && !transmitStartDelay->isScheduled() && !replenishmentTimer->isScheduled()) {
        // MacEvent::DATA_RX_READY triggered by transmitting ending but with packet ready
        // Perform carrier sense if there is a currentTxFrame
        if (!txQueue->isEmpty()) {
            ASSERT(not txQueue->isEmpty());
            setupTransmission();
        }
        return stateTxEnter();
    }
    else if (event == MacEvent::TX_START && !replenishmentTimer->isScheduled()) {
        // MacEvent::TX_START triggered by wait before transmit after rxing or txing
        // Check if there are packets to send and if so, send them
        if (currentTxFrame == nullptr) {
            ASSERT(not txQueue->isEmpty());
            setupTransmission();
        }
        return stateTxEnter();
    }
    else if (event == MacEvent::REPLENISH_TIMEOUT) {
        // Check if there is enough energy. If not, replenish to maintain above tx threshold
        if (!transmissionStartEnergyCheck()) {
            // Turn off and let the SimpleEpEnergyManager turn back on at the on threshold
            LifecycleOperation::StringMap params;
            auto* operation = new ModuleStopOperation();
            operation->initialize(networkNode, params);
            initiateOperation(operation);
        }
        else {
            // Now got enough energy so transmit by triggering ackBackoff
            if (!transmitStartDelay->isScheduled())
                scheduleAt(simTime(), transmitStartDelay);
        }
    }
    return macState;
}

bool ORWMac::stateTxProcess(const MacEvent& event, cMessage* const msg) {
    switch (txDataState){
    case TxDataState::DATA_WAIT:
        stepBackoffSM(event);
        if(event==MacEvent::TX_READY){
            stateTxDataWaitExitEnterAckWait();
        }
        else if(event==MacEvent::DATA_RECEIVED){
            handleCoincidentalOverheardData(check_and_cast<Packet*>(msg));
            EV_WARN <<  "Discarding overheard data as busy transmitting" << endl;
            delete msg;
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

ORWMac::State ORWMac::stateTxEnter()
{
    dataMinExpectedCost = EqDC(25.5);
    // Reset statistic variable counting ack rounds (from transmitter perspective)
    acknowledgmentRound = 0;
    txInProgressTries++;
    stateTxEnterDataWait();
    return State::TRANSMIT;
}

void ORWMac::stateTxEnterDataWait()
{
    txDataState = TxDataState::DATA_WAIT;
    // use activeBackoff for backoff state machine
    ASSERT(activeBackoff == nullptr);
    activeBackoff = new CSMATxUniformBackoff(this, dataRadio,
            0.0, ackWaitDuration/3);
    if(dataRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER){
        activeBackoff->startTxOrBackoff();
    }
    else{
        activeBackoff->startCold();
    }
}

void ORWMac::stateTxDataWaitExitEnterAckWait()
{
    delete activeBackoff;
    activeBackoff = nullptr;
    Packet* dataFrame = currentTxFrame->dup();
    if(datagramPostRoutingHook(dataFrame)!=INetfilter::IHook::Result::ACCEPT){
        EV_ERROR << "Aborted transmission of data is unimplemented." << endl;
    }

    // Begin Transmission
    encapsulate(dataFrame);
    sendDown(dataFrame);
    txDataState = TxDataState::DATA;
}

void ORWMac::stateTxAckWaitProcess(const MacEvent& event, cMessage * const msg) {
    if(event == MacEvent::DATA_RECEIVED){
        EV_DEBUG << "Data Ack Received";
        auto receivedData = check_and_cast<Packet* >(msg);
        auto receivedAck = receivedData->peekAtFront<ORWGram>();
        if(receivedAck->getType() == ORW_ACK &&
                receivedAck->getReceiverAddress() == networkInterface->getMacAddress() ){
            const double weighting = 0.8/pow(2, acknowledgmentRound-1);
            emitEncounterFromWeightedPacket(expectedEncounterSignal, weighting, receivedData);

            acknowledgedForwarders++;
            // If acknowledging node is packet destination
            // Set MinExpectedCost to 0 for the next data packet
            // This stops nodes other than the destination participating
            // if checkDataPacketEqDC is enabled
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
            EV_WARN <<  "Discarding overheard data as busy transmitting" << endl;
            delete receivedData;
        }
    }
    else if(event == MacEvent::ACK_TIMEOUT){
        if(acknowledgmentRound <= 1){
            // At the end of the first ack round, notify of expecting encounters
            emit(listenForEncountersEndedSignal, (double)acknowledgedForwarders);
        }
        emit(ACKreceivedSignal, (double)acknowledgedForwarders);

        auto broadcastTag = currentTxFrame->findTag<EqDCBroadcast>();

        // TODO: Get required forwarders count from packetTag from n/w layer
        // TODO: Test this with more nodes should this include forwarders from prev timeslot?
        const int supplementaryForwarders = acknowledgedForwarders - requiredForwarders;
        if(broadcastTag != nullptr){
            // Don't resend data, broadcasts only get sent once
            completePacketTransmission();
            stateTxEnterEnd();
        }
        else if( dataRadio->getReceptionState() != IRadio::RECEPTION_STATE_IDLE ){
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

void ORWMac::stateTxEnterEnd()
{
    //The Radio Receive->Sleep triggers next SM transition
    txDataState = TxDataState::END;
    dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
}
