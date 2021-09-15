/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "common/oppDefs.h"
#include "linklayer/ORWMac.h"
#include "PacketConsumptionTracking.h"
#include "PacketConsumptionTag_m.h"
#include "networklayer/OpportunisticRoutingHeader_m.h"

using namespace oppostack;
using namespace inet;

Define_Module(PacketConsumptionTracking);

void PacketConsumptionTracking::initialize()
{
    routingTable = check_and_cast<ORWRoutingTable*>(getCModuleFromPar(par("routingTable"), this));
    macLayer = check_and_cast<ORWMac*>(getCModuleFromPar(par("wakeUpMacModule"),this));
    macEnergyMonitor = check_and_cast<MacEnergyMonitor*>(getCModuleFromPar(par("wakeUpMacMonitorModule"), this));
    macLayer->registerHook(0, this);
}

simsignal_t PacketConsumptionTracking::packetReceivedEnergyConsumedSignal = cComponent::registerSignal("packetReceivedEnergyConsumed");
simsignal_t PacketConsumptionTracking::packetReceivedEqDCSignal = cComponent::registerSignal("packetReceivedEqDC");
void oppostack::PacketConsumptionTracking::reportReception(EqDC estCost, inet::J energyConsumed)
{
    emit(packetReceivedEqDCSignal, estCost.get());
    emit(packetReceivedEnergyConsumedSignal, energyConsumed.get());
}

void PacketConsumptionTracking::accumulateHopTagToRoute(Ptr<HopConsumptionTag> const hopTag, Ptr<PacketConsumptionTag> const packetTag) const
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
    auto hopTagCheck = networkHeader->findTag<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
    if(hopTagCheck!=nullptr){
        // Get mutable tag that already exists
        auto hopTag = networkHeader->addTagIfAbsent<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
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
    auto packetTag = networkHeader->addTagIfAbsent<PacketConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
    auto hopTagCheck = networkHeader->findTag<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
    if(hopTagCheck!=nullptr){
        // Get mutable tag that already exists
        auto hopTag = networkHeader->addTagIfAbsent<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
        accumulateHopTagToRoute(hopTag, packetTag);
        // Remove per hop tag
        networkHeader->removeTag<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength));
    }
    else{
        EV_ERROR << "Missing HopConsumptionTag at received node" << endl;
    }
    // If packet is at the destination, reportReception cost to destination to forwarding and source nodes
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
    // Offset and length passed as kludge to allow chunkLength to change without readding region tags
    auto tag = networkHeader->addTag<HopConsumptionTag>(B(0),B(OpportunisticRoutingHeader::headerByteLength)); // Must error if tag exists (undef. behaviour)
    tag->setEnergyConsumed(J(0.0)); // Dummy Value, will be overwritten in postRoutingHook
    tag->setSource(this);
    tag->setEstimatedCost(routingTable->calculateUpwardsCost(networkHeader->getDestAddr()));
    datagram->insertAtFront(networkHeader);
    return IHook::Result::ACCEPT;
}
