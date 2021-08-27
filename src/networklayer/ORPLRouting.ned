// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

//
// Extension of ORW Routing for downward routing
//
simple ORPLRouting extends ORWRouting
{
    parameters:
    	@class(ORPLRouting);
        double routingSetProportion = default(0.1);
        double routingSetBroadcastProportion = default(0.4);
}
