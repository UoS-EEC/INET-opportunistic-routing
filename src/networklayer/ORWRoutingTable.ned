// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

moduleinterface IOpportunisticRoutingTable
{
    @display("i=block/table");
}

//
// Table of neighbor nodes and associated EqDC value for ORWRouting
//
simple ORWRoutingTable like IOpportunisticRoutingTable
{
parameters:
    @class(ORWRoutingTable);
    string interfaceTableModule;   // The path to the InterfaceTable module
    string addressType @enum("mac","modulepath","moduleid") = default("modulepath");
    string hubAddress = default("routingHub");
    int hubExpectedCost @unit(ExpectedCost) = default(255ExpectedCost);
    double forwardingCost @unit(EqDC) = default(0EqDC); //TODO: Calculate based on actual effort required
    string encountersSourceModule = default("^.^");
    string arpModule = default("^.arp");
    int probCalcEncountersThresholdMax = 100;
    @signal[updatedEqDCValue](type = double);
    @signal[vagueNeighbors](type = long);
    @signal[sureNeighbors](type = long);
    @statistic[updatedEqDCValue](title="Calculated value of node EqDC"; record=vector,histogram);
    @statistic[vagueNeighbors](title="Downward neighbors only encountered once in the last sampling period"; record=vector,histogram);
    @statistic[sureNeighbors](title="Downward neighbors encountered more than once in the last neighbor sampling period"; record=vector,histogram);
}
