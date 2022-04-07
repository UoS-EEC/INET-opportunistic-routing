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

module WakeUpMacInterface extends NetworkInterface like IWirelessInterface
{
    parameters:
        @class(WakeUpMacInterface);
        string interfaceTableModule;
        string energySourceModule = default("");
        string address = default("auto"); // MAC address as hex string (12 hex digits), or
                                          // "auto". "auto" values will be replaced by
                                          // a generated MAC address in init stage 0.
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.energySourceModule = default(absPath(this.energySourceModule));
        **.antenna.mobility.typename = "";
        @display("i=block/ifcard;bgb=,,white");

        //Radio settings
        *.centerFrequency = 868MHz; //Data and wake-up radio do not need to be same band
        *.energyConsumer.sleepPowerConsumption = 4.5mW;
        *.energyConsumer.transmitterIdlePowerConsumption = 1mW;

    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @labels(ISignal);
        input wakeUpRadioIn @labels(ISignal2);

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
            energyConsumer.receiverIdlePowerConsumption = 40mW;
            energyConsumer.receiverReceivingPowerConsumption = 45mW;
            energyConsumer.receiverBusyPowerConsumption = 45mW;
            energyConsumer.transmitterTransmittingPowerConsumption = 75mW;

        }
        wakeUpRadio: Ieee802154NarrowbandRadio {
            @display("p=295,179");
            // From TI CC1120 868MHz Tranceiver
            // Reduced X->Listening transition to 100us for WuRx abstraction
            switchingTimes = "ms 0 0.400 0.100 1 1 0.100 0 0.100 0.166 0.166 0.100 0.4 0 0.5 0.5 0.100 0 0.050 0 0.050 0.100 0 0.050 0.050 0";
            receiver.energyDetection = default(-65dBm);
            receiver.sensitivity = default(-55dBm); // Magno et al.
            // @brief not useful at the moment {
            receiver.bitrate = default(10kbps); // 10.1109/WiMOB.2014.6962155
            // }(see computeIsListeningPossible and computeBitErrorRate)
            transmitter.power = default(0.0315W); // 15dBm
            transmitter.bitrate = default(10kbps);
            // @brief not useful at the moment {
            transmitter.modulation = "BPSK"; //Closest Inet match to 4-GFSK
            receiver.modulation = "BPSK";
            // }(see computeIsListeningPossible and computeBitErrorRate)
            energyConsumer.typename = "SensorStateBasedEpEnergyConsumer";
            energyConsumer.switchingPowerConsumption = default(20uW);
            energyConsumer.receiverIdlePowerConsumption = default(8uW);
            energyConsumer.receiverBusyPowerConsumption = default(8uW);
            energyConsumer.receiverReceivingPowerConsumption = default(8uW);
            energyConsumer.transmitterTransmittingPowerConsumption = default(150mW);

        }
        mac: WakeUpMacLayer {
            radioTurnaroundTime = 50us; // Should match the dataRadio switching times 
            dynamicWakeUpChecking = default(true);
            @display("p=208,120");
        }
    connections:
        upperLayerIn --> mac.upperLayerIn;
        mac.lowerLayerOut --> dataRadio.upperLayerIn;
        mac.wakeUpRadioOut --> wakeUpRadio.upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        radioIn --> { @display("m=s"); } --> dataRadio.radioIn;
        wakeUpRadioIn --> { @display("m=s"); } --> wakeUpRadio.radioIn;
        dataRadio.upperLayerOut --> mac.lowerLayerIn;
        wakeUpRadio.upperLayerOut --> mac.wakeUpRadioIn;

}
