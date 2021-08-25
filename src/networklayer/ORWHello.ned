// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;

import inet.applications.generic.IpvxTrafGen;
import inet.queueing.contract.IPacketQueue;

//
// Simple manager of hello messages broadcast to neighbor nodes
// Determines rate of sending hello messages
//
module ORWHello extends IpvxTrafGen
{
    parameters:
    	@display("i=block/broadcast");
    	@class(ORWHello);
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
