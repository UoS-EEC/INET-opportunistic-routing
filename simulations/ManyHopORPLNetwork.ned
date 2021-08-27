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

network ManyHopORPLNetwork extends ManyHopORWNetwork
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        transmittingNode1.typename = "IntermittentORPLNode";
        transmittingNode2.typename = "IntermittentORPLNode";
        receivingNode.typename = "IntermittentORPLNode";
        node*.typename =  "IntermittentORPLNode";
        transmittingNode1.packetGenerator.destAddresses = 
        	"receivingNode(modulepath) transmittingNode2(modulepath) receivingNode(modulepath)";
        transmittingNode2.packetGenerator.destAddresses = 
        	"receivingNode(modulepath) receivingNode(modulepath) transmittingNode1(modulepath) receivingNode(modulepath)";
        transmittingNode2.packetGenerator.sendInterval = 15s;
        node*.energyGenerator.powerGeneration = default(1.5mW);
    submodules:
}
