// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.ORWLinkLayerNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;


network MacLinkTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        radioMedium.backgroundNoise.power = default(-110dBm);
        @display("bgb=147.92384,61.53135;scale=4");
        *.wlan[0].mac.checkDataPacketEqDC = false;
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        receivingNode: ORWLinkLayerNode {
            @display("p=71.68298,45.578777");
            packetGenerator.productionInterval = 1000s;
            energyGenerator.powerGeneration = 5mW;
            *.hubExpectedCost = default(100ExpectedCost);
        }
        transmittingNode: ORWLinkLayerNode {
            @display("p=62.774406,37.4989");
            energyGenerator.powerGeneration = 6.5mW;
            *.hubExpectedCost = default(150ExpectedCost);
        }
        overhearingNode: ORWLinkLayerNode {
            @display("p=62.774406,50.758183");
            packetGenerator.productionInterval = 900s;
            energyGenerator.powerGeneration = 4.5mW;
            *.hubExpectedCost = default(100ExpectedCost);
        }
        visualizer: IntegratedCanvasVisualizer {
            dataLinkVisualizer.displayLinks = true;
            @display("p=81.656815,18.343197");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}
