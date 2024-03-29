/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "inet/common/INETUtils.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "ORPLRoutingTable.h"
#include "OpportunisticRoutingHeader_m.h"
#include "RoutingSetExt_m.h"
#include <algorithm>
#include <functional>

#include "../linklayer/ORWGram_m.h"
#include "common/oppDefs.h"
using namespace oppostack;
using namespace inet;

Define_Module(ORPLRoutingTable);

simsignal_t ORPLRoutingTable::downwardSetSizeSignal = cComponent::registerSignal("downwardSetSize");

int ORPLRoutingTable::getNumRoutes()
{
    Enter_Method("ORPLRoutingTable::getNumRoutes()");
    auto isNeighborEntryActive = [=](std::pair<inet::L3Address, NeighborEntry> node)
        {return node.second.recentInteractionProb > 0;};
    const int activeRoutingSetLength = std::count_if(routingSetTable.begin(), routingSetTable.end(), isNeighborEntryActive );
    const int activeEncountersLength =  std::count_if(encountersTable.begin(), encountersTable.end(), isNeighborEntryActive );
    return activeRoutingSetLength + activeEncountersLength;
}

// TODO: replace with IRoute*
std::pair<const L3Address ,int > ORPLRoutingTable::getRoute(int k)
{
    Enter_Method("ORPLRoutingTable::getRoute(k)");
    auto isNeighborEntryActive = [=](std::pair<inet::L3Address, NeighborEntry> node)
        {return node.second.recentInteractionProb > 0;};
    const int activeRoutingSetLength = std::count_if(routingSetTable.begin(), routingSetTable.end(), isNeighborEntryActive );
    const int activeEncountersLength =  std::count_if(encountersTable.begin(), encountersTable.end(), isNeighborEntryActive );
    ASSERT(k < activeRoutingSetLength + activeEncountersLength);

    // get the kth active in joined routingSet and Encounters
    // Big kludge, needs to store all routes together in std::vector<IRoute*> and return pointer automatically
    if (k < activeRoutingSetLength){
        // must be in RoutingSet
        int activeCount = 0;
        for(auto node: routingSetTable){
            if(isNeighborEntryActive(node)){
                if(activeCount == k)return std::make_pair(node.first,ExpectedCost(node.second.lastEqDC).get());
                activeCount++;
            }
        }
    }
    else{
        // must be in encountersTable
        int activeCount = 0;
        for(auto node: encountersTable){
            if(isNeighborEntryActive(node)){
                if(activeCount == k-activeRoutingSetLength)return std::make_pair(node.first,ExpectedCost(node.second.lastEqDC).get());
                activeCount++;
            }
        }
    }
    throw cRuntimeError("Unknown route requested");
}


EqDC ORPLRoutingTable::calculateUpwardsCost(const inet::L3Address destination) const
{
    Enter_Method("ORPLRoutingTable::calculateUpwardsCost(address)");
    const NetworkInterface* interface = interfaceTable->findFirstNonLoopbackInterface();

    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    else if(interface->getNetworkAddress() == rootAddress && destination != rootAddress){
        // This node is the root and the destination is not the root return minimum cost
        // As packet will be accepted but not delivered here
        if(forwardingCostW < EqDC(0.1)){
            return ExpectedCost(EqDC(0.1));
        }
        return ExpectedCost(forwardingCostW);

    }
    else{
        // Node can calculate UpwardsCost to root, all other dest are meaningless
        return ORWRoutingTable::calculateUpwardsCost(rootAddress);
    }
}

EqDC ORPLRoutingTable::calculateDownwardsCost(const inet::L3Address& destination) const
{
    Enter_Method("ORPLRoutingTable::calculateDownwardsCost(address)");

    EqDC ownEqDCEstimate = calculateCostToRoot();

    // Utility functions
    // TODO: Clarify if checking both recentInteractionProb and node.interactionsTotal is problematic
    // When is recentInteractionProb == 0 but interactionsTotal > 2
    auto isNeighborEntryActive = [=](const NeighborEntry node)
        {return node.recentInteractionProb > 0 || node.interactionsTotal > 2;};
    auto isNeighborEntryDownwards = [=](const NeighborEntry node)
        {return node.lastEqDC >= ownEqDCEstimate;};

    const auto& immediateNeighbor = encountersTable.find(destination);
    const auto& downwardsSetNode = routingSetTable.find(destination);
    const NetworkInterface* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    else if(immediateNeighbor != encountersTable.end() && isNeighborEntryActive(immediateNeighbor->second) && isNeighborEntryDownwards(immediateNeighbor->second)){
        if(forwardingCostW <= EqDC(0.1)){
            return EqDC(0.1);
        }
        return EqDC(forwardingCostW);
    }
    else if(downwardsSetNode != routingSetTable.end() && isNeighborEntryActive(downwardsSetNode->second)){
        if(forwardingCostW <= EqDC(0.1)){
            return 2.0*EqDC(0.1);
        }
        return 2.0*EqDC(forwardingCostW);
    }
    const EqDC estimatedCost = EqDC(25.5);
    return ExpectedCost(estimatedCost);
}

