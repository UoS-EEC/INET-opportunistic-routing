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
using namespace oppostack;
using namespace inet;
using namespace inet::units::values;

Define_Module(WuMacEnergyMonitor);

/**
 * Energy consumption statistics
 */
simsignal_t WuMacEnergyMonitor::receptionConsumptionSignal = cComponent::registerSignal("receptionConsumption");
simsignal_t WuMacEnergyMonitor::falseWakeUpConsumptionSignal = cComponent::registerSignal("falseWakeUpConsumption");
simsignal_t WuMacEnergyMonitor::transmissionConsumptionSignal = cComponent::registerSignal("transmissionConsumption");
simsignal_t WuMacEnergyMonitor::unknownConsumptionSignal = cComponent::registerSignal("unknownConsumption");


/**
 * Mode watching signals
 */
simsignal_t WuMacEnergyMonitor::wakeUpModeStartSignal = cComponent::registerSignal("wakeUpModeStart");
simsignal_t WuMacEnergyMonitor::receptionEndedSignal = cComponent::registerSignal("receptionEnded");
simsignal_t WuMacEnergyMonitor::falseWakeUpEndedSignal = cComponent::registerSignal("falseWakeUpEnded");
simsignal_t WuMacEnergyMonitor::transmissionModeStartSignal = cComponent::registerSignal("transmissionModeStart");
simsignal_t WuMacEnergyMonitor::transmissionEndedSignal = cComponent::registerSignal("transmissionEnded");

void WuMacEnergyMonitor::initialize(int stage)
{
    OperationalBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        const char* energyStoragePath = par("energyStorage");
        cModule* storageModule = getModuleByPath(energyStoragePath);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        cModule* macModule = getParentModule();
        macModule->subscribe(wakeUpModeStartSignal, this);
        macModule->subscribe(receptionEndedSignal, this);
        macModule->subscribe(falseWakeUpEndedSignal, this);
        macModule->subscribe(transmissionModeStartSignal, this);
        macModule->subscribe(transmissionEndedSignal, this);
    }
}

void WuMacEnergyMonitor::handleStartOperation(inet::LifecycleOperation* operation)
{
    if(inProgress!=SIMSIGNAL_NULL)resumeMonitoring();
}

void WuMacEnergyMonitor::resumeMonitoring()
{
    storedEnergyStartValue = energyStorage->getResidualEnergyCapacity();
    initialEnergyGeneration = energyStorage->getTotalPowerGeneration();
    storedEnergyStartTime = simTime();
}

void WuMacEnergyMonitor::startMonitoring(simsignal_t startSignal)
{
    if(storedEnergyStartTime == 0 || storedEnergyStartValue != J(0) || initialEnergyGeneration == W(0)){
        // finish energy consumption monitoring was not called, catch and emit signal and warning
        EV_WARN << "Unhandled Energy Consumption Monitoring" << endl;
    }
    inProgress = startSignal;
    pausedIntermediateConsumption = J(0);
    resumeMonitoring();
}

void WuMacEnergyMonitor::finishMonitoring(simsignal_t collectionSignal)
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



void WuMacEnergyMonitor::handleStopOperation(inet::LifecycleOperation* operation)
{
    if(inProgress!=SIMSIGNAL_NULL)pauseMonitoring();
}

void WuMacEnergyMonitor::handleCrashOperation(inet::LifecycleOperation* operation)
{
    finishMonitoring(SIMSIGNAL_NULL);
}

void oppostack::WuMacEnergyMonitor::pauseMonitoring()
{
    pausedIntermediateConsumption = pausedIntermediateConsumption + calculateDeltaEnergyConsumption();
    storedEnergyStartValue = J(0);
    initialEnergyGeneration = W(0);
    storedEnergyStartTime = 0;
}

void WuMacEnergyMonitor::receiveSignal(cComponent* source, simsignal_t signalID, bool b, cObject* details)
{
    if(signalID != wakeUpModeStartSignal &&
            signalID != receptionEndedSignal &&
            signalID != falseWakeUpEndedSignal &&
            signalID != transmissionModeStartSignal &&
            signalID != transmissionEndedSignal ){
        return;
    }
    else if(inProgress == signalID){
        // Ignore as monitoring this mode already
    }
    else if(signalID == receptionEndedSignal || signalID == falseWakeUpEndedSignal){
        if(inProgress == wakeUpModeStartSignal){
            if(signalID == receptionEndedSignal){
                finishMonitoring(receptionConsumptionSignal);
            }
            else{
                finishMonitoring(falseWakeUpConsumptionSignal);
            }
        }
        else if(inProgress!=SIMSIGNAL_NULL){
            finishMonitoring(unknownConsumptionSignal);
        }
    }
    else if(signalID == transmissionEndedSignal){
        if(inProgress == transmissionModeStartSignal){
            finishMonitoring(transmissionConsumptionSignal);
        }
        else if(inProgress!=SIMSIGNAL_NULL){
            finishMonitoring(unknownConsumptionSignal);
        }
    }
    else if(inProgress==SIMSIGNAL_NULL){
        ASSERT(signalID == wakeUpModeStartSignal || signalID == transmissionModeStartSignal);
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

void oppostack::WuMacEnergyMonitor::handleMessageWhenUp(cMessage* msg)
{
}
