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

import inet.queueing.contract.IPacketQueue;
//
// Simple manager of hello messages broadcast to neighbor nodes
// Determines rate of sending hello messages
// TODO: Inherit from IpvxTrafGen or include that as submodule 
//
module ORPLHello 
{
    parameters:
        @class(ORPLHello);
    	@display("i=block/broadcast");
        string packetSignalSourceModule = default("^.np");
        double poweredPacketInterval @unit(s) = default(200s); // Depends on power generation and consumption
        double intermittentPacketRate = default(0.3);// Each cycle should transmit with this avg probability
        
        // What ORPL destinations (DAG roots/hubAddress) are relevant
        int numDestinations = 1;
    	string hubAddress = default("routingHub");
        string helloDestination = this.hubAddress;// TODO: Make plural when relevant
    gates:
        output lowerLayerOut;
    submodules:
        destinationRecord: <default("DropHeadQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(numDestinations*10);
                @display("p=100,100;q=l2queue");
        }
}
