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

#include "ORPLRoutingTable.h"
#include "OpportunisticRoutingHeader_m.h"
#include "RoutingSetExt_m.h"
#include <algorithm>
#include <functional>
#include "common/oppDefs.h"
#include "../linklayer/WakeUpGram_m.h"
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
}


EqDC ORPLRoutingTable::calculateUpwardsCost(const inet::L3Address destination) const
{
    const InterfaceEntry* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() != rootAddress){
        return ORWRoutingTable::calculateUpwardsCost(rootAddress);
    }
    else if(destination == rootAddress){
        // Not at destination so there must be some forwarding cost
        if(forwardingCostW < EqDC(0.1)){
            return ExpectedCost(EqDC(0.1));
        }
        return ExpectedCost(forwardingCostW);
    }
    else{
        // Return something else
        return ORWRoutingTable::calculateUpwardsCost(destination);
    }
}

EqDC ORPLRoutingTable::calculateDownwardsCost(L3Address destination)
{
    Enter_Method("ORPLRoutingTable::calculateUpwardsCost(address, ..)");

    const EqDC estimatedCost = ExpectedCost(calculateUpwardsCost(destination));
    // Limit resolution and add own routing cost before reporting.
    return ExpectedCost(estimatedCost + forwardingCostW);
}

void ORPLRoutingTable::activateWarmUpRoutingData()
{
    EqDC ownEqDCEstimate = calculateUpwardsCost(rootAddress);
    for(auto& node: routingSetTable){
        if(node.second.interactionsTotal > 0 && node.second.lastEqDC >= ownEqDCEstimate){
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
        if(packet->hasAtFront<WakeUpGram>()){
            indicatedMinCostToSink = packet->peekAtFront<WakeUpGram>()->getExpectedCostInd();
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
                if(minCostForDownwardNodes >= calculateUpwardsCost(rootAddress)){
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
