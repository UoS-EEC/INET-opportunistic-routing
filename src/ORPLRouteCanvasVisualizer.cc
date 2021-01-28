//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "ORPLRouteCanvasVisualizer.h"
#include "OpportunisticRpl.h"
using namespace oppostack;

Define_Module(ORPLRouteCanvasVisualizer);

bool ORPLRouteCanvasVisualizer::isPathStart(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathStart(module) )
        return true;
    if (dynamic_cast<OpportunisticRpl* >(module) != nullptr)
        return true;
    return false;
}

bool ORPLRouteCanvasVisualizer::isPathEnd(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathEnd(module) )
        return true;
    if (dynamic_cast<OpportunisticRpl* >(module) != nullptr)
        return true;
    return false;
}

bool ORPLRouteCanvasVisualizer::isPathElement(cModule* const module) const {
    if (visualizer::NetworkRouteCanvasVisualizer::isPathElement(module) )
        return true;
    if (dynamic_cast<OpportunisticRpl* >(module) != nullptr)
        return true;
    return false;
}
