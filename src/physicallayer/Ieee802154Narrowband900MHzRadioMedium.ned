// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.physicallayer;

import inet.physicallayer.common.packetlevel.RadioMedium;

module Ieee802154Narrowband900MHzRadioMedium extends RadioMedium
{
    parameters:
        backgroundNoise.typename = default("IsotropicScalarBackgroundNoise");

        mediumLimitCache.maxTransmissionDuration = default(20ms);
        mediumLimitCache.centerFrequency = 900 MHz;

        // 802.15.4-2006, page 266
        pathLoss.typename = default("BreakpointPathLoss");
        pathLoss.breakpointDistance = 8 m;
        pathLoss.l01 = 31.53;
        pathLoss.alpha1 = 2;
        pathLoss.l02 = 49.59;
        pathLoss.alpha2 = 3.3;

        // Reflects the thermal noise for the receiver sensitivity
        // TODO couple with sensitivity
        backgroundNoise.power = default(-110dBm);
        backgroundNoise.dimensions = default("time");
}