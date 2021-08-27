// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;


//
// ORPLLayer
// Opportunistic Routing layer supports modules using opportunistic next-hop
// selection using an EqDC metic calculated by the routing table to estimate
// cost of transmission to the hub node
//
module ORPLLayer extends OpportunisticRoutingLayer
{
    parameters:
        np.typename = default("ORPLRouting");
        routingTable.typename = default("ORPLRoutingTable");
}
