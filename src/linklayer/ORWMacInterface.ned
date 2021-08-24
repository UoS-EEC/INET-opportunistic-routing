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
// opportunisticInterface
// Implementation of a wake up and transmit commmunication interface 
// Each transmission consists of a beacon transmission, followed by 
// a pause, before the data transmission. 
// Optionally, it can transmit an acknowledgement after receiving the data.
// 
// The radios use the INET implementation of IEEE802.15.4 Narrowband Radios
// with timing and power consumption information evaluated from the CC1120.
// The WakeUpRadio is an abstraction of a standalone WuRx being used with 
// a high power radio that is actually part of the main radio.
//

import inet.linklayer.contract.IWirelessInterface;
import inet.networklayer.common.InterfaceEntry;
import inet.physicallayer.contract.packetlevel.IRadio;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandDimensionalRadio;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandRadio;

module ORWMacInterface extends InterfaceEntry like IWirelessInterface
{
    parameters:
        @class(ORWMacInterface);
        string interfaceTableModule;
        string energySourceModule = default("");
        string address = default("auto"); // MAC address as hex string (12 hex digits), or
                                          // "auto". "auto" values will be replaced by
                                          // a generated MAC address in init stage 0.
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.energySourceModule = default(absPath(energySourceModule));
        **.antenna.mobility.typename = "";
        @display("i=block/ifcard");

        //Radio settings
        *.centerFrequency = 868MHz; //Data and wake-up radio do not need to be same band
        *.energyConsumer.sleepPowerConsumption = 4.5mW;
        *.energyConsumer.transmitterIdlePowerConsumption = 1mW;

    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @labels(ISignal);

    submodules:
        dataRadio: Ieee802154NarrowbandRadio {
            @display("p=122,179");
        	// From TI CC1120 868MHz Tranceiver
            switchingTimes = "ms 0 0.400 1 1 1 0.100 0 0.166 0.166 0.166 0.100 0 0 0.050 0.050 0.100 0 0.050 0 0.050 0.100 0 0.050 0.050 0";
            receiver.energyDetection = -100dBm;
            receiver.sensitivity = -100dBm;
            transmitter.power = 0.001W; // 0dBm
            // @brief not useful at the moment {
            transmitter.bitrate = 200kbps;
            transmitter.modulation = "QPSK"; //Closest Inet match to 4-GFSK
            receiver.modulation = "QPSK";
            // }(see computeIsListeningPossible and computeBitErrorRate)
            energyConsumer.typename = "SensorStateBasedEpEnergyConsumer";
            energyConsumer.switchingPowerConsumption = 10mW;
            energyConsumer.receiverIdlePowerConsumption = 6mW;
            energyConsumer.receiverReceivingPowerConsumption = 45mW;
            energyConsumer.receiverBusyPowerConsumption = 45mW;
            energyConsumer.transmitterTransmittingPowerConsumption = 60mW;

        }
        mac: ORWMac {
			radioTurnaroundTime = 50us; // Should match the dataRadio switching times 
            @display("p=208,120");
        }
    connections:
        upperLayerIn --> mac.upperLayerIn;
        mac.lowerLayerOut --> dataRadio.upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        radioIn --> { @display("m=s"); } --> dataRadio.radioIn;
        dataRadio.upperLayerOut --> mac.lowerLayerIn;

}
