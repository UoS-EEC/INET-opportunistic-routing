/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef ORPLROUTECANVASVISUALIZER_H_
#define ORPLROUTECANVASVISUALIZER_H_

#include <inet/visualizer/networklayer/NetworkRouteCanvasVisualizer.h>

namespace oppostack{

class ORWRouteCanvasVisualizer : public inet::visualizer::NetworkRouteCanvasVisualizer{
public:
    ORWRouteCanvasVisualizer() : inet::visualizer::NetworkRouteCanvasVisualizer() {};
protected:
  virtual bool isPathStart(cModule *module) const override;
  virtual bool isPathEnd(cModule *module) const override;
  virtual bool isPathElement(cModule *module) const override;

};

} //namespace oppostack
#endif /* ORPLROUTECANVASVISUALIZER_H_ */
