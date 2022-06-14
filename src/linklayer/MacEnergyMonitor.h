/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_MACENERGYMONITOR_H_
#define LINKLAYER_MACENERGYMONITOR_H_

#include <omnetpp.h>
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/common/Units.h>
#include <inet/common/lifecycle/OperationalBase.h>
#include <inet/common/lifecycle/LifecycleOperation.h>
#include <inet/common/lifecycle/ModuleOperations.h>

using namespace omnetpp;
namespace oppostack {

/**
 * Receive Signals from IOberservableMac about starting and stopping of reception or transmission for energy monitoring
 * Implemented using Stop and Start operation to pause monitoring when interrupted due to node shutdown
 */
class MacEnergyMonitor : public inet::OperationalBase, public inet::cListener
{
public:
    MacEnergyMonitor():inet::OperationalBase(),
    energyStorage(nullptr),
    macModule(nullptr){};

    static simsignal_t receptionConsumptionSignal;
    static simsignal_t falseReceptionConsumptionSignal;
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
    virtual bool isInitializeStage(int stage) const override { return stage == inet::INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) const override { return stage == inet::ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) const override { return stage == inet::ModuleStopOperation::STAGE_LINK_LAYER; }

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

#endif /* LINKLAYER_MACENERGYMONITOR_H_ */
