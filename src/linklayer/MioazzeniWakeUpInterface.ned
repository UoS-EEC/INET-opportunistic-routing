// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

// Mioazenni, Sawan et al. 10.1109/ISCAS.2011.5937620
// Two stage wake-up receiver using 10kbps fixed length then 200kbps data for addressing
module MioazenniWakeUpInterface extends WakeUpMacInterface
{
	// Quicker detection configuration
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 300uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 32.8uW;// @3V from 2.4uW @ 1V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 32.8uW;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 45.8uW;
        wakeUpRadio.receiver.bitrate = 200kbps;
        wakeUpRadio.transmitter.bitrate = 200kbps;
        wakeUpRadio.receiver.energyDetection = -75dBm;
        wakeUpRadio.receiver.energyDetection = -79dBm;
		mac.wakeUpMessageDuration = 0.9ms;
}
