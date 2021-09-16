// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.WakeUpLinkLayerNode;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;


network WakeUpLinkTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=147.92384,61.53135;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        receivingWakeUpNode: WakeUpLinkLayerNode {
            @display("p=71.68298,45.578777");
            packetGenerator.productionInterval = 1000s;
        }
        transmittingWakeUpNode: WakeUpLinkLayerNode {
            @display("p=62.774406,37.4989");
        }
        overhearingWakeUpNode: WakeUpLinkLayerNode {
            @display("p=62.774406,50.758183");
            packetGenerator.productionInterval = 900s;
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
        configurator: Ipv4NetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}
