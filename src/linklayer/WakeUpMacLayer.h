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
        transmitStartDelay(nullptr),
        receiveTimeout(nullptr),
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
    using MacProtocolBase::receiveSignal;
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
    enum class MacState {
        WAKE_UP_IDLE, // WuRx listening
        WAKE_UP_WAIT, // WuRx receiving or processing
        RECEIVE, // Data radio listening, receiving and ack following wake-up or initial data
        AWAIT_TRANSMIT, // DATA radio listening but with packet waiting to be transmitted
        TRANSMIT // Transmitting (Wake-up, pause, transmit and wait for ack)
    };

    enum class TxDataState {
        WAKE_UP_WAIT, // Tx wake-up when radio ready and CSMA finished
        WAKE_UP, // Tx Wake-up in progress
        DATA_WAIT, // Wait for receivers to wake-up
        DATA, // Send data when radio ready
        ACK_WAIT, // Listen for node acknowledging
        END // Reset
    };

    enum class WuWaitState{
        IDLE, // WuRx listening
        APPROVE_WAIT, // Wait for approval for wake-up (call to net layer)
        DATA_RADIO_WAIT, // Wait for the data radio to start
        ABORT // Shutdown data radio and restart wake-up radio
    };

    enum class RxState{
        IDLE,
        DATA_WAIT,
        ACK, // Transmitting ACK after receiving
        FINISH // Immediately set timeout and enter main sm idle
    };

    /** @brief MAC state machine events.*/
    enum class MacEvent {
        QUEUE_SEND,
        TX_START,
        CSMA_BACKOFF,
        TX_READY,
        TX_END,
        ACK_TIMEOUT,
        WU_START,
        WU_APPROVE,
        WU_REJECT,
        DATA_TIMEOUT,
        DATA_RX_IDLE,
        DATA_RX_READY,
        DATA_RECEIVED,
        REPLENISH_TIMEOUT
    };

    // Translate WakeUpMacLayer Events to BackoffBase Events
    CSMATxBackoffBase::t_backoff_state stepBackoffSM(const MacEvent event){
        if(activeBackoff == nullptr){
            return CSMATxBackoffBase::BO_WAIT;
        }
        switch(event){
            case MacEvent::TX_READY:
                return activeBackoff->process(CSMATxBackoffBase::EV_TX_READY);
            case MacEvent::DATA_RX_READY:
                return activeBackoff->process(CSMATxBackoffBase::EV_RX_READY);
            case MacEvent::CSMA_BACKOFF:
                return activeBackoff->process(CSMATxBackoffBase::EV_BACKOFF_TIMER);
            default:
                return activeBackoff->process(CSMATxBackoffBase::EV_NULL);;
        }
    }

    /** @name Protocol timer messages */
    /*@{*/
    cMessage *transmitStartDelay;
    cMessage *ackBackoffTimer;
    cMessage *receiveTimeout;
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
    MacState macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stateProcess(const MacEvent& event, cMessage *msg);
    void stateListeningEnterAlreadyListening();
    void stateListeningEnter();
    void stateListeningIdleEnterAlreadyListening();
    void stateAwaitTransmitEnterAlreadyListening();
    void stateWakeUpIdleProcess(const MacEvent& event, omnetpp::cMessage* const msg);
    void stateAwaitTransmitProcess(const MacEvent& event, omnetpp::cMessage* const msg);
    EqDC acceptDataEqDCThreshold = EqDC(25.5);
    int rxAckRound = 0;
    RxState rxState;
    void StateReceiveEnter();
    void stateReceiveEnterDataWait();
    void stateReceiveAckEnterReceiveDataWait();
    void stateReceiveEnterFinishDropReceived(const inet::PacketDropReason reason);
    void stateReceiveEnterFinish();
    void stateReceiveProcessDataTimeout();
    void stateReceiveEnterAck();
    void stateReceiveExitAck();
    void stateReceiveAckProcessBackoff(const MacEvent& event);
    void stateReceiveExitDataWait();
    virtual void stateReceiveProcess(const MacEvent& event, cMessage *msg);
  private:
    void handleCoincidentalOverheardData(inet::Packet* receivedData);
    void handleOverheardAckInDataReceiveState(const Packet * const msg);
    void stateReceiveDataWaitProcessDataReceived(cMessage *msg);
    void stateReceiveAckProcessDataReceived(cMessage *msg);
    void completePacketReception();

  protected:
    Packet* buildAck(const Packet* subject) const;
    void updateMacState(const MacState& newMacState){ macState = newMacState; };
    /** @brief Transmitter State Machine **/
    TxDataState txDataState;
    void stateTxEnter();
    void stateTxEnterDataWait();
    void stateTxDataWaitExitEnterAckWait();
    void stateTxWakeUpWaitExit();
    void stateTxEnterEnd();
    void stateTxProcess(const MacEvent& event, cMessage* msg);
    Packet* buildWakeUp(const Packet* subject, const int retryCount) const;
    const int requiredForwarders = 1;
    int acknowledgedForwarders = 0;
    int acknowledgmentRound = 1;
    int maxWakeUpTries = 1;
    int txInProgressForwarders = 0;
    int txInProgressTries = 0;
    ExpectedCost dataMinExpectedCost = EqDC(25.5);
    simtime_t dataTransmissionDelay = 0;
    virtual void stateTxAckWaitProcess(const MacEvent& event, cMessage *msg);

    /** @brief Wake-up listening State Machine **/
    WuWaitState wuState;
    void stateWakeUpWaitEnter();
    void stateWakeUpWaitExitToListening();
    void stateWakeUpWaitApproveWaitEnter(omnetpp::cMessage* const msg);
    void stateWakeUpProcess(const MacEvent& event, cMessage *msg);

    /** @brief Receiving and acknowledgement **/
    cMessage *currentRxFrame;
    void dropCurrentRxFrame(PacketDropDetails& details);
    void encapsulate(Packet* msg) const;
    void decapsulate(Packet* msg) const;
    void setupTransmission();

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
    bool transmissionStartEnergyCheck() const;
};

const Protocol WuMacProtocol("WuMac", "WuMac", Protocol::LinkLayer);

} //namespace oppostack

#endif
