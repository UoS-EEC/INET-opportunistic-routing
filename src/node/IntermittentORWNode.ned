// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.node;

//
// Node with insufficient power for constant operation requires intermittency
//
module IntermittentORWNode extends ORWLayerNode
{
    parameters:
        @display("i=old/x_green");
        // Drastically reduced energy
        energyGenerator.powerGeneration = default(3mW); 
}
