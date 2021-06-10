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
#include <inet/networklayer/contract/INetfilter.h>

#include "../networklayer/ORWRouting.h"
#include "common/Units.h"
#include "WakeUpGram_m.h"

namespace oppostack{

using namespace omnetpp;
using namespace inet;

/**
 * WakeUpMacLayer - Implements two stage message transmission of
 * high power wake up followed by the data message
 */
class WakeUpMacLayer : public MacProtocolBase, public IMacProtocol, public NetfilterBase
{
  public:
    WakeUpMacLayer()
      : MacProtocolBase(),
        wakeUpBackoffTimer(nullptr),
        ackBackoffTimer(nullptr),
        wuTimeout(nullptr),
        dataRadio(nullptr),
        wakeUpRadio(nullptr),
        activeRadio(nullptr),
        energyStorage(nullptr),
        networkNode(nullptr),
        replenishmentTimer(nullptr),
        currentRxFrame(nullptr),
        txBackoffTimer(nullptr)
      {}
    virtual ~WakeUpMacLayer();
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleUpperCommand(cMessage *msg) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleLowerCommand(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent* source, simsignal_t signalID, intval_t value, cObject* details) override;

    // Timer used by child classes
    cMessage *txBackoffTimer;

    /** @brief MAC state machine events.*/
    enum t_mac_event {
        EV_QUEUE_SEND,
        EV_TX_START,
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

    class CSMATxBackoffBase{
      private:
        simtime_t initialBackoff;
        physicallayer::IRadio* activeRadio;
        power::IEpEnergyStorage* energyStorage;
        J transmissionStartMinEnergy = J(0.0);
        void cancelBackoffTimer(){
            parent->cancelEvent(parent->txBackoffTimer);
        }
      public:
        enum t_backoff_state {
            BO_OFF,
            BO_WAIT,
            BO_SWITCHING,
            BO_FINISHED,
        };
        enum t_backoff_ev {
            EV_START,
            EV_BACKOFF_TIMER,
            EV_RX_READY,
            EV_TX_READY,
            EV_TX_ABORT,
        };
      protected:
        WakeUpMacLayer* parent;
        virtual simtime_t calculateBackoff(CSMATxBackoffBase::t_backoff_ev returnEv) const = 0;

        t_backoff_state state = BO_OFF;
      public:
        CSMATxBackoffBase(WakeUpMacLayer* _parent,
                physicallayer::IRadio* _activeRadio,
                power::IEpEnergyStorage* _energyStorage,
                J _transmissionStartMinEnergy,
                simtime_t initial_backoff):
                    parent(_parent),
                    activeRadio(_activeRadio),
                    energyStorage(_energyStorage),
                    transmissionStartMinEnergy(_transmissionStartMinEnergy),
                    initialBackoff(initial_backoff)
                    {};
        void setRadioToTransmitIfFreeOrDelay(){
            using namespace inet::physicallayer;
            IRadio::ReceptionState receptionState = activeRadio->getReceptionState();
            bool isIdle = (receptionState == IRadio::RECEPTION_STATE_IDLE)
                            && activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER;
            if(isIdle){
                //Start the transmission state machine
                activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                state = BO_SWITCHING;
            }
            else{
                t_backoff_ev returnEv = EV_BACKOFF_TIMER;
                parent->scheduleAt(simTime()+calculateBackoff(returnEv), parent->txBackoffTimer);
                if(returnEv == EV_TX_ABORT){
                    state = BO_OFF;
                }
                else{
                    state = BO_WAIT;
                }
            }

        }
        bool startCold(){
            if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
                return false;
            }
            process(EV_START);
            return true;
        }
        bool startInRx(){
            if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
                return false;
            }
            using namespace inet::physicallayer;
            ASSERT(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER);

            IRadio::ReceptionState receptionState = activeRadio->getReceptionState();
            bool isIdle = (receptionState == IRadio::RECEPTION_STATE_IDLE)
                            && activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER;
            if(isIdle){
                //Start the transmission state machine
                activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                state = BO_SWITCHING;
            }
            else{
                parent->scheduleAt(simTime()+initialBackoff, parent->txBackoffTimer);
                state = BO_WAIT;
            }

            return true;
        }
        t_backoff_state process(const t_backoff_ev& event){
            using namespace inet::physicallayer;
            switch(state){
                case BO_OFF:
                    if(event == EV_START){
                        activeRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                        state = BO_WAIT;
                    }
                    break;
                case BO_WAIT:
                    if( (event == EV_RX_READY || event == EV_BACKOFF_TIMER) && !parent->txBackoffTimer->isScheduled() ){
                        // EV_BACKOFF_TIMER triggered by need for retry of packet
                        // EV_RX_READY triggered when radio listening (after Rx->Listening transition)
                        // TODO: but check ackBackoffTimer as also triggered by (Rx->Listening)
                        // Perform carrier sense if there is a currentTxFrame
                        setRadioToTransmitIfFreeOrDelay();
                    }
                    break;
                case BO_SWITCHING:
                    if( event == EV_TX_READY ){
                        state = BO_FINISHED;
                    }
                    break;
                case BO_FINISHED:
                    break;
            }
            return state;
        }

