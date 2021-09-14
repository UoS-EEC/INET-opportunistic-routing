/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "common/oppDefs.h"
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include <inet/physicallayer/base/packetlevel/FlatTransmitterBase.h>
#include "IObservableMac.h"
#include "MacEnergyMonitor.h"
#include "ORWGram_m.h"

using namespace oppostack;
using namespace inet;
Define_Module(MacEnergyMonitor);
/**
 * Energy consumption statistics
 */
simsignal_t MacEnergyMonitor::receptionConsumptionSignal = cComponent::registerSignal("receptionConsumption");
simsignal_t MacEnergyMonitor::falseReceptionConsumptionSignal = cComponent::registerSignal("falseReceptionConsumption");
simsignal_t MacEnergyMonitor::transmissionConsumptionSignal = cComponent::registerSignal("transmissionConsumption");
simsignal_t MacEnergyMonitor::unknownConsumptionSignal = cComponent::registerSignal("unknownConsumption");

void MacEnergyMonitor::initialize(int stage)
{
    OperationalBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        macModule = getParentModule();
        // Check it is the correct type
        check_and_cast<IObservableMac*>(macModule);

        cModule* storageModule = getCModuleFromPar(macModule->par("energyStorage"), macModule);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        macModule->subscribe(IObservableMac::receptionStartedSignal, this);
        macModule->subscribe(IObservableMac::receptionEndedSignal, this);
        macModule->subscribe(IObservableMac::receptionDroppedSignal, this);
        macModule->subscribe(IObservableMac::transmissionStartedSignal, this);
        macModule->subscribe(IObservableMac::transmissionEndedSignal, this);
    }
}

void MacEnergyMonitor::handleStartOperation(inet::LifecycleOperation* const operation)
{
    if(inProgress!=SIMSIGNAL_NULL)resumeMonitoring();
}

void MacEnergyMonitor::resumeMonitoring()
{
    storedEnergyStartValue = energyStorage->getResidualEnergyCapacity();
    initialEnergyGeneration = energyStorage->getTotalPowerGeneration();
    storedEnergyStartTime = simTime();
}

void MacEnergyMonitor::handleStopOperation(inet::LifecycleOperation* const operation)
{
    if(inProgress!=SIMSIGNAL_NULL)pauseMonitoring();
}

void MacEnergyMonitor::pauseMonitoring()
{
    pausedIntermediateConsumption = pausedIntermediateConsumption + calculateDeltaEnergyConsumption();
    storedEnergyStartValue = J(0);
    initialEnergyGeneration = W(0);
    storedEnergyStartTime = 0;
}

void MacEnergyMonitor::startMonitoring(simsignal_t const startSignal)
{
    if(storedEnergyStartTime == 0 || storedEnergyStartValue != J(0) || initialEnergyGeneration == W(0)){
        // finish energy consumption monitoring was not called, catch and emit signal and warning
        EV_WARN << "Unhandled Energy Consumption Monitoring" << endl;
    }
    inProgress = startSignal;
    pausedIntermediateConsumption = J(0);
    resumeMonitoring();
}

void MacEnergyMonitor::finishMonitoring(simsignal_t const collectionSignal)
{
    ASSERT(collectionSignal == receptionConsumptionSignal ||
            collectionSignal == falseReceptionConsumptionSignal ||
            collectionSignal == transmissionConsumptionSignal ||
            collectionSignal == unknownConsumptionSignal);
    // Calculate the energy consumed by approximating energy generation as average over consumption period
    if(storedEnergyStartValue <= J(0) && storedEnergyStartTime <= 0 && initialEnergyGeneration < W(0)){
        EV_WARN << "Energy Monitoring must be started before calling finish" << endl;
    };
    const J calculatedConsumedE = pausedIntermediateConsumption + calculateDeltaEnergyConsumption();
    if(calculatedConsumedE < J(0)){
        EV_WARN << "Cannot calculate task energy consumption" << endl;
    }
    if(collectionSignal != SIMSIGNAL_NULL){
        emit(collectionSignal, calculatedConsumedE.get());
    }

    inProgress = SIMSIGNAL_NULL;
    pausedIntermediateConsumption = J(0);
    storedEnergyStartValue = J(0);
    initialEnergyGeneration = W(0);
    storedEnergyStartTime = 0;
}




