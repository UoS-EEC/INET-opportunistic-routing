/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_WAKEUPMACLAYER_H_
#define LINKLAYER_WAKEUPMACLAYER_H_

#include <omnetpp.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>
#include <inet/common/Protocol.h>

#include "../networklayer/ORWRouting.h"
#include "common/Units.h"
#include "ORWMac.h"

namespace oppostack{

using namespace omnetpp;
using namespace inet;

/**
 * WakeUpMacLayer - Implements two stage message transmission of
 * high power wake up followed by the data message
 */
class WakeUpMacLayer : public ORWMac
{
  public:
    WakeUpMacLayer()
      : ORWMac(),
        wakeUpRadio(nullptr),
        activeRadio(nullptr)
      {}
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    using ORWMac::receiveSignal;
    virtual void receiveSignal(cComponent* source, simsignal_t signalID, intval_t value, cObject* details) override;

  protected:
    /** @brief User Configured parameters */
    simtime_t txWakeUpWaitDuration = 0;
    simtime_t wuApproveResponseLimit = 0;
    bool dyanmicWakeUpChecking = false;

    // TODO: Replace by type to represent accept, reject messages
    const int WAKEUP_APPROVE = 502;
    const int WAKEUP_REJECT = 503;

protected:
    enum class WuWaitState{
        IDLE, // WuRx listening
        APPROVE_WAIT, // Wait for approval for wake-up (call to net layer)
        DATA_RADIO_WAIT, // Wait for the data radio to start
        ABORT // Shutdown data radio and restart wake-up radio
    };

    int wakeUpRadioInGateId = -1;
    int wakeUpRadioOutGateId = -1;

    /** @brief The radio. */
    physicallayer::IRadio *wakeUpRadio;
    physicallayer::IRadio *activeRadio;


    virtual void initialize(int stage) override;
    void changeActiveRadio(physicallayer::IRadio*);
    // Check if message comes from lower gate or wake-up radio
    virtual bool isLowerMessage(cMessage* msg) override{
        return ORWMac::isLowerMessage(msg) || msg->getArrivalGateId() == wakeUpRadioInGateId;
    };
    void queryWakeupRequest(Packet* wakeUp);


  protected:
    /** @brief Execute a step in the MAC state machine */
    virtual void stateProcess(const MacEvent& event, cMessage *msg) override;
    /** @name Receiving State variables and event processing */
    /*@{*/
    virtual State stateListeningEnter() override;
    State stateWakeUpIdleProcess(const MacEvent& event, omnetpp::cMessage* const msg);
    virtual State stateAwaitTransmitProcess(const MacEvent& event, omnetpp::cMessage* const msg) override;
    virtual State stateReceiveEnter() override;
    /*@}*/

  protected:
    /** @brief Transmitter State Machine **/
    virtual State stateTxEnter() override;
    void stateTxWakeUpWaitExit();
    /* @brief Extends base to add wake-up transmission before data */
    virtual bool stateTxProcess(const MacEvent& event, cMessage* msg) override;
    Packet* buildWakeUp(const Packet* subject, const int retryCount) const;

    /** @brief Wake-up listening State Machine **/
    WuWaitState wuState;
    void stateWakeUpWaitEnter();
    State stateWakeUpWaitExitToListening();
    State stateWakeUpWaitApproveWaitEnter(omnetpp::cMessage* const msg);
    State stateWakeUpProcess(const MacEvent& event, cMessage *msg);
};
} //namespace oppostack

#endif // ifdef LINKLAYER_WAKEUPMACLAYER_H_
