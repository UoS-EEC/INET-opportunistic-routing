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

// Hambeck, Mahlknecht et al. 10.1109/ISCAS.2011.5937620
// Dutycycled OOK receiver using very low energy oscillator to repeatedly check medium
module HambeckWakeUpInterfaceBase extends WakeUpMacInterface
{
	// Quicker detection configuration
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 30uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 3uW;// @3V from 2.4uW @ 1V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 70uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 70uW;
        wakeUpRadio.receiver.bitrate = 200kbps;
        wakeUpRadio.transmitter.bitrate = 200kbps;
        wakeUpRadio.receiver.energyDetection = -72dBm;
		mac.dynamicWakeUpChecking = false;
		mac.checkDataPacketEqDC = true;
}
module HambeckWakeUpInterface1 extends HambeckWakeUpInterfaceBase
{
	// Quicker detection configuration
    parameters:
        wakeUpRadio.receiver.sensitivity = -64dBm;
        mac.wakeUpMessageDuration = 1ms;
}

module HambeckWakeUpInterface2 extends HambeckWakeUpInterfaceBase
{
	// Slower detection configuration
    parameters:
        wakeUpRadio.receiver.sensitivity = -71dBm;
        mac.wakeUpMessageDuration = 5ms;
}
