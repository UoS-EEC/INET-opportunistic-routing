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
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/common/Protocol.h"
#include "OpportunisticRpl.h"

using namespace omnetpp;
using namespace inet;

/**
 * WakeUpMacLayer - Implements two stage message transmission of
 * high power wake up followed by the data message
 */
class WakeUpMacLayer : public MacProtocolBase, public IMacProtocol
{
  public:
    WakeUpMacLayer()
      : MacProtocolBase(),
        wakeUpBackoffTimer(nullptr),
        ackBackoffTimer(nullptr),
        wuTimeout(nullptr),
        wuPacketInProgress(nullptr),
        txPacketInProgress(nullptr),
        rxPacketInProgress(nullptr),
        wakeUpRadio(nullptr),
        dataRadio(nullptr),
        activeRadio(nullptr),
        routingModule(nullptr),
        ackWaitDuration(0),
        txWakeUpWaitDuration(0),
        dataListeningDuration(0),
        wuApproveResponseLimit(0),
        candiateRelayContentionProbability(0.7),
        cumulativeAckBackoff(0),
        acknowledgedForwarders(0),
        maxWakeUpRetries(4), // TODO: Parameterize
        txInProgressForwarders(0),
        txInProgressRetries(0)
      {}
    virtual ~WakeUpMacLayer();
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleUpperCommand(cMessage *msg) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleLowerCommand(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;



  protected:
    /** @brief User Configured parameters */
    simtime_t txWakeUpWaitDuration;
    simtime_t ackWaitDuration;
    simtime_t dataListeningDuration;
    simtime_t wuApproveResponseLimit;

    // TODO: Replace by type to represent accept, reject messages
    const int WAKEUP_APPROVE = 502;
    const int WAKEUP_REJECT = 503;
    double candiateRelayContentionProbability;


    /** @brief MAC high level states */
    enum t_mac_state {
        S_IDLE, // WuRx listening
        S_WAKEUP_LSN, // WuRx receiving or processing
        S_RECEIVE, // Data radio listening
        S_TRANSMIT, // Transmitting (Wake-up, pause, transmit and wait for ack)
        S_ACK // Transmitting ACK after receiving
    };

    enum t_tx_state {
        TX_IDLE, // Transmitter off
        TX_WAKEUP_WAIT, // Tx wake-up when radio ready till wake-up finish
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
        //TODO: implement
    };

    /** @brief MAC state machine events.*/
    enum t_mac_event {
        EV_QUEUE_SEND,
        EV_TX_START,
        EV_WAKEUP_BACKOFF,
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
    };


    /** @name Protocol timer messages */
    /*@{*/
    cMessage *wakeUpBackoffTimer;
    cMessage *ackBackoffTimer;
    cMessage *wuTimeout;
    /*@}*/
    int wakeUpRadioInGateId = -1;
    int wakeUpRadioOutGateId = -1;
    cMessage *wuPacketPrototype;

    /** @brief The radio. */
    physicallayer::IRadio *dataRadio;
    physicallayer::IRadio *wakeUpRadio;
    physicallayer::IRadio *activeRadio;
    physicallayer::IRadio::TransmissionState transmissionState;
    physicallayer::IRadio::ReceptionState receptionState;

    virtual void initialize(int stage) override;
    void changeActiveRadio(physicallayer::IRadio*);
    virtual bool isLowerMessage(cMessage *message) override;
    virtual void configureInterfaceEntry() override;
    OpportunisticRpl* routingModule;
    void queryWakeupRequest(Packet *wakeUp);

    t_mac_state macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stepMacSM(t_mac_event event, cMessage *msg);
    simtime_t cumulativeAckBackoff;
    virtual void stepRxAckProcess(t_mac_event event, cMessage *msg);
    Packet* buildAck(const Packet* subject) const;
    void updateMacState(t_mac_state newMacState);
    /** @brief Transmitter State Machine **/
    bool txStateChange = false;
    t_tx_state txState;
    void stepTxSM(t_mac_event event, cMessage *msg);
    Packet* buildWakeUp(const Packet* subject) const;
    int acknowledgedForwarders;
    int maxWakeUpRetries;
    int txInProgressForwarders;
    int txInProgressRetries;
    virtual void stepTxAckProcess(t_mac_event event, cMessage *msg);
    void updateTxState(t_tx_state newTxState);
    /** @brief Wake-up listening State Machine **/

    bool wuStateChange = false;
    t_wu_state wuState;
    void stepWuSM(t_mac_event event, cMessage *msg);
    void updateWuState(t_wu_state newWuState);
    /** @brief Receiving and acknowledgement **/
    // TODO: implement

    cMessage *wuPacketInProgress;
    Packet *txPacketInProgress;
    cMessage *rxPacketInProgress;
    void encapsulate(Packet *msg);
    void decapsulate(Packet *msg);
    bool startImmediateTransmission(cMessage *msg); // Return false if immediate transmission is not possible

    // OperationalBase:
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

};

const Protocol WuMacProtocol("WuMac", "WuMac", Protocol::LinkLayer);

#endif
