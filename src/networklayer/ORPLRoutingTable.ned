// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

//
// Extension of ORWRoutingTable to store Downward routing set
//
simple ORPLRoutingTable extends ORWRoutingTable
{
    @class(ORPLRoutingTable);
    @signal[downwardSetSize](type = long);
    @statistic[downwardSetSize](title="Downward routing set size"; record=vector, last);
    bool printRoutingTables = default(true); 
}
