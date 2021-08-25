/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_ORWHELLO_H_
#define NETWORKLAYER_ORWHELLO_H_

#include <omnetpp.h>
#include <inet/applications/generic/IpvxTrafGen.h>
#include <inet/queueing/contract/IPacketQueue.h>
#include "OpportunisticRoutingHeader_m.h"

namespace oppostack{

/**
 * Simple module to send hello messages if messages are infrequent
 */
class ORWHello : public inet::IpvxTrafGen, public inet::cListener
{
public:
    ORWHello() : inet::IpvxTrafGen(),
        sentMessageQueue(nullptr),
        packetSourceModule(nullptr){};
protected:
    virtual void initialize(int stage) override;

    virtual inet::L3Address chooseDestAddr() override;
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

#endif /* NETWORKLAYER_ORWHELLO_H_ */
