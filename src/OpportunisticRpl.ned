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

package oppostack;
import inet.networklayer.base.NetworkProtocolBase;
import inet.common.LayeredProtocolBase;
import inet.networklayer.contract.INetworkProtocol;

//
// OpportunisticRpl as proposed by Duquennoy et al.
//
simple OpportunisticRpl extends NetworkProtocolBase like INetworkProtocol
{
    parameters:
        @class(OpportunisticRpl);
        string arpModule = default("^.arp");
        string hubAddress = default("");
        int hubExpectedCost @unit(ExpectedCost) = default(255ExpectedCost);
        string routingTableModule;
        int initialTTL = default(30);
        @statistic[packetDropNoRouteFound](title="packet drop: no route found"; source=packetDropReasonIsNoRouteFound(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverflow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropHopLimitReached](title="packet drop: hop limit reached"; source=packetDropReasonIsHopLimitReached(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropDuplicateDetected](title="packet drop: duplicate detected, duplicate not delivered"; source=packetDropReasonIsDuplicateDetected(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
}
