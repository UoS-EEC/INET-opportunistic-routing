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
    void setupTransmission();
    bool transmissionStartEnergyCheck() const;
    /*@}*/
};

} /* namespace oppostack */

#endif /* LINKLAYER_ORWMAC_H_ */
