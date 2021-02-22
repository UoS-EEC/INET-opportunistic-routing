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
    	
    	// From IpvxTraffGen modified send interval
        double startTime @unit(s) = default(uniform(0s,200s)); // time of sending the first packet
        double stopTime @unit(s) = default(-1s);  // time of finishing sending, negative values mean forever
        volatile double sendInterval @unit(s) = default(200s); // may be a random value, e.g. exponential(1)
        int numPackets = default(-1); // max number of packets to generate, -1 means forever
        int protocol = 241; // value for IPv4, IPv6 protocol field, accepts only between 143 and 254
        volatile int packetLength @unit(B) = 0; // packet length in bytes
        string destAddresses = this.hubAddress; // list of destination addresses, separated by spaces
        
        @signal[packetSent](type=inet::Packet);
        @statistic[packetSent](title="packets sent"; source=packetSent; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        
        string packetSignalSourceModule = default("^.np");
        double intermittentPacketRate = default(0.3);// Each cycle should transmit with this avg probability
                
        // What ORPL destinations (DAG roots/hubAddress) are relevant
        int numDestinations = 1;
    	string hubAddress = default("routingHub(modulepath)");
    gates:
        output lowerLayerOut;
        input lowerLayerIn;
    submodules:
        destinationRecord: <default("DropHeadQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(numDestinations*10);
                @display("p=100,100;q=l2queue");
        }
}
