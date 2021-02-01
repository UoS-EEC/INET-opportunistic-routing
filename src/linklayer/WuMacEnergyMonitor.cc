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

#include <inet/power/contract/IEpEnergyStorage.h>
#include "WuMacEnergyMonitor.h"
#include <inet/common/ModuleAccess.h>
#include "WakeUpMacLayer.h"

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
        cModule* macModule = getParentModule();

        cModule* storageModule = getModuleFromPar<cModule>(macModule->par("energyStorage"), macModule);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        macModule->subscribe(WakeUpMacLayer::wakeUpModeStartSignal, this);
        macModule->subscribe(WakeUpMacLayer::receptionEndedSignal, this);
        macModule->subscribe(WakeUpMacLayer::falseWakeUpEndedSignal, this);
        macModule->subscribe(WakeUpMacLayer::transmissionModeStartSignal, this);
        macModule->subscribe(WakeUpMacLayer::transmissionEndedSignal, this);
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
    else if(inProgress == WakeUpMacLayer::wakeUpModeStartSignal &&
            (signalID == WakeUpMacLayer::receptionEndedSignal || signalID == WakeUpMacLayer::falseWakeUpEndedSignal) ){
        // reception and wakeUpEnded signals can only end wakeUpModeStarted signal, otherwise it goes to unknown
        if(signalID == WakeUpMacLayer::receptionEndedSignal){
            finishMonitoring(receptionConsumptionSignal);
        }
        else{
            finishMonitoring(falseWakeUpConsumptionSignal);
        }
    }
    else if(inProgress == WakeUpMacLayer::transmissionModeStartSignal &&
            signalID == WakeUpMacLayer::transmissionEndedSignal){
        finishMonitoring(transmissionConsumptionSignal);
    }
    else if(inProgress==SIMSIGNAL_NULL &&
            signalID == WakeUpMacLayer::wakeUpModeStartSignal || signalID == WakeUpMacLayer::transmissionModeStartSignal){
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
