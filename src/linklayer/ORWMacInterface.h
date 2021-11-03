/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_ORWMACINTERFACE_H_
#define LINKLAYER_ORWMACINTERFACE_H_

#include <inet/networklayer/common/InterfaceEntry.h>
#include <inet/physicallayer/contract/packetlevel/IRadio.h>

namespace oppostack{
using inet::physicallayer::IRadio;
class ORWMacInterface : public inet::InterfaceEntry{
public:
    virtual const IRadio* getInitiationRadio() const{
        const cModule* mod = getModuleByPath(".dataRadio");
        return dynamic_cast<const IRadio*>(mod);
    }
};

} // namespace oppostack
#endif /* LINKLAYER_WAKEUPMACINTERFACE_H_ */
