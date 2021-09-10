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

//
// TODO auto-generated type
//
module HighSensitivityWakeUpMacInterface extends WakeUpMacInterface
{
    // Huang et al. 2010  51uW wake-up receiver with offset and noise suppression
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = default(100uW);
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = default(51uW); 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = default(51uW) ;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = default(51uW);
        wakeUpRadio.receiver.bitrate = 100kbps;
        wakeUpRadio.transmitter.bitrate = 100kbps;
        wakeUpRadio.receiver.sensitivity = default(-75dBm);
        wakeUpRadio.receiver.sensitivity = default(-80dBm);
}
