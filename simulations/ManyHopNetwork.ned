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
//
// Testing ORPL across greater numbers of nodes where multihop is required
//
network ManyHopNetwork
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=182.41661,111.69078;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=81.656815,18.343197");
            networkRouteVisualizer.typename = "ORPLRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
        receivingWakeUpNode: ORPLLayerNode {
            @display("p=69.486916,47.401928");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 0;
        }
        node1: ORPLLayerNode {
            @display("p=75.38029,57.849983");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: ORPLLayerNode {
            @display("p=89.56389,64.06527");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 3;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: ORPLLayerNode {
            @display("p=79.99075,66.52429");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: ORPLLayerNode {
            @display("p=79.99075,79.452095");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 5;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: ORPLLayerNode {
            @display("p=93.38868,76.33648");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 7;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: ORPLLayerNode {
            @display("p=91.31692,89.72325");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 8;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: ORPLLayerNode {
            @display("p=107.41292,77.77078");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 10;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: ORPLLayerNode {
            @display("p=105.97862,92.11375");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 12;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node9: ORPLLayerNode {
            @display("p=112.31025,66.25497");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 11;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node10: ORPLLayerNode {
            @display("p=117.966156,51.711193");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 14;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode1: ORPLLayerNode {
            @display("p=66.455765,52.590897");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode2: ORPLLayerNode {
            @display("p=130.89395,62.215027");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 1s;
        }
}
