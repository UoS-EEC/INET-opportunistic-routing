// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

//
// Opportunistic Compatible table that always returns fixed cost for 
// routing to sink. Used for testing MAC layer as mock higher layer.
//
simple FixedCostTable extends RoutingTableBase
{
parameters:
    @class(FixedCostTable);
}
