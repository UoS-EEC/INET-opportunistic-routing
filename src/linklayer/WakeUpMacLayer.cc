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
#include <inet/physicallayer/base/packetlevel/FlatTransmitterBase.h>
#include <inet/physicallayer/contract/packetlevel/IRadioMedium.h>
#include <inet/physicallayer/backgroundnoise/IsotropicScalarBackgroundNoise.h>
#include <inet/physicallayer/base/packetlevel/FlatReceiverBase.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/common/packet/chunk/Chunk.h>

#include "../networklayer/ORWRouting.h"
#include "WakeUpGram_m.h"
#include "common/EqDCTag_m.h"
#include "common/EncounterDetails_m.h"

using namespace inet;
using physicallayer::IRadio;
using physicallayer::FlatTransmitterBase;
using namespace oppostack;

Define_Module(WakeUpMacLayer);

void WakeUpMacLayer::initialize(int const stage) {
    MacProtocolBase::initialize(stage);
//    // Allow serialization to better represent conflicting radio protocols
//    Chunk::enableImplicitChunkSerialization = true;
    if (stage == INITSTAGE_LOCAL) {
        //Register the Wake-up radio gates
        wakeUpRadioInGateId = findGate("wakeUpRadioIn");
        wakeUpRadioOutGateId = findGate("wakeUpRadioOut");

        //Create timer messages
        wakeUpBackoffTimer = new cMessage("wake-up backoff");
        wuTimeout = new cMessage("wake-up accept timeout");
        ackBackoffTimer = new cMessage("ack wait timer");
        replenishmentTimer = new cMessage("replenishment check timeout");
        dataListeningDuration = par("dataListeningDuration");
        txWakeUpWaitDuration = par("txWakeUpWaitDuration");
        ackWaitDuration = par("ackWaitDuration");
        initialContentionDuration = ackWaitDuration/3;
        wuApproveResponseLimit = par("wuApproveResponseLimit");
        candiateRelayContentionProbability = par("candiateRelayContentionProbability");
        transmissionStartMinEnergy = J(par("transmissionStartMinEnergy"));

        maxWakeUpTries = par("maxWakeUpTries");

        const char *radioModulePath = par("dataRadioModule");
        cModule *radioModule = getModuleByPath(radioModulePath);
        dataRadio = check_and_cast<IRadio *>(radioModule);
        const char* wakeUpRadioModulePath = par("wakeUpRadioModule");
        radioModule = getModuleByPath(wakeUpRadioModulePath);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);

        const char* energyStoragePath = par("energyStorage");
        cModule* storageModule = getModuleByPath(energyStoragePath);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);
        const char* networkNodePath = par("networkNode");
        networkNode = getModuleByPath(networkNodePath);

        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));

        // Retransmission reduction through data packet updating
        recheckDataPacketEqDC = par("recheckDataPacketEqDC");
        skipDirectTxFinalAck = recheckDataPacketEqDC && par("skipDirectTxFinalAck");


        /*
         * Warning of noise in radio medium exceeding energy detection threshold
         */
        auto medium = getModuleFromPar<physicallayer::IRadioMedium>(radioModule->par("radioMediumModule"), radioModule);
        auto noiseModel = medium->getBackgroundNoise();
        auto scalarNoiseModel = check_and_cast_nullable<const physicallayer::IsotropicScalarBackgroundNoise*>(noiseModel);
        auto dataReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(dataRadio->getReceiver());
        if(scalarNoiseModel && dataReceiverModel)
            if(scalarNoiseModel->getPower() > dataReceiverModel->getEnergyDetection())
                cRuntimeError("Background noise power is greater than data radio energy detection threshold. Radio will always be \"Busy\"");
        auto wakeUpReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(wakeUpRadio->getReceiver());
        if(scalarNoiseModel && wakeUpReceiverModel)
            if(scalarNoiseModel->getPower() > wakeUpReceiverModel->getEnergyDetection())
                cRuntimeError("Background noise power is greater than wake up radio energy detection threshold. Radio will always be \"Busy\"");


        /*
         * Calculation of invalid parameter combinations at the receiver
         * - MAC Layer requires tight timing during a communication negotiation
         * - Incorrect timing increases the collision and duplication probability
         * After the wake-up contention to receive and forward occurs
         * The physical MTU is limited by the time spent dataListening after sending an ACK
         * If an ACK is sent at the start of the ACK period, the data may not be fully
         * received until remainder of ack period + 1/5(ack period)
         */
        auto lengthPrototype = makeShared<WakeUpDatagram>();
        const b ackBits = b(lengthPrototype->getChunkLength());
        auto dataTransmitter = check_and_cast<const FlatTransmitterBase *>(dataRadio->getTransmitter());
        const bps bitrate = dataTransmitter->getBitrate();
        const int maxAckCount = std::floor(ackWaitDuration.dbl()*bitrate.get()/ackBits.get());
        ASSERT2(maxAckCount > requiredForwarders + 2, "Ack wait duration is too small for multiple forwarders.");
        ASSERT(maxAckCount < 20);
        const double remainingAckProportion = (double)(maxAckCount-1)/(double)(maxAckCount);
        ASSERT(remainingAckProportion < 1 && remainingAckProportion > 0);
        // 0.2*ackWaitDuration currently Hardcoded into TX_DATA state
        const b phyMaxBits = b( std::floor( (dataListeningDuration.dbl() - (remainingAckProportion+0.2)*ackWaitDuration.dbl())*bitrate.get() ) );
        phyMtu = B(phyMaxBits.get()/8); // Integer division implicitly (and correctly) rounds down

        /*
         * Calculation of Ack contention parameters
         * - The entire ack message must be sent within the ackWaitDuration.
         *   So must start by ackWaitDuration - ackDuration
         * - If the ack wait duration is too close to the "turnaround" (Rx->Tx)
         *   of the data radio, then collision probability is high,
         *   for 4 responding nodes, expect 2 must be uninterrupted
         *   So collision prob must be under 25%
         * - If the radio is still contending for an ack near the end of the
         *   ackWaitDuration, set a minimumContentionWindow so contention
         *   probability is < 50%
         */
        const simtime_t turnaroundTime = par("radioTurnaroundTime");
        const simtime_t ackDuration = SimTime(ackBits.get()/bitrate.get());
        ackTxWaitDuration = ackWaitDuration - ackDuration;
        const double initialCollisionProbability = 1 - std::exp(-4.0*turnaroundTime.dbl()/initialContentionDuration.dbl());
        minimumContentionWindow = 2.0/std::log(2)*turnaroundTime;
        ASSERT(initialContentionDuration > minimumContentionWindow);
        ASSERT(initialCollisionProbability < 0.25);

    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // Register signals for handling radio mode changes
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
    txQueue->pushPacket(packet);
    if(macState == S_IDLE){
        stepMacSM(EV_QUEUE_SEND, packet);
    }
    // Else, packet will be sent when it returns to S_IDLE
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
    if(operationalState == OPERATING && activeRadioComponent && activeRadioComponent == source){
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
    else if(msg == replenishmentTimer){
        stepMacSM(EV_REPLENISH_TIMEOUT, msg);
    }
    else if( activeBackoff && activeBackoff->isBackoffTimer(msg) ){
        stepMacSM(EV_CSMA_BACKOFF, msg);
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

void WakeUpMacLayer::stepMacSM(const t_mac_event& event, cMessage * const msg) {
    if(event == EV_DATA_RECEIVED){
        // TODO: Update neighbor tables
    }
    // Operate State machine based on current state and event
    const IRadio::RadioMode wuRadioMode = wakeUpRadio->getRadioMode();

    switch (macState){
    case S_IDLE:
        ASSERT(wuRadioMode == IRadio::RADIO_MODE_RECEIVER
                || wuRadioMode == IRadio::RADIO_MODE_SWITCHING);
        cancelEvent(replenishmentTimer);
        if(event == EV_WU_START){
            // Start the wake-up state machine
            stepWuSM(event, msg);
            updateMacState(S_WAKEUP_LSN);
        }
        else if((event == EV_ACK_TIMEOUT || event == EV_DATA_RX_READY) && !ackBackoffTimer->isScheduled()){
            // EV_ACK_TIMEOUT triggered by need for retry of packet
            // but check ackBackoffTimer in case of longer backoff before transmission
            // Perform carrier sense if there is a currentTxFrame
            if(currentTxFrame != nullptr){
                auto minimumBackoff = txWakeUpWaitDuration;
                auto maximumBackoff = txWakeUpWaitDuration + dataListeningDuration;
                activeBackoff = new CSMATxUniformBackoff(this, activeRadio, energyStorage,
                        transmissionStartMinEnergy, minimumBackoff, maximumBackoff);
                if( activeBackoff->startTxOrDelay(minimumBackoff, maximumBackoff) ){
                    updateTxState(TX_WAKEUP_WAIT);
                    updateMacState(S_TRANSMIT);
                }
                else{
                    // Schedule switch to replenishment state, after small wait for other packets
                    scheduleAt(simTime() + SimTime(1, SimTimeUnit::SIMTIME_S), replenishmentTimer);
                    updateMacState(S_IDLE);
                    delete activeBackoff;
                    activeBackoff = nullptr;
                }
            }
            // Else do nothing, wait for packet from upper or wake-up
        }
        else if(event == EV_QUEUE_SEND || event == EV_WAKEUP_BACKOFF){
            // EV_WAKEUP_BACKOFF triggered by wait before transmit when busy rxing or txing
            // Check if there are packets to send and if so, send them
            if(txQueue->isEmpty()){
                // Do nothing
                updateMacState(S_IDLE);
            }
            else if(wakeUpRadio->getRadioMode() == IRadio::RADIO_MODE_SWITCHING){
                // Schedule timer to start backoff after short time if nothing in currentTxFrame buffer
                if( currentTxFrame == nullptr  && !wakeUpBackoffTimer->isScheduled() ){
                    scheduleAt(simTime() + wuApproveResponseLimit, wakeUpBackoffTimer);
                }
            }
            else if(setupTransmission()){
                updateTxState(TX_WAKEUP_WAIT);
                updateMacState(S_TRANSMIT);
            }
            else{
                // Schedule switch to replenishment state, after small wait for other packets
                scheduleAt(simTime() + SimTime(1, SimTimeUnit::SIMTIME_S), replenishmentTimer);
                updateMacState(S_IDLE);
                delete activeBackoff;
                activeBackoff = nullptr;
            }
        }
        else if(event == EV_REPLENISH_TIMEOUT){
            // Check if there is enough energy. If not, replenish to maintain above tx threshold
            if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
                // Turn off and let the SimpleEpEnergyManager turn back on at the on threshold
                LifecycleOperation::StringMap params;
                auto *operation = new ModuleStopOperation();
                operation->initialize(networkNode, params);
                lifecycleController.initiateOperation(operation);
            }
            else{
                // Now got enough energy so transmit by triggering ackBackoff
                if(!ackBackoffTimer->isScheduled())
                    scheduleAt(simTime(), ackBackoffTimer);
            }
        }
        else{
            EV_WARN << "Wake-up MAC received unhandled event" << msg << endl;
        }
        break;
    case S_TRANSMIT:
        stepTxSM(event, msg);
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

// Receive acknowledgement process that can be overridden
void WakeUpMacLayer::stepRxAckProcess(const t_mac_event& event, cMessage * const msg) {
    if(activeBackoff){
        const auto backoffResult = stepBackoffSM(event);
        if(backoffResult==CSMATxBackoffBase::BO_FINISHED){
            // send acknowledgement packet when radio is ready
            sendDown(buildAck(check_and_cast<Packet*>(currentRxFrame)));
            updateMacState(S_ACK);
            delete activeBackoff;
            activeBackoff = nullptr;
        }
        else if(backoffResult==CSMATxBackoffBase::BO_OFF){
            // Backoff has been aborted
            // Drop packet and schedule immediate timeout
            PacketDropDetails details;
            EV_WARN << "Dropping packet because of channel congestion";
            details.setReason(PacketDropReason::INCORRECTLY_RECEIVED);
            dropCurrentRxFrame(details);

            cancelEvent(wuTimeout);
            scheduleAt(simTime(), wuTimeout);
            delete activeBackoff;
            activeBackoff = nullptr;
        }
    }
    if(event == EV_DATA_RECEIVED){
        handleDataReceivedInAckState(msg);
    }
    else if(event == EV_TX_END){
        // return to receive mode (via receive wait) when ack transmitted
        // For follow up packet
        updateMacState(S_WAKEUP_LSN);
        updateWuState(WU_WAKEUP_WAIT);
        cancelEvent(wuTimeout);
        dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    }
    else if(event == EV_WU_TIMEOUT){
        // The receiving has timed out, if packet is received process
        completePacketReception();
        // Return to idle listening
        changeActiveRadio(wakeUpRadio);
        cancelEvent(wuTimeout);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        scheduleAt(simTime(), replenishmentTimer);
        updateMacState(S_IDLE);
    }
}

void WakeUpMacLayer::handleDataReceivedInAckState(cMessage * const msg) {
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<WakeUpGram>();
    // TODO: Update neighbor table?
    if(incomingMacData->getType()==WU_DATA && currentRxFrame == nullptr){
        auto incomingFullHeader = incomingFrame->peekAtFront<WakeUpDatagram>();
        updateMacState(S_ACK);
        // Store the new received packet
        currentRxFrame = incomingFrame;
        // Cancel delivery timer until next round
        cancelEvent(wuTimeout);

        // Check using packet data that accepting wake-up is still correct
        INetfilter::IHook::Result preRoutingResponse = datagramPreRoutingHook(incomingFrame);
        if(preRoutingResponse != IHook::ACCEPT){
            // New information in the data packet means do not accept data packet
            PacketDropDetails details;
            details.setReason(PacketDropReason::OTHER_PACKET_DROP);
            dropCurrentRxFrame(details);
            // Send immediate wuTimeout to trigger EV_WU_TIMEOUT
            scheduleAt(simTime(), wuTimeout);

            // TODO: Send "Negative Ack"?
        }
        else{
            auto acceptThresholdTag = incomingFrame->findTag<EqDCReq>();
            EqDC newAcceptThreshold = acceptThresholdTag!=nullptr ? acceptThresholdTag->getEqDC() : EqDC(25.5);
            // If better EqDC threshold, update
            if(newAcceptThreshold < acceptDataEqDCThreshold){
                acceptDataEqDCThreshold = newAcceptThreshold;
            }

            // Reset cumulative ack backoff
            if(activeBackoff)
                delete activeBackoff;
            // Limit collisions with exponentially decreasing backoff, ack takes about 1ms anyway,
            // with minimum contention window derived from the Rx->tx turnaround time
            activeBackoff = new CSMATxRemainderReciprocalBackoff(this,
                    activeRadio,
                    energyStorage,
                    J(0), ackTxWaitDuration, minimumContentionWindow);
            activeBackoff->delayCarrierSense( uniform(0, initialContentionDuration) );
            rxAckRound++;
        }
    }
    else if(incomingMacData->getType()==WU_DATA/* && currentRxFrame != nullptr*/){
        updateMacState(S_ACK);
        // Compare the received data to stored data
        Packet* storedFrame = check_and_cast<Packet*>(currentRxFrame);
        auto storedMacData = storedFrame->peekAtFront<WakeUpGram>();
        // Intentionally promote variable to extended WakeUpDatagram
        auto incomingMacData = incomingFrame->peekAtFront<WakeUpDatagram>();
        if(storedMacData->getTransmitterAddress() == incomingMacData->getTransmitterAddress()){
            // Enough to say packet matches, meaning retransmission due to forwarder contention

            // Replace the currentRxFrame
            delete currentRxFrame;
            currentRxFrame = check_and_cast<Packet*>(incomingFrame);
            // TODO: Cancel own ack if new data has expected cost of zero
            // along with stepTxAckProcess this can improve the chances that only the dataDestination responds.
            // Begin random relay contention

            // Cancel delivery timer until next round
            cancelEvent(wuTimeout);

            // Check if the retransmited packet still accepted
            // Packet will change if transmitter receives ack from the final destination
            // to improve the chances that only the data destination responds.
            if(recheckDataPacketEqDC && datagramPreRoutingHook(incomingFrame) != INetfilter::IHook::ACCEPT){
                // New information in the data packet means do not accept data packet
                PacketDropDetails details;
                details.setReason(PacketDropReason::OTHER_PACKET_DROP);
                dropCurrentRxFrame(details);
                // Wait to overhear Acks before EV_WU_TIMEOUT
                scheduleAt(simTime() + initialContentionDuration, wuTimeout);
            }
            else{
                // Begin random relay contention
                double relayDiceRoll = uniform(0,1);
                bool destinationAckPersistance = recheckDataPacketEqDC && (acceptDataEqDCThreshold == EqDC(0.0) );
                if(destinationAckPersistance && skipDirectTxFinalAck){
                    // Received Direct Tx and so stop extra ack
                    // Send immediate wuTimeout to trigger EV_WU_TIMEOUT
                    scheduleAt(simTime(), wuTimeout);
                }
                else if(destinationAckPersistance ||
                        relayDiceRoll<candiateRelayContentionProbability){
                    // Continue to contend for packet
                    rxAckRound++;
                    activeBackoff = new CSMATxRemainderReciprocalBackoff(this,
                            activeRadio,
                            energyStorage,
                            J(0), ackTxWaitDuration, minimumContentionWindow);
                    activeBackoff->delayCarrierSense( uniform(0, initialContentionDuration) );
                }
                else{
                    // Drop out of forwarder contention
                    PacketDropDetails details;
                    details.setReason(PacketDropReason::DUPLICATE_DETECTED);
                    dropCurrentRxFrame(details);
                    // Send immediate wuTimeout to trigger EV_WU_TIMEOUT
                    scheduleAt(simTime(), wuTimeout);
                    EV_DEBUG  << "Detected other relay so discarding packet" << endl;
                }
            }
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
        // Only count coincidental ack in the first round to reduce double counting
        if(rxAckRound<=1){
            EncounterDetails details;
            details.setEncountered(incomingMacData->getTransmitterAddress());
            details.setCurrentEqDC(incomingMacData->getExpectedCostInd());
            emit(coincidentalEncounterSignal, 1.0, &details);
        }
        delete incomingFrame;
    }
    else{
        updateMacState(S_RECEIVE);
    }
}

void WakeUpMacLayer::stepTxSM(const t_mac_event& event, cMessage* const msg) {
    // Needed for S_TRANSMIT backoff in switch
    auto backoffResult = CSMATxBackoffBase::BO_WAIT;

    switch (txState){
    case TX_WAKEUP_WAIT:
        if(activeBackoff != nullptr){
            backoffResult = stepBackoffSM(event);
        }
        if(backoffResult == CSMATxBackoffBase::BO_FINISHED){
            // TODO: Change this to a short WU packet
            cMessage* const currentTxWakeUp = check_and_cast<cMessage*>(buildWakeUp(currentTxFrame, txInProgressTries));
            send(currentTxWakeUp, wakeUpRadioOutGateId);
            txInProgressTries++;
            updateTxState(TX_WAKEUP);
            delete activeBackoff;
            activeBackoff = nullptr;
            EV_DEBUG << "TX SM in TX_WAKEUP_WAIT";
        }
        break;
    case TX_WAKEUP:
        if(event == EV_TX_END){
            // Wake-up transmission has ended, start wait backoff for neighbors to wake-up
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateTxState(TX_DATA_WAIT);
            scheduleAt(simTime() + txWakeUpWaitDuration, wakeUpBackoffTimer);
            EV_DEBUG << "TX SM: TX_WAKEUP_WAIT --> TX_DATA_WAIT";
            // Reset statistic variable counting ack rounds (from transmitter perspective)
            acknowledgmentRound = 0;
        }
        break;
    case TX_DATA_WAIT:
        if(activeBackoff != nullptr){
            backoffResult = stepBackoffSM(event);
        }
        else if(event==EV_WAKEUP_BACKOFF){
            // use activeBackoff for backoff state machine
            ASSERT(activeBackoff == nullptr);
            activeBackoff = new CSMATxUniformBackoff(this, activeRadio, energyStorage,
                    J(0.0), 0.0, ackWaitDuration/3);
            if(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER){
                activeBackoff->startTxOrBackoff();
            }
            else{
                activeBackoff->startCold();
            }
        }
        if(backoffResult == CSMATxBackoffBase::BO_FINISHED){
            Packet* dataFrame = currentTxFrame->dup();
            if(datagramPostRoutingHook(dataFrame)!=INetfilter::IHook::Result::ACCEPT){
                EV_ERROR << "Aborted transmission of data is unimplemented." << endl;
                // Taken from TX_END
                changeActiveRadio(wakeUpRadio);
                wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                scheduleAt(simTime() + SimTime(1, SimTimeUnit::SIMTIME_S), replenishmentTimer);
                updateMacState(S_IDLE);
                updateTxState(TX_WAKEUP_WAIT);
                break;
            }
            encapsulate(dataFrame);
            sendDown(dataFrame);
            updateTxState(TX_DATA);
        }
        break;
    case TX_DATA:
    case TX_ACK_WAIT:
        stepTxAckProcess(event, msg);
        break;
    case TX_END:
        // End transmission by turning the radio off and start listening on wake-up radio
        dataMinExpectedCost = EqDC(25.5);
        changeActiveRadio(wakeUpRadio);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        scheduleAt(simTime() + SimTime(1, SimTimeUnit::SIMTIME_S), replenishmentTimer);
        updateMacState(S_IDLE);
        updateTxState(TX_WAKEUP_WAIT);
        emit(transmissionEndedSignal, true);
        if(txInProgressForwarders<=0){
            PacketDropDetails details;
            // This reason could also justifiably be LIFETIME_EXPIRED
            details.setReason(PacketDropReason::NO_ROUTE_FOUND);
            dropCurrentTxFrame(details);
        }
        else {
            deleteCurrentTxFrame();
            emit(transmissionTriesSignal, txInProgressTries);
        }
        break;
    default:
        EV_DEBUG << "Unhandled TX State. Return to idle" << endl;
        updateTxState(TX_WAKEUP_WAIT);
    }
}

void WakeUpMacLayer::stepTxAckProcess(const t_mac_event& event, cMessage * const msg) {
    if(event == EV_TX_END){
        delete activeBackoff;
        activeBackoff = nullptr;
        //reset confirmed forwarders count
        acknowledgedForwarders = 0;
        // Increase acknowledgment round value
        acknowledgmentRound++;
        // Schedule acknowledgement wait timeout
        scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
        updateMacState(S_TRANSMIT);
        updateTxState(TX_ACK_WAIT);
        dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);


        if (skipDirectTxFinalAck && dataMinExpectedCost == EqDC(0)) {
            // Only the final dest will Ack when expectedCost=0 (A DirectTx)
            // therefore do not retransmit dataPacketAgain
            // Even if destination does not Ack again.
            // This is the same as a broadcast transmission, so tag it as such
            currentTxFrame->addTagIfAbsent<EqDCBroadcast>();

            //Reduce time spent waiting for ack
            cancelEvent(ackBackoffTimer);
            scheduleAt(simTime() + initialContentionDuration, ackBackoffTimer);
        }
    }
    else if(event == EV_DATA_RECEIVED){
        EV_DEBUG << "Data Ack Received";
        auto receivedData = check_and_cast<Packet* >(msg);
        auto receivedAck = receivedData->popAtFront<WakeUpGram>();
        updateMacState(S_TRANSMIT);
        if(receivedAck->getType() == WU_ACK){
            EncounterDetails details;
            details.setEncountered(receivedAck->getTransmitterAddress());
            details.setCurrentEqDC(receivedAck->getExpectedCostInd());
            emit(expectedEncounterSignal, 1.0/acknowledgmentRound, &details);
            // TODO: Update neighbors and check source and dest address match
            // count first few ack
            acknowledgedForwarders++;
            if(acknowledgedForwarders>=maxForwarders){
                // Skip listening for any more and send data again to reduce forwarders
                updateTxState(TX_DATA);
                scheduleAt(simTime(), wakeUpBackoffTimer);
                cancelEvent(ackBackoffTimer);
            }
            else{
                updateTxState(TX_ACK_WAIT);
            }
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
            EncounterDetails details;
            details.setEncountered(receivedAck->getTransmitterAddress());
            details.setCurrentEqDC(receivedAck->getExpectedCostInd());
            emit(coincidentalEncounterSignal, 2.0, &details);
            updateTxState(TX_ACK_WAIT);
            EV_DEBUG <<  "Discarding overheard data as busy transmitting" << endl;
            delete receivedData;
        }
    }
    else if(event == EV_ACK_TIMEOUT){
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
            updateTxState(TX_END);
            updateMacState(S_TRANSMIT);
            //The Radio Receive->Sleep triggers next SM transition
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        }
        else if( activeRadio->getReceptionState() != IRadio::RECEPTION_STATE_IDLE ){
            // Data, possibly ACK or contending wake-up still in progress
            scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
        }
        else if(supplementaryForwarders>0){
            // Go straight to immediate data retransmission to reduce forwarders
            updateTxState(TX_DATA_WAIT);
            scheduleAt(simTime(), wakeUpBackoffTimer);
            acknowledgmentRound++;
            updateMacState(S_TRANSMIT);
        }
        else{
            txInProgressForwarders = txInProgressForwarders+acknowledgedForwarders; // TODO: Check forwarders uniqueness
            emit(ackContentionRoundsSignal, acknowledgmentRound);
            if(txInProgressForwarders<requiredForwarders && txInProgressTries<maxWakeUpTries){
                // Try transmitting wake-up again after standard ack backoff
                scheduleAt(simTime() + ackWaitDuration, ackBackoffTimer);
                // Break into "transitionToIdle()" (see TX_END)
                dataMinExpectedCost = EqDC(25.5);
                changeActiveRadio(wakeUpRadio);
                wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                scheduleAt(simTime(), replenishmentTimer);
                updateMacState(S_IDLE);
                updateTxState(TX_WAKEUP_WAIT);
            }
            else{
                updateTxState(TX_END);
                updateMacState(S_TRANSMIT);
                //The Radio Receive->Sleep triggers next SM transition
                dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            }
        }
    }
}

void WakeUpMacLayer::stepWuSM(const t_mac_event& event, cMessage * const msg) {
    wuStateChange = false;
    switch(wuState){
    case WU_IDLE:
        if(event==EV_WU_START){
            changeActiveRadio(dataRadio);
            dataRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            emit(receptionStartedSignal, true);
            updateWuState(WU_APPROVE_WAIT);
            scheduleAt(simTime() + wuApproveResponseLimit, wuTimeout);
            Packet* wuPkt = check_and_cast<Packet*>(msg);
            acceptDataEqDCThreshold = EqDC(25.5);
            auto wakeUpHeader = wuPkt->peekAtFront<WakeUpGram>();
            EncounterDetails details;
            details.setEncountered(wakeUpHeader->getTransmitterAddress());
            details.setCurrentEqDC(wakeUpHeader->getExpectedCostInd());
            emit(coincidentalEncounterSignal, 2.0, &details);
            queryWakeupRequest(wuPkt);
        }
        break;
    case WU_APPROVE_WAIT:
        if(event==EV_WU_APPROVE){
            updateMacState(S_WAKEUP_LSN);
            updateWuState(WU_WAKEUP_WAIT);
            rxAckRound = 0;
            cancelEvent(wuTimeout);
            // Cancel transmit packet backoff till receive is done
            cancelEvent(ackBackoffTimer);
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
            scheduleAt(simTime(), replenishmentTimer);
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
            emit(receptionDroppedSignal, true);
            updateWuState(WU_IDLE);
            scheduleAt(simTime(), replenishmentTimer);
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

void WakeUpMacLayer::configureInterfaceEntry() {
    // generate a link-layer address to be used as interface token for IPv6
    auto lengthPrototype = makeShared<WakeUpDatagram>();
    const B interfaceMtu = phyMtu-B(lengthPrototype->getChunkLength());
    ASSERT2(interfaceMtu >= B(80), "The interface MTU available to the net layer is too small (under 80 bytes)");
    interfaceEntry->setMtu(interfaceMtu.get());
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setPointToPoint(false);
}

void WakeUpMacLayer::cancelAllTimers()
{
    cancelEvent(wakeUpBackoffTimer);
    cancelEvent(ackBackoffTimer);
    cancelEvent(wuTimeout);
    cancelEvent(replenishmentTimer);
}

void WakeUpMacLayer::deleteAllTimers(){
    delete wakeUpBackoffTimer;
    delete ackBackoffTimer;
    delete wuTimeout;
    delete replenishmentTimer;
}

WakeUpMacLayer::~WakeUpMacLayer() {
    cancelAllTimers();
    deleteAllTimers();
}

void WakeUpMacLayer::changeActiveRadio(physicallayer::IRadio* const newActiveRadio) {
    if(activeRadio->getId() != newActiveRadio->getId()){ // TODO: Check ID?
        activeRadio->setRadioMode(IRadio::RADIO_MODE_OFF);
        activeRadio = newActiveRadio;
        transmissionState = activeRadio->getTransmissionState();
        receptionState = activeRadio->getReceptionState();
    }
}

bool WakeUpMacLayer::setupTransmission() {
    //Cancel transmission timers
    cancelEvent(wakeUpBackoffTimer);
    cancelEvent(ackBackoffTimer);
    cancelEvent(wuTimeout);
    cancelEvent(replenishmentTimer);
    //Reset progress counters
    txInProgressForwarders = 0;

    if(currentTxFrame!=nullptr){
        PacketDropDetails details;
        details.setReason(PacketDropReason::QUEUE_OVERFLOW);
        dropCurrentTxFrame(details);
    }
    popTxQueue();
    if(datagramLocalOutHook(currentTxFrame)!=INetfilter::IHook::Result::ACCEPT){
        updateMacState(S_IDLE);
        return false;
    }

    activeBackoff = new CSMATxUniformBackoff(this, activeRadio, energyStorage, transmissionStartMinEnergy,
            0, txWakeUpWaitDuration);
    const simtime_t delayIfBusy = txWakeUpWaitDuration + dataListeningDuration;
    if (activeBackoff->startTxOrDelay(delayIfBusy)) {
        // Backoff or transition to listening has started
        return true;
    }
    else{
        return false;
    }
}

void WakeUpMacLayer::dropCurrentRxFrame(PacketDropDetails& details)
{
    emit(packetDroppedSignal, currentRxFrame, &details);
    emit(receptionDroppedSignal, true);
    delete currentRxFrame;
    currentRxFrame = nullptr;
}

void WakeUpMacLayer::completePacketReception()
{
    // The receiving has timed out, if packet is received process
    if (currentRxFrame != nullptr) {
        Packet* pkt = dynamic_cast<Packet*>(currentRxFrame);
        decapsulate(pkt);
        pkt->removeTagIfPresent<EqDCReq>();
        pkt->removeTagIfPresent<EqDCInd>();
        pkt->trim();
        if(datagramLocalInHook(pkt)!=IHook::Result::ACCEPT){
            EV_ERROR << "Aborted reception of data is unimplemented" << endl;
        }
        else{
            sendUp(pkt);
        }
        currentRxFrame = nullptr;
        emit(receptionEndedSignal, true);
    }
}

void WakeUpMacLayer::queryWakeupRequest(Packet* wakeUp) {
    const auto header = wakeUp->peekAtFront<WakeUpGram>();
    if(header->getType()!=WakeUpGramType::WU_BEACON){
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

void WakeUpMacLayer::encapsulate(Packet* const pkt) const{ // From CsmaCaMac
    auto macHeader = makeShared<WakeUpDatagram>();
    setBeaconFieldsFromTags(pkt, macHeader);
    const auto upwardsTag = pkt->findTag<EqDCUpwards>();
    if(upwardsTag != nullptr){
        macHeader->setUpwards(upwardsTag->isUpwards());
    }

    pkt->insertAtFront(macHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
}

Packet* WakeUpMacLayer::buildAck(const Packet* receivedFrame) const{
    auto receivedMacData = receivedFrame->peekAtFront<WakeUpGram>();
    auto ackPacket = makeShared<WakeUpAck>();
    ackPacket->setTransmitterAddress(interfaceEntry->getMacAddress());
    ackPacket->setReceiverAddress(receivedMacData->getTransmitterAddress());
    // Look for EqDCInd tag to send info in response
    auto costIndTag = receivedFrame->findTag<EqDCInd>();
    if(costIndTag!=nullptr)
        ackPacket->setExpectedCostInd(costIndTag->getEqDC());
    else
        cRuntimeError("WakeUpMacLayer must respond with a cost");
    auto frame = new Packet("CsmaAck");
    frame->insertAtFront(ackPacket);
    frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
    return frame;
}

void WakeUpMacLayer::setBeaconFieldsFromTags(const Packet* subject,
        const inet::Ptr<WakeUpBeacon>& wuHeader) const
{
    const auto equivalentDCTag = subject->findTag<EqDCReq>();
    const auto equivalentDCInd = subject->findTag<EqDCInd>();
    // Default min is 255, can be updated when ack received from dest
    ExpectedCost minExpectedCost = dataMinExpectedCost;
    if (equivalentDCTag != nullptr && equivalentDCTag->getEqDC() < minExpectedCost) {
        minExpectedCost = equivalentDCTag->getEqDC();
        ASSERT(minExpectedCost >= ExpectedCost(0));
    }
    wuHeader->setMinExpectedCost(minExpectedCost);
    wuHeader->setExpectedCostInd(equivalentDCInd->getEqDC());
    wuHeader->setTransmitterAddress(interfaceEntry->getMacAddress());
    wuHeader->setReceiverAddress(subject->getTag<MacAddressReq>()->getDestAddress());
}

Packet* WakeUpMacLayer::buildWakeUp(const Packet *subject, const int retryCount) const{
    auto wuHeader = makeShared<WakeUpBeacon>();
    setBeaconFieldsFromTags(subject, wuHeader);
    auto frame = new Packet("wake-up", wuHeader);
    frame->addTag<PacketProtocolTag>()->setProtocol(&WuMacProtocol);
    return frame;
}

void WakeUpMacLayer::decapsulate(Packet* const pkt) const{ // From CsmaCaMac
    auto macHeader = pkt->popAtFront<WakeUpDatagram>();
    auto addressInd = pkt->addTagIfAbsent<MacAddressInd>();
    addressInd->setSrcAddress(macHeader->getTransmitterAddress());
    addressInd->setDestAddress(macHeader->getReceiverAddress());
    auto payloadProtocol = ProtocolGroup::ipprotocol.getProtocol(245);
    pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
}

void WakeUpMacLayer::handleStartOperation(LifecycleOperation *operation) {
    // complete unfinished reception
    completePacketReception();
    updateMacState(S_IDLE);
    updateWuState(WU_IDLE);
    updateTxState(TX_WAKEUP_WAIT);
    activeRadio = wakeUpRadio;
    // Will trigger sending of any messages in txQueue
    wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    transmissionState = activeRadio->getTransmissionState();
    receptionState = activeRadio->getReceptionState();
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
            emit(transmissionEndedSignal, true);
        }
    }
    else if(currentRxFrame != nullptr){
        // Check if in ack backoff period and if waiting to send ack
        if(wuTimeout->isScheduled() && ackBackoffTimer->isScheduled()){
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
    updateMacState(S_IDLE);
    interfaceEntry->setCarrier(false);
    interfaceEntry->setState(InterfaceEntry::State::DOWN);
}
