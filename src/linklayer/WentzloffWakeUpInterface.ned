// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

// Roberts, Wentzloff et al. 10.1109/RFIC.2012.6242302
// Custom Chip using Oscillator tank to harvest low input levels reduceing E requirement
module WentzloffWakeUpInterface1 extends WakeUpMacInterface
{
	// Somple rectifier detection
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 4uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.392uW;// @3V from 2.4uW @ 1V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 0.392uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 0.392uW;
        wakeUpRadio.receiver.bitrate = 200kbps;
        wakeUpRadio.transmitter.bitrate = 200kbps;
        mac.wakeUpMessageDuration = 10ms; //? 
        wakeUpRadio.receiver.energyDetection = -41dBm;
		mac.dynamicWakeUpChecking = false;
		mac.fixedWakeUpChecking = false;
		mac.checkDataPacketEqDC = true;
}

// 10.1109/CICC.2013.6658500
module WentzloffWakeUpInterface2 extends WakeUpMacInterface
{
	// Has Correlator for 31bits requireing 62 bit signal
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 10uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.8544uW;// @3V from 2.4uW @ 1V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 0.8544uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 0.8544uW;
        wakeUpRadio.receiver.sensitivity = -42dBm;
        mac.wakeUpMessageDuration = 4.96ms;
		mac.dynamicWakeUpChecking = false;
		mac.fixedWakeUpChecking = true;
		mac.checkDataPacketEqDC = true;
}
