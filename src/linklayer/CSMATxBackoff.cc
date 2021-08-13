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

bool CSMATxBackoffBase::isCarrierFree() const
{
    IRadio::ReceptionState receptionState = activeRadio->getReceptionState();
    return (receptionState == IRadio::RECEPTION_STATE_IDLE)
            && activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER;
}

void CSMATxBackoffBase::startTxOrBackoff(){
    if( isCarrierFree() ){
        //Start the transmission state machine
        activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        state = State::SWITCHING;
    }
    else{
        Event returnEv = Event::BACKOFF_TIMER;
        const SimTime delay = calculateBackoff(returnEv);
        if(returnEv == Event::TX_ABORT){
            state = State::OFF;
        }
        else{
            delayCarrierSense(delay);
        }
    }

}

void CSMATxBackoffBase::startCold(){
    process(Event::START);
}

void CSMATxBackoffBase::delayCarrierSense(simtime_t delay)
{
    ASSERT(activeRadio->getRadioMode() == IRadio::RADIO_MODE_RECEIVER);

    cumulativeAckBackoff += delay;
    parent->scheduleAt(simTime()+delay, txBackoffTimer);
    state = State::WAIT;
}

void CSMATxBackoffBase::startTxOrDelay(simtime_t minDelay, simtime_t maxDelay){
    if( isCarrierFree() ){
        //Start the transmission state machine
        activeRadio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        state = State::SWITCHING;
    }
    else{
        if(minDelay == maxDelay) delayCarrierSense(minDelay);
        else delayCarrierSense( parent->uniform(minDelay, maxDelay) );
    }
}

CSMATxBackoffBase::State CSMATxBackoffBase::process(const Event& event){
    switch(state){
        case State::OFF:
            if(event == Event::START){
                activeRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                state = State::WAIT;
            }
            break;
        case State::WAIT:
            if( (event == Event::RX_READY || event == Event::BACKOFF_TIMER) && !txBackoffTimer->isScheduled() ){
                // Event::BACKOFF_TIMER triggered by need for retry of packet
                // Event::RX_READY triggered when radio listening (after Rx->Listening transition)
                // TODO: but check ackBackoffTimer as also triggered by (Rx->Listening)
                // Perform carrier sense if there is a currentTxFrame
                startTxOrBackoff();
            }
            break;
        case State::SWITCHING:
            if( event == Event::TX_READY ){
                state = State::FINISHED;
            }
            break;
        case State::FINISHED:
            break;
    }
    return state;
}

CSMATxBackoffBase::~CSMATxBackoffBase(){
    cancelBackoffTimer();
    delete txBackoffTimer;
}

simtime_t CSMATxUniformBackoff::calculateBackoff(Event& returnEv) const{
    return parent->uniform(minBackoff, maxBackoff);
}

simtime_t CSMATxRemainderReciprocalBackoff::calculateBackoff(Event& returnEv) const{
    // Calculate backoff from remaining time
    const simtime_t delayWindow = (maxBackoff - cumulativeAckBackoff)/2;
    if(delayWindow > minimumContentionWindow){
        // Add resultant random backoff to cumulative total
        auto delay = parent->uniform(0,delayWindow);
        return delay;
    }
    else{
        returnEv = Event::TX_ABORT;
        return SimTime(-1, SimTimeUnit::SIMTIME_S);
    }
}

} /* namespace oppostack */
