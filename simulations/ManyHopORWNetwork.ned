// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.IntermittentORWNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;
//
// Testing ORW across greater numbers of nodes where multihop is required
//
network ManyHopORWNetwork
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = false;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        radioMedium.backgroundNoise.power = default(-110dBm);
        *.mac.candidateRelayContentionProbability = 0.5;
        @display("bgb=778.1475,677.9825;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=81.656815,18.343197");
            networkRouteVisualizer.typename = "ORWRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
            networkRouteVisualizer.packetFilter = "appData*";
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
        receivingWakeUpNode: ORWLayerNode {
            @display("p=69.486916,47.401928");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 0;
        }
        node1: ORWLayerNode {
            @display("p=75.38029,57.849983");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: ORWLayerNode {
            @display("p=89.56389,64.06527");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 3;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: ORWLayerNode {
            @display("p=79.99075,66.52429");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 6;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: ORWLayerNode {
            @display("p=79.99075,79.452095");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 5;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: ORWLayerNode {
            @display("p=93.38868,76.33648");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 7;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: ORWLayerNode {
            @display("p=91.31692,89.72325");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 8;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: ORWLayerNode {
            @display("p=107.41292,77.77078");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 10;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: ORWLayerNode {
            @display("p=105.97862,92.11375");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 12;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node9: ORWLayerNode {
            @display("p=112.31025,66.25497");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 11;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node10: ORWLayerNode {
            @display("p=117.966156,51.711193");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 14;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode1: ORWLayerNode {
            @display("p=66.455765,52.590897");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode2: ORWLayerNode {
            @display("p=130.89395,62.215027");
            generic.np.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.np.hubExpectedCost = 2;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.sendInterval = 1s;
        }
}
