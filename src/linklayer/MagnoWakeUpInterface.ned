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

// Magno, Benini et al 10.1109/TII.2016.2524982
// Wake up radio using always on ULP comparator and PIC to 
// decode the received data addresses
module MagnoWakeUpInterface1 extends WakeUpMacInterface
{
	// WUR1 (TLV3691) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 3uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.320uW;// @3V from 0.192uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 70.32uW;// @3V from 0.320uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 70.32uW;
        wakeUpRadio.receiver.bitrate = 10kbps;
        wakeUpRadio.transmitter.bitrate = 10kbps;
        wakeUpRadio.receiver.sensitivity = -32dBm;
        wakeUpRadio.receiver.energyDetection = -35dBm;
}

module MagnoWakeUpInterface2 extends WakeUpMacInterface
{
	// WUR2 (AS1976) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 8uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.810uW;// @3V from 0.405uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 70.81uW;// @3V from 0.810uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 70.81uW;
        wakeUpRadio.receiver.bitrate = 10kbps;
        wakeUpRadio.transmitter.bitrate = 10kbps;
        wakeUpRadio.receiver.sensitivity = -42dBm;
        wakeUpRadio.receiver.energyDetection = -45dBm;
}

module MagnoWakeUpInterface3 extends WakeUpMacInterface
{
	// WUR3 (LPV7215) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 15uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 1.77uW;// @3V from 1.24uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 71.78uW;// @3V from 1.77uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 71.78uW;
        wakeUpRadio.receiver.bitrate = 10kbps;
        wakeUpRadio.transmitter.bitrate = 10kbps;
        wakeUpRadio.receiver.sensitivity = -55dBm;
        wakeUpRadio.receiver.energyDetection = -58dBm;
}
