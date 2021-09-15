// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later


import inet.common.INETDefs;
import inet.common.TagBase;
import inet.networklayer.common.L3Address;
import inet.common.Units;
import common.Units;
namespace oppostack;
cplusplus{{
using namespace inet::units::values;
}}
//
// Region tag for Network layer headers to record energy consumed from each hop
//
class PacketConsumptionTag extends inet::TagBase {
    omnetpp::cComponent* source[];
    oppostack::EqDC estimatedCost[];
    J energyConsumed[];
}

class HopConsumptionTag extends inet::TagBase {
    omnetpp::cComponent* source;
    oppostack::EqDC estimatedCost;
    J energyConsumed;
}