/************************
 * file:        WakeUpMacLayer.h
 *
 * author:      Edward Longman, (Ieee802154Mac authors Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *                Marc Loebbers, Yosia Hadisusanto)
 *
 * copyright:    (C) 2007-2009 CSEM SA
 *              (C) 2009 T.U. Eindhoven
 *                (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *
 */
#ifndef __WAKEUPMAC_WAKEUPMACLAYER_H_
#define __WAKEUPMAC_WAKEUPMACLAYER_H_

#include <omnetpp.h>
#include <inet/linklayer/base/MacProtocolBase.h>
#include <inet/linklayer/contract/IMacProtocol.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/common/lifecycle/LifecycleController.h>
#include <inet/common/lifecycle/NodeStatus.h>
#include <inet/common/Protocol.h>

#include "../networklayer/ORWRouting.h"
#include "common/Units.h"
#include "WakeUpGram_m.h"
#include "CSMATxBackoff.h"
#include "IOpportunisticLinkLayer.h"
#include "IObservableMac.h"

namespace oppostack{

using namespace omnetpp;
using namespace inet;

/**
 * WakeUpMacLayer - Implements two stage message transmission of
 * high power wake up followed by the data message
 */
class WakeUpMacLayer : public MacProtocolBase, public IMacProtocol, public IOpportunisticLinkLayer, public IObservableMac
{
  public:
    WakeUpMacLayer()
      : MacProtocolBase(),
        wakeUpBackoffTimer(nullptr),
        wuTimeout(nullptr),
        dataRadio(nullptr),
        wakeUpRadio(nullptr),
        activeRadio(nullptr),
        energyStorage(nullptr),
        networkNode(nullptr),
        replenishmentTimer(nullptr),
        currentRxFrame(nullptr),
        activeBackoff(nullptr)
      {}
    virtual ~WakeUpMacLayer();
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleUpperCommand(cMessage *msg) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleLowerCommand(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent* source, simsignal_t signalID, intval_t value, cObject* details) override;

  protected:
    /** @brief User Configured parameters */
    simtime_t txWakeUpWaitDuration = 0;
    simtime_t ackWaitDuration = 0;
    simtime_t dataListeningDuration = 0;
    simtime_t wuApproveResponseLimit = 0;

    // TODO: Replace by type to represent accept, reject messages
    const int WAKEUP_APPROVE = 502;
    const int WAKEUP_REJECT = 503;
    double candiateRelayContentionProbability = 0.7;
    inet::B phyMtu = B(255);

    /** @brief Calculated (in initialize) parameters */
    simtime_t initialContentionDuration = 0;
    simtime_t ackTxWaitDuration = 0;
    simtime_t minimumContentionWindow = 0;

    bool recheckDataPacketEqDC;
    bool skipDirectTxFinalAck = false;
protected:
    /** @brief MAC high level states */
    enum t_mac_state {
        S_IDLE, // WuRx listening
        S_WAKEUP_LSN, // WuRx receiving or processing
        S_RECEIVE, // Data radio listening
        S_TRANSMIT, // Transmitting (Wake-up, pause, transmit and wait for ack)
        S_ACK // Transmitting ACK after receiving
    };

    enum t_tx_state {
        TX_WAKEUP_WAIT, // Tx wake-up when radio ready and CSMA finished
        TX_WAKEUP, // Tx Wake-up in progress
        TX_DATA_WAIT, // Wait for receivers to wake-up
        TX_DATA, // Send data when radio ready
        TX_ACK_WAIT, // TODO: Listen for node acknowledging
        TX_END // Reset
    };

    enum t_wu_state{
        WU_IDLE, // WuRx listening
        WU_APPROVE_WAIT, // Wait for approval for wake-up (call to net layer)
        WU_WAKEUP_WAIT, // Wait for the data radio to start
        WU_ABORT // Shutdown data radio and restart wake-up radio
    };

    enum t_rx_state{
        RX_IDLE,
        RX_RECEIVE,
    };

    /** @brief MAC state machine events.*/
    enum t_mac_event {
        EV_QUEUE_SEND,
        EV_WAKEUP_BACKOFF,
        EV_CSMA_BACKOFF,
        EV_TX_READY,
        EV_TX_END,
        EV_ACK_TIMEOUT,
        EV_WU_START,
        EV_WU_APPROVE,
        EV_WU_REJECT,
        EV_WU_TIMEOUT,
        EV_DATA_RX_IDLE,
        EV_DATA_RX_READY,
        EV_DATA_RECEIVED,
        EV_REPLENISH_TIMEOUT
    };

