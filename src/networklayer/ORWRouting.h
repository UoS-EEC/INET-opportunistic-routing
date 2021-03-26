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

#ifndef NETWORKLAYER_ORWROUTING_H_
#define NETWORKLAYER_ORWROUTING_H_

#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/base/NetworkProtocolBase.h>
#include <inet/common/LayeredProtocolBase.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/networklayer/contract/INetworkProtocol.h>
#include <inet/networklayer/contract/IRoutingTable.h>
#include <inet/networklayer/contract/IArp.h>
#include "common/EncounterDetails_m.h"
#include <set>
#include <map>
#include <algorithm> // For std::find

#include "common/Units.h"
#include "ORWRoutingTable.h"

namespace oppostack{

using namespace inet;

extern const Protocol OpportunisticRouting;

template <class T>
class OrderedDropHeadQueue{
private:
    std::deque<T> q;
    unsigned int maxSize;
public:
    OrderedDropHeadQueue(int _size = 64):
        q(_size), maxSize(_size){}

    const typename std::deque<T>::const_iterator getIndex(const T& element) const{
        return std::find(q.begin(),q.end(),element);
    }
    bool find(const T& element) const{
        return getIndex(element) != q.end();
    }

    void insert(const T& element){
        auto duplicate = getIndex(element);
        if(duplicate!=q.end()){
            q.erase(duplicate);
        }
        while(q.size()>=maxSize){
            q.pop_front();
        }
        q.push_back(element);
    }
};

class ORWRouting : public NetworkProtocolBase, public INetworkProtocol{
public:
    ORWRouting()
        : NetworkProtocolBase(),
        nextForwardTimer(nullptr),
        forwardingBackoff(2, SIMTIME_MS),
        routingTable(nullptr),
        arp(nullptr),
        waitingPacket(nullptr){}
    ~ORWRouting();
    virtual void initialize(int stage) override;
protected:
    cMessage* nextForwardTimer;
    // Crude net layer backoff to reduce contention of forwarded packets with multiple forwarders
    simtime_t forwardingBackoff;
    uint8_t initialTTL = 3; // Overwritten by NED

    ORWRoutingTable *routingTable; // TODO: Make IRoutingTable if features allow
    IArp *arp;

    L3Address nodeAddress;
    L3Address rootAddress;

    Packet* waitingPacket;
    uint16_t sequenceNumber = 0;

    // Address and Sequence number record of packet received or sent
    OrderedDropHeadQueue<oppostack::PacketRecord> packetHistory;
    bool messageKnown(const oppostack::PacketRecord record);


    virtual void encapsulate(Packet* packet);
    virtual void decapsulate(Packet* packet) const;
    virtual void setDownControlInfo(Packet* packet, const MacAddress& macMulticast, const EqDC& routingCost, const EqDC& onwardCost) const;

    const Protocol& getProtocol() const override { return OpportunisticRouting; }

    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void handleUpperPacket(Packet* packet) override;
    virtual void queueDelayed(Packet* const packet, const simtime_t delay);
    virtual void dropPacket(Packet* packet, PacketDropDetails& details);
    virtual void handleLowerPacket(Packet* packet) override;

    virtual void handleStartOperation(LifecycleOperation* op) override;
    virtual void handleStopOperation(LifecycleOperation* op) override;
    virtual void handleCrashOperation(LifecycleOperation* op) override;
};

} // namespace oppostack

#endif /* NETWORKLAYER_ORWROUTING_H_ */
