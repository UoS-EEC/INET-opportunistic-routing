// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

//
// Table of neighbor nodes and associated EqDC value for ORWRouting
//
simple ORWRoutingTable extends RoutingTableBase 
{
parameters:
    @class(ORWRoutingTable);
    string hubAddress = default("routingHub");
    string arpModule = default("^.arp");
    int probCalcEncountersThresholdMax = 100;
    string encountersSourceModule = default("^.^");
    @signal[updatedEqDCValue](type = double);
    @signal[vagueNeighbors](type = long);
    @signal[sureNeighbors](type = long);
    @statistic[updatedEqDCValue](title="Calculated value of node EqDC"; record=vector,histogram);
    @statistic[vagueNeighbors](title="Downward neighbors only encountered once in the last sampling period"; record=vector,histogram);
    @statistic[sureNeighbors](title="Downward neighbors encountered more than once in the last neighbor sampling period"; record=vector,histogram);
}