    // Translate WakeUpMacLayer Events to BackoffBase Events
    CSMATxBackoffBase::t_backoff_state stepBackoffSM(const t_mac_event event){
        switch(event){
            case EV_TX_READY:
                return activeBackoff->process(CSMATxBackoffBase::EV_TX_READY);
            case EV_DATA_RX_READY:
                return activeBackoff->process(CSMATxBackoffBase::EV_RX_READY);
            case EV_CSMA_BACKOFF:
                return activeBackoff->process(CSMATxBackoffBase::EV_BACKOFF_TIMER);
            default: break;
        }
        return activeBackoff->process(CSMATxBackoffBase::EV_NULL);
    }

    /** @name Protocol timer messages */
    /*@{*/
    cMessage *wakeUpBackoffTimer;
    cMessage *ackBackoffTimer;
    cMessage *wuTimeout;
    /*@}*/
    CSMATxBackoffBase* activeBackoff;

    int wakeUpRadioInGateId = -1;
    int wakeUpRadioOutGateId = -1;

    /** @brief The radio. */
    physicallayer::IRadio *dataRadio;
    physicallayer::IRadio *wakeUpRadio;
    physicallayer::IRadio *activeRadio;
    physicallayer::IRadio::TransmissionState transmissionState;
    physicallayer::IRadio::ReceptionState receptionState;

    inet::power::IEpEnergyStorage* energyStorage;
    inet::LifecycleController lifecycleController;
    cModule* networkNode;

    J lastEnergyStorageLevel = J(0.0);
    J transmissionStartMinEnergy = J(0.0);
    simtime_t replenishmentCheckRate = SimTime(1, SimTimeUnit::SIMTIME_S);
    cMessage* replenishmentTimer;

    virtual void initialize(int stage) override;
    virtual void cancelAllTimers();
    virtual void deleteAllTimers();
    void changeActiveRadio(physicallayer::IRadio*);
    // Check if message comes from lower gate or wake-up radio
    virtual bool isLowerMessage(cMessage* msg) override{
        return MacProtocolBase::isLowerMessage(msg) || msg->getArrivalGateId() == wakeUpRadioInGateId;
    };
    virtual void configureInterfaceEntry() override;
    void queryWakeupRequest(Packet* wakeUp);


  protected:
    t_mac_state macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stepMacSM(const t_mac_event& event, cMessage *msg);
    EqDC acceptDataEqDCThreshold = EqDC(25.5);
    int rxAckRound = 0;
    virtual void stepRxAckProcess(const t_mac_event& event, cMessage *msg);
  private:
    void handleDataReceivedInAckState(cMessage *msg);
    void completePacketReception();

  protected:
    Packet* buildAck(const Packet* subject) const;
    void updateMacState(const t_mac_state& newMacState){ macState = newMacState; };
    /** @brief Transmitter State Machine **/
    t_tx_state txState;
    void stepTxSM(const t_mac_event& event, cMessage* msg);
    Packet* buildWakeUp(const Packet* subject, const int retryCount) const;
    const int requiredForwarders = 1;
    const int maxForwarders = 4;
    int acknowledgedForwarders = 0;
    int acknowledgmentRound = 1;
    int maxWakeUpTries = 1;
    int txInProgressForwarders = 0;
    int txInProgressTries = 0;
    ExpectedCost dataMinExpectedCost = EqDC(25.5);
    simtime_t dataTransmissionDelay = 0;
    virtual void stepTxAckProcess(const t_mac_event& event, cMessage *msg);
    void updateTxState(const t_tx_state& newTxState){ txState = newTxState; };
    /** @brief Wake-up listening State Machine **/

    bool wuStateChange = false;
    t_wu_state wuState;
    void stepWuSM(const t_mac_event& event, cMessage *msg);
    void updateWuState(const t_wu_state& newWuState){
        wuStateChange = true;
        wuState = newWuState;
    };
    /** @brief Receiving and acknowledgement **/
    cMessage *currentRxFrame;
    void dropCurrentRxFrame(PacketDropDetails& details);
    void encapsulate(Packet* msg) const;
    void decapsulate(Packet* msg) const;
    bool setupTransmission(); // Return false if immediate transmission is not possible

    // OperationalBase:
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;
    void setBeaconFieldsFromTags(const inet::Packet* subject,
            const inet::Ptr<WakeUpBeacon>& wuHeader) const;
};

const Protocol WuMacProtocol("WuMac", "WuMac", Protocol::LinkLayer);

} //namespace oppostack

#endif
