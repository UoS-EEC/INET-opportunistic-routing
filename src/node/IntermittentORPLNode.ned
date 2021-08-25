// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.node;

//
// Node with insufficient power for constant operation requires intermittency
//
module IntermittentORPLNode extends ORPLLayerNode
{
    parameters:
        @display("i=old/x_green");
        // Drastically reduced energy, tx rate over 0.01Hz will cause intermittency
        energyGenerator.powerGeneration = default(14uW); 
}
