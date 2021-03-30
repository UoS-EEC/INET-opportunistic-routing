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
    auto routingSetExtension = routingHeader->getOptions().findByType(RoutingSetExt::extType,0);

    ORWRouting::handleLowerPacket(packet);
}
