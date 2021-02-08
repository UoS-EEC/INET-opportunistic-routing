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

using namespace oppostack;
using namespace inet;

void PacketConsumptionTracking::initialize(int stage)
{
    if(stage == INITSTAGE_NETWORK_LAYER){
        routingTable = getModuleFromPar<ORPLRoutingTable>(par("routingTable"), this);
        macEnergyMonitor = getModuleFromPar<WuMacEnergyMonitor>(par("wakeUpMacMonitorModule"), this);
    }
}

INetfilter::IHook::Result oppostack::PacketConsumptionTracking::datagramPostRoutingHook(Packet* datagram)
{
    // TODO: Fetch existing energy consumption, add estimated data tx and listening consump
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result oppostack::PacketConsumptionTracking::datagramLocalInHook(Packet* datagram)
{
    // TODO: Add energy for receiving to packet
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result oppostack::PacketConsumptionTracking::datagramLocalOutHook(Packet* datagram)
{
    // TODO: Add empty tag to packet
    return IHook::Result::ACCEPT;
}
