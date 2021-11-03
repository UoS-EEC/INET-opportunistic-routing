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

#ifndef NETWORKLAYER_ORWNETWORKCONFIGURATOR_H_
#define NETWORKLAYER_ORWNETWORKCONFIGURATOR_H_

#include <inet/networklayer/configurator/nexthop/NextHopNetworkConfigurator.h>
#include <inet/physicallayer/contract/packetlevel/IRadioMedium.h>
#include <inet/common/Units.h>
#include "common/Units.h"
#include "linklayer/ORWMacInterface.h"

namespace oppostack {

class ORWNetworkConfigurator: public inet::NextHopNetworkConfigurator {
protected:
    const inet::physicallayer::IRadioMedium *radioMedium{nullptr};
    cModule* routingHub{nullptr};

    virtual void initialize(int stage) override;
    virtual inet::m computeMaxRange(const inet::Hz frequency, const double antennaGain, const inet::W maxTransmissionPower, const inet::W minReceptionPower) const;

    const ORWMacInterface* getFirstORWInterface(
            inet::IInterfaceTable *nodeInterfaces) const;
    inet::Hz computeAppTotalLoad(const cModule *nodeModule) const;
    inet::unit computeHopsEstimate(const inet::Coord rootPosition, const inet::m maxRange, const Node *sourceNode) const;
    inet::Hz computeNodeLoadContribution(const inet::Coord rootPosition, const Node* sourceNode, const inet::m maxRange) const;
    EqDC estimatePerNodeEqDC(const inet::Coord rootPosition, const Node* node, const inet::Hz loadEstimate, const inet::m maxRange) const;
    double computeNodeLoadContribution(Node* sourceNode, inet::Coord rootPosition, double maxRange) const;
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ORWNETWORKCONFIGURATOR_H_ */
