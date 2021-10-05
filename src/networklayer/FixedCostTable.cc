/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "FixedCostTable.h"
#include "linklayer/ORWGram_m.h"
#include "common/EqDCTag_m.h"

namespace oppostack {

Define_Module(FixedCostTable);

void FixedCostTable::initialize(int stage) {
    RoutingTableBase::initialize(stage);
    if(stage == inet::INITSTAGE_LOCAL){
        ownCost = ExpectedCost(par("hubExpectedCost"));
    }
}

EqDC FixedCostTable::calculateUpwardsCost(
        const inet::L3Address destination) const {
    Enter_Method("FixedCostTable::calculateUpwardsCost(address)");
    const inet::NetworkInterface* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    return ExpectedCost(std::min(ownCost + forwardingCostW, EqDC(25.5)));

}

inet::INetfilter::IHook::Result FixedCostTable::datagramPreRoutingHook(
        inet::Packet *datagram) {
    // Close to ORWRoutingTable::datagramPreRoutingHook
    auto header = datagram->peekAtFront<ORWGram>();
    const auto destAddr = header->getReceiverAddress();

    EqDC upwardsCostToRoot = calculateUpwardsCost(destAddr);
    datagram->addTagIfAbsent<EqDCInd>()->setEqDC(ownCost + forwardingCostW);

    for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i){
        auto interfaceEntry = interfaceTable->getInterface(i);
        if(destAddr == interfaceEntry->getMacAddress()){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(0.0));
            return IHook::Result::ACCEPT;
        }
    }
    const auto headerType = header->getType();
    if(headerType==ORWGramType::ORW_BEACON ||
            headerType==ORWGramType::ORW_DATA){
        auto costHeader = datagram->peekAtFront<ORWBeacon>();
        if(destAddr == inet::MacAddress::BROADCAST_ADDRESS){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(25.5));
            //TODO: Check OpportunisticRoutingHeader for further forwarding confirmation
            return IHook::Result::ACCEPT;
        }
        else if(upwardsCostToRoot<=costHeader->getMinExpectedCost()){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(upwardsCostToRoot);
            //TODO: Check OpportunisticRoutingHeader for further forwarding confirmation
            return IHook::Result::ACCEPT;
        }
    }
    return IHook::Result::DROP;
}

} /* namespace oppostack */
