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

simsignal_t PacketConsumptionTracking::packetReceivedEnergyConsumedSignal = cComponent::registerSignal("packetReceivedEnergyConsumed");
simsignal_t PacketConsumptionTracking::packetReceivedEqDCSignal = cComponent::registerSignal("packetReceivedEqDC");
void oppostack::PacketConsumptionTracking::reportReception(EqDC estCost, inet::J energyConsumed)
{
    emit(packetReceivedEqDCSignal, estCost.get());
    emit(packetReceivedEnergyConsumedSignal, energyConsumed.get());
}

void PacketConsumptionTracking::accumulateHopTagToRoute(HopConsumptionTag* const hopTag, PacketConsumptionTag* const packetTag) const
{
    // Tag must exist from sender module
    J receiverEnergy = macEnergyMonitor->calculateDeltaEnergyConsumption() + hopTag->getEnergyConsumed();
    if(packetTag!=nullptr){
        // Copy hop tag elements into end of tag array
        packetTag->insertSource(hopTag->getSourceForUpdate());
        packetTag->insertEnergyConsumed(hopTag->getEnergyConsumed());
        packetTag->insertEstimatedCost(hopTag->getEstimatedCost());
    }
    else{
        EV_ERROR << "Missing PacketConsumptionTag at received node";
    }
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramPostRoutingHook(Packet* datagram)
{
    // TODO: Fetch existing energy consumption, add estimated data tx and listening consump
    b packetLength = b(datagram->getBitLength());
    auto networkHeader = datagram->removeAtFront<OpportunisticRoutingHeader>();
    auto hopTagCheck = networkHeader->findTag<HopConsumptionTag>();
    if(hopTagCheck!=nullptr){
        // Get mutable tag that already exists
        auto hopTag = networkHeader->addTagIfAbsent<HopConsumptionTag>();
        // TODO: Get from data radio parameters
        J txAckEstimate = macEnergyMonitor->calcTxAndAckEstConsumption(packetLength);
        hopTag->setEnergyConsumed(macEnergyMonitor->calculateDeltaEnergyConsumption()+txAckEstimate);
    }
    else{
        EV_ERROR << "Missing HopConsumptionTag at transmitting node" << endl;
    }
    datagram->insertAtFront(networkHeader);
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramLocalInHook(Packet* datagram)
{
    // Add energy for receiving to packet
    auto networkHeader = datagram->removeAtFront<OpportunisticRoutingHeader>();
    auto packetTag = networkHeader->addTagIfAbsent<PacketConsumptionTag>();
    auto hopTagCheck = networkHeader->findTag<HopConsumptionTag>();
    if(hopTagCheck!=nullptr){
        // Get mutable tag that already exists
        auto hopTag = networkHeader->addTagIfAbsent<HopConsumptionTag>();
        accumulateHopTagToRoute(hopTag, packetTag);
        // Remove per hop tag
        networkHeader->removeTag<HopConsumptionTag>(b(0), b(-1));
    }
    else{
        EV_ERROR << "Missing HopConsumptionTag at received node" << endl;
    }
    if(networkHeader->getDestAddr() == routingTable->getRouterIdAsGeneric()){
        // Log energy consumed for packet with reportReception() at each source component
        const size_t hops = packetTag->getSourceArraySize();
        ASSERT(hops == packetTag->getEnergyConsumedArraySize() && hops == packetTag->getEnergyConsumedArraySize());
        auto culmulativeEnergy = J(0.0);
        for(int i=hops-1;i>=0;i--){
            cComponent* source = packetTag->getSourceForUpdate(i);
            auto estimatedCost = packetTag->getEstimatedCost(i);
            culmulativeEnergy += packetTag->getEnergyConsumed(i);
            check_and_cast<PacketConsumptionTracking*>(source)->reportReception(estimatedCost, culmulativeEnergy);
        }
    }
    datagram->insertAtFront(networkHeader);
    return IHook::Result::ACCEPT;
}

INetfilter::IHook::Result PacketConsumptionTracking::datagramLocalOutHook(Packet* datagram)
{
    auto networkHeader = datagram->removeAtFront<OpportunisticRoutingHeader>();
    auto tag = networkHeader->addTag<HopConsumptionTag>(); // Must error if tag exists (undef. behaviour)
    tag->setEnergyConsumed(J(0.0)); // Dummy Value, will be overwritten in postRoutingHook
    tag->setSource(this);
    tag->setEstimatedCost(routingTable->calculateEqDC(networkHeader->getDestAddr()));
    datagram->insertAtFront(networkHeader);
    return IHook::Result::ACCEPT;
}