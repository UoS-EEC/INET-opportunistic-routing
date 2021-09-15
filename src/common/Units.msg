// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

cplusplus {{
#include "Units.h"
#include "inet/common/Units.h"

using namespace inet::units::values;
}}

class J    { @existingClass; @primitive; @fromString(J(string2double($)));    @toString(unit2string($)); @defaultValue(J(NaN)); }

namespace oppostack;

class ExpectedCost
{
    @existingClass;
    @opaque;
    @toString(.str());
}
class EqDC
{
    @existingClass;
    @opaque;
    @toString(.str());
}
