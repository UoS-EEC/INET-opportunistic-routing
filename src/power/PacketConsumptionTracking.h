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

#ifndef POWER_PACKETCONSUMPTIONTRACKING_H_
#define POWER_PACKETCONSUMPTIONTRACKING_H_

#include <inet/networklayer/contract/INetfilter.h>

#include "linklayer/ORWMac.h"
#include "../networklayer/ORWRoutingTable.h"
#include "linklayer/WuMacEnergyMonitor.h"
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
    WuMacEnergyMonitor* macEnergyMonitor;
    ORWRoutingTable* routingTable; // TODO: Replace with IRoutingTable
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
