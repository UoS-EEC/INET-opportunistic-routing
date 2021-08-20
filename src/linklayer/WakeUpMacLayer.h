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
#include <inet/common/lifecycle/LifecycleController.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include <inet/common/lifecycle/NodeStatus.h>
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
        transmitStartDelay(nullptr),
        wakeUpRadio(nullptr),
        activeRadio(nullptr),
        networkNode(nullptr),
        replenishmentTimer(nullptr),
        currentRxFrame(nullptr)
      {}
    ~WakeUpMacLayer();
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    using MacProtocolBase::receiveSignal;
    virtual void receiveSignal(cComponent* source, simsignal_t signalID, intval_t value, cObject* details) override;

  protected:
    /** @brief User Configured parameters */
    simtime_t txWakeUpWaitDuration = 0;
    simtime_t wuApproveResponseLimit = 0;

    // TODO: Replace by type to represent accept, reject messages
    const int WAKEUP_APPROVE = 502;
    const int WAKEUP_REJECT = 503;
    double candiateRelayContentionProbability = 0.7;

    bool recheckDataPacketEqDC;
    bool skipDirectTxFinalAck = false;
protected:
    enum class WuWaitState{
        IDLE, // WuRx listening
        APPROVE_WAIT, // Wait for approval for wake-up (call to net layer)
        DATA_RADIO_WAIT, // Wait for the data radio to start
        ABORT // Shutdown data radio and restart wake-up radio
    };

    /** @name Protocol timer messages */
    /*@{*/
    cMessage *transmitStartDelay;
    /*@}*/

    int wakeUpRadioInGateId = -1;
    int wakeUpRadioOutGateId = -1;

    /** @brief The radio. */
    physicallayer::IRadio *wakeUpRadio;
    physicallayer::IRadio *activeRadio;
    physicallayer::IRadio::TransmissionState transmissionState;
    physicallayer::IRadio::ReceptionState receptionState;

    inet::LifecycleController lifecycleController;
    cModule* networkNode;

    simtime_t replenishmentCheckRate = SimTime(1, SimTimeUnit::SIMTIME_S);
    cMessage* replenishmentTimer;

    virtual void initialize(int stage) override;
    virtual void cancelAllTimers() override;
    void deleteAllTimers();
    void changeActiveRadio(physicallayer::IRadio*);
    // Check if message comes from lower gate or wake-up radio
    virtual bool isLowerMessage(cMessage* msg) override{
        return MacProtocolBase::isLowerMessage(msg) || msg->getArrivalGateId() == wakeUpRadioInGateId;
    };
    void queryWakeupRequest(Packet* wakeUp);


  protected:
    State macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stateProcess(const MacEvent& event, cMessage *msg);
    State stateListeningEnterAlreadyListening();
    State stateListeningEnter();
    void stateListeningIdleEnterAlreadyListening();
    void stateAwaitTransmitEnterAlreadyListening();
    State stateWakeUpIdleProcess(const MacEvent& event, omnetpp::cMessage* const msg);
    State stateAwaitTransmitProcess(const MacEvent& event, omnetpp::cMessage* const msg);
    EqDC acceptDataEqDCThreshold = EqDC(25.5);
    int rxAckRound = 0;
    RxState rxState;
    State  stateReceiveEnter();
    void stateReceiveEnterDataWait();
    void stateReceiveAckEnterReceiveDataWait();
    void stateReceiveEnterFinishDropReceived(const inet::PacketDropReason reason);
    void stateReceiveEnterFinish();
    void stateReceiveProcessDataTimeout();
    void stateReceiveEnterAck();
    void stateReceiveExitAck();
    void stateReceiveAckProcessBackoff(const MacEvent& event);
    void stateReceiveExitDataWait();
    /*
     * Member to process events when in the Receive state
     * Overridable by inheriting classes for other reciever behavior
     * @ return Is receiveState finished
     */
    virtual bool stateReceiveProcess(const MacEvent& event, cMessage *msg);
  private:
    void handleCoincidentalOverheardData(inet::Packet* receivedData);
    void handleOverheardAckInDataReceiveState(const Packet * const msg);
    void stateReceiveDataWaitProcessDataReceived(cMessage *msg);
    void stateReceiveAckProcessDataReceived(cMessage *msg);
    void completePacketReception();

  protected:
    Packet* buildAck(const Packet* subject) const;
    /** @brief Transmitter State Machine **/
    TxDataState txDataState;
    State stateTxEnter();
    void stateTxEnterDataWait();
    void stateTxDataWaitExitEnterAckWait();
    void stateTxWakeUpWaitExit();
    void stateTxEnterEnd();
    /*
     * Member to process events when in the Transmit state
     * Overridable by inheriting classes for other trasmitter behavior
     * @ return Is transmit State finished
     */
    virtual bool stateTxProcess(const MacEvent& event, cMessage* msg);
    Packet* buildWakeUp(const Packet* subject, const int retryCount) const;
    int acknowledgedForwarders = 0;
    int acknowledgmentRound = 1;
    int maxWakeUpTries = 1;
    int txInProgressTries = 0;
    ExpectedCost dataMinExpectedCost = EqDC(25.5);
    simtime_t dataTransmissionDelay = 0;
    virtual void stateTxAckWaitProcess(const MacEvent& event, cMessage *msg);

    /** @brief Wake-up listening State Machine **/
    WuWaitState wuState;
    void stateWakeUpWaitEnter();
    State stateWakeUpWaitExitToListening();
    State stateWakeUpWaitApproveWaitEnter(omnetpp::cMessage* const msg);
    State stateWakeUpProcess(const MacEvent& event, cMessage *msg);

    /** @brief Receiving and acknowledgement **/
    cMessage *currentRxFrame;
    void dropCurrentRxFrame(PacketDropDetails& details);
    void encapsulate(Packet* msg) const;
    void decapsulate(Packet* msg) const;

    // OperationalBase:
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    /** @brief Packet management **/
    void setBeaconFieldsFromTags(const inet::Packet* subject,
            const inet::Ptr<WakeUpBeacon>& wuHeader) const;
    void dropCurrentTxFrame(inet::PacketDropDetails& details) override{
        MacProtocolBase::dropCurrentTxFrame(details);
        emit(transmissionTriesSignal, txInProgressTries);
        emit(transmissionEndedSignal, true);
    }
    void deleteCurrentTxFrame() override{
        MacProtocolBase::deleteCurrentTxFrame();
        emit(transmissionTriesSignal, txInProgressTries);
        emit(transmissionEndedSignal, true);
    }

    void completePacketTransmission();
};

const Protocol WuMacProtocol("WuMac", "WuMac", Protocol::LinkLayer);

} //namespace oppostack

#endif
