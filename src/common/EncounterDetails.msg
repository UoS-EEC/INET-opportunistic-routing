// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import inet.common.INETDefs;
import inet.linklayer.common.MacAddress;
import inet.networklayer.common.L3Address;
import Units;
namespace oppostack;

//
// EncounterDetails stores information about an encounter for updating a relevant routing table
//
class EncounterDetails extends cObject{
    inet::MacAddress encountered;
    EqDC currentEqDC;
}

class PacketRecord extends cObject{
	inet::L3Address source;
    unsigned int seqNo;
}

cplusplus(PacketRecord) {{
    public:
        bool operator ==(const PacketRecord& b) const
        {
        	return getSource() == b.getSource() && getSeqNo() == b.getSeqNo();
        }
}}