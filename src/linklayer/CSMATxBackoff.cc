/*
 * CSMATxBackoffBase.cpp
 *
 *  Created on: 11 Jun 2021
 *      Author: Edward
 */

#include "CSMATxBackoff.h"

namespace oppostack {
using namespace inet;
using namespace inet::physicallayer;

void CSMATxBackoffBase::setRadioToTransmitIfFreeOrDelay(){
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
        const SimTime nextAckAttempt = simTime() + calculateBackoff(returnEv);
        if(returnEv == EV_TX_ABORT){
            state = BO_OFF;
        }
        else{
            parent->scheduleAt(nextAckAttempt, txBackoffTimer);
            state = BO_WAIT;
        }
    }

}

bool CSMATxBackoffBase::startCold(){
    if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
        return false;
    }
    process(EV_START);
    return true;
}

bool CSMATxBackoffBase::startInRx(){
    if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
        return false;
    }
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
        parent->scheduleAt(simTime()+initialBackoff, txBackoffTimer);
        state = BO_WAIT;
    }

    return true;
}

CSMATxBackoffBase::t_backoff_state CSMATxBackoffBase::process(const t_backoff_ev& event){
    switch(state){
        case BO_OFF:
            if(event == EV_START){
                activeRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                state = BO_WAIT;
            }
        case BO_WAIT:
            if( (event == EV_RX_READY || event == EV_BACKOFF_TIMER) && !txBackoffTimer->isScheduled() ){
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

CSMATxBackoffBase::~CSMATxBackoffBase(){
    if(txBackoffTimer){
        cancelBackoffTimer();
        delete txBackoffTimer;
        txBackoffTimer = nullptr;
    }
}

simtime_t CSMATxUniformBackoff::calculateBackoff(t_backoff_ev& returnEv){
    return parent->uniform(minBackoff, maxBackoff);
}

simtime_t CSMATxRemainderReciprocalBackoff::calculateBackoff(t_backoff_ev& returnEv){
    // Calculate backoff from remaining time
    const simtime_t delayWindow = (maxBackoff - cumulativeAckBackoff)/2;
    if(delayWindow > minimumContentionWindow){
        // Add resultant random backoff to cumulative total
        auto delay = parent->uniform(0,delayWindow);
        cumulativeAckBackoff += delay;
        return delay;
    }
    else{
        returnEv = EV_TX_ABORT;
        return SimTime(-1, SimTimeUnit::SIMTIME_S);
    }
}

bool CSMATxRemainderReciprocalBackoff::startInRxFromInitialWindow()
{
    if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
        return false;
    }
    ASSERT(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER);

    cumulativeAckBackoff = parent->uniform(0, initialContentionWindow);
    parent->scheduleAt(simTime()+cumulativeAckBackoff, txBackoffTimer);
    state = BO_WAIT;
    return true;
}

} /* namespace oppostack */
