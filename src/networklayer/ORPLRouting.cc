/*
 * ORPLRouting.cpp
 *
 *  Created on: 24 Mar 2021
 *      Author: Edward
 */

#include "ORPLRouting.h"
#include "RoutingSetFooter_m.h"

using namespace oppostack;
Define_Module(ORPLRouting);

void ORPLRouting::handleLowerPacket(Packet* const packet)
{
    //auto footer = packet->popAtBack<RoutingSetFooter>(b(0));

    ORWRouting::handleLowerPacket(packet);
}
