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

#include <inet/linklayer/contract/IMacProtocol.h>
#include <inet/linklayer/base/MacProtocolBase.h>
#include "IOpportunisticLinkLayer.h"
#include "IObservableMac.h"

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
protected:
    virtual void initialize(int stage) override;
    virtual void cancelAllTimers();
    void deleteAllTimers();
    ~ORWMac();
};

} /* namespace oppostack */

#endif /* LINKLAYER_ORWMAC_H_ */
