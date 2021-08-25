// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import inet.common.packet.chunk.Chunk;
import inet.common.Units;
import inet.common.TlvOptions;
import inet.networklayer.common.L3Address;

namespace oppostack;

cplusplus{{
}}
class RoutingSetExt extends inet::TlvOptionBase{
	inet::L3Address entry[];
	length = 512;
	type = 241;
};

cplusplus(RoutingSetExt) {{
    static const short extType = 241;
	virtual short getLength() const override; // Get the compressed length of the Extension
}}