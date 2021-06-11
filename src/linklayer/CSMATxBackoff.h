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
  private:
    simtime_t initialBackoff;
    cMessage *txBackoffTimer;
    inet::physicallayer::IRadio* activeRadio;
    inet::power::IEpEnergyStorage* energyStorage;
    inet::J transmissionStartMinEnergy = inet::J(0.0);

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
    virtual simtime_t calculateBackoff(t_backoff_ev returnEv) const = 0;

    t_backoff_state state = BO_OFF;
  public:
    CSMATxBackoffBase(cSimpleModule* _parent,
            inet::physicallayer::IRadio* _activeRadio,
            inet::power::IEpEnergyStorage* _energyStorage,
            inet::J _transmissionStartMinEnergy,
            simtime_t initial_backoff):
                parent(_parent),
                activeRadio(_activeRadio),
                energyStorage(_energyStorage),
                transmissionStartMinEnergy(_transmissionStartMinEnergy),
                initialBackoff(initial_backoff){
        txBackoffTimer = new cMessage("tx backoff timer");
    };
    void setRadioToTransmitIfFreeOrDelay();
    bool startCold();
    bool startInRx();
    t_backoff_state process(const t_backoff_ev& event);

    virtual ~CSMATxBackoffBase();
};

class CSMATxUniformBackoff : public CSMATxBackoffBase{
    simtime_t minBackoff;
    simtime_t maxBackoff;
public:
    CSMATxUniformBackoff(cSimpleModule* parent,
            inet::physicallayer::IRadio* activeRadio,
            inet::power::IEpEnergyStorage* energyStorage,
            inet::J transmissionStartMinEnergy, simtime_t min_backoff, simtime_t max_backoff):
                CSMATxBackoffBase(parent, activeRadio, energyStorage, transmissionStartMinEnergy, 0),
                minBackoff(min_backoff),
                maxBackoff(max_backoff){};
protected:
    virtual simtime_t calculateBackoff(t_backoff_ev returnEv) const override;
};

class CSMATxRemainderReciprocalBackoff : public CSMATxBackoffBase{

    simtime_t max_backoff;

public:
    CSMATxRemainderReciprocalBackoff(cSimpleModule* parent,
            inet::physicallayer::IRadio* activeRadio,
            inet::power::IEpEnergyStorage* energyStorage,
            inet::J transmissionStartMinEnergy,
            simtime_t _max_backoff):
                CSMATxBackoffBase(parent, activeRadio, energyStorage, transmissionStartMinEnergy, 0),
                max_backoff(_max_backoff){};
protected:
    virtual simtime_t calculateBackoff(t_backoff_ev returnEv) const override;
};

} /* namespace oppostack */

#endif /* LINKLAYER_CSMATXBACKOFF_H_ */
