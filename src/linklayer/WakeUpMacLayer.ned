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

package oppostack.linklayer;
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;
import inet.queueing.contract.IPacketQueue;

// Communicates using two radios (IRadio), one for wake-up one for data. 
// Two radio model is an abstraction of using wake-up receiver and data radio to 
// send the wake-up. 
//
// Also monitors the energyStorage to ensure there is enough energy to send message
//
// Implements negotiation of packet acceptance when multiple nodes competing to forward.
// If transmitter: after sending wake-up and data, if multiple acks received, the
// data (and not wake-up) is resent, until there is only 1 ack received.
// If receiver: after wake-up and data reception ack is sent, if no more data received
// then the data is sent up, if data re-received, then candiateRelayContentionProbability 
// decides if it will send ack to receive data again else it gives up.
// 
// Encounter signals emitted to build a model of the probability of encountering 
// neighboring nodes.
// 
// ExpectedCost is a 8bit measurement of effort required to get packet to sink
// ExpectedCost = EqDC/10. If ack received from destination (ExpectedCost=0) then 
// the data packet has it's cost field updated.

module WakeUpMacLayer extends MacProtocolBase like IMacProtocol, IOpportunisticLinkLayer
{
    parameters:
        @class(WakeUpMacLayer);
        // Assumes MAC layer is within an interface
        string dataRadioModule = default("^.dataRadio");   // The path to the Radio module
        string wakeUpRadioModule = default("^.wakeUpRadio");   // The path to the Wake up radio
        string energyStorage = default("^.^.energyStorage");
        string networkNode = default("^.^");
        double transmissionStartMinEnergy @unit(J) = 0.004345J; // Don't start transmitting unless there is at least this in storage

        double txWakeUpWaitDuration @unit(s) = default(0.0006 s); // Must be bigger than radio OFF -> idle
        double ackWaitDuration @unit(s) = default(0.0024 s); // Must be bigger than radio Rx -> Tx
        double radioTurnaroundTime @unit(s) = default(50us);
        double dataListeningDuration @unit(s) = default(0.0085 s); // How long to listen after a wake-up
        double wuApproveResponseLimit @unit(s) = default(1000ns);
        double candiateRelayContentionProbability = default(0.7); // If another forwarder detected, how likely is this node to contend for relay rights
        int maxWakeUpTries = default(4);

        // After receiving wake-up, recheck the data before sending ack and accepting
        bool recheckDataPacketEqDC = default(true);
        // If transmitter receives ack from the final dest, and recheckDataPacketEqDC is
        // performed then don't resend data, as only the final dest will accept EqDC=0
        bool skipDirectTxFinalAck = default(false);

        // WakeUpMac retry signals and statistics
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
    gates:
        input wakeUpRadioIn;
        output wakeUpRadioOut;
    submodules:
        queue: <default("DropHeadQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(2);
                @display("p=100,100;q=l2queue");
        }
        monitor: WuMacEnergyMonitor {
            @display("p=100,200");
        }

}
