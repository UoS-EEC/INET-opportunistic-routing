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
#include "inet/networklayer/common/L3Address.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/networklayer/contract/IArp.h"
#include <set>
#include <map>

using namespace inet;

const Protocol OpportunisticRouting("ORPL", "ORPL", Protocol::LinkLayer);

class OpportunisticRpl : public NetworkProtocolBase, public INetworkProtocol{
public:
    OpportunisticRpl()
        : NetworkProtocolBase(),
        nextForwardTimer(nullptr),
        interfaceTable(nullptr),
        routingTable(nullptr),
        arp(nullptr),
        waitingPacket(nullptr){}
    virtual void initialize(int stage) override;
    typedef uint16_t ExpectedCost;
    bool queryAcceptPacket(MacAddress destination, ExpectedCost currentExpectedCost);
protected:
    cMessage* nextForwardTimer;
    simtime_t forwardingSpacing;

    IInterfaceTable *interfaceTable;
    IRoutingTable *routingTable;
    IArp *arp;

    L3Address nodeAddress;

    Packet* waitingPacket;

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
    virtual void setDownControlInfo(Packet* packet, MacAddress macMulticast, ExpectedCost expectedCost);

    const Protocol& getProtocol() const override { return OpportunisticRouting; }

    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void queuePacket(Packet* packet);
    virtual void handleLowerPacket(Packet *packet) override;

    virtual void finish() override;
    virtual void handleStartOperation(LifecycleOperation* op) override;
    virtual void handleStopOperation(LifecycleOperation* op) override;
    virtual void handleCrashOperation(LifecycleOperation* op) override;
};

#endif /* OPPORTUNISTICRPL_H_ */
