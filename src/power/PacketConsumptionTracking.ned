// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.power;

//
// PacketConsumptionTracking Tracks the energy consumption per hop of packets
// and the EqDC to provide a comparison. It is a cross layer module observing
// both the network layer and the link/mac layer
//
simple PacketConsumptionTracking
{
    @class(PacketConsumptionTracking);
    string routingTable = default("^.generic.routingTable");
    string wakeUpMacModule = default("^.wlan[0].mac");
    string wakeUpMacMonitorModule = default("^.wlan[0].mac.monitor");
    
    @signal[packetReceivedEqDC](type=double);
    @signal[packetReceivedEnergyConsumed](type=double);
    @statistic[packetReceivedEqDC](title="EqDC value used for packet when sent"; record=vector,histogram);
    @statistic[packetReceivedEnergyConsumed](title="Energy used by packet for transmission to destination"; record=vector,histogram);
}
