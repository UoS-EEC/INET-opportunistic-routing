// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import inet.node.inet.INetworkNode;
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
        receivingNode: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=350.86916,235.01928");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 00ExpectedCost;
            energyGenerator.powerGeneration = 6mW;
        }
        node1: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=375.38029,285.849983");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 02ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=419,285");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 02ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=395.99075,330.52429");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 02ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=350,354");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 04ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=479,309");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 07ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=395,397");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 07ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=443,354");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 07ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=455,241");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 10ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node9: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=336,414");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 10ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node10: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=509,211");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 10ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingNode1: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=291,610");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = default("receivingNode(modulepath)");
            packetGenerator.sendInterval = default(5s);
        }
        transmittingNode2: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=705,289");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = default("receivingNode(modulepath)");
            packetGenerator.sendInterval = default(1s);
        }
        node11: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=405,455");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 12ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node12: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=455,431");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 12ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node13: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=493,367");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 12ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node14: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=550,323");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node15: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=541,265");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node16: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=588,217");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node17: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=336,475");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node18: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=283,467");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node19: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=291,518");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 14ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node20: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=228,467");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 16ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node21: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=395,518");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 16ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node22: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=479,491");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 16ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node23: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=517,431");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 17ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node24: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=580,378");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 17ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node25: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=618,276");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node26: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=642,330");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node27: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=631,391");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node28: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=588,455");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node29: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=533,499");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node30: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=455,560");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node31: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=375,568");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 18ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node32: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=277,568");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node33: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=213,542");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node34: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=204,617");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node35: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=375,641");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node36: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=443,624");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 20ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node37: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=533,578");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node38: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=580,530");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node39: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=666,499");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node40: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=666,431");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node41: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=718,378");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node42: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=674,235");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node43: <default("IntermittentORWNode")> like INetworkNode {
            @display("p=742,241");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 22ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
}