INetfilter::IHook::Result ORPLRoutingTable::datagramPreRoutingHook(Packet* datagram)
{
    auto routingDecision = ORWRoutingTable::datagramPreRoutingHook(datagram);
    if(routingDecision != IHook::Result::ACCEPT){
        EqDC downwardRoutingThreshold;
        auto routingHeader = getOpportunisticRoutingHeaderFromPacket((cObject*) datagram, downwardRoutingThreshold);
        bool isDownwards = routingHeader != nullptr && !routingHeader->isUpwards();
        if(isDownwards && downwardRoutingThreshold < calculateUpwardsCost(rootAddress)){
            // Check if destination is in routingSet
            if(calculateDownwardsCost(routingHeader->getDestAddr()) < EqDC(25.5) ){
                return IHook::Result::ACCEPT;
            }
        }
    }
    return routingDecision;
}

void ORPLRoutingTable::activateWarmUpRoutingData()
{
    EqDC ownEqDCEstimate = calculateUpwardsCost(rootAddress);
    for(auto& node: routingSetTable){
        if(node.second.interactionsTotal > 0 && node.second.lastEqDC > ownEqDCEstimate){
            node.second.recentInteractionProb = 1.0;
        }
        else{
            node.second.recentInteractionProb = 0.0;
        }
        node.second.interactionsTotal = 0;
    }
    ORWRoutingTable::activateWarmUpRoutingData();

    // Emit downwards nodes information (including immediate downward neighbours)
    int downwardsSetSize = countDownwardNodes(ownEqDCEstimate);
    emit(downwardSetSizeSignal, downwardsSetSize);

}

void ORPLRoutingTable::initialize(int stage)
{
    ORWRoutingTable::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        cModule* encountersModule = getCModuleFromPar(par("encountersSourceModule"), this);
        encountersModule->subscribe(packetReceivedFromLowerSignal, this);
        (bool)par("printRoutingTables");// Check that the parameter is the correct value for later use
    }
}

const Ptr<const OpportunisticRoutingHeader> ORPLRoutingTable::getOpportunisticRoutingHeaderFromPacket(const cObject* msg, EqDC& indicatedMinCostToSink)
{
    // Peek chunk at start
    auto packet = check_and_cast<const Packet*>(msg);
    auto firstHeader = packet->peekAtFront();
    // Drop if chunk at start the only thing in the packet
    auto secondHeaderOffset = firstHeader->getChunkLength();
    if( secondHeaderOffset >= packet->getDataLength() ){
        return nullptr;
    }
    // Check and convert network header to OpportunisticRoutingHeader
    if( packet->hasAt<OpportunisticRoutingHeader>(secondHeaderOffset)){
        // TODO: Remove when EqDC indicator sent in OpportunisticRoutingHeader
        if(packet->hasAtFront<ORWGram>()){
            indicatedMinCostToSink = packet->peekAtFront<ORWGram>()->getExpectedCostInd();
        }

        // Peek at Network Header using offset of length of chunk at start.
        return packet->peekAt<OpportunisticRoutingHeader>(secondHeaderOffset);
    }
    return nullptr;
}

void ORPLRoutingTable::addToDownwardsWarmupSet(const inet::L3Address destination, const EqDC minimumCostToRoot)
{
    auto isFirstSinceReset = routingSetTable[destination].interactionsTotal == 0;
    auto minimumCostHasIncreased = routingSetTable[destination].lastEqDC < minimumCostToRoot;
    auto hasNeverHadRecordedCost = routingSetTable[destination].lastEqDC == EqDC(25.5); // Needed for initialization when many nodes have cost=25.5
    routingSetTable[destination].interactionsTotal++; // incremented AFTER isFirstSinceReset definition;
    if( isFirstSinceReset || minimumCostHasIncreased || hasNeverHadRecordedCost){
        routingSetTable[destination].lastEqDC = minimumCostToRoot;
    }
}

void ORPLRoutingTable::receiveSignal(cComponent* source, omnetpp::simsignal_t signalID, cObject* msg,
        cObject* details)
{
    if(signalID == packetReceivedFromLowerSignal){
        EqDC minCostForDownwardNodes;
        auto header = getOpportunisticRoutingHeaderFromPacket(msg, minCostForDownwardNodes);
        if(header!=nullptr){
            // Check if header has options
            auto headerOptions = header->getOptions();
            auto routingSetExtId = headerOptions.findByType(RoutingSetExt::extType,0);
            if(routingSetExtId!=-1){
                // Get RoutingSetExt from TlvOptions for sharedRoutingSet
                auto routingSetExtension = headerOptions.getTlvOption(routingSetExtId);
                auto sharedRoutingSetExt = check_and_cast<const RoutingSetExt*>(routingSetExtension);
                // Pass extracted routingSet into merge routing set function
                if(minCostForDownwardNodes >= calculateUpwardsCost(rootAddress) + forwardingCostW){
                // if(minCostForDownwardNodes > calculateUpwardsCost(rootAddress)){
                    // Observed Routing Set is downwards from root
                    for(int k=0; k<sharedRoutingSetExt->getEntryArraySize(); k++ ){
                        addToDownwardsWarmupSet(sharedRoutingSetExt->getEntry(k), minCostForDownwardNodes);
                    }
                }
            }
        }
    }
}

