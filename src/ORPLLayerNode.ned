//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package wakeupmac;
import inet.applications.generic.IpvxTrafGen;
import inet.node.base.NetworkLayerNodeBase;
import inet.queueing.source.ActivePacketSource;
import inet.queueing.sink.PassivePacketSink;

//
// TODO auto-generated type
//
module ORPLLayerNode extends NetworkLayerNodeBase
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
        energyGenerator.powerGeneration = default(20mW); // Abundant energy for now
        energyGenerator.generationInterval = 0.5s;
        energyGenerator.sleepInterval = 0s;
        wlan[*].dataRadio.antenna.mobilityModule = default("^.^.^.mobility");
        wlan[*].wakeUpRadio.antenna.mobilityModule = default("^.^.^.mobility");
		// Disable default ipv4 enable generic
        hasIpv4 = false;
        hasGn = true;
        generic.typename = "OpportunisticRoutingLayer";
        generic.interfaceTableModule = default(".interfaceTable");
        generic.hubAddress = "10.0.0.255";
    submodules:
        packetGenerator: IpvxTrafGen {
            @display("p=437.84998,114.975");
            packetLength = 8B;
            sendInterval = default(1s);
            //packetData = 0x0F0F0F0FF0F0F0F0;
            protocol = 240;//Protocol::
        }
    connections:
        packetGenerator.ipOut --> generic.transportIn;
        packetGenerator.ipIn <-- generic.transportOut;
}
