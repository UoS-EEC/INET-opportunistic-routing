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

#include <inet/common/ModuleAccess.h>
#include "PacketConsumptionTracking.h"
#include "PacketConsumptionTag_m.h"
#include "networklayer/OpportunisticRoutingHeader_m.h"

using namespace oppostack;
using namespace inet;

void PacketConsumptionTracking::initialize(int stage)
{
    if(stage == INITSTAGE_NETWORK_LAYER){
        routingTable = getModuleFromPar<ORPLRoutingTable>(par("routingTable"), this);
        macEnergyMonitor = getModuleFromPar<WuMacEnergyMonitor>(par("wakeUpMacMonitorModule"), this);
    }
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramPostRoutingHook(Packet* datagram)
{
    // TODO: Fetch existing energy consumption, add estimated data tx and listening consump
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramLocalInHook(Packet* datagram)
{
    // TODO: Add energy for receiving to packet
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramLocalOutHook(Packet* datagram)
{
    // TODO: Add empty tag to packet
    auto networkHeader = datagram->removeAtFront<OpportunisticRoutingHeader>();
    auto tag = networkHeader->addTag<HopConsumptionTag>(); // Must error if tag exists (undef. behaviour)
    tag->setEnergyConsumed(J(0.0));
    tag->setSource(routingTable->getRouterIdAsGeneric());
    tag->setEstimatedCost(routingTable->calculateEqDC(networkHeader->getDestAddr()));
    datagram->insertAtFront(networkHeader);
    return IHook::Result::ACCEPT;
}
