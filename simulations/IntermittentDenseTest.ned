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
import wakeupmac.IntermittentORPLNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;


network IntermittentDenseTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=78.790436,65.78085;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        receivingWakeUpNode: ORPLLayerNode {
            @display("p=44.15929,27.301802");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 0;
        }
        transmittingWakeUpNode: IntermittentORPLNode {
            @display("p=28.034737,46.358093");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 9;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.startTime = 0.5s;
            energyGenerator.powerGeneration = 200uW;
        }
        node1: IntermittentORPLNode {
            @display("p=44.9627,33.68679");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 1;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: IntermittentORPLNode {
            @display("p=38.62,35.378174");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: IntermittentORPLNode {
            @display("p=31.149708,30.966473");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 4;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: IntermittentORPLNode {
            @display("p=40.452335,39.183796");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: IntermittentORPLNode {
            @display("p=33.404892,41.861824");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: IntermittentORPLNode {
            @display("p=38.901897,47.358833");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: IntermittentORPLNode {
            @display("p=25.6527,39.183796");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: IntermittentORPLNode {
            @display("p=31.149708,50.60066");
            generic.np.hubAddress = "receivingWakeUpNode";
            generic.np.hubExpectedCost = 8;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=18.323357,5.13054");
            networkRouteVisualizer.typename = "ORPLRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}