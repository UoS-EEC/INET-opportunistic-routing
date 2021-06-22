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

#include "IObservableMac.h"

namespace oppostack {
using namespace omnetpp;
/**
 * Mac monitoring signals
 */
simsignal_t IObservableMac::receptionStartedSignal = cComponent::registerSignal("receptionStarted");
simsignal_t IObservableMac::receptionEndedSignal = cComponent::registerSignal("receptionEnded");
simsignal_t IObservableMac::receptionDroppedSignal = cComponent::registerSignal("receptionDropped");
simsignal_t IObservableMac::transmissionStartedSignal = cComponent::registerSignal("transmissionStarted");
simsignal_t IObservableMac::transmissionEndedSignal = cComponent::registerSignal("transmissionEnded");

} /* namespace oppostack */
