// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.networklayer;
import inet.networklayer.base.NetworkProtocolBase;
import inet.common.LayeredProtocolBase;
import inet.networklayer.contract.INetworkProtocol;

//
// FixedCostRouting
//
simple FixedCostRouting extends ORWRouting like INetworkProtocol
{
    parameters:
        @class(FixedCostRouting);
}