int ORPLRoutingTable::countDownwardNodes(const EqDC ownEqDCEstimate) const
{
    int downwardsSetSize = 0;

    // Utility functions
    auto isNeighborEntryActive = [=](const NeighborEntry node)
        {return node.recentInteractionProb > 0;};
    auto isNeighborEntryDownwards = [=](const NeighborEntry node)
        {return node.lastEqDC >= ownEqDCEstimate;};

    // Loop through merged downward set from neighbour
    for (const auto& nodePair : routingSetTable) {
        const auto nodeEntry = nodePair.second;
        // Only count active downward routing set entries
        if (isNeighborEntryActive(nodeEntry) && isNeighborEntryDownwards(nodeEntry)) {
            auto encountersTblRes = encountersTable.find(nodePair.first);
            if (encountersTblRes != encountersTable.end() && isNeighborEntryActive(encountersTblRes->second)) {
            //if (encountersTblRes != encountersTable.end() && isNeighborEntryActive(encountersTable[node.first])) {
                // Node is active immediate neighbor so don't count here, count with encountersTable neighbors
            }
            else {
                // Node not encountersTable neighbour so count from routing set
                downwardsSetSize++;
            }
        }
    }
    // loop through encountersTable
    for (const auto& nodePair : encountersTable) {
        const auto node = nodePair.second;
        if (isNeighborEntryActive(node) && isNeighborEntryDownwards(node)) {
            downwardsSetSize++;
        }
    }
    return downwardsSetSize;
}

void ORPLRoutingTable::printRoutingTable()
{
    EqDC ownEqDCEstimate = calculateUpwardsCost(rootAddress);
    // Utility functions
    auto isNeighborEntryActive = [=](const NeighborEntry node)
        {return node.recentInteractionProb > 0;};
    auto isNeighborEntryDownwards = [=](const NeighborEntry node)
        {return node.lastEqDC > ownEqDCEstimate;};
    EV_INFO << "-- Opportunistic routing table --\n" << endl;
    EV_INFO << inet::utils::stringf("%-16s %-10s %-8s %s\n",
            "Name", "LatestEqDC", "Direct", "In Merged Set");
    L3AddressResolver resolver;
    // Loop through merged downward set from neighbour
    for (const auto& nodePair : encountersTable) {
        const auto nodeEntry = nodePair.second;
        // Only count active downward routing set entries
        if (isNeighborEntryActive(nodeEntry) && isNeighborEntryDownwards(nodeEntry)) {
            auto routingSetRes = routingSetTable.find(nodePair.first);
            if (routingSetRes != routingSetTable.end() && isNeighborEntryActive(routingSetRes->second)) {
                // Node is in downwards set so don't print here, but with routingSetTable neighbors
            }
            else {
                // Node not routingSetTable neighbour so print
                auto nodeModule = resolver.findHostWithAddress(nodePair.first);
                if (nodeModule!=nullptr) {
                    EV_INFO << inet::utils::stringf("%-16.15s   %7.1f      %-4s %4s\n",
                        nodeModule->getName(), nodeEntry.lastEqDC, "X", "-") << endl;
                }
            }
        }
    }
    // loop through routingSetTable
    for (const auto& nodePair : routingSetTable) {
        const auto node = nodePair.second;
        if (isNeighborEntryActive(node) && isNeighborEntryDownwards(node)) {
            const auto& directEncounterPair = encountersTable.find(nodePair.first);
            const bool isDirect = directEncounterPair != encountersTable.end() && isNeighborEntryActive(directEncounterPair->second);
            EqDC latestCost = isDirect ? directEncounterPair->second.lastEqDC : node.lastEqDC;
            auto nodeModule = resolver.findHostWithAddress(nodePair.first);
            // Node not routingSetTable neighbour so print
            if (nodeModule!=nullptr) {
                EV << inet::utils::stringf("%-16.15s   %7.1f      %-4s %4s\n",
                    nodeModule->getName(), latestCost, isDirect ? "X" : "-", "X") << endl;
            }
        }
    }
    EV << "-------------------------" << endl;
}

void ORPLRoutingTable::finish()
{
    cComponent::finish();
    if((bool)par("printRoutingTables")){
        EV_INFO << "Node " << interfaceTable->getHostModule()->getName() << endl;
        printRoutingTable();
    }
}
