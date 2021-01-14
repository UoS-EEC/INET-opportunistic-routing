//
// This program is property of its copyright holder. All rights reserved.
//

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.networklayer.common.L3Address;
import Units;
namespace inet;

class EqDCReq extends TagBase
{
    orpl::EqDC eqDC;
}
class EqDCInd extends TagBase
{
    orpl::EqDC eqDC = orpl::EqDC(25.5);
}
class EqDCBroadcast extends TagBase
{
    bool isBroadcast = true;
}