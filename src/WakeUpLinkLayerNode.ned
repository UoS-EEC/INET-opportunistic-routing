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
