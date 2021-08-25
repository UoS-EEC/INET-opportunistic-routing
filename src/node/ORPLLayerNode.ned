// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.node;
import inet.applications.generic.IpvxTrafGen;
import inet.node.base.NetworkLayerNodeBase;
import inet.queueing.source.ActivePacketSource;
import inet.queueing.sink.PassivePacketSink;
import oppostack.power.PacketConsumptionTracking;
//
// TODO auto-generated type
//
module ORPLLayerNode extends ORWLayerNode
{
    parameters:
        @display("i=misc/receiving");
        generic.typename = default("ORPLLayer");
        
}
