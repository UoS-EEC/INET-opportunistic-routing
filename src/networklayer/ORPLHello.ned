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

import inet.applications.generic.IpvxTrafGen;
import inet.queueing.contract.IPacketQueue;
//
// Simple manager of hello messages broadcast to neighbor nodes
// Determines rate of sending hello messages
//
module ORPLHello extends IpvxTrafGen
{
    parameters:
    	@display("i=block/broadcast");
    	@class(ORPLHello);
    	// Override sending characteristics for IpvxTrafGen
        startTime @unit(s) = default(uniform(0s,200s)); // time of sending the first hello
        sendInterval @unit(s) = default(200s); // set minimum frequency to send packets (including from other layers
        protocol = 241; // value for IPv4, IPv6 protocol field, accepts only between 143 and 254
        packetLength @unit(B) = 0; // packet length in bytes
        destAddresses = this.hubAddress; // list of destination addresses, separated by spaces
        
        string packetSignalSourceModule = default("^.np");
        double intermittentPacketRate = default(0.3);// Each cycle should transmit with this avg probability
                
        // What ORPL destinations (DAG roots/hubAddress) are relevant
    	string hubAddress = default("routingHub(modulepath)");
    submodules:
        destinationRecord: <default("DropHeadQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(10);
                @display("p=100,100;q=l2queue");
        }
}
