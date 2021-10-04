// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

//
// TODO auto-generated type
//
module HighSensitivityWakeUpMacInterface extends WakeUpMacInterface
{
    // Huang et al. 2010  51uW wake-up receiver with offset and noise suppression
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = default(100uW);
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = default(51uW); 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = default(51uW) ;
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = default(51uW);
        wakeUpRadio.receiver.bitrate = 100kbps;
        wakeUpRadio.transmitter.bitrate = 100kbps;
        wakeUpRadio.receiver.sensitivity = default(-75dBm);
        wakeUpRadio.receiver.energyDetection = default(-80dBm);
}