void MacEnergyMonitor::handleCrashOperation(inet::LifecycleOperation* const operation)
{
    finishMonitoring(SIMSIGNAL_NULL);
}

void MacEnergyMonitor::receiveSignal(cComponent* const source, simsignal_t const signalID, bool const b, cObject* const details)
{
    if(inProgress == signalID){
        // Ignore as monitoring this mode already
    }
    else if(inProgress == IObservableMac::receptionStartedSignal &&
            isMatchingEndedSignal(signalID) ){
        // reception and wakeUpEnded signals can only end wakeUpModeStarted signal, otherwise it goes to unknown
        if(signalID == IObservableMac::receptionEndedSignal){
            finishMonitoring(receptionConsumptionSignal);
        }
        else{
            finishMonitoring(falseReceptionConsumptionSignal);
        }
    }
    else if(inProgress == IObservableMac::transmissionStartedSignal &&
            isMatchingEndedSignal(signalID) ){
        finishMonitoring(transmissionConsumptionSignal);
    }
    else if(inProgress==SIMSIGNAL_NULL &&
            (signalID == IObservableMac::receptionStartedSignal || signalID == IObservableMac::transmissionStartedSignal) ){
        startMonitoring(signalID);
    }
    else{
        // unknown collection in progress
        finishMonitoring(unknownConsumptionSignal);
    }
}

const J MacEnergyMonitor::calculateDeltaEnergyConsumption() const
{
    const J deltaEnergy = energyStorage->getResidualEnergyCapacity() - storedEnergyStartValue;
    const W averageGeneration = 0.5 * (energyStorage->getTotalPowerGeneration() + initialEnergyGeneration);
    const s deltaTime = s((simTime() - storedEnergyStartTime).dbl());
    const J calculatedConsumedEnergy = -deltaEnergy + averageGeneration * deltaTime;
    if(!(deltaEnergy < J(0) && averageGeneration > W(0) && deltaTime > s(0))){
        EV_WARN << "Cannot calculate task energy consumption" << endl;
        return J(0.0);
    }
    return calculatedConsumedEnergy;
}

const inet::J oppostack::MacEnergyMonitor::calcTxAndAckEstConsumption(inet::b packetLength) const
{// TODO: Use MacEstimateCostProcess on NetworkInterface
    using namespace inet;
    const simtime_t ackWait = macModule->par("ackWaitDuration");
    const cModule* dataRadio = macModule->getParentModule()->getSubmodule("dataRadio");
    if(dataRadio==nullptr){
        throw cRuntimeError("Cannot get dataRadio for energy estimation");
        return J(0.0);
    }
    const auto dummyHeader = ORWDatagram();
    const b headerLength = dummyHeader.getChunkLength();
    if(dataRadio==nullptr){
        throw cRuntimeError("Cannot get radio consumer for energy estimation");
        return J(0.0);
    }
    auto dataTransmitter = check_and_cast<const physicallayer::FlatTransmitterBase*>(check_and_cast<const physicallayer::IRadio* >(dataRadio)->getTransmitter());
    const bps bitrate = bps(dataTransmitter->getBitrate());
    const cModule* energyConsumer = dataRadio->getSubmodule("energyConsumer");
    const s Tau_tx = (packetLength+headerLength)/bitrate;
    const W P_tx = W(energyConsumer->par("transmitterTransmittingPowerConsumption"));
    const s Tau_ackLi = s(ackWait.dbl());
    const W P_ackLi = W(energyConsumer->par("receiverIdlePowerConsumption"));
    const J E_TxAndAckLi = P_tx*Tau_tx + Tau_ackLi*P_ackLi;
    return E_TxAndAckLi;
}

bool oppostack::MacEnergyMonitor::isMatchingEndedSignal(const simsignal_t endedSignal)
{
    if(inProgress == IObservableMac::receptionStartedSignal &&
                (endedSignal == IObservableMac::receptionEndedSignal || endedSignal == IObservableMac::receptionDroppedSignal) ){
        return true;
    }
    else if(inProgress == IObservableMac::transmissionStartedSignal &&
            endedSignal == IObservableMac::transmissionEndedSignal){
        return true;
    }
    return false;
}
