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

bool CSMATxBackoffBase::isCarrierFree()
{
    IRadio::ReceptionState receptionState = activeRadio->getReceptionState();
    return (receptionState == IRadio::RECEPTION_STATE_IDLE)
            && activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER;
}

void CSMATxBackoffBase::startTxOrBackoff(){
    if( isCarrierFree() ){
        //Start the transmission state machine
        activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        state = BO_SWITCHING;
    }
    else{
        t_backoff_ev returnEv = EV_BACKOFF_TIMER;
        const SimTime delay = calculateBackoff(returnEv);
        if(returnEv == EV_TX_ABORT){
            state = BO_OFF;
        }
        else{
            delayCarrierSense(delay);
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

void CSMATxBackoffBase::delayCarrierSense(simtime_t delay)
{
    ASSERT(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER);

    cumulativeAckBackoff += delay;
    parent->scheduleAt(simTime()+delay, txBackoffTimer);
    state = BO_WAIT;
}

bool CSMATxBackoffBase::startTxOrDelay(simtime_t minDelay, simtime_t maxDelay){
    if(energyStorage->getResidualEnergyCapacity() < transmissionStartMinEnergy){
        return false;
    }

    if( isCarrierFree() ){
        //Start the transmission state machine
        activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        state = BO_SWITCHING;
    }
    else{
        if(minDelay == maxDelay) delayCarrierSense(minDelay);
        else delayCarrierSense( parent->uniform(minDelay, maxDelay) );
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
                startTxOrBackoff();
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
        return delay;
    }
    else{
        returnEv = EV_TX_ABORT;
        return SimTime(-1, SimTimeUnit::SIMTIME_S);
    }
}

} /* namespace oppostack */
