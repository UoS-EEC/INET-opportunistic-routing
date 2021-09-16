// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.ORWLayerNode;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;


network ORWTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=147.92384,61.53135;scale=4");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=18.343197,17.751482");
        }
        routingHub: ORWLayerNode {
            @display("p=71.68298,45.578777");
            wlan[0].mac.address = "0000DA7ABA5E";
            generic.np.hubAddress = "routingHub(modulepath)";
            generic.np.hubExpectedCost = 0;
        }
        transmittingWakeUpNode: ORWLayerNode {
            @display("p=62.774406,37.4989");
            generic.np.hubAddress = "routingHub(modulepath)";
            generic.np.hubExpectedCost = 4ExpectedCost;
            packetGenerator.destAddresses = "routingHub(modulepath)";
            packetGenerator.startTime = 0.5s;
        }
        overhearingWakeUpNode: ORWLayerNode {
            @display("p=62.774406,50.758183");
            generic.np.hubAddress = "routingHub(modulepath)";
            generic.np.hubExpectedCost = 2ExpectedCost;
            packetGenerator.destAddresses = "routingHub(modulepath)";
            packetGenerator.sendInterval = 5s;
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
            networkRouteVisualizer.typename = "ORWRouteCanvasVisualizer";
            networkRouteVisualizer.displayRoutes = true;
        }
        configurator: NextHopNetworkConfigurator {
            @display("p=141.42014,15.976333");
        }
}
