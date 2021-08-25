// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.node;

import inet.node.base.LinkLayerNodeBase;
import inet.queueing.source.ActivePacketSource;
import inet.queueing.sink.PassivePacketSink;


module WakeUpLinkLayerNode extends LinkLayerNodeBase
{
    parameters:
        numWlanInterfaces = 1;
        wlan[0].typename = "WakeUpMacInterface";
        mobility.typename = "StationaryMobility";
        hasStatus = true; // Needed for node UP/DOWN
        energyStorage.typename = "SimpleEpEnergyStorage";
        energyStorage.nominalCapacity = 0.045J;
        energyManagement.typename = "SimpleEpEnergyManagement";
        energyManagement.nodeShutdownCapacity = 0.002J;
        energyManagement.nodeStartCapacity = 0.006J;
        energyGenerator.typename = "AlternatingEpEnergyGenerator";
        energyGenerator.powerGeneration = 20mW; // Abundant energy for now
        energyGenerator.generationInterval = 0.5s;
        energyGenerator.sleepInterval = 0s;
        wlan[*].dataRadio.antenna.mobilityModule = default("^.^.^.mobility");
        wlan[*].wakeUpRadio.antenna.mobilityModule = default("^.^.^.mobility");
    submodules:
        packetGenerator: ActivePacketSource {
            @display("p=507.15,297.675");
            packetLength = 8b;
            productionInterval = default(1s);
            packetData = 0x0F0F0F0FF0F0F0F0;
        }
        packetNullSink: PassivePacketSink {
            @display("p=360.675,288.22498");
        }
    connections:
        packetGenerator.out --> wlan[0].upperLayerIn;
        packetNullSink.in <-- wlan[0].upperLayerOut;
}
