// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

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
import inet.networklayer.common.NetworkInterface;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadio;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandDimensionalRadio;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandRadio;

module ORWMacInterface extends NetworkInterface like IWirelessInterface
{
    parameters:
        @class(ORWMacInterface);
        string interfaceTableModule;
        string energySourceModule = default("");
        string address @mutable = default("auto"); // MAC address as hex string (12 hex digits), or
                                          // "auto". "auto" values will be replaced by
                                          // a generated MAC address in init stage 0.
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.energySourceModule = default(absPath(this.energySourceModule));
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
            transmitter.power = default(0.001W); // 0dBm
            // @brief not useful at the moment {
            transmitter.bitrate = 50kbps;
            transmitter.modulation = "QPSK"; //Closest Inet match to 4-GFSK
            receiver.modulation = "QPSK";
            // }(see computeIsListeningPossible and computeBitErrorRate)
            energyConsumer.typename = "SensorStateBasedEpEnergyConsumer";
            energyConsumer.switchingPowerConsumption = 10mW;
            energyConsumer.receiverIdlePowerConsumption = 6mW;
            energyConsumer.receiverReceivingPowerConsumption = 45mW;
            energyConsumer.receiverBusyPowerConsumption = 45mW;
            energyConsumer.transmitterTransmittingPowerConsumption = 75mW;

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
