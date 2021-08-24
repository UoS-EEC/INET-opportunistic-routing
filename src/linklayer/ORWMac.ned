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

//
// Opportunistic Routing for WSN implementation of Ghadimi, Duquennoy et al. 
// by Edward Longman, University of Southampton
//
module ORWMac extends MacProtocolBase like IMacProtocol, IOpportunisticLinkLayer
{
    parameters:
        @class(ORWMac);
        // Assumes MAC layer is within an interface
        string dataRadioModule = default("^.dataRadio");   // The path to the Radio module
        string energyStorage = default("^.^.energyStorage");
        string networkNode = default("^.^");
        
        double transmissionStartMinEnergy @unit(J) = 0.004345J; // Don't start transmitting unless there is at least this in storage
        double radioTurnaroundTime @unit(s) = default(50us);
        double dataListeningDuration @unit(s) = default(0.0085 s); // How long to listen after a wake-up
        double ackWaitDuration @unit(s) = default(0.0024 s); // Must be bigger than radio Rx -> Tx
        double candiateRelayContentionProbability = default(0.7); // If another forwarder detected, how likely is this node to contend for relay rights
        int maxTxTries = default(4);
        
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
}
