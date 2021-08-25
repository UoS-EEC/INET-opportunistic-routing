/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ORWRouting.h"
#include <inet/common/INETDefs.h>
#include <inet/common/ProtocolTag_m.h>
#include "OpportunisticRoutingHeader_m.h"
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/ProtocolGroup.h>

#include "common/oppDefs.h"
#include "common/Units.h"
#include "common/EqDCTag_m.h"
#include "ORWRoutingTable.h"

using namespace oppostack;


Define_Module(ORWRouting);

const inet::Protocol oppostack::OpportunisticRouting("Opportunistic", "Opportunistic", Protocol::NetworkLayer);

void ORWRouting::initialize(int const stage) {
    NetworkProtocolBase::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        nextForwardTimer = new cMessage("Forwarding Backoff Timer");
        auto const routingTableModule = getCModuleFromPar(par("routingTableModule"), this);
        routingTable = check_and_cast<ORWRoutingTable*>(routingTableModule);
        arp = inet::getModuleFromPar<IArp>(par("arpModule"), this);
        initialTTL = par("initialTTL");
    }
    else if (stage == INITSTAGE_NETWORK_CONFIGURATION){
        ProtocolGroup::ipprotocol.addProtocol(245, &OpportunisticRouting);
        registerService(Protocol::nextHopForwarding, gate("transportIn"), gate("queueIn"));
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        if (ie != nullptr)
            nodeAddress = ie->getNetworkAddress();
        else
            throw cRuntimeError("No non-loopback interface found!");

        const char* rootParameter = par("hubAddress");
        L3AddressResolver().tryResolve(rootParameter, rootAddress, L3AddressResolver::ADDR_MODULEPATH);
    }
}

inet::MacAddress ORWRouting::getOutboundMacAddress(const Packet* packet) const
{
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    inet::L3Address destinationAddress = header->getDestAddr();
    if( not destinationAddress.isUnspecified()){
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        return arp->resolveL3Address(destinationAddress, ie);
    }
    return MacAddress::STP_MULTICAST_ADDRESS;
}

void ORWRouting::handleUpperPacket(Packet* const packet) {
    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    //TODO: check tags assigned by higher layer
    if(addressReq->getDestAddress().getType() == L3Address::NONE){
        addressReq->setDestAddress(rootAddress);
        EV_WARN << "ORW: setting received packet address to default hub address" << endl;
    }
    encapsulate(packet);
    auto outboundMacAddress = getOutboundMacAddress(packet);
    EqDC nextHopCost = EqDC(25.5);
    EqDC ownCost = routingTable->calculateUpwardsCost(rootAddress, nextHopCost);
    setDownControlInfo(packet, outboundMacAddress, ownCost, nextHopCost);
    sendDown(packet);
}

void ORWRouting::advanceHeaderOneHop(const inet::Ptr<oppostack::OpportunisticRoutingHeader>& mutableHeader)
{
    // Decrease TTL, set routing cost threshold and Forward.
    auto newTtl = mutableHeader->getTtl() - 1;
    mutableHeader->setTtl(newTtl);
    // Remove unprocessed Tlv options as we can't process them.
    auto mutableOptions = mutableHeader->getOptionsForUpdate();
    while (size_t length = mutableOptions.getTlvOptionArraySize()) {
        mutableOptions.eraseTlvOption(length - 1);
    }
}

void ORWRouting::forwardPacket(EqDC ownCost, EqDC nextHopCost, Packet* const packet)
{
    // Update packet header before forwarding
    auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
    // Decrease TTL, set routing cost threshold and Forward.
    advanceHeaderOneHop(mutableHeader);
    packet->insertAtFront(mutableHeader);
    auto outboundMacAddress = getOutboundMacAddress(packet);
    if (outboundMacAddress == MacAddress::UNSPECIFIED_ADDRESS) {
        EV_WARN << "Forwarding message to unknown L3Address" << endl;
    }
    setDownControlInfo(packet, outboundMacAddress, ownCost, nextHopCost);
    // Delay forwarded packets to reduce physical layer contention
    queueDelayed(packet, uniform(0, forwardingBackoff));
}

void ORWRouting::deduplicateAndDeliver(const inet::Ptr<const oppostack::OpportunisticRoutingHeader>& header,
        Packet* const packet)
{
    // Check for duplicates
    oppostack::PacketRecord pktRecord;
    pktRecord.setSource(header->getSourceAddress());
    pktRecord.setSeqNo(header->getId());
    if (messageKnown(pktRecord)) {
        // Don't deliver duplicates to higher levels
        PacketDropDetails details;
        details.setReason(PacketDropReason::DUPLICATE_DETECTED);
        dropPacket(packet, details);
    }
    else {
        packetHistory.insert(pktRecord);
        decapsulate(packet);
        sendUp(packet);
    }
}

void ORWRouting::handleLowerPacket(Packet* const packet) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    auto const payloadLength = header->getLength() - header->getChunkLength();
    inet::L3Address destinationAddress = header->getDestAddr();
    EqDC nextHopCost = EqDC(25.5);
    EqDC ownCost = routingTable->calculateUpwardsCost(destinationAddress, nextHopCost);
    if(payloadLength<B(1)){
        // No data contained so silently accept packet
        // This only occurs when OpportunisticRpl sends hello messages
        delete packet; // TODO: emit removedPacket signal as well
    }
    else if (ownCost == EqDC(0.0)) {
        // Check for duplicates
        deduplicateAndDeliver(header, packet);
    }
    else if(packet->findTag<EqDCReq>()!=nullptr || nextHopCost < EqDC(25.5)){
        // "trim" required to remove the popped headers from lower layers
        packet->trim();
        // Packet not destined for this node
        // Route to a node upwards
        forwardPacket(ownCost, nextHopCost, packet);
    }
    else{
        PacketDropDetails details;
        details.setReason(PacketDropReason::NO_ROUTE_FOUND);
        dropPacket(packet, details);
    }
}

