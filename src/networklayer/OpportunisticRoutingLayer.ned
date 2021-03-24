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

package oppostack.networklayer;

import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.common.SimpleNetworkLayer;
import inet.networklayer.nexthop.NextHopRoutingTable;
import inet.applications.generic.IIpvxTrafficGenerator;


//
// OpportunisticRoutingLayer
// Opportunistic Routing layer supports modules using opportunistic next-hop
// selection using an EqDC metic calculated by the routing table to estimate
// cost of transmission to the hub node
//
module OpportunisticRoutingLayer extends SimpleNetworkLayer
{
    parameters:
        arp.typename = "GlobalArp";
        arp.addressType = "modulepath";
        np.typename = "ORWRouting";
        routingTable.typename = "ORWRoutingTable";
        interfaceTableModule = "^.interfaceTable";
        *.routingTableModule = default(absPath(".routingTable"));
        *.arpModule = default(absPath(".arp"));
    submodules:
        routingTable: <default("ORWRoutingTable")> like IOpportunisticRoutingTable if typename != "" {
            parameters:
                @display("p=449.53998,299.13;is=m");
        }
        helloManager: <default("ORWHello")> like IIpvxTrafficGenerator if typename != "" {
            @display("p=150,100");
        }
    connections:
        up.in++ <-- helloManager.ipOut;
        up.out++ --> helloManager.ipIn;
}
