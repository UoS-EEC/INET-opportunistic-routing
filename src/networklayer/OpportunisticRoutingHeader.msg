// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

//
// Opportunistic routing header with some common fields to Ipv6
//
import inet.networklayer.common.IpProtocolId;
import inet.common.Units;
import inet.common.TlvOptions;
import inet.networklayer.contract.NetworkHeaderBase;
import inet.common.packet.chunk.Chunk;
cplusplus {{
#include <inet/common/ProtocolGroup.h>

using namespace inet;
}}
namespace oppostack;

class OpportunisticRoutingHeader extends inet::NetworkHeaderBase{
    inet::IpProtocolId version;
    inet::B length;
    uint16_t id;
    inet::L3Address   srcAddr;
    inet::L3Address   destAddr;
    inet::IpProtocolId   protocolId;
    uint8_t ttl;
    bool isUpwards = true;
    // uint4_t fragCount;
    // uint4_t fragId;
    // uint8_t errorCorrection;
    chunkLength = inet::B(headerByteLength);
    
    inet::TlvOptions options; // array of option pointers, option pointers owned by datagram
    
}

cplusplus(OpportunisticRoutingHeader) {{
    static const short headerByteLength = 16;
  public:
    /**
     * Calculates the length of the OpportunisticRoutingHeader plus the options
     */
    virtual B calculateHeaderByteLength() const;
    
    virtual L3Address getSourceAddress() const override { return getSrcAddr(); }
    virtual void setSourceAddress(const L3Address& address) override { setSrcAddr(address); }
    virtual L3Address getDestinationAddress() const override { return getDestAddr(); }
    virtual void setDestinationAddress(const L3Address& address) override { setDestAddr(address); }
    virtual const Protocol *getProtocol() const override { return ProtocolGroup::ipprotocol.findProtocol(getProtocolId()); }
    virtual void setProtocol(const Protocol *protocol) override { setProtocolId(static_cast<IpProtocolId>(ProtocolGroup::ipprotocol.getProtocolNumber(protocol))); }
    virtual bool isFragment() const override { return false; }
}}