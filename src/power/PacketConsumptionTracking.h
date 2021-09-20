/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef POWER_PACKETCONSUMPTIONTRACKING_H_
#define POWER_PACKETCONSUMPTIONTRACKING_H_

#include <inet/networklayer/contract/INetfilter.h>

#include "linklayer/MacEnergyMonitor.h"
#include "linklayer/ORWMac.h"
#include "networklayer/RoutingTableBase.h"
#include "PacketConsumptionTag_m.h"


namespace oppostack {

class PacketConsumptionTracking : public omnetpp::cSimpleModule, public inet::NetfilterBase::HookBase
{
public:
    PacketConsumptionTracking(){}

    static simsignal_t packetReceivedEqDCSignal;
    static simsignal_t packetReceivedEnergyConsumedSignal;
protected:
    virtual void initialize() override;
    MacEnergyMonitor* macEnergyMonitor;
    RoutingTableBase* routingTable; // TODO: Replace with IRoutingTable
    ORWMac* macLayer;

public:
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet*) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramPostRoutingHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramLocalInHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramLocalOutHook(inet::Packet *datagram) override;
    void reportReception(EqDC estCost, inet::J energyConsumed);
private:
    void accumulateHopTagToRoute(HopConsumptionTag* tag, PacketConsumptionTag* packetTag) const;
};

} /* namespace oppostack */

#endif /* POWER_PACKETCONSUMPTIONTRACKING_H_ */
