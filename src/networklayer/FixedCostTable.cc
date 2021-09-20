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
    const inet::InterfaceEntry* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    return ExpectedCost(std::min(ownCost + forwardingCostW, EqDC(25.5)));

}

inet::INetfilter::IHook::Result FixedCostTable::datagramPreRoutingHook(
        inet::Packet *datagram) {
    auto header = datagram->peekAtFront<ORWGram>();
    const auto destAddr = header->getReceiverAddress();

    datagram->addTagIfAbsent<EqDCInd>()->setEqDC(ownCost + forwardingCostW);
    for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i){
        auto interfaceEntry = interfaceTable->getInterface(i);
        if(destAddr == interfaceEntry->getMacAddress()){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(0.0));
            return IHook::Result::ACCEPT;
        }
    }
    datagram->addTagIfAbsent<EqDCReq>()->setEqDC(ownCost);
    return IHook::Result::ACCEPT;
}

} /* namespace oppostack */
