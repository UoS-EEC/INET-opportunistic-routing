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

network IntermittentDenseTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        radioMedium.backgroundNoise.power = default(-110dBm);
        @display("bgb=78.790436,65.78085;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        receivingWakeUpNode: ORWLayerNode {
            @display("p=44.15929,27.301802");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 0;
        }
        transmittingWakeUpNode: IntermittentORWNode {
            @display("p=28.034737,46.358093");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 200ExpectedCost;
            packetGenerator.destAddresses = "receivingWakeUpNode(modulepath)";
            packetGenerator.startTime = 0.5s;
            energyGenerator.powerGeneration = 200uW;
        }
        node1: IntermittentORWNode {
            @display("p=44.9627,33.68679");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node2: IntermittentORWNode {
            @display("p=38.62,35.378174");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node3: IntermittentORWNode {
            @display("p=31.149708,30.966473");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node4: IntermittentORWNode {
            @display("p=40.452335,39.183796");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node5: IntermittentORWNode {
            @display("p=33.404892,41.861824");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node6: IntermittentORWNode {
            @display("p=38.901897,47.358833");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node7: IntermittentORWNode {
            @display("p=25.6527,39.183796");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        node8: IntermittentORWNode {
            @display("p=31.149708,50.60066");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=18.323357,5.13054");
            networkRouteVisualizer.typename = "ORWRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
            networkRouteVisualizer.packetFilter = "appData*";
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}
