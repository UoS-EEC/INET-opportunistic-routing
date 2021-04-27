//
// This program is property of its copyright holder. All rights reserved.
//

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