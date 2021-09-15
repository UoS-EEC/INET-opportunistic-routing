/*
 * CSMATxBackoffBase.h
 *
 *  Created on: 11 Jun 2021
 *      Author: Edward
 */

#ifndef LINKLAYER_CSMATXBACKOFF_H_
#define LINKLAYER_CSMATXBACKOFF_H_

#include <omnetpp.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>
#include <inet/power/contract/IEpEnergyStorage.h>

using namespace omnetpp;

namespace oppostack {

class CSMATxBackoffBase{
  protected:
    cMessage *txBackoffTimer;
    inet::physicallayer::IRadio* activeRadio;
    simtime_t cumulativeAckBackoff = 0;

    bool isCarrierFree() const;
    void cancelBackoffTimer(){
        parent->cancelEvent(txBackoffTimer);
    }
  public:
    bool isBackoffTimer(cMessage* msg) const{
        return msg == txBackoffTimer;};
    enum class State {
        OFF,
        WAIT,
        SWITCHING,
        FINISHED,
    };
    enum class Event {
        START,
        BACKOFF_TIMER,
        RX_READY,
        TX_READY,
        TX_ABORT,
        NONE
    };
  protected:
    cSimpleModule* parent;
    virtual simtime_t calculateBackoff(Event& returnEv) const = 0;
    State state{State::OFF};
  public:
    CSMATxBackoffBase(cSimpleModule* _parent,
            inet::physicallayer::IRadio* _activeRadio):
                parent(_parent),
                activeRadio(_activeRadio),
                txBackoffTimer( new cMessage("tx backoff timer") ){};
    void startTxOrBackoff();
    void startTxOrDelay(simtime_t delay){ startTxOrDelay(delay, delay); };
    void startTxOrDelay(simtime_t minDelay, simtime_t maxDelay);
    void delayCarrierSense(simtime_t delay);
    void startCold();
    State process(const Event& event);

    virtual ~CSMATxBackoffBase();
};

class CSMATxUniformBackoff : public CSMATxBackoffBase{
    const simtime_t minBackoff;
    const simtime_t maxBackoff;
public:
    CSMATxUniformBackoff(cSimpleModule* parent,
            inet::physicallayer::IRadio* activeRadio,
            simtime_t min_backoff, simtime_t max_backoff):
                CSMATxBackoffBase(parent, activeRadio),
                minBackoff(min_backoff),
                maxBackoff(max_backoff){};
protected:
    virtual simtime_t calculateBackoff(Event& returnEv) const override;
};

class CSMATxRemainderReciprocalBackoff : public CSMATxBackoffBase{

    simtime_t maxBackoff;
    simtime_t minimumContentionWindow = 0;
public:
    CSMATxRemainderReciprocalBackoff(cSimpleModule* parent,
            inet::physicallayer::IRadio* activeRadio,
            simtime_t _maxBackoff, simtime_t _minimumContentionWindow):
                CSMATxBackoffBase(parent, activeRadio),
                 maxBackoff(_maxBackoff), minimumContentionWindow(_minimumContentionWindow){
        ASSERT(minimumContentionWindow > 0);
    };
protected:
    virtual simtime_t calculateBackoff(Event& returnEv) const override;
};

} /* namespace oppostack */

#endif /* LINKLAYER_CSMATXBACKOFF_H_ */
