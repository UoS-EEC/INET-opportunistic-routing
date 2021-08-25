/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "OpportunisticRoutingHeader_m.h"

using namespace oppostack;

inet::B OpportunisticRoutingHeader::calculateHeaderByteLength() const
{
    int length = headerByteLength + options.getLength();

    return B(length);
}



