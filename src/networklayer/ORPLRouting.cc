/*
 * ORPLRouting.cpp
 *
 *  Created on: 24 Mar 2021
 *      Author: Edward
 */

#include "ORPLRouting.h"
#include "OpportunisticRoutingHeader_m.h"
#include "RoutingSetExt_m.h"

using namespace oppostack;
Define_Module(ORPLRouting);

void ORPLRouting::handleLowerPacket(Packet* const packet)
{
    auto routingHeader = packet->peekAtFront<OpportunisticRoutingHeader>();
    auto routingSetExtId = routingHeader->getOptions().findByType(RoutingSetExt::extType,0);
    if(routingSetExtId!=-1){
        auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
        auto mutableOptions = mutableHeader->getOptionsForUpdate();
        auto routingSetExtension = mutableOptions.dropTlvOption(routingSetExtId);
        packet->insertAtFront(mutableHeader);
    }

    ORWRouting::handleLowerPacket(packet);
}
