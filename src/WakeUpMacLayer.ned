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
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;



module WakeUpMacLayer extends MacProtocolBase like IMacProtocol
{
    parameters:
        @class(WakeUpMacLayer);
        // Assumes MAC layer is within an interface
        string dataRadioModule = default("^.dataRadio");   // The path to the Radio module
        string wakeUpRadioModule = default("^.wakeUpRadio");   // The path to the Wake up radio
        string routingModule = default("^.^.generic.np");
        
        double txWakeUpWaitDuration @unit(s) = default(0.0006 s); // Must be bigger than radio OFF -> idle
        double ackWaitDuration @unit(s) = default(0.004 s); // Must be bigger than radio Rx -> Tx
        double dataListeningDuration @unit(s) = default(0.008 s); // How long to listen after a wake-up
        double wuApproveResponseLimit @unit(s) = default(1000ns);
        double candiateRelayContentionProbability = default(0.7); // If another forwarder detected, how likely is this node to contend for relay rights
        
        @statistic[packetDropNoRouteFound](title="packet drop: no route found"; source=packetDropReasonIsNoRouteFound(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverflow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropDuplicateDetected](title="packet drop: duplicate detected, contention stopped"; source=packetDropReasonIsDuplicateDetected(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropIncorrectlyReceived](title="packet drop: incorrectly received"; source=packetDropReasonIsIncorrectlyReceived(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropInterfaceDown](title="packet drop: interface down"; source=packetDropReasonIsInterfaceDown(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
    gates:
        input wakeUpRadioIn;
        output wakeUpRadioOut;
}
