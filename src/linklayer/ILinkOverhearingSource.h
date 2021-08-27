/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include <omnetpp.h>

#ifndef ILINKOVERHEARINGSOURCE_H_
#define ILINKOVERHEARINGSOURCE_H_

namespace oppostack {

class ILinkOverhearingSource {
public:
    /**
     * Neighbor Update signals definitions
     */
    static omnetpp::simsignal_t expectedEncounterSignal;
    static omnetpp::simsignal_t coincidentalEncounterSignal;
    static omnetpp::simsignal_t listenForEncountersEndedSignal;
};

} /* namespace oppostack */

#endif /* ILINKOVERHEARINGSOURCE_H_ */
