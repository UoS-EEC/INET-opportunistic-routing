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

#ifndef LINKLAYER_WUMACENERGYMONITOR_H_
#define LINKLAYER_WUMACENERGYMONITOR_H_

#include <omnetpp.h>
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/common/Units.h>
#include <inet/common/lifecycle/OperationalBase.h>
#include <inet/common/lifecycle/LifecycleOperation.h>
#include <inet/common/lifecycle/ModuleOperations.h>

using namespace omnetpp;
using namespace inet;
namespace oppostack {

/**
 * Receive Signals from WakeUpMac about starting and stopping of reception or transmission
 * Implemented using Stop and Start operation to pause monitoring when interrupted due to node shutdown
 */
class WuMacEnergyMonitor : public inet::OperationalBase, public inet::cListener
{
public:
    WuMacEnergyMonitor():inet::OperationalBase(),
    energyStorage(nullptr),
    macModule(nullptr){};

    static simsignal_t receptionConsumptionSignal;
    static simsignal_t falseWakeUpConsumptionSignal;
    static simsignal_t transmissionConsumptionSignal;
    static simsignal_t unknownConsumptionSignal;
  private:
    simsignal_t inProgress = SIMSIGNAL_NULL;
    inet::units::values::J storedEnergyStartValue = inet::units::values::J(0.0);
    inet::units::values::J pausedIntermediateConsumption = inet::units::values::J(0.0);
    inet::units::values::W initialEnergyGeneration;
    simtime_t storedEnergyStartTime;

    inet::power::IEpEnergyStorage* energyStorage;
    cModule* macModule;
  protected:
    void initialize(int stage) override;
    virtual bool isInitializeStage(int stage) override { return stage == inet::INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) override { return stage == inet::ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) override { return stage == inet::ModuleStopOperation::STAGE_LINK_LAYER; }

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, bool b, cObject *details) override;

    // OperationalBase:
    virtual void handleMessageWhenUp(cMessage *msg) override {};
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

    virtual void resumeMonitoring();
    virtual void startMonitoring(simsignal_t startSignal);
    virtual void finishMonitoring(simsignal_t stopSignal);
    virtual void pauseMonitoring();
public:
    const inet::units::values::J calculateDeltaEnergyConsumption() const;
    const inet::units::values::J calcTxAndAckEstConsumption(inet::units::values::b packetLength) const;
    bool isMatchingEndedSignal(const simsignal_t endedSignal);
};

} /* namespace oppostack */

#endif /* LINKLAYER_WUMACENERGYMONITOR_H_ */
