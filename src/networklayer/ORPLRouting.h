/*
 * ORPLRouting.h
 *
 *  Created on: 24 Mar 2021
 *      Author: Edward
 */

#ifndef NETWORKLAYER_ORPLROUTING_H_
#define NETWORKLAYER_ORPLROUTING_H_

#include "ORWRouting.h"

namespace oppostack {

class ORPLRouting : public ORWRouting
{
private:
    void initialize(int stage) override;
    void handleLowerPacket(Packet* const packet) override;
    void setDownControlInfo(Packet* const packet, const MacAddress& macMulticast, const EqDC& costIndicator, const EqDC& onwardCost) const override;
    std::set<L3Address> getSharingRoutingSet() const;
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ORPLROUTING_H_ */
