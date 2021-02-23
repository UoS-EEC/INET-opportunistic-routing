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


//
// OpportunisiticRoutingWSN
// Selection of modules forming network layer
//
module OpportunisticRoutingLayer extends SimpleNetworkLayer
{
    parameters:
        arp.typename = "GlobalArp";
        arp.addressType = "modulepath";
        np.typename = "OpportunisticRpl";
        interfaceTableModule = "^.interfaceTable";
        *.routingTableModule = default(absPath(".routingTable"));
        *.arpModule = default(absPath(".arp"));
    submodules:
        routingTable: ORPLRoutingTable {
            parameters:
                @display("p=449.53998,299.13;is=m");
        }
        helloManager: ORPLHello {
            @display("p=150,100");
        }
    connections:
        up.in++ <-- helloManager.ipOut;
        up.out++ --> helloManager.ipIn;
}
