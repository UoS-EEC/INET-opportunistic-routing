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

#include "common/oppDefs.h"
#include "linklayer/WakeUpMacLayer.h"
#include "PacketConsumptionTracking.h"
#include "PacketConsumptionTag_m.h"
#include "networklayer/OpportunisticRoutingHeader_m.h"

using namespace oppostack;
using namespace inet;

Define_Module(PacketConsumptionTracking);

void PacketConsumptionTracking::initialize()
{
    routingTable = check_and_cast<ORPLRoutingTable*>(getCModuleFromPar(par("routingTable"), this));
    macLayer = check_and_cast<WakeUpMacLayer*>(getCModuleFromPar(par("wakeUpMacModule"),this));
    macEnergyMonitor = check_and_cast<WuMacEnergyMonitor*>(getCModuleFromPar(par("wakeUpMacMonitorModule"), this));
    macLayer->registerHook(0, this);
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramPostRoutingHook(Packet* datagram)
{
    // TODO: Fetch existing energy consumption, add estimated data tx and listening consump
    // Remove per hop tag
    auto networkHeader = datagram->removeAtFront<OpportunisticRoutingHeader>();
    auto tag = networkHeader->findTag<HopConsumptionTag>();
    if(tag!=nullptr){
        // Tag must exist from sender module
        J hopEnergy = macEnergyMonitor->calculateDeltaEnergyConsumption() + tag->getEnergyConsumed();
        networkHeader->removeTag<HopConsumptionTag>(b(0), b(-1));
    }
    else{
        EV_ERROR << "Missing Hop Consumption Tag at received node";
    }
    datagram->insertAtFront(networkHeader);
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
