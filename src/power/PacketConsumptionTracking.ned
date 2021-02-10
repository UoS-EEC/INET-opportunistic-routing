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
    @statistic[packetReceivedEqDC](title="EqDC value used for packet when sent"; record=vector);
    @statistic[packetReceivedEnergyConsumed](title="Energy used by packet for transmission to destination"; record=vector);
}
