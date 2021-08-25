/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_IOBSERVABLEMAC_H_
#define LINKLAYER_IOBSERVABLEMAC_H_
#include <omnetpp.h>

namespace oppostack {

class IObservableMac
{
public:
    /**
     * Mac monitoring signals
     */
    static omnetpp::simsignal_t receptionStartedSignal;
    static omnetpp::simsignal_t receptionEndedSignal;
    static omnetpp::simsignal_t receptionDroppedSignal;
    static omnetpp::simsignal_t transmissionStartedSignal;
    static omnetpp::simsignal_t transmissionEndedSignal;
};

} /* namespace oppostack */

#endif /* LINKLAYER_IOBSERVABLEMAC_H_ */
