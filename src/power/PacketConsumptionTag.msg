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


import inet.common.INETDefs;
import inet.common.TagBase;
import inet.networklayer.common.L3Address;
import inet.common.Units;
import common.Units;
namespace oppostack;
cplusplus{{
using namespace inet::units::values;
}}
//
// Region tag for Network layer headers to record energy consumed from each hop
//
class PacketConsumptionTag extends inet::TagBase {
    omnetpp::cComponent* source[];
    oppostack::EqDC estimatedCost[];
    inet::J energyConsumed[];
}

class HopConsumptionTag extends inet::TagBase {
    omnetpp::cComponent* source;
    oppostack::EqDC estimatedCost;
    inet::J energyConsumed;
}