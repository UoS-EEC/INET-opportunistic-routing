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
using namespace oppostack;
using namespace inet;

Define_Module(ORPLRoutingTable);

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

EqDC ORPLRoutingTable::calculateDownwardsCost(L3Address destination)
{
    Enter_Method("ORPLRoutingTable::calculateUpwardsCost(address, ..)");

    const EqDC estimatedCost = ExpectedCost(calculateUpwardsCost(destination));
    // Limit resolution and add own routing cost before reporting.
    return ExpectedCost(estimatedCost + forwardingCostW);
}

void ORPLRoutingTable::initialize(int stage)
{
    ORWRoutingTable::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        cModule* encountersModule = getCModuleFromPar(par("encountersSourceModule"), this);
        encountersModule->subscribe(packetReceivedFromLowerSignal, this);
    }
}

const Ptr<const OpportunisticRoutingHeader> ORPLRoutingTable::getOpportunisticRoutingHeaderFromPacket(const cObject* msg)
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
        // Peek at Network Header using offset of length of chunk at start.
        return packet->peekAt<OpportunisticRoutingHeader>(secondHeaderOffset);
    }
    return nullptr;
}

void ORPLRoutingTable::receiveSignal(cComponent* source, omnetpp::simsignal_t signalID, cObject* msg,
        cObject* details)
{
    if(signalID == packetReceivedFromLowerSignal){
        auto header = getOpportunisticRoutingHeaderFromPacket(msg);
        if(header!=nullptr){
            // Check if header has options
            auto headerOptions = header->getOptions();
            auto routingSetExtId = headerOptions.findByType(RoutingSetExt::extType,0);
            if(routingSetExtId!=-1){
                // Get RoutingSetExt from TlvOptions for sharedRoutingSet
                auto routingSetExtension = headerOptions.getTlvOption(routingSetExtId);
                auto sharedRoutingSetExt = check_and_cast<const RoutingSetExt*>(routingSetExtension);
                // Pass extracted routingSet into merge routing set function
            }
        }
    }
}
