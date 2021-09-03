// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;


simple MacEnergyMonitor
{
    parameters:
        @class(MacEnergyMonitor);
        // WakeUpMac energy consumption signals and statistics
        @signal[receptionConsumption](type=double);
        @signal[falseReceptionConsumption](type=double);
        @signal[transmissionConsumption](type=double);
        @signal[unknownConsumption](type=double);
        @statistic[receptionConsumption](title="Energy consumption for reception"; record=histogram);
        @statistic[falseReceptionConsumption](title="Energy consumption for false wake up"; record=histogram);
        @statistic[transmissionConsumption](title="Energy consumption for transmission"; record=histogram);
        @statistic[unknownConsumption](title="Energy consumption of unknown source"; record=histogram);

        @display("i=block/control");
}
