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

package oppostack.linklayer;


simple WuMacEnergyMonitor
{
    parameters:
        @class(WuMacEnergyMonitor);
        string energyStorage = default("^.^.^.energyStorage");
        
        // WakeUpMac energy consumption signals and statistics
        @signal[receptionConsumption](type=double);
        @signal[falseWakeUpConsumption](type=double);
        @signal[transmissionConsumption](type=double);
        @signal[unknownConsumption](type=double);
        @statistic[receptionConsumption](title="Energy consumption for reception"; record=histogram);
        @statistic[falseWakeUpConsumption](title="Energy consumption for false wake up"; record=histogram);
        @statistic[transmissionConsumption](title="Energy consumption for transmission"; record=histogram);
        @statistic[unknownConsumption](title="Energy consumption of unknown source"; record=histogram);

        @display("i=block/control");
}