        // Convert WakeUpMacLayer Events to BackoffBase Events
        t_backoff_state process(const WakeUpMacLayer::t_mac_event event){
            switch(event){
                case WakeUpMacLayer::EV_TX_READY:
                    return process(EV_TX_READY);
                case WakeUpMacLayer::EV_DATA_RX_READY:
                    return process(EV_RX_READY);
                case WakeUpMacLayer::EV_WAKEUP_BACKOFF:
                    return process(EV_BACKOFF_TIMER);
            }
            return state;
        }
        ~CSMATxBackoffBase(){
            if(parent->txBackoffTimer){
                cancelBackoffTimer();
            }
        }
    };
    class CSMATxUniformBackoff : public CSMATxBackoffBase{

        simtime_t max_backoff;
    public:
        CSMATxUniformBackoff(WakeUpMacLayer* parent,
                physicallayer::IRadio* activeRadio,
                power::IEpEnergyStorage* energyStorage,
                J transmissionStartMinEnergy, simtime_t _max_backoff):
                    CSMATxBackoffBase(parent, activeRadio, energyStorage, transmissionStartMinEnergy, 0),
                    max_backoff(_max_backoff){};
    protected:
        virtual simtime_t calculateBackoff(CSMATxBackoffBase::t_backoff_ev returnEv) const override{
            return parent->uniform(0.0, max_backoff);
        }
    };
    class CSMATxRemainderReciprocalBackoff : public CSMATxBackoffBase{

        simtime_t max_backoff;

    public:
        CSMATxRemainderReciprocalBackoff(WakeUpMacLayer* parent,
                physicallayer::IRadio* activeRadio,
                power::IEpEnergyStorage* energyStorage,
                J transmissionStartMinEnergy,
                simtime_t _max_backoff):
                    CSMATxBackoffBase(parent, activeRadio, energyStorage, transmissionStartMinEnergy, 0),
                    max_backoff(_max_backoff){};
    protected:
        virtual simtime_t calculateBackoff(CSMATxBackoffBase::t_backoff_ev returnEv) const override{
            // TODO:
        }
    };

    CSMATxBackoffBase* activeBackoff;
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
    bool stopDirectTxExtraAck = false;
public:
    /**
     * Mac statistics
     */
    static simsignal_t transmissionTriesSignal;
    static simsignal_t ackContentionRoundsSignal;

    /**
     * Neighbor Update signals definitions TODO: Move elsewhere
     */
    static simsignal_t expectedEncounterSignal;
    static simsignal_t coincidentalEncounterSignal;
    static simsignal_t listenForEncountersEndedSignal;

    /**
     * Mac monitoring signals
     */
    static simsignal_t wakeUpModeStartSignal;
    static simsignal_t receptionEndedSignal;
    static simsignal_t falseWakeUpEndedSignal;
    static simsignal_t transmissionModeStartSignal;
    static simsignal_t transmissionEndedSignal;
protected:
    /** @brief MAC high level states */
    enum t_mac_state {
        S_REPLENISH, // No listening, to charge storage
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

    /** @name Protocol timer messages */
    /*@{*/
    cMessage *wakeUpBackoffTimer;
    cMessage *ackBackoffTimer;
    cMessage *wuTimeout;
    /*@}*/
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
    virtual bool isLowerMessage(cMessage* message) override;
    virtual void configureInterfaceEntry() override;
    void queryWakeupRequest(Packet* wakeUp);
    simtime_t setRadioToTransmitIfFreeOrDelay(cMessage* timer, const simtime_t& maxDelay);
    void setWuRadioToTransmitIfFreeOrDelay(const t_mac_event& event, cMessage* timer, const simtime_t& maxDelay);


  protected:
    // NetFilter functions:
    // @brief called before a packet arriving from the network is accepted/acked
    virtual IHook::Result datagramPreRoutingHook(Packet *datagram);
    // @brief datagramForwardHook not implemented since MAC does not forward -> see datagramLocalOutHook
    // called before a packet arriving from the network is delivered via the network
    // @brief called before a packet is delivered via the network
    virtual IHook::Result datagramPostRoutingHook(Packet *datagram);
    // @brief called before a packet arriving from the network is delivered locally
    virtual IHook::Result datagramLocalInHook(Packet *datagram);
    // @brief called before a packet arriving locally is delivered to the network
    virtual IHook::Result datagramLocalOutHook(Packet *datagram);

    // @brief reinjecting datagram means nothing at mac layer currently
    virtual void reinjectQueuedDatagram(const Packet *datagram) override{};
    // @brief dropQueuedDatagram cannot drop due to forced const argument
    virtual void dropQueuedDatagram(const Packet* datagram) override{};

    t_mac_state macState; //Record the current state of the MAC State machine
    /** @brief Execute a step in the MAC state machine */
    void stepMacSM(const t_mac_event& event, cMessage *msg);
    simtime_t cumulativeAckBackoff = 0;
    EqDC acceptDataEqDCThreshold = EqDC(25.5);
    int rxAckRound = 0;
    virtual void stepRxAckProcess(const t_mac_event& event, cMessage *msg);
  private:
    void handleDataReceivedInAckState(cMessage *msg);
    void completePacketReception();

  protected:
    Packet* buildAck(const Packet* subject) const;
    void updateMacState(const t_mac_state& newMacState);
    /** @brief Transmitter State Machine **/
    bool txStateChange = false;
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
    void updateTxState(const t_tx_state& newTxState);
    /** @brief Wake-up listening State Machine **/

    bool wuStateChange = false;
    t_wu_state wuState;
    void stepWuSM(const t_mac_event& event, cMessage *msg);
    void updateWuState(const t_wu_state& newWuState);
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
