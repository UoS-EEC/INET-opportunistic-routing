/*
 * CSMATxBackoffBase.h
 *
 *  Created on: 11 Jun 2021
 *      Author: Edward
 */

#ifndef LINKLAYER_CSMATXBACKOFF_H_
#define LINKLAYER_CSMATXBACKOFF_H_

#include <omnetpp.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>
#include <inet/power/contract/IEpEnergyStorage.h>

using namespace omnetpp;

namespace oppostack {

class CSMATxBackoffBase{
  protected:
    cMessage *txBackoffTimer;
    inet::physicallayer::IRadio* activeRadio;
    simtime_t cumulativeAckBackoff = 0;

    bool isCarrierFree();
    void cancelBackoffTimer(){
        parent->cancelEvent(txBackoffTimer);
    }
  public:
    bool isBackoffTimer(cMessage* msg){return msg == txBackoffTimer;};
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
        EV_NULL
    };
  protected:
    cSimpleModule* parent;
    virtual simtime_t calculateBackoff(t_backoff_ev& returnEv) = 0;
    t_backoff_state state = BO_OFF;
  public:
    CSMATxBackoffBase(cSimpleModule* _parent,
            inet::physicallayer::IRadio* _activeRadio):
                parent(_parent),
                activeRadio(_activeRadio){
        txBackoffTimer = new cMessage("tx backoff timer");
    };
    void startTxOrBackoff();
    void startTxOrDelay(simtime_t delay){ startTxOrDelay(delay, delay); };
    void startTxOrDelay(simtime_t minDelay, simtime_t maxDelay);
    void delayCarrierSense(simtime_t delay);
    void startCold();
    t_backoff_state process(const t_backoff_ev& event);

    virtual ~CSMATxBackoffBase();
};

class CSMATxUniformBackoff : public CSMATxBackoffBase{
    simtime_t minBackoff;
    simtime_t maxBackoff;
public:
    CSMATxUniformBackoff(cSimpleModule* parent,
            inet::physicallayer::IRadio* activeRadio,
            simtime_t min_backoff, simtime_t max_backoff):
                CSMATxBackoffBase(parent, activeRadio),
                minBackoff(min_backoff),
                maxBackoff(max_backoff){};
protected:
    virtual simtime_t calculateBackoff(t_backoff_ev& returnEv) override;
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
    virtual simtime_t calculateBackoff(t_backoff_ev& returnEv) override;
};

} /* namespace oppostack */

#endif /* LINKLAYER_CSMATXBACKOFF_H_ */
