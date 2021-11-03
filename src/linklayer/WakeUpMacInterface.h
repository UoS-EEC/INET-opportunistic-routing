/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef LINKLAYER_WAKEUPMACINTERFACE_H_
#define LINKLAYER_WAKEUPMACINTERFACE_H_

#include "ORWMacInterface.h"

namespace oppostack{

class WakeUpMacInterface : public ORWMacInterface{
public:
    virtual const IRadio* getInitiationRadio() const override{
        const auto mod = getModuleByPath(".wakeUpRadio");
        return dynamic_cast<const IRadio*>(mod);
    }
};

} // namespace oppostack
#endif /* LINKLAYER_WAKEUPMACINTERFACE_H_ */
