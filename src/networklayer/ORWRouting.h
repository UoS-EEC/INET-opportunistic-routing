/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

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
#include "OpportunisticRoutingHeader_m.h"
#include <set>
#include <map>
#include <algorithm> // For std::find

#include "common/Units.h"
#include "ORWRoutingTable.h"

namespace oppostack{

extern const inet::Protocol OpportunisticRouting;

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

class ORWRouting : public inet::NetworkProtocolBase, public inet::INetworkProtocol{
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
    inet::IArp *arp;

    inet::L3Address nodeAddress;
    inet::L3Address rootAddress;

    inet::Packet* waitingPacket;
    uint16_t sequenceNumber = 0;

    // Address and Sequence number record of packet received or sent
    OrderedDropHeadQueue<oppostack::PacketRecord> packetHistory{512};
    bool messageKnown(const oppostack::PacketRecord record);


    virtual void encapsulate(inet::Packet* packet);
    virtual void decapsulate(inet::Packet* packet) const;
    virtual void setDownControlInfo(inet::Packet* packet, const inet::MacAddress& macMulticast, const EqDC& routingCost, const EqDC& onwardCost) const;

    const inet::Protocol& getProtocol() const override { return OpportunisticRouting; }

    virtual inet::MacAddress getOutboundMacAddress(const inet::Packet* packet) const;

    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void handleUpperPacket(inet::Packet* packet) override;
    virtual void queueDelayed(inet::Packet* const packet, const simtime_t delay);
    virtual void dropPacket(inet::Packet* packet, PacketDropDetails& details);
    virtual void handleLowerPacket(inet::Packet* packet) override;

    virtual void handleStartOperation(inet::LifecycleOperation* op) override;
    virtual void handleStopOperation(inet::LifecycleOperation* op) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* op) override;
    virtual void forwardPacket(EqDC ownCost, EqDC nextHopCost, inet::Packet* const packet);
    void deduplicateAndDeliver(const inet::Ptr<const oppostack::OpportunisticRoutingHeader>& header,
            inet::Packet* const packet);

private:
    void advanceHeaderOneHop(const inet::Ptr<OpportunisticRoutingHeader>& mutableHeader);
};

} // namespace oppostack

#endif /* NETWORKLAYER_ORWROUTING_H_ */
