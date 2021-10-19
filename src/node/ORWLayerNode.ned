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
// Node using cross layer ORW functionality to opportunistically route data
//
module ORWLayerNode extends NetworkLayerNodeBase
{
    parameters:
        @display("i=misc/receiving");
        numWlanInterfaces = 1;
        wlan[0].typename = default("WakeUpMacInterface");
        mobility.typename = default("StationaryMobility");
        hasStatus = true; // Needed for node UP/DOWN
        energyStorage.typename = "SimpleEpEnergyStorage";
        energyStorage.nominalCapacity = default(0.045J);
        energyManagement.typename = "SimpleEpEnergyManagement";
        energyManagement.nodeShutdownCapacity = default(0.002J);
        energyManagement.nodeStartCapacity = default(0.004J);
        energyGenerator.typename = "AlternatingEpEnergyGenerator";
        energyGenerator.powerGeneration = default(20mW); // Abundant energy for now
        energyGenerator.generationInterval = 0.5s;
        energyGenerator.sleepInterval = 0s;
        wlan[*].dataRadio.antenna.mobilityModule = default("^.^.^.mobility");
        wlan[*].wakeUpRadio.antenna.mobilityModule = default("^.^.^.mobility");
		// Disable default ipv4 enable generic
        hasIpv4 = false;
        hasGn = true;
        generic.typename = default("OpportunisticRoutingLayer");
        generic.interfaceTableModule = default(".interfaceTable");
        generic.hubAddress = "10.0.0.255";
        generic.routingTable.encountersSourceModule = default("^.^.wlan[0].mac");
    submodules:
        packetGenerator: IpvxTrafGen {
            @display("p=437.84998,114.975");
            packetLength = 8B;
            sendInterval = default(1s);
            //packetData = 0x0F0F0F0FF0F0F0F0;
            protocol = 240;//Protocol::
        }
        packetMonitor: PacketConsumptionTracking{
            
        }
    connections:
        packetGenerator.ipOut --> generic.transportIn;
        packetGenerator.ipIn <-- generic.transportOut;
}
