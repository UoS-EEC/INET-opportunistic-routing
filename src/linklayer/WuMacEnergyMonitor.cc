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

#include "common/oppDefs.h"
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include <inet/physicallayer/base/packetlevel/FlatTransmitterBase.h>
#include "WuMacEnergyMonitor.h"
#include "IObservableMac.h"
#include "ORWGram_m.h"

using namespace oppostack;
Define_Module(WuMacEnergyMonitor);
/**
 * Energy consumption statistics
 */
simsignal_t WuMacEnergyMonitor::receptionConsumptionSignal = cComponent::registerSignal("receptionConsumption");
simsignal_t WuMacEnergyMonitor::falseWakeUpConsumptionSignal = cComponent::registerSignal("falseWakeUpConsumption");
simsignal_t WuMacEnergyMonitor::transmissionConsumptionSignal = cComponent::registerSignal("transmissionConsumption");
simsignal_t WuMacEnergyMonitor::unknownConsumptionSignal = cComponent::registerSignal("unknownConsumption");

void WuMacEnergyMonitor::initialize(int stage)
{
    OperationalBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        macModule = getParentModule();

        cModule* storageModule = getCModuleFromPar(macModule->par("energyStorage"), macModule);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        macModule->subscribe(IObservableMac::receptionStartedSignal, this);
        macModule->subscribe(IObservableMac::receptionEndedSignal, this);
        macModule->subscribe(IObservableMac::receptionDroppedSignal, this);
        macModule->subscribe(IObservableMac::transmissionStartedSignal, this);
        macModule->subscribe(IObservableMac::transmissionEndedSignal, this);
    }
}

void WuMacEnergyMonitor::handleStartOperation(inet::LifecycleOperation* const operation)
{
    if(inProgress!=SIMSIGNAL_NULL)resumeMonitoring();
}

void WuMacEnergyMonitor::resumeMonitoring()
{
    storedEnergyStartValue = energyStorage->getResidualEnergyCapacity();
    initialEnergyGeneration = energyStorage->getTotalPowerGeneration();
    storedEnergyStartTime = simTime();
}

void WuMacEnergyMonitor::handleStopOperation(inet::LifecycleOperation* const operation)
{
    if(inProgress!=SIMSIGNAL_NULL)pauseMonitoring();
}

void WuMacEnergyMonitor::pauseMonitoring()
{
    pausedIntermediateConsumption = pausedIntermediateConsumption + calculateDeltaEnergyConsumption();
    storedEnergyStartValue = J(0);
    initialEnergyGeneration = W(0);
    storedEnergyStartTime = 0;
}

void WuMacEnergyMonitor::startMonitoring(simsignal_t const startSignal)
{
    if(storedEnergyStartTime == 0 || storedEnergyStartValue != J(0) || initialEnergyGeneration == W(0)){
        // finish energy consumption monitoring was not called, catch and emit signal and warning
        EV_WARN << "Unhandled Energy Consumption Monitoring" << endl;
    }
    inProgress = startSignal;
    pausedIntermediateConsumption = J(0);
    resumeMonitoring();
}

void WuMacEnergyMonitor::finishMonitoring(simsignal_t const collectionSignal)
{
    ASSERT(collectionSignal == receptionConsumptionSignal ||
            collectionSignal == falseWakeUpConsumptionSignal ||
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




void WuMacEnergyMonitor::handleCrashOperation(inet::LifecycleOperation* const operation)
{
    finishMonitoring(SIMSIGNAL_NULL);
}

void WuMacEnergyMonitor::receiveSignal(cComponent* const source, simsignal_t const signalID, bool const b, cObject* const details)
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
            finishMonitoring(falseWakeUpConsumptionSignal);
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

const J WuMacEnergyMonitor::calculateDeltaEnergyConsumption() const
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

const inet::J oppostack::WuMacEnergyMonitor::calcTxAndAckEstConsumption(inet::b packetLength) const
{// TODO: Use MacEstimateCostProcess on InterfaceEntry
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

bool oppostack::WuMacEnergyMonitor::isMatchingEndedSignal(const simsignal_t endedSignal)
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
