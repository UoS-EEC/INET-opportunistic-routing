//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

//
// Opportunistic routing header with some common fields to 
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