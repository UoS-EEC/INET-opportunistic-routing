// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.networklayer.common.L3Address;
import Units;
namespace oppostack;

class EqDCReq extends inet::TagBase
{
    oppostack::EqDC eqDC;
}
class EqDCInd extends inet::TagBase
{
    oppostack::EqDC eqDC = oppostack::EqDC(25.5);
}
class EqDCBroadcast extends inet::TagBase
{
    bool isBroadcast = true;
}
class EqDCUpwards extends inet::TagBase
{
    bool isUpwards = true;
}