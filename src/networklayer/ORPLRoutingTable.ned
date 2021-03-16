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

package oppostack.networklayer;

//
// TODO auto-generated type
//
simple ORPLRoutingTable
{
parameters:
    @class(ORPLRoutingTable);
    string interfaceTableModule;   // The path to the InterfaceTable module
    @display("i=block/table");
    string addressType @enum("mac","modulepath","moduleid") = default("modulepath");
    string hubAddress = default("routingHub");
    int hubExpectedCost @unit(ExpectedCost) = default(255ExpectedCost);
    double forwardingCost @unit(EqDC) = default(0EqDC); //TODO: Calculate based on actual effort required
    string encountersSourceModule = default("^.^");
    string arpModule = default("^.arp");
    int probCalcEncountersThresholdMax = 100;
    @signal[updatedEqDCValue](type = double);
    @signal[vagueNeighbors](type = long);
    @signal[sureNeighbors](type = long);
    @statistic[updatedEqDCValue](title="Calculated value of node EqDC"; record=vector,histogram);
    @statistic[vagueNeighbors](title="Downward neighbors only encountered once in the last sampling period"; record=vector,histogram);
    @statistic[sureNeighbors](title="Downward neighbors encountered more than once in the last neighbor sampling period"; record=vector,histogram);
}
