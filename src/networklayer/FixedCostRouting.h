/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_FIXEDCOSTROUTING_H_
#define NETWORKLAYER_FIXEDCOSTROUTING_H_

#include <inet/networklayer/base/NetworkProtocolBase.h>
#include <inet/networklayer/contract/INetworkProtocol.h>

#include "RoutingTableBase.h"
namespace oppostack {

class FixedCostRouting : public inet::NetworkProtocolBase, public inet::INetworkProtocol{
protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    RoutingTableBase *routingTable; // TODO: Make IRoutingTable if features allow
    virtual void setDownControlInfo(inet::Packet* packet, const inet::MacAddress& macMulticast, const EqDC& routingCost, const EqDC& onwardCost) const;

    virtual void handleUpperPacket(inet::Packet* packet) override;
    virtual void handleLowerPacket(inet::Packet* packet) override;

    const inet::Protocol& getProtocol() const override;

    // OperationalBase : No timers, so no operations required
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override{};
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override{};
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_FIXEDCOSTROUTING_H_ */
