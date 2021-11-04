/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_ORWMACINTERFACE_H_
#define LINKLAYER_ORWMACINTERFACE_H_

#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>

namespace oppostack{

using inet::physicallayer::IRadio;
class ORWMacInterface : public inet::NetworkInterface{
public:
    virtual const IRadio* getInitiationRadio() const{
        const cModule* mod = getModuleByPath(".dataRadio");
        return dynamic_cast<const IRadio*>(mod);
    }
};

} // namespace oppostack
#endif /* LINKLAYER_WAKEUPMACINTERFACE_H_ */
