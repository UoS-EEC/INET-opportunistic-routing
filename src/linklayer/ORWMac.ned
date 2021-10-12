// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;
import inet.queueing.contract.IPacketQueue;

//
// Opportunistic Routing for WSN by E Ghadimi, O Landsiedel et al..,
// OMNeT implementation by Edward Longman, University of Southampton
//
// Implements negotiation of packet acceptance when multiple nodes competing to forward.
// If transmitter: after sending and data, if multiple acks received, the
// data is resent, until there is only 1 ack received.
// If receiver: after and data reception ack is sent, if no more data received
// then the data is sent up, if data re-received, then candiateRelayContentionProbability 
// decides if it will send ack to receive data again else it gives up.
//
// Also monitors the energyStorage to ensure there is enough energy to send message
// 
// Encounter signals emitted to build a model of the probability of encountering 
// neighboring nodes.
// 
// ExpectedCost is a 8bit measurement of effort required to get packet to sink
// ExpectedCost = EqDC/10. If ack received from destination (ExpectedCost=0) then 
// the data packet has it's cost field updated.
module ORWMac extends MacProtocolBase like IMacProtocol, IOpportunisticLinkLayer
{
    parameters:
        @class(ORWMac);
        // Assumes MAC layer is within an interface
        string dataRadioModule = default("^.dataRadio");   // The path to the Radio module
        string energyStorage = default("^.^.energyStorage");
        string networkNode = default("^.^");
        
        // Protocol parameters
        double transmissionStartMinEnergy @unit(J) = 0.004345J; // Don't start transmitting unless there is at least this in storage
        double radioTurnaroundTime @unit(s) = default(50us);
        double dataListeningDuration @unit(s) = default(0.0085 s); // How long to listen before data negotiation finished
        double ackWaitDuration @unit(s) = default(0.0024 s); // Must be bigger than radio Rx -> Tx
        double candiateRelayContentionProbability = default(0.7); // If another forwarder detected, how likely is this node to contend for relay rights
        int maxTxTries = default(4);
        
        // ORWMac retry signals and statistics
        @signal[linkBroken](type=inet::Packet);
        @statistic[linkBroken](title="link break"; source=linkBroken; record=count; interpolationmode=none);
        @signal[transmissionTries](type=long);
        @signal[ackContentionRounds](type=long);
        @statistic[transmissionTries](title="Number of Tries till packet discarded or received"; record=histogram,vector);
        @statistic[ackContentionRounds](title="Number of ack contention rounds for each packet"; record=histogram,vector);
        
        // Weighted Encounters signals and with attached L2Addr and EDC/EquivalentTx(EqTx) details
        @signal[coincidentalEncounter](type=double);
        @signal[expectedEncounter](type=double);
        @signal[listenForEncountersEnded](type=double);
        @statistic[coincidentalEncounter](title="Weighted coincidental neighbor encounter"; record=vector,count);
        @statistic[expectedEncounter](title="Weighted expected neighbor encounter"; record=vector,count);
        @statistic[listenForEncountersEnded](title="Ended expected encounters listening period"; record=vector,count);

		// Mac Energy Consumption Monitoring Signals
        @signal[receptionStarted](type=bool);
        @signal[receptionEnded](type=bool);
        @signal[receptionDropped](type=bool);
        @signal[transmissionStarted](type=bool);
        @signal[transmissionEnded](type=bool);

        // Generic packet loss statistics
        @statistic[packetDropNoRouteFound](title="packet drop: no route found"; source=packetDropReasonIsNoRouteFound(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverflow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropDuplicateDetected](title="packet drop: duplicate detected, contention stopped"; source=packetDropReasonIsDuplicateDetected(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropIncorrectlyReceived](title="packet drop: incorrectly received"; source=packetDropReasonIsIncorrectlyReceived(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropInterfaceDown](title="packet drop: interface down"; source=packetDropReasonIsInterfaceDown(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
	submodules:
        queue: <default("DropHeadQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(2);
                @display("p=100,100;q=l2queue");
        }
        monitor: MacEnergyMonitor {
            @display("p=100,200");
        }
}
