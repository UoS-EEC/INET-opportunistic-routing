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

package wakeupmac.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import wakeupmac.ORPLLayerNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;


network ORPLTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=147.92384,61.53135;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        receivingWakeUpNode: ORPLLayerNode {
            @display("p=71.68298,45.578777");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 0;
        }
        transmittingWakeUpNode: ORPLLayerNode {
            @display("p=62.774406,37.4989");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 4;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.startTime = 0.5s;
        }
        overhearingWakeUpNode: ORPLLayerNode {
            @display("p=62.774406,50.758183");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 5s;
        }
//        transmittingNode: AdhocHost {
//            numApps = 1;
//            app[0].typename = "UdpBasicApp";
//            app[0].destAddresses = "receivingNode";
//            app[0].destPort = 80;
//            app[0].messageLength = 10B;
//            app[0].sendInterval = 1s;
//            @display("p=62.774406,37.4989");
//            wlan[0].typename = "Ieee802154NarrowbandInterface";
//            wlan[0].radio.centerFrequency = 868MHz;
//            wlan[0].radio.transmitter.power = 0.01W; // 15dBm
//        }
//        receivingNode: AdhocHost {
//            numApps = 1;
//            app[0].typename = "UdpSink";
//            app[0].localPort = 80;
//            @display("p=62.774406,50.758183");
//            wlan[0].typename = "Ieee802154NarrowbandInterface";
//            wlan[0].radio.centerFrequency = 868MHz;
//            wlan[0].radio.transmitter.power = 0.01W; // 15dBm
//        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=81.656815,18.343197");
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}
