/*
 * ORPLRouting.cpp
 *
 *  Created on: 24 Mar 2021
 *      Author: Edward
 */

#include "ORPLRouting.h"
#include "ORPLRoutingTable.h"
#include "OpportunisticRoutingHeader_m.h"
#include "../common/EqDCTag_m.h"
#include "RoutingSetExt_m.h"
#include <inet/networklayer/nexthop/NextHopRoute.h>
#include "../common/Util.h"
#include <set>

using namespace oppostack;
Define_Module(ORPLRouting);

void oppostack::ORPLRouting::initialize(int stage)
{
    ORWRouting::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        check_and_cast<ORPLRoutingTable*>(routingTable);
        routingSetProportion = par("routingSetProportion");
        routingSetBroadcastProportion = par("routingSetBroadcastProportion");
        ASSERT(routingSetProportion > 0.05);
        ASSERT(routingSetBroadcastProportion > 0.2);
    }
}

void ORPLRouting::forwardPacket(EqDC ownCost, EqDC nextHopCost, Packet* const packet)
{
    // Update ORPL Specific fields from tags
    auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
    auto upwardsTag = packet->findTag<EqDCUpwards>();
    if(upwardsTag == nullptr)
        mutableHeader->setIsUpwards(true);
    else
        mutableHeader->setIsUpwards(upwardsTag->isUpwards());
    // Set upwards identifier
    packet->insertAtFront(mutableHeader);
    ORWRouting::forwardPacket(ownCost, nextHopCost, packet);
}

void ORPLRouting::handleLowerPacket(Packet* const packet)
{
    auto routingHeader = packet->peekAtFront<OpportunisticRoutingHeader>();
    auto headerOptions = routingHeader->getOptions();
    auto routingSetExtId = headerOptions.findByType(RoutingSetExt::extType,0);
    if(routingSetExtId!=-1){
        // Remove and delete the shared routing set from the incoming packet
        auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
        auto mutableOptions = mutableHeader->getOptionsForUpdate();
        mutableOptions.eraseTlvOption(routingSetExtId);
        mutableHeader->setOptions(mutableOptions);
        mutableHeader->setChunkLength(mutableHeader->calculateHeaderByteLength());
        packet->insertAtFront(mutableHeader);
    }

    ORWRouting::handleLowerPacket(packet);
}

std::set<L3Address> ORPLRouting::getSharingRoutingSet() const
{
    // Only get the routing set that should be shared, excluding some directly connected nodes
    auto routingTable = check_and_cast<ORPLRoutingTable*>(this->routingTable);
    size_t knownDestsCount = routingTable->getNumRoutes();
    // Might be smaller but guaranteed not to exceed knownDests size
    std::set<L3Address> sharingRoutingSet;
    std::set<L3Address> excludedRoutingSet;
    // TODO: Should forwarding cost be included in minDownwardsMetric?
    EqDC minDownwardsMetric = routingTable->calculateUpwardsCost(rootAddress);
    for (int i = 0; i < knownDestsCount; i++) {
        // TODO: get destRoute directly from routing table with getRoute
        std::pair<L3Address, int> destinationExpectedCostPair = routingTable->getRoute(i);
        NextHopRoute destRoute;
        destRoute.setDestination(destinationExpectedCostPair.first);
        destRoute.setMetric(destinationExpectedCostPair.second);
        if (ExpectedCost(destRoute.getMetric()) >= minDownwardsMetric) {
            // add to the end of sharingRoutingSet and increment size
            sharingRoutingSet.insert(destRoute.getDestinationAsGeneric());
            if (contains(excludedRoutingSet, destRoute.getDestinationAsGeneric())) {
                EV_ERROR << "Node appears in both excluded set and sharing set. Potential routing loop.";
            }
        }
        else // else exclude neighbor from sharing set as not downwards
        {
            excludedRoutingSet.insert(destRoute.getDestinationAsGeneric());
            if (contains(sharingRoutingSet, destRoute.getDestinationAsGeneric())) {
                EV_ERROR << "Node appears in both excluded set and sharing set. Potential routing loop.";
            }
        }
    }
    return sharingRoutingSet;
}

void ORPLRouting::setDownControlInfo(Packet* const packet, const MacAddress& macMulticast, const EqDC& costIndicator, const EqDC& onwardCost) const
{
    // TODO: Add routingSetExt TlvOption header occasionally
    bool isBroadcast = packet->findTag<EqDCBroadcast>() != nullptr;
    double addRoutingSetRand = uniform(0,1);
    if(isBroadcast && addRoutingSetRand < routingSetBroadcastProportion || addRoutingSetRand < routingSetProportion){
        // Only get the routing set that should be shared, excluding some directly connected nodes
        std::set<L3Address> sharingRoutingSet = getSharingRoutingSet();
        // Insert routing set into routingSetExt header if there are neighboring nodes.
        if(sharingRoutingSet.size() > 0){
            auto routingSetExtension = new RoutingSetExt();
            for(auto sharingEntry: sharingRoutingSet){
                routingSetExtension->insertEntry(sharingEntry);
            }

            auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
            auto mutableOptions = mutableHeader->getOptionsForUpdate();
            mutableOptions.insertTlvOption(routingSetExtension);
            mutableHeader->setOptions(mutableOptions);
            mutableHeader->setChunkLength(mutableHeader->calculateHeaderByteLength());
            packet->insertAtFront(mutableHeader);
        }
    }
    ORWRouting::setDownControlInfo(packet, macMulticast, costIndicator, onwardCost);
}

