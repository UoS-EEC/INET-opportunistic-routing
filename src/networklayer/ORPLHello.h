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

#ifndef NETWORKLAYER_ORPLHELLO_H_
#define NETWORKLAYER_ORPLHELLO_H_

#include <omnetpp.h>
#include <inet/applications/generic/IpvxTrafGen.h>
#include <inet/queueing/contract/IPacketQueue.h>
#include "OpportunisticRoutingHeader_m.h"

namespace oppostack{

/**
 * Simple module to send hello messages if messages are infrequent
 */
class ORPLHello : public inet::IpvxTrafGen, public inet::cListener
{
public:
    ORPLHello() : inet::IpvxTrafGen(),
        sentMessageQueue(nullptr),
        packetSourceModule(nullptr){};
protected:
    virtual void initialize(int stage) override;

    virtual inet::L3Address chooseDestAddr();
    virtual void sendPacket();

    virtual void handleStartOperation(inet::LifecycleOperation* op) override;
protected:
    double minTransmissionProbability = 0;
    inet::queueing::IPacketQueue* sentMessageQueue;
    omnetpp::cModule* packetSourceModule;
    int onOffCycles = 0;

    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, cObject* msg, cObject *details) override;

    std::pair<int, inet::L3Address> quietestDestination() const;
};

} //namespace oppostack

#endif /* NETWORKLAYER_ORPLHELLO_H_ */
