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

package wakeupmac;
import inet.networklayer.contract.INetworkProtocol;

//
// TODO auto-generated type
//
module OpportunisticRpl like INetworkProtocol
{
    parameters:
        arpModule = default("^.arp");
        interfaceTableModule = default("^.^.interfaceTable");
        hubAddress = default("");
        hubExpectedCost = default(65535);
    gates:
        input transportIn @labels(INetworkProtocolControlInfo/down);
        output transportOut @labels(INetworkProtocolControlInfo/up);
        input queueIn @labels(INetworkHeader);
        output queueOut @labels(INetworkHeader);
}
