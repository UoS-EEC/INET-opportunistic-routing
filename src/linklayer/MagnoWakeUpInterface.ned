// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

// Magno, Benini et al 10.1109/TII.2016.2524982
// Wake up radio using always on ULP comparator and PIC to 
// decode the received data addresses
module MagnoWakeUpInterfaceBase extends WakeUpMacInterface{
    parameters:
        wakeUpRadio.receiver.bitrate = 10kbps;
        wakeUpRadio.transmitter.bitrate = 10kbps;
}

module MagnoWakeUpInterface1 extends MagnoWakeUpInterfaceBase
{
	// WUR1 (TLV3691) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 3uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.192uW;// @3V from 0.192uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 70.192uW;// @3V from 0.320uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 70.192uW;
        wakeUpRadio.receiver.sensitivity = -32dBm;
        wakeUpRadio.receiver.energyDetection = -35dBm;
}
module MagnoWakeUpInterface2 extends MagnoWakeUpInterfaceBase
{
	// WUR2 (AS1976) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 8uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 0.810uW;// @3V from 0.405uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 70.81uW;// @3V from 0.810uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 70.81uW;
        wakeUpRadio.receiver.sensitivity = -42dBm;
        wakeUpRadio.receiver.energyDetection = -45dBm;
}
module MagnoWakeUpInterface3 extends MagnoWakeUpInterfaceBase
{
	// WUR3 (LPV7215) + PIC version
    parameters:
        wakeUpRadio.energyConsumer.switchingPowerConsumption = 15uW;
        wakeUpRadio.energyConsumer.receiverIdlePowerConsumption = 1.77uW;// @3V from 1.24uW @ 1.8V 
        wakeUpRadio.energyConsumer.receiverBusyPowerConsumption = 71.78uW;// @3V from 1.77uW + 63uW @ 1.8V
        wakeUpRadio.energyConsumer.receiverReceivingPowerConsumption = 71.78uW;
        wakeUpRadio.receiver.sensitivity = -55dBm;
        wakeUpRadio.receiver.energyDetection = -58dBm;
}
