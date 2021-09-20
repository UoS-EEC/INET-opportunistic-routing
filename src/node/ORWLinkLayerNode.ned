// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.node;

import inet.node.base.LinkLayerNodeBase;
import inet.queueing.source.ActivePacketSource;
import inet.queueing.sink.PassivePacketSink;

import oppostack.networklayer.FixedCostTable;
import oppostack.networklayer.FixedCostRouting;


module ORWLinkLayerNode extends LinkLayerNodeBase
{
    parameters:
        numWlanInterfaces = 1;
        wlan[0].typename = "ORWMacInterface";
        mobility.typename = "StationaryMobility";
        hasStatus = true; // Needed for node UP/DOWN
        energyStorage.typename = "SimpleEpEnergyStorage";
        energyStorage.nominalCapacity = default(0.045J);
        energyManagement.typename = "SimpleEpEnergyManagement";
        energyManagement.nodeShutdownCapacity = default(0.002J);
        energyManagement.nodeStartCapacity = default(0.006J);
        energyGenerator.typename = "AlternatingEpEnergyGenerator";
        energyGenerator.powerGeneration = default(20mW); // Abundant energy for now
        energyGenerator.generationInterval = default(0.5s);
        energyGenerator.sleepInterval = default(0s);
        wlan[*].dataRadio.antenna.mobilityModule = default("^.^.^.mobility");
        wlan[*].wakeUpRadio.antenna.mobilityModule = default("^.^.^.mobility");
    submodules:
        packetGenerator: ActivePacketSource {
            @display("p=507.15,297.675");
            packetLength = 8b;
            productionInterval = default(1s);
            packetData = 0x0F0F0F0FF0F0F0F0;
        }
        fixedRouting: FixedCostRouting {
            routingTableModule = "^.routingTable";
        }
        routingTable: FixedCostTable {
            forwardingCost = 0.2EqDC;
        }
        packetNullSink: PassivePacketSink {
            @display("p=360.675,288.22498");
        }
    connections:
        packetGenerator.out --> fixedRouting.transportIn;
        packetNullSink.in <-- fixedRouting.transportOut;
        fixedRouting.queueOut -->  wlan[0].upperLayerIn;
        fixedRouting.queueIn <-- wlan[0].upperLayerOut;
}
