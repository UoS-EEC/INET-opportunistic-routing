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

package oppostack;

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
    
    @signal[updatedEqDCValue](type = double);
    @statistic[updatedEqDCValue](title="Calculated value of node EqDC"; record=vector);
}
