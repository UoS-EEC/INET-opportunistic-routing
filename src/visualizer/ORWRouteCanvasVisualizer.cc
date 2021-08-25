/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ORWRouteCanvasVisualizer.h"

#include "../networklayer/ORWRouting.h"
using namespace oppostack;

Define_Module(ORWRouteCanvasVisualizer);

bool ORWRouteCanvasVisualizer::isPathStart(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathStart(module) )
        return true;
    if (dynamic_cast<ORWRouting* >(module) != nullptr)
        return true;
    return false;
}

bool ORWRouteCanvasVisualizer::isPathEnd(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathEnd(module) )
        return true;
    if (dynamic_cast<ORWRouting* >(module) != nullptr)
        return true;
    return false;
}

bool ORWRouteCanvasVisualizer::isPathElement(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathElement(module) )
        return true;
    if (dynamic_cast<ORWRouting* >(module) != nullptr)
        return true;
    return false;
}
