// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

// Abe, Morie et al. 10.1109/VLSIC.2014.6858382
// 2 step wake up receiver, energy then data
module AbeWakeUpInterface extends WakeUpMacInterface
{
	// Very short wake-ups FPGA does decoding that could be configured for dynamic wake-ups
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 4uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 88uW; // @3V from 44.2uW + 2uW (clocks) at 0.7V
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 1576uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 1576uW;
        wakeUpRadio.receiver.bitrate = 50kbps;
        wakeUpRadio.transmitter.bitrate = 50kbps;
        mac.wakeUpMessageDuration = 1ms; // Energy detection before wake up message
        wakeUpRadio.receiver.energyDetection = -87dBm;
        wakeUpRadio.receiver.sensitivity = -87dBm;
		mac.dynamicWakeUpChecking = true;
		mac.fixedWakeUpChecking = true;
}