void ORWRouting::encapsulate(Packet* const packet) {
    auto header = makeShared<OpportunisticRoutingHeader>();
    if(packet->findTag<L3AddressReq>()!=nullptr){
        header->setDestAddr(packet->getTag<L3AddressReq>()->getDestAddress());
    }
    else{
        header->setDestAddr( L3Address() );
        EV_WARN << "Packet sent with no destination";
    }
    header->setId(sequenceNumber++);
    header->setLength(packet->getDataLength() + header->getChunkLength());
    auto protocolTag = packet->findTag<PacketProtocolTag>();
    if(protocolTag != nullptr){
        const Protocol* protocol = protocolTag->getProtocol();
        if(protocol != nullptr){
            header->setProtocol(protocol);
        }
        else{
            throw cRuntimeError("Invalid protocol value");
        }
    }
    else{
        header->setProtocol(&Protocol::manet);
    }
    header->setSrcAddr(nodeAddress);
    header->setTtl(initialTTL);
    header->setVersion(IpProtocolId::IP_PROT_MANET);
    packet->insertAtFront(header);
}

void ORWRouting::setDownControlInfo(Packet* const packet, const MacAddress& macMulticast, const EqDC& costIndicator, const EqDC& onwardCost) const
{
    packet->addTagIfAbsent<MacAddressReq>()->setDestAddress(macMulticast);
    if(packet->findTag<EqDCReq>()==nullptr){
        packet->addTag<EqDCReq>()->setEqDC(onwardCost); // Set expected cost of any forwarder
    }
    packet->addTagIfAbsent<EqDCInd>()->setEqDC(costIndicator); // Indicate own routingCost for updating metric
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&OpportunisticRouting);
    packet->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(&OpportunisticRouting);
}

void ORWRouting::decapsulate(Packet* const packet) const
{
    auto networkHeader = packet->popAtFront<OpportunisticRoutingHeader>();
    auto payloadLength = networkHeader->getLength() - networkHeader->getChunkLength();
    if (packet->getDataLength() < B(payloadLength) ) {
        throw cRuntimeError("Data error: illegal payload length");     //FIXME packet drop
    }
    if (packet->getDataLength() > B(payloadLength) )
        packet->setBackOffset(packet->getFrontOffset() + B(payloadLength));
    auto protocolInd = packet->addTagIfAbsent<NetworkProtocolInd>();
    protocolInd->setProtocol(&getProtocol());
    protocolInd->setNetworkProtocolHeader(networkHeader);
    auto payloadProtocol = networkHeader->getProtocol();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<L3AddressInd>()->setSrcAddress(networkHeader->getSrcAddr());
}

void ORWRouting::queueDelayed(Packet* const packet, const simtime_t delay) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    // If packet lifetime not expired and there is space in queue
    if(waitingPacket == nullptr && header->getTtl()>0){
        // queue packet
        waitingPacket = packet;
        // If forwarding delay timer expired, reset
        if(!nextForwardTimer->isScheduled()){
            // send packet after scheduled timer
            scheduleAt(simTime()+delay, nextForwardTimer);
            waitingPacket = packet;
        }
    }
    else{
        //drop packet
        EV_INFO << "ORW: dropping packet at " << nodeAddress << " to " << header->getDestAddr() << endl;
        PacketDropDetails details;
        if (waitingPacket != nullptr){
            details.setReason(PacketDropReason::QUEUE_OVERFLOW);
        }
        else details.setReason(PacketDropReason::HOP_LIMIT_REACHED);
        dropPacket(packet, details);
    }
}

void ORWRouting::dropPacket(Packet* const packet, PacketDropDetails& details)
{
    emit(packetDroppedSignal, packet, &details);
    delete packet;
}


void ORWRouting::handleSelfMessage(cMessage* const msg) {
    if(msg == nextForwardTimer){
        //Resend the message in the queue
        if(waitingPacket != nullptr){
            sendDown(waitingPacket);
            waitingPacket = nullptr;
            scheduleAt(simTime()+forwardingBackoff, nextForwardTimer);
        }
    }
}

void ORWRouting::handleStartOperation(LifecycleOperation *op) {
    if(waitingPacket!=nullptr){
        // send packet after scheduled timer
        scheduleAt(simTime()+forwardingBackoff, nextForwardTimer);
    }
}

void ORWRouting::handleStopOperation(LifecycleOperation *op) {
    cancelEvent(nextForwardTimer);
}

void ORWRouting::handleCrashOperation(LifecycleOperation *op) {
    handleStopOperation(op);
}

bool ORWRouting::messageKnown(const oppostack::PacketRecord record)
{
    return packetHistory.find(record);
}

oppostack::ORWRouting::~ORWRouting()
{
    cancelAndDelete(nextForwardTimer);
    if(waitingPacket != nullptr){
        delete waitingPacket;
        waitingPacket = nullptr;
    }
}
