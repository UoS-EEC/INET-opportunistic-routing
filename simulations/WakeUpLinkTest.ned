// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.simulations;
import oppostack.node.ORWLinkLayerNode;

network WakeUpLinkTest extends MacLinkTest
{
    parameters:
        radioMedium.mediumLimitCache.maxTransmissionDuration = 30ms;
        *.wlan[0].typename = default("WakeUpMacInterface");
        redundantWakeUpNode.*.hubExpectedCost = 250ExpectedCost;
    submodules:
        // Node that is out of range
        redundantWakeUpNode: ORWLinkLayerNode {
            @display("p=56.415688,43.666378");
            packetGenerator.productionInterval = 1000s;
        }
}
