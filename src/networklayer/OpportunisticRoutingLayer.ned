// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

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
        np.typename = default("ORWRouting");
        routingTable.typename = default("ORWRoutingTable");
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
