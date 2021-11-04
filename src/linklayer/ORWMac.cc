/* Copyright (c) 2021, University of Southampton and Contributors.
 * Some elements taken from (Ieee802154Mac authors Jerome Rousselot,
 * Marcel Steine, Amre El-Hoiydi, Marc Loebbers, Yosia Hadisusanto)
 * (C) 2007-2009 CSEM SA
 * (C) 2009 T.U. Eindhoven
 * (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ORWMac.h"
#include <inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h>
#include <inet/physicallayer/wireless/common/backgroundnoise/IsotropicScalarBackgroundNoise.h>
#include <inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h>
#include <inet/common/ModuleAccess.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/common/ProtocolGroup.h>
#include "common/EqDCTag_m.h"
#include "common/EncounterDetails_m.h"

using namespace oppostack;
using namespace inet;
using inet::physicallayer::IRadio;
using physicallayer::FlatTransmitterBase;

Define_Module(ORWMac);

void ORWMac::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // create timer messages
        ackBackoffTimer = new cMessage("ack wait timer");
        receiveTimeout = new cMessage("wake-up wait timer");
        replenishmentTimer = new cMessage("replenishment check timeout");
        transmitStartDelay = new cMessage("transmit backoff");

        //load parameters
        transmissionStartMinEnergy = J(par("transmissionStartMinEnergy"));
        dataListeningDuration = par("dataListeningDuration");
        ackWaitDuration = par("ackWaitDuration");
        initialContentionDuration = ackWaitDuration/3;
        candiateRelayContentionProbability = par("candiateRelayContentionProbability");

        // link direct module interfaces
        const char* energyStoragePath = par("energyStorage");
        cModule* storageModule = getModuleByPath(energyStoragePath);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        const char *radioModulePath = par("dataRadioModule");
        cModule *radioModule = getModuleByPath(radioModulePath);
        dataRadio = check_and_cast<IRadio *>(radioModule);

        const char* networkNodePath = par("networkNode");
        networkNode = getModuleByPath(networkNodePath);

        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));

        // Parameter validation

        /*
         * Warning of noise in radio medium exceeding energy detection threshold
         */
        auto medium = getModuleFromPar<physicallayer::IRadioMedium>(radioModule->par("radioMediumModule"), radioModule);
        auto noiseModel = medium->getBackgroundNoise();
        auto scalarNoiseModel = check_and_cast_nullable<const physicallayer::IsotropicScalarBackgroundNoise*>(noiseModel);
        auto dataReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(dataRadio->getReceiver());
        if(scalarNoiseModel && dataReceiverModel)
            if(scalarNoiseModel->getPower() > dataReceiverModel->getEnergyDetection())
                throw cRuntimeError("Background noise power is greater than data radio energy detection threshold. Radio will always be \"Busy\"");

        /*
         * Calculation of invalid parameter combinations at the receiver
         * - MAC Layer requires tight timing during a communication negotiation
         * - Incorrect timing increases the collision and duplication probability
         * After the data reception contention to receive and forward occurs
         * The physical MTU is limited by the time spent dataListening after sending an ACK
         * If an ACK is sent at the start of the ACK period, the data may not be fully
         * received until remainder of ack period + 1/5(ack period)
         */
        auto lengthPrototype = makeShared<ORWDatagram>();
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

        // Initial state handled by handleStartOperation()
    }
}

void ORWMac::configureNetworkInterface() {
    // generate a link-layer address to be used as interface token for IPv6
    auto lengthPrototype = makeShared<ORWDatagram>();
    const B interfaceMtu = phyMtu-B(lengthPrototype->getChunkLength());
    ASSERT2(interfaceMtu >= B(80), "The interface MTU available to the net layer is too small (under 80 bytes)");
    networkInterface->setMtu(interfaceMtu.get());
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(true);
}

