// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import oppostack.node.ORWLayerNode;
import oppostack.physicallayer.Ieee802154Narrowband900MHzRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import oppostack.networklayer.ORWNetworkConfigurator;
//
// Testing ORPL across greater numbers of nodes where multihop is required
//
network ManyHopNetwork
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        *.wlan[0].typename = "ORWMacInterface";
        @display("bgb=1623.5829,1195.168;scale=4");
        **.hubAddress = "receivingWakeUpNode(modulepath)";
        *.wlan[0].dataRadio.displayCommunicationRange = true;
		**.routingTable.forwardingCost = 0.4EqDC;
    submodules:
        radioMedium: Ieee802154Narrowband900MHzRadioMedium {
            @display("p=18.343197,17.751482");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=81.656815,18.343197");
            networkRouteVisualizer.typename = "ORWRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
            networkRouteVisualizer.packetFilter = "totalLength({1..}*B)";
            dataLinkVisualizer.displayLinks = true;
            dataLinkVisualizer.packetFilter = "appData-*";
        }
        configurator: ORWNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
        receivingWakeUpNode: ORWLayerNode {
            @display("p=690.486916,470.401928");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 0;
        }
        node1: ORWLayerNode {
            @display("p=750.38029,570.849983");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: ORWLayerNode {
            @display("p=890.56389,640.06527");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 3;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: ORWLayerNode {
            @display("p=790.99075,660.52429");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: ORWLayerNode {
            @display("p=790.99075,790.452095");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 5;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: ORWLayerNode {
            @display("p=930.38868,760.33648");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 7;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: ORWLayerNode {
            @display("p=910.31692,890.72325");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 8;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: ORWLayerNode {
            @display("p=1070.41292,770.77078");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 10;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: ORWLayerNode {
            @display("p=1050.97862,920.11375");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 12;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node9: ORWLayerNode {
            @display("p=1120.31025,660.25497");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 11;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node10: ORWLayerNode {
            @display("p=1170.966156,510.711193");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 14;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode1: ORWLayerNode {
            @display("p=660.455765,520.590897");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode2: ORWLayerNode {
            @display("p=1300.89395,620.215027");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 1s;
        }
}
