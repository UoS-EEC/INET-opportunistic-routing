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
#include <algorithm>
#include <functional>
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