void ORWMac::cancelAllTimers() {
    if (ackBackoffTimer == nullptr
            && receiveTimeout == nullptr
            && replenishmentTimer == nullptr
            && transmitStartDelay == nullptr)
        return;
    cancelEvent(ackBackoffTimer);
    cancelEvent(receiveTimeout);
    cancelEvent(replenishmentTimer);
    cancelEvent(transmitStartDelay);
}

void ORWMac::deleteAllTimers() {
    delete ackBackoffTimer;
    delete receiveTimeout;
    delete replenishmentTimer;
    delete transmitStartDelay;
}

ORWMac::~ORWMac() {
    delete currentRxFrame;
    cancelAllTimers();
    deleteAllTimers();
}


void ORWMac::handleUpperPacket(Packet* const packet) {
    // step Mac state machine
    // Make Mac owned copy of message
    auto addressRequest =packet->addTagIfAbsent<MacAddressReq>();
    if(addressRequest->getDestAddress() == MacAddress::UNSPECIFIED_ADDRESS){
        addressRequest->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    }
    if(addressRequest->getDestAddress() == MacAddress::BROADCAST_ADDRESS)
        packet->addTagIfAbsent<EqDCBroadcast>();
    txQueue->enqueuePacket(packet);
    stateProcess(MacEvent::QUEUE_SEND, packet);
}

void ORWMac::handleLowerPacket(Packet* const packet) {
    if(packet->getArrivalGateId() == lowerLayerInGateId){
        EV_DEBUG << "Received  main packet" << endl;
        stateProcess(MacEvent::DATA_RECEIVED, packet);
    }
    else{
        MacProtocolBase::handleLowerCommand(packet);
    }
}

void ORWMac::handleSelfMessage(cMessage *msg) {
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
    else {
        EV_WARN << "Unhandled self message" << endl;
    }
}

void ORWMac::handleRadioSignal(const simsignal_t signalID,
        const intval_t value) {
    if (signalID == IRadio::transmissionStateChangedSignal) {
        // Handle both the data transmission ending and the wake-up transmission ending.
        // They should never happen at the same time, so one variable is enough
        // to manage both
        IRadio::TransmissionState newRadioTransmissionState =
                static_cast<IRadio::TransmissionState>(value);
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING
                && newRadioTransmissionState
                        == IRadio::TRANSMISSION_STATE_IDLE) {
            // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
            auto msg = new cMessage("Transmission over");
            stateProcess(MacEvent::TX_END, msg);
            delete msg;
        } else if (transmissionState == IRadio::TRANSMISSION_STATE_UNDEFINED
                && newRadioTransmissionState
                        == IRadio::TRANSMISSION_STATE_IDLE) {
            // radio has finished switching to startup
            // But handled by radioModeChanged due to signal order bugfix
        } else {
            EV_DEBUG<< "Unhandled transmitter state transition" << endl;
        }
        transmissionState = newRadioTransmissionState;
    } else if (signalID == IRadio::receptionStateChangedSignal) {
        // Handle radio moving to receive mode
        IRadio::ReceptionState newRadioReceptionState =
                static_cast<IRadio::ReceptionState>(value);
        if (receptionState == IRadio::RECEPTION_STATE_UNDEFINED
                && (newRadioReceptionState == IRadio::RECEPTION_STATE_IDLE
                        || newRadioReceptionState == IRadio::RECEPTION_STATE_BUSY)) {
            // radio has finished switching to listening
            auto msg = new cMessage("Reception ready");
            stateProcess(MacEvent::DATA_RX_READY, msg);
            delete msg;
        } else {
            EV_DEBUG << "Unhandled reception state transition" << endl;
        }
        receptionState = newRadioReceptionState;
    } else if (signalID == IRadio::radioModeChangedSignal) {
        // Handle radio switching into sleep mode and into transmitter mode, since radio mode fired last for transmitter mode
        IRadio::RadioMode newRadioMode = static_cast<IRadio::RadioMode>(value);
        if (newRadioMode == IRadio::RADIO_MODE_SLEEP) {
            auto msg = new cMessage("Radio switched to sleep");
            stateProcess(MacEvent::DATA_RX_IDLE, msg);
            delete msg;
        } else if (newRadioMode == IRadio::RADIO_MODE_TRANSMITTER) {
            auto msg = new cMessage("Transmitter Started");
            stateProcess(MacEvent::TX_READY, msg);
            delete msg;
        }
        receptionState = IRadio::RECEPTION_STATE_UNDEFINED;
        transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;
    }
}

