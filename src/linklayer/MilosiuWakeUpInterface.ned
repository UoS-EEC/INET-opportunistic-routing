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

// Milosiu, Oehler et al. 10.1109/ESSCIRC.2013.6649154
// Wake up radio using comparator duty cycling to reduce power consumption
// Minimum received signal strength increased by 1dB for SAW filter
// Energy detection threshold reduced by 5dBm for up to 10 bit errors
module MilosiuWuRx1 extends WakeUpMacInterface
{
	// 8.1kbps variation
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 900uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 91.3uW;// @3V from 86.7uW @ 2.5V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 91.3uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 91.3uW;
        wakeUpRadio.receiver.bitrate = 8.1kbps;
        wakeUpRadio.transmitter.bitrate = 8.1kbps;
        wakeUpRadio.receiver.sensitivity = -82dBm;
        wakeUpRadio.receiver.energyDetection = -87dBm;
}

module MilosiuWuRx2 extends WakeUpMacInterface
{
	// 1.024kbps variation
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 350uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 35.8uW;// @3V from 34uW @ 2.5V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 35.8uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 35.8uW;
        wakeUpRadio.receiver.bitrate = 1.024kbps;
        wakeUpRadio.transmitter.bitrate = 1.024kbps;
        wakeUpRadio.receiver.sensitivity = -82dBm;
        wakeUpRadio.receiver.energyDetection = -87dBm;
}
