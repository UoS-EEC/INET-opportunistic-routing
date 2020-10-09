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
        txPacketInProgress(nullptr),
        wakeUpRadio(nullptr),
        dataRadio(nullptr),
        activeRadio(nullptr),
        ackWaitDuration(0),
        txWakeUpWaitDuration(0),
        headerLength(16),
        wuLength(4),
        dataListeningDuration(0),
        wuAcceptResponseLimit(0)
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
    simtime_t wuAcceptResponseLimit;
    int headerLength;
    int wuLength;

    // TODO: Replace by type to represent accept, reject messagess
    const int WAKEUP_APPROVE = 502;
    const int WAKEUP_REJECT = 503;


    /** @brief MAC high level states */
    enum t_mac_state {
        S_IDLE,
        S_WAKEUP_LSN,
        S_RECEIVE,
        S_TRANSMIT
    };

    enum t_tx_state {
        TX_IDLE,
        TX_WAKEUP_WAIT,
        TX_DATA_WAIT,
        TX_DATA,
        TX_ACK_WAIT,
        TX_END
    };

    enum t_wu_state{
        WU_IDLE,
        WU_APPROVE_WAIT,
        WU_WAKEUP_WAIT,
        WU_LISTEN,
        WU_ABORT
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
    void queryWakeupRequest(cMessage *wakeUp);

    t_mac_state macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stepMacSM(t_mac_event event, cMessage *msg);
    void updateMacState(t_mac_state newMacState);
    /** @brief Transmitter State Machine **/
    t_tx_state txState;
    void stepTxSM(t_mac_event event, cMessage *msg);
    void updateTxState(t_tx_state newTxState);
    /** @brief Wake-up listening State Machine **/
    t_wu_state wuState;
    void stepWuSM(t_mac_event event, cMessage *msg);
    void updateWuState(t_wu_state newWuState);
    /** @brief Receiving and acknowledgement **/
    // TODO: implement

    cMessage *wuPacketInProgress;
    cMessage *txPacketInProgress;
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
