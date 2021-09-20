// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;


moduleinterface IOpportunisticRoutingTable
{
    @display("i=block/table");
}

simple RoutingTableBase like IOpportunisticRoutingTable
{
    string interfaceTableModule;   // The path to the InterfaceTable module
    string addressType @enum("mac","modulepath","moduleid") = default("modulepath");
    int hubExpectedCost @unit(ExpectedCost) = default(255ExpectedCost);
    double forwardingCost @unit(EqDC) = default(0EqDC); //TODO: Calculate based on actual effort required
}
