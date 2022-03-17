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

package oppostack.physicallayer;


import inet.physicallayer.wireless.common.medium.RadioMedium;
//
// Radio medium model derived from Martinez, Sala 2005 paper "An accurate radio 
// channel model for  wireless sensor networks simulation
// Does not implement the normally distribted path loss, only the two slope
// model, verified by their range and connectivity experiments
//
module MartinezOpen900MHzRadioMedium extends RadioMedium
{
    parameters:
        backgroundNoise.typename = default("IsotropicScalarBackgroundNoise");

        mediumLimitCache.maxTransmissionDuration = default(20ms);
        mediumLimitCache.centerFrequency = 900 MHz;

        pathLoss.typename = default("BreakpointPathLoss");
        pathLoss.breakpointDistance = 6.3m;
        pathLoss.l01 = 31.53;
        pathLoss.alpha1 = 2.34;
        pathLoss.l02 = 50.1;
        pathLoss.alpha2 = 2.73;

        // Reflects the thermal noise for the receiver sensitivity
        // TODO couple with sensitivity
        backgroundNoise.power = default(-110dBm);
        backgroundNoise.dimensions = default("time");
}
module MartinezYard900MHzRadioMedium extends RadioMedium
{
    parameters:
        backgroundNoise.typename = default("IsotropicScalarBackgroundNoise");

        mediumLimitCache.maxTransmissionDuration = default(20ms);
        mediumLimitCache.centerFrequency = 900 MHz;

        pathLoss.typename = default("BreakpointPathLoss");
        pathLoss.breakpointDistance = 3.2m;
        pathLoss.l01 = 31.53;
        pathLoss.alpha1 = 2.76;
        pathLoss.l02 = 45.5;
        pathLoss.alpha2 = 4;

        // Reflects the thermal noise for the receiver sensitivity
        // TODO couple with sensitivity
        backgroundNoise.power = default(-110dBm);
        backgroundNoise.dimensions = default("time");
}
