// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.visualizer;

import inet.visualizer.canvas.networklayer.NetworkRouteCanvasVisualizer;

//
// Routing visualizer that is compatible with ORWRouting modules
//
module ORWRouteCanvasVisualizer extends NetworkRouteCanvasVisualizer
{
    parameters:
        @class(ORWRouteCanvasVisualizer);
        fadeOutMode = "simulationTime";
        fadeOutTime = 100s;
}
