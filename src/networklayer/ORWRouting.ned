// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;
import inet.networklayer.base.NetworkProtocolBase;
import inet.common.LayeredProtocolBase;
import inet.networklayer.contract.INetworkProtocol;

//
// ORWRouting (Opportunistic Routing Scheme for Wireless Sensor Networks) as proposed by Duquennoy et al.
//
simple ORWRouting extends NetworkProtocolBase like INetworkProtocol
{
    parameters:
        @class(ORWRouting);
        string arpModule = default("^.arp");
        string hubAddress = default("");
        string routingTableModule;
        int initialTTL = default(30);
        @statistic[packetDropNoRouteFound](title="packet drop: no route found"; source=packetDropReasonIsNoRouteFound(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverflow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropHopLimitReached](title="packet drop: hop limit reached"; source=packetDropReasonIsHopLimitReached(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropDuplicateDetected](title="packet drop: duplicate detected, duplicate not delivered"; source=packetDropReasonIsDuplicateDetected(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
     
        @signal[ForwPacketSent](type=inet::Packet);
        @statistic[Forwpacktsent](title="ForwPacketSent"; source=ForwPacketSent; record=count; interpolationmode=none);
}
