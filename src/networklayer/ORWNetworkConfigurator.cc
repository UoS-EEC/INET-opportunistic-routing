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

#include <inet/physicallayer/wireless/common/base/packetlevel/NarrowbandTransmitterBase.h>
#include "ORWNetworkConfigurator.h"
#include "../linklayer/ORWMacInterface.h"
#include "ORWRoutingTable.h"
#include <math.h>

using namespace inet;
using namespace inet::physicallayer;

namespace oppostack {

Define_Module(ORWNetworkConfigurator);

const ORWMacInterface* ORWNetworkConfigurator::getFirstORWInterface(
        inet::IInterfaceTable *nodeInterfaces) const {
    for (int j = 0; j < nodeInterfaces->getNumInterfaces(); j++) {
        const auto interfaceJ = nodeInterfaces->getInterface(j);
        if (interfaceJ->isWireless()
                && dynamic_cast<ORWMacInterface*>(interfaceJ)) {
            return dynamic_cast<ORWMacInterface*>(interfaceJ);
        }
    }
    return nullptr;
}

Hz ORWNetworkConfigurator::computeAppTotalLoad(const cModule *nodeModule) const {
    // TODO: Loop through app[...]
    auto packetSource = nodeModule->getSubmodule("packetGenerator");
    int parIdx = packetSource->findPar("destAddresses");
    if(parIdx >= 0)
        if(packetSource->par(parIdx).isEmptyString())
            return Hz(0.0);
    return 1.0 / s(packetSource->par("sendInterval"));
}

unit ORWNetworkConfigurator::computeHopsEstimate(const Coord rootPosition, const m maxRange, const Node *sourceNode) const {
    const auto nodeInterfaces = sourceNode->interfaceTable;
    const ORWMacInterface *primaryInterface = getFirstORWInterface(nodeInterfaces);
    const IRadio *initialContactRadio = primaryInterface->getInitiationRadio();
    Coord targetPosition =
            initialContactRadio->getAntenna()->getMobility()->getCurrentPosition();
    const m distance { rootPosition.distance(targetPosition) };
    const unit hops { std::max(1.0, unit(distance / maxRange).get() / sqrt(2)) };
    return hops;
}

Hz ORWNetworkConfigurator::computeNodeLoadContribution(const Coord rootPosition, const Node* sourceNode, const m maxRange) const
{
    const unit hops = computeHopsEstimate(rootPosition, maxRange, sourceNode);
    const cModule* nodeModule = sourceNode->getModule();
    Hz load{computeAppTotalLoad(nodeModule)};
    return load*hops;
}

EqDC ORWNetworkConfigurator::estimatePerNodeEqDC(const Coord rootPosition, const Node* node, const Hz loadEstimate, const m maxRange) const{
    const auto host = node->getModule();
    const unit hops = computeHopsEstimate(rootPosition, maxRange, node);
    //TODO: Replace with call to NextHopNetowrkConfigurator::findRoutingTable()
    auto routingTable = dynamic_cast<ORWRoutingTable *>(host->findModuleByPath(".generic.routingTable"));
    return routingTable->estimateEqDC(loadEstimate, hops);
}

void ORWNetworkConfigurator::initialize(int stage){
    NextHopNetworkConfigurator::initialize(stage);
    if(stage == INITSTAGE_NETWORK_CONFIGURATION && par("estimateInitialEqDC")){
        // extract topology into the Topology object, then fill in a LinkInfo[] vector
        extractTopology(topology);
        radioMedium = check_and_cast<IRadioMedium *>(getParentModule()->getSubmodule("radioMedium"));
        const char* rootParameter = par("hubAddress");
        L3Address rootAddress;
        L3AddressResolver().tryResolve(rootParameter, rootAddress, L3AddressResolver::ADDR_MODULEPATH);
        routingHub = L3AddressResolver().findHostWithAddress(rootAddress);

        IInterfaceTable* rootInterfaces = L3AddressResolver().interfaceTableOf(routingHub);
        const ORWMacInterface* rootORWInterface = getFirstORWInterface(rootInterfaces);
        const IRadio* rootInitiationRadio = rootORWInterface->getInitiationRadio();
        Coord rootPosition = rootInitiationRadio->getAntenna()->getMobility()->getCurrentPosition();
        auto* narrowbandTransmitter = check_and_cast<const NarrowbandTransmitterBase *>(
                                                rootInitiationRadio->getTransmitter());
        m maxRange = computeMaxRange( narrowbandTransmitter->getCenterFrequency(), 1.0,
                                      rootInitiationRadio->getReceiver()->getMinReceptionPower(),
                                      rootInitiationRadio->getTransmitter()->getMaxPower() );
        Hz networkLoadEstimate{0};
        for (int i = 0; i < topology.getNumNodes(); i++) {
            Node* sourceNode = (Node *)topology.getNode(i);
            networkLoadEstimate += computeNodeLoadContribution(rootPosition, sourceNode, maxRange);
        }
        const Hz perNodeLoadEstimate = networkLoadEstimate/topology.getNumNodes();
        for (int i = 0; i < topology.getNumNodes(); i++) {
            Node* node= (Node *)topology.getNode(i);
            estimatePerNodeEqDC(rootPosition, node, perNodeLoadEstimate, maxRange);
        }
    }
}

m ORWNetworkConfigurator::computeMaxRange(const Hz frequency, const double antennaGain, const W receptionPower, const W transmissionPower) const{
    const double loss = unit(receptionPower / transmissionPower).get() / antennaGain / antennaGain;
    return radioMedium->getPathLoss()->computeRange(radioMedium->getPropagation()->getPropagationSpeed(), frequency, loss);
}
} /* namespace oppostack */
