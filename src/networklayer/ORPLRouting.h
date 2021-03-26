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
public:
    ORPLRouting() : ORWRouting() {};
private:
    void handleLowerPacket(Packet* const packet) override;
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ORPLROUTING_H_ */