void ORWMac::receiveSignal(cComponent *source, simsignal_t signalID,
        intval_t value, cObject *details) {
    Enter_Method_Silent();
    // Check it is for the active radio
    cComponent* dataRadioComponent = check_and_cast_nullable<cComponent*>(dataRadio);
    if(operationalState == OPERATING && dataRadio && dataRadioComponent == source){
        handleRadioSignal(signalID, value);
    }
}

bool ORWMac::transmissionStartEnergyCheck() const
{
    return energyStorage->getResidualEnergyCapacity() >= transmissionStartMinEnergy;
}

void ORWMac::setupTransmission() {
    //Cancel transmission timers
    //Reset progress counters
    txInProgressForwarders = 0;

    if(currentTxFrame!=nullptr){
        PacketDropDetails details;
        details.setReason(PacketDropReason::QUEUE_OVERFLOW);
        dropCurrentTxFrame(details);
    }
    popTxQueue();
    if(datagramLocalOutHook(currentTxFrame)!=INetfilter::IHook::Result::ACCEPT){
        throw cRuntimeError("Unhandled rejection of packet at transmission setup");
    }
}

void ORWMac::setBeaconFieldsFromTags(const Packet* subject,
        const inet::Ptr<ORWBeacon>& wuHeader) const
{
    const auto equivalentDCTag = subject->findTag<EqDCReq>();
    const auto equivalentDCInd = subject->findTag<EqDCInd>();
    const auto macAddressTag = subject->getTag<MacAddressReq>();
    // Default min is 255, can be updated when ack received from dest
    ExpectedCost minExpectedCost = dataMinExpectedCost;
    if(equivalentDCTag == nullptr && macAddressTag->getDestAddress() != MacAddress::BROADCAST_ADDRESS){
        minExpectedCost = EqDC(0);
    }
    else if (equivalentDCTag != nullptr && equivalentDCTag->getEqDC() < minExpectedCost) {
        minExpectedCost = equivalentDCTag->getEqDC();
        ASSERT(minExpectedCost >= ExpectedCost(0));
    }
    wuHeader->setMinExpectedCost(minExpectedCost);
    wuHeader->setExpectedCostInd(equivalentDCInd->getEqDC());
    wuHeader->setTransmitterAddress(networkInterface->getMacAddress());;
    wuHeader->setReceiverAddress(macAddressTag->getDestAddress());
}

void ORWMac::encapsulate(Packet* const pkt) const{ // From CsmaCaMac
    auto macHeader = makeShared<ORWDatagram>();
    setBeaconFieldsFromTags(pkt, macHeader);
    const auto upwardsTag = pkt->findTag<EqDCUpwards>();
    if(upwardsTag != nullptr){
        macHeader->setUpwards(upwardsTag->isUpwards());
    }

    pkt->insertAtFront(macHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&ORWProtocol);
}

void ORWMac::completePacketTransmission()
{
    emit(ackContentionRoundsSignal, acknowledgmentRound);
    bool sufficientForwarders = txInProgressForwarders >= requiredForwarders
            || currentTxFrame->findTag<EqDCBroadcast>();
    if (sufficientForwarders) {
        deleteCurrentTxFrame();
        emit(transmissionEndedSignal, true);
    }
    else if (txInProgressTries >= maxTxTries) {
        emit(linkBrokenSignal, currentTxFrame);
        // not sufficient forwarders and retry limit reached and
        PacketDropDetails details;
        // This reason could also justifiably be LIFETIME_EXPIRED
        details.setReason(PacketDropReason::NO_ROUTE_FOUND);
        dropCurrentTxFrame(details);
        emit(transmissionEndedSignal, true);
    }
}

