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
        txPacketInProgress(nullptr),
        wakeUpRadio(nullptr),
        dataRadio(nullptr),
        activeRadio(nullptr),
        ackWaitDuration(0),
        txWakeUpWaitDuration(0),
        headerLength(16),
        wuLength(4)
      {}
    virtual ~WakeUpMacLayer();
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleLowerCommand(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;



  protected:
    /** @brief User Configured parameters */
    simtime_t txWakeUpWaitDuration;
    simtime_t ackWaitDuration;
    int headerLength;
    int wuLength;


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

    /** @brief MAC state machine events.*/
    enum t_mac_event {
        EV_QUEUE_SEND,
        EV_TX_START,
        EV_WAKEUP_BACKOFF,
        EV_TX_READY,
        EV_TX_END
    };


    /** @name Protocol timer messages */
    /*@{*/
    cMessage *wakeUpBackoffTimer;
    cMessage *ackBackoffTimer;
    /*@}*/
    int wakeUpRadioInGateId = -1;
    int wakeUpRadioOutGateId = -1;
    cMessage *wuPacketPrototype;

    /** @brief The radio. */
    physicallayer::IRadio *dataRadio;
    physicallayer::IRadio *wakeUpRadio;
    physicallayer::IRadio *activeRadio;
    physicallayer::IRadio::TransmissionState transmissionState;

    virtual void initialize(int stage) override;
    void changeActiveRadio(physicallayer::IRadio*);
    virtual bool isLowerMessage(cMessage *message) override;
    virtual void configureInterfaceEntry() override;

    t_mac_state macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stepMacSM(t_mac_event event, cMessage *msg);
    void updateMacState(t_mac_state newMacState);
    /** @brief Transmitter State Machine **/
    t_tx_state txState;
    void stepTxSM(t_mac_event event, cMessage *msg);
    void updateTxState(t_tx_state newTxState);

    cMessage *wuPacketInProgress;
    cMessage *txPacketInProgress;
    void encapsulate(Packet *msg);
    bool startImmediateTransmission(cMessage *msg); // Return false if immediate transmission is not possible

    // OperationalBase:
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

};

const Protocol WuMacProtocol("WuMac", "WuMac", Protocol::LinkLayer);

#endif
