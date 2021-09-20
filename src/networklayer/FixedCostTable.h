/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_FIXEDCOSTTABLE_H_
#define NETWORKLAYER_FIXEDCOSTTABLE_H_

#include "RoutingTableBase.h"

namespace oppostack {

class FixedCostTable: public RoutingTableBase {
    EqDC ownCost{0.0};
public:
    virtual void initialize(int stage) override;
    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination) const override;
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_FIXEDCOSTTABLE_H_ */
