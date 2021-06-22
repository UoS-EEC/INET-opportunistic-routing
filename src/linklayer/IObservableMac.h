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

#ifndef LINKLAYER_IOBSERVABLEMAC_H_
#define LINKLAYER_IOBSERVABLEMAC_H_
#include <omnetpp.h>

namespace oppostack {

class IObservableMac
{
public:
    /**
     * Mac monitoring signals
     */
    static omnetpp::simsignal_t receptionStartedSignal;
    static omnetpp::simsignal_t receptionEndedSignal;
    static omnetpp::simsignal_t receptionDroppedSignal;
    static omnetpp::simsignal_t transmissionStartedSignal;
    static omnetpp::simsignal_t transmissionEndedSignal;
};

} /* namespace oppostack */

#endif /* LINKLAYER_IOBSERVABLEMAC_H_ */
