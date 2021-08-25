// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.ORPLLayerNode;
import oppostack.node.IntermittentORPLNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;

network IntermittentDownwardORPLTest extends IntermittentDenseORPLTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=78.790436,65.78085;scale=4");
    submodules:
        branched1: IntermittentORPLNode {
            @display("p=48.14748,32.471558");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = default("");
            packetGenerator.sendInterval = default(5s);
        }
        branched2: IntermittentORPLNode {
            @display("p=49.76484,36.950394");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched3: IntermittentORPLNode {
            @display("p=54.990147,33.591267");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched4: IntermittentORPLNode {
            @display("p=57.976036,39.0654");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched5: IntermittentORPLNode {
            @display("p=61.335163,35.333035");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingCrossBranch: IntermittentORPLNode {
            @display("p=63.763332,41.848225");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 200ExpectedCost;
            packetGenerator.destAddresses = default("transmittingWakeUpNode(modulepath) receivingWakeUpNode(modulepath)");
            packetGenerator.startTime = 0.5s;
            packetGenerator.sendInterval = default(40s);
            energyGenerator.powerGeneration = default(40uW);
        }
}
