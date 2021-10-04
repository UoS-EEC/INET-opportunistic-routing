/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "FixedCostRouting.h"

#include "ORWRouting.h"
#include "FixedCostTable.h"
#include <inet/common/ProtocolTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include "common/EqDCTag_m.h"
#include "common/oppDefs.h"

using namespace inet;
namespace oppostack {

Define_Module(FixedCostRouting);

void FixedCostRouting::initialize(int stage) {
    NetworkProtocolBase::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        routingTable = check_and_cast<RoutingTableBase*>(getCModuleFromPar(par("routingTableModule"), this));
    }
    else if (stage == INITSTAGE_NETWORK_CONFIGURATION){
        ProtocolGroup::ipprotocol.addProtocol(245, &OpportunisticRouting);
        registerProtocol(Protocol::nextHopForwarding, gate("transportIn"), gate("queueIn"));
    }
}
void FixedCostRouting::setDownControlInfo(Packet *packet,
        const MacAddress &macMulticast, const EqDC &costIndicator,
        const EqDC &onwardCost) const {
    if(packet->findTag<EqDCReq>()==nullptr){
        packet->addTag<EqDCReq>()->setEqDC(onwardCost); // Set expected cost of any forwarder
    }
    packet->addTagIfAbsent<EqDCInd>()->setEqDC(costIndicator); // Indicate own routingCost for updating metric
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&OpportunisticRouting);
    packet->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(&OpportunisticRouting);
    packet->addTagIfAbsent<MacAddressReq>()->setDestAddress(macMulticast);
}

void FixedCostRouting::handleUpperPacket(Packet *packet) {
    EqDC nextHopCost = EqDC(25.5);
    EqDC ownCost = routingTable->calculateUpwardsCost(MacAddress::STP_MULTICAST_ADDRESS, nextHopCost);
    setDownControlInfo(packet, MacAddress::STP_MULTICAST_ADDRESS, ownCost, nextHopCost);
    sendDown(packet);
}

void FixedCostRouting::handleLowerPacket(Packet *packet) {
    packet->addTagIfAbsent<L3AddressInd>()
            ->setSrcAddress(packet->getTag<MacAddressInd>()->getSrcAddress());
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::nextHopForwarding);
    sendUp(packet);
}

const inet::Protocol& FixedCostRouting::getProtocol() const  {
    return OpportunisticRouting;
}

} /* namespace oppostack */

