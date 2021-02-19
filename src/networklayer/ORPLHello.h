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
#include <inet/applications/base/ApplicationBase.h>
#include <inet/queueing/contract/IPacketQueue.h>
#include "OpportunisticRoutingHeader_m.h"

namespace oppostack{

/**
 * Simple module to send hello messages if messages are infrequent
 */
class ORPLHello : public inet::ApplicationBase, public inet::cListener
{
public:
    ORPLHello() : inet::ApplicationBase(),
        retransmissionTimer(nullptr),
        sentMessageQueue(nullptr),
        packetSourceModule(nullptr){};
    ~ORPLHello();
protected:
    // From Ipvx Traff gen
    omnetpp::simtime_t startTime;
    omnetpp::simtime_t stopTime;
    // ...
    int numPackets = 0;

    inet::cMessage* retransmissionTimer;
    omnetpp::simtime_t retransmissionDelay = 0;
    double minTransmissionProbability = 0;
    inet::queueing::IPacketQueue* sentMessageQueue;
    omnetpp::cModule* packetSourceModule;
    inet::L3Address helloDestination;//TODO: Make plural when relevant
    int onOffCycles = 0;
    const inet::Protocol* protocol;

    virtual void initialize(int stage) override;

    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, cObject* msg, cObject *details) override;
    virtual void handleStartOperation(inet::LifecycleOperation* op) override;
    virtual void handleStopOperation(inet::LifecycleOperation* op) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* op) override;

    virtual void handleMessageWhenUp(omnetpp::cMessage *message); // From LayeredProtocolBase
    virtual void handleSelfMessage(omnetpp::cMessage* msg);
    virtual void sendHelloBroadcast(inet::L3Address destination);
    void rescheduleTransmissionTimer();
};

} //namespace oppostack

#endif /* NETWORKLAYER_ORPLHELLO_H_ */