void ORWMac::decapsulate(Packet* const pkt) const{ // From CsmaCaMac
    auto macHeader = pkt->popAtFront<ORWDatagram>();
    auto addressInd = pkt->addTagIfAbsent<MacAddressInd>();
    addressInd->setSrcAddress(macHeader->getTransmitterAddress());
    addressInd->setDestAddress(macHeader->getReceiverAddress());
    auto payloadProtocol = ProtocolGroup::ipprotocol.getProtocol(245);
    pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
}

Packet* ORWMac::buildAck(const Packet* receivedFrame) const{
    auto receivedMacData = receivedFrame->peekAtFront<ORWGram>();
    auto ackPacket = makeShared<ORWAck>();
    ackPacket->setTransmitterAddress(networkInterface->getMacAddress());
    ackPacket->setReceiverAddress(receivedMacData->getTransmitterAddress());
    // Look for EqDCInd tag to send info in response
    auto costIndTag = receivedFrame->findTag<EqDCInd>();
    if(costIndTag!=nullptr)
        ackPacket->setExpectedCostInd(costIndTag->getEqDC());
    else
        cRuntimeError("ORWMac must respond with a cost");
    auto frame = new Packet("ORWAck");
    frame->insertAtFront(ackPacket);
    frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&ORWProtocol);
    return frame;
}

void ORWMac::dropCurrentRxFrame(PacketDropDetails& details)
{
    emit(packetDroppedSignal, currentRxFrame, &details);
    emit(receptionDroppedSignal, true);
    delete currentRxFrame;
    currentRxFrame = nullptr;
}

void ORWMac::emitEncounterFromWeightedPacket(
        omnetpp::simsignal_t signal, double weight, const inet::Packet *data) {
    auto receivedHeader = data->peekAtFront<ORWGram>();
    EncounterDetails details;
    details.setEncountered(receivedHeader->getTransmitterAddress());
    details.setCurrentEqDC(receivedHeader->getExpectedCostInd());
    emit(signal, weight, &details);
}

void ORWMac::handleOverheardAckInDataReceiveState(const Packet * const msg){
    // Overheard Ack from neighbor
    EV_WARN << "Overheard Ack from neighbor is it worth sending own ACK?" << endl;
    // Leave in current mac state which could be MacState::RECEIVE or S_ACK
    // Only count coincidental ack in the first round to reduce double counting
    if(rxAckRound<=1){
        emitEncounterFromWeightedPacket(coincidentalEncounterSignal, 1.0, msg);
    }
}

void ORWMac::completePacketReception()
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

void ORWMac::handleStartOperation(LifecycleOperation *operation) {
    // complete unfinished reception
    completePacketReception();
    macState = stateListeningEnter();
    networkInterface->setState(NetworkInterface::State::UP);
    networkInterface->setCarrier(true);
}

void ORWMac::handleStopOperation(LifecycleOperation *operation) {
    handleCrashOperation(operation);
}

void ORWMac::handleCrashOperation(LifecycleOperation* const operation) {
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
        if(deferredDuplicateDrop){
            // Complete defer packet drop
            PacketDropDetails details;
            details.setReason(PacketDropReason::DUPLICATE_DETECTED);
            dropCurrentRxFrame(details);
        }
        else if(receiveTimeout->isScheduled() && ackBackoffTimer->isScheduled()){
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
    deferredDuplicateDrop = false;
    // Stop all signals from being interpreted
    networkInterface->setCarrier(false);
    networkInterface->setState(NetworkInterface::State::DOWN);
}
