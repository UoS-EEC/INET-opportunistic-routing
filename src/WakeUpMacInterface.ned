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
//
// WakeUpMACInterface
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

module WakeUpMacInterface extends InterfaceEntry like IWirelessInterface
{
    parameters:
        @class(InterfaceEntry);
        string interfaceTableModule;
        string energySourceModule = default("");
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.energySourceModule = default(absPath(energySourceModule));
        @display("i=block/ifcard");
        
        //Radio settings
        *.centerFrequency = 868MHz;
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
            switchingTimes = "us 0 400 1000 1000 1000 100 0 166 166 166 100 0 0 50 50 100 0 50 0 50 100 0 50 50 0";
            receiver.energyDetection = -100dBm;
            receiver.sensitivity = -115dBm;
            transmitter.power = 0.001W; // 15dBm
            energyConsumer.typename = "SensorStateBasedEpEnergyConsumer";
            energyConsumer.switchingPowerConsumption = 10mW;
            energyConsumer.receiverIdlePowerConsumption = 6mW;
        	energyConsumer.receiverReceivingPowerConsumption = 45mW;
            energyConsumer.receiverBusyPowerConsumption = 45mW;
            energyConsumer.transmitterTransmittingPowerConsumption = 60mW; 
            
        }
        wakeUpRadio: Ieee802154NarrowbandRadio {
            @display("p=295,179");
        	// From TI CC1120 868MHz Tranceiver
            // Reduced X->Listening transition to 100us for WuRx abstraction
        	switchingTimes = "us 0 400 100 1000 1000 100 0 100 166 166 100 0 0 50 50 100 0 50 0 50 100 0 50 50 0"; 
            receiver.energyDetection = -65dBm;
            receiver.sensitivity = -50dBm;
            transmitter.power = 0.030W; // 15dBm
            energyConsumer.typename = "SensorStateBasedEpEnergyConsumer";
            energyConsumer.switchingPowerConsumption = 20uW;
            energyConsumer.receiverIdlePowerConsumption = 8uW;
        	energyConsumer.receiverBusyPowerConsumption = 8uW;
        	energyConsumer.receiverReceivingPowerConsumption = 8uW;
            energyConsumer.transmitterTransmittingPowerConsumption = 150mW;
            
        }
        mac: WakeUpMacLayer{
        	
        }
    connections:
        upperLayerIn --> mac.upperLayerIn;
        mac.lowerLayerOut --> dataRadio.upperLayerIn;
        mac.wakeUpRadioOut --> wakeUpRadio.upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        radioIn --> { @display("m=s"); } --> dataRadio.radioIn;
        radioIn --> { @display("m=s"); } --> wakeUpRadio.radioIn;
        dataRadio.upperLayerOut --> mac.lowerLayerIn;
        wakeUpRadio.upperLayerOut --> mac.wakeUpRadioIn;
        
}
