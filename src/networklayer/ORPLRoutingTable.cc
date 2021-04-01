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
    int count = std::count_if(routingSetTable.begin(), routingSetTable.end(), isNeighborEntryActive );
    count +=  std::count_if(encountersTable.begin(), encountersTable.end(), isNeighborEntryActive );
    return count;
}

// TODO: replace with IRoute*
std::pair<L3Address ,int > ORPLRoutingTable::getRoute(int k)
{
    Enter_Method("ORPLRoutingTable::getRoute(k)");
    cRuntimeError("ORPLRoutingTable::getRoute() is an unimplemented stub");
}

EqDC ORPLRoutingTable::calculateDownwardsCost(L3Address destination)
{
    Enter_Method("ORPLRoutingTable::calculateUpwardsCost(address, ..)");

    const EqDC estimatedCost = ExpectedCost(calculateUpwardsCost(destination));
    // Limit resolution and add own routing cost before reporting.
    return ExpectedCost(estimatedCost + forwardingCostW);
}
