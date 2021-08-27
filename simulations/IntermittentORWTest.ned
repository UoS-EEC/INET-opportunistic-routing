// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.ORWLayerNode;
import oppostack.node.IntermittentORWNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;

network IntermittentORWTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = false;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        radioMedium.backgroundNoise.power = default(-110dBm);
        @display("bgb=573,583;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=83.43197,77.51482");
        }
        receivingNode: ORWLayerNode {
            @display("p=441.5929,273.01802");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 0;
        }
        transmittingWakeUpNode: IntermittentORWNode {
            @display("p=280.34737,463.58093");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 200ExpectedCost;
            packetGenerator.destAddresses = "receivingNode(modulepath)";
            packetGenerator.startTime = 0.5s;
            energyGenerator.powerGeneration = 6mW;
            packetGenerator.sendInterval = 10s;
        }
        node1: IntermittentORWNode {
            @display("p=449.627,336.8679");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: IntermittentORWNode {
            @display("p=386.2,353.78174");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: IntermittentORWNode {
            @display("p=311.49708,309.66473");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: IntermittentORWNode {
            @display("p=404.52335,391.83796");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: IntermittentORWNode {
            @display("p=334.04892,418.61824");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: IntermittentORWNode {
            @display("p=389.01897,473.58833");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: IntermittentORWNode {
            @display("p=256.527,391.83796");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: IntermittentORWNode {
            @display("p=311.49708,506.0066");
            generic.*.hubAddress = "receivingNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=21,78");
            networkRouteVisualizer.typename = "ORWRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
            networkRouteVisualizer.packetFilter = "appData*";
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=150,78");
        }
}
