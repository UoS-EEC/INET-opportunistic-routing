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

#ifndef OPPORTUNISTICRPL_H_
#define OPPORTUNISTICRPL_H_

#include "inet/common/ProtocolTag_m.h"
#include "inet/networklayer/base/NetworkProtocolBase.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/networklayer/contract/IArp.h"
#include <set>
#include <map>

using namespace inet;

const Protocol OpportunisticRouting("ORPL", "ORPL", Protocol::NetworkLayer);

class OpportunisticRpl : public NetworkProtocolBase, public INetworkProtocol{
public:
    OpportunisticRpl()
        : NetworkProtocolBase(),
        nextForwardTimer(nullptr),
        routingTable(nullptr),
        arp(nullptr),
        waitingPacket(nullptr),
        initialTTL(3),
        sequenceNumber(0){}
    virtual void initialize(int stage) override;
    typedef uint16_t ExpectedCost;
    bool queryAcceptPacket(const MacAddress& destination, const ExpectedCost& currentExpectedCost) const;
protected:
    cMessage* nextForwardTimer;
    simtime_t forwardingSpacing;
    uint8_t initialTTL;

    IRoutingTable *routingTable;
    IArp *arp;

    L3Address nodeAddress;

    Packet* waitingPacket;
    uint16_t sequenceNumber;
    // Address and Sequence number record of packet received or sent
    typedef struct packetRecord{
         MacAddress source;
         unsigned int seq;
    } packetRecord;
    typedef std::set<packetRecord> packetHistory;

    typedef std::map<L3Address, ExpectedCost> recordTable;
    recordTable expectedCostTable;

    virtual void encapsulate(Packet* packet);
    virtual void decapsulate(Packet* packet);
    virtual void setDownControlInfo(Packet* packet, const MacAddress& macMulticast, const ExpectedCost& expectedCost);

    const Protocol& getProtocol() const override { return OpportunisticRouting; }

    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void handleUpperPacket(Packet* packet) override;
    virtual void queuePacket(Packet* packet);
    virtual void dropPacket(Packet* packet, PacketDropDetails& details);
    virtual void handleLowerPacket(Packet* packet) override;

    virtual void finish() override;
    virtual void handleStartOperation(LifecycleOperation* op) override;
    virtual void handleStopOperation(LifecycleOperation* op) override;
    virtual void handleCrashOperation(LifecycleOperation* op) override;
};

#endif /* OPPORTUNISTICRPL_H_ */
