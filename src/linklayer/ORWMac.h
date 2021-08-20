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

#ifndef LINKLAYER_ORWMAC_H_
#define LINKLAYER_ORWMAC_H_

// Class base
#include <inet/linklayer/contract/IMacProtocol.h>
#include <inet/linklayer/base/MacProtocolBase.h>
#include "IOpportunisticLinkLayer.h"
#include "IObservableMac.h"

// Variables within class
#include <inet/power/contract/IEpEnergyStorage.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include "WakeUpGram_m.h"
#include "CSMATxBackoff.h"

namespace oppostack {

/*
 * Mac Layer implementation of ORW Mac - Ghadimi et al.
 * Separated out from WakeUpMacLayer
 */
class ORWMac:
        public inet::IMacProtocol,
        public inet::MacProtocolBase,
        public IOpportunisticLinkLayer,
        public IObservableMac {
protected:
    /* @name In/Out Processing */
    /*@{*/
    virtual void handleUpperCommand(omnetpp::cMessage *msg) override{
        EV_WARN << "Unhandled Upper Command" << endl;
    }
    virtual void handleLowerCommand(omnetpp::cMessage *msg) override{
        EV_WARN << "Unhandled Lower Command" << endl;
    };
    /*@}*/

    /** @brief MAC high level states */
    enum class State {
        WAKE_UP_IDLE, // WuRx waiting for wake-up subclass
        DATA_IDLE, // Data radio waiting
        WAKE_UP_WAIT, // WuRx receiving or processing for wake-up subclass
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
    CSMATxBackoffBase* activeBackoff{nullptr};
    CSMATxBackoffBase::State stepBackoffSM(const MacEvent event){
        if(activeBackoff == nullptr){
            return CSMATxBackoffBase::State::WAIT;
        }
        switch(event){
            case MacEvent::TX_READY:
                return activeBackoff->process(CSMATxBackoffBase::Event::TX_READY);
            case MacEvent::DATA_RX_READY:
                return activeBackoff->process(CSMATxBackoffBase::Event::RX_READY);
            case MacEvent::CSMA_BACKOFF:
                return activeBackoff->process(CSMATxBackoffBase::Event::BACKOFF_TIMER);
            default:
                return activeBackoff->process(CSMATxBackoffBase::Event::NONE);;
        }
    }
    /** @name Protocol timer messages */
    /*@{*/
    inet::cMessage *receiveTimeout{nullptr};
    inet::cMessage *ackBackoffTimer{nullptr};
    /*@}*/

    /** @brief The radio. */
    inet::physicallayer::IRadio *dataRadio{nullptr};

    /** @brief User Configured parameters */
    omnetpp::simtime_t dataListeningDuration{0};
    omnetpp::simtime_t ackWaitDuration{0};

    /** @brief Calculated (in initialize) parameters */
    /*@{*/
    const int requiredForwarders{1};
    inet::B phyMtu{255};
    omnetpp::simtime_t initialContentionDuration{0};
    omnetpp::simtime_t ackTxWaitDuration{0};
    omnetpp::simtime_t minimumContentionWindow{0};
    /*@}*/

    inet::power::IEpEnergyStorage* energyStorage{nullptr};

    virtual void initialize(int stage) override;
    virtual void configureInterfaceEntry() override;
    virtual void cancelAllTimers();
    void deleteAllTimers();
    ~ORWMac();

    /** @name Transmit functions and variables */
    /*@{*/
    inet::J transmissionStartMinEnergy{0.0};
    int txInProgressForwarders{0};
    int maxTxTries = 1;
    int txInProgressTries = 0;
    int acknowledgedForwarders = 0;
    int acknowledgmentRound = 1;
    void setupTransmission();
    bool transmissionStartEnergyCheck() const;
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
    /*@}*/
};

} /* namespace oppostack */

#endif /* LINKLAYER_ORWMAC_H_ */
