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

#include "inet/common/INETDefs.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/networklayer/nexthop/NextHopRoutingTable.h"
#include "OpportunisticRoutingHeader_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ProtocolGroup.h"
#include "OpportunisticRpl.h"
#include "ExpectedCostTag_m.h"

Define_Module(OpportunisticRpl);

void OpportunisticRpl::initialize(int const stage) {
    NetworkProtocolBase::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        nextForwardTimer = new cMessage("Forwarding Backoff Timer");
        // This is a crude measure to stop the Mac buffer becoming overloaded
        forwardingSpacing = SimTime(30, SIMTIME_MS);
        routingTable = getModuleFromPar<NextHopRoutingTable>(par("routingTableModule"), this);
        arp = getModuleFromPar<IArp>(par("arpModule"), this);
        initialTTL = par("initialTTL");
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        ProtocolGroup::ipprotocol.addProtocol(245, &OpportunisticRouting);
        registerService(Protocol::nextHopForwarding, gate("transportIn"), gate("queueIn"));
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        if (ie != nullptr)
            nodeAddress = ie->getNetworkAddress();
        else
            throw cRuntimeError("No non-loopback interface found!");

        // Initialize expectedCost table
        L3Address hubAddress;
        L3AddressResolver().tryResolve(par("hubAddress"), hubAddress, L3AddressResolver::ADDR_MODULEPATH);
        ExpectedCost hubExpectedCost(par("hubExpectedCost"));
        if(!hubAddress.isUnspecified()){
            expectedCostTable.insert(std::pair<L3Address, ExpectedCost>(hubAddress, hubExpectedCost));
        }
        else{
            EV_WARN << "Unspecified hubAddress" << endl;
        }
    }
}

void OpportunisticRpl::handleUpperPacket(Packet* const packet) {
    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    //TODO: check tags assigned by higher layer
    if(addressReq->getDestAddress().getType() == L3Address::NONE){
        addressReq->setDestAddress(L3Address(par("hubAddress")));
        EV_WARN << "ORPL, setting received packet address to default hub address" << endl;
    }
    encapsulate(packet);
    queuePacket(packet);
}

void OpportunisticRpl::handleLowerPacket(Packet* const packet) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    if(header->getDestAddr()==nodeAddress){
        decapsulate(packet);
        sendUp(packet);
    }
    else if(expectedCostTable.find(header->getDestAddr())!=expectedCostTable.end()){
        // Route to a node in the routing table
        // Packet not destined for this node
        // Decrease TTL, calculate expectedCost and Forward.
        // "trim" required to remove the popped headers from lower layers
        packet->trim();
        auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
        auto newTtl = mutableHeader->getTtl()-1;
        mutableHeader->setTtl(newTtl);
        auto outboundMacAddress =  MacAddress::STP_MULTICAST_ADDRESS;
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        outboundMacAddress = arp->resolveL3Address(mutableHeader->getDestAddr(), ie);
        if(outboundMacAddress == MacAddress::UNSPECIFIED_ADDRESS){
            EV_WARN << "Forwarding message to unknown L3Address" << endl;
        }
        packet->insertAtFront(mutableHeader);
        ExpectedCost currentExpectedCost = expectedCostTable.at(header->getDestAddr());
        setDownControlInfo(packet, outboundMacAddress, currentExpectedCost);
        queuePacket(packet);
    }
    else{
        delete packet;
    }
}

void OpportunisticRpl::encapsulate(Packet* const packet) {
    auto header = makeShared<OpportunisticRoutingHeader>();
    auto outboundMacAddress =  MacAddress::STP_MULTICAST_ADDRESS;
    if(packet->findTag<L3AddressReq>()!=nullptr){
        header->setDestAddr(packet->getTag<L3AddressReq>()->getDestAddress());
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        outboundMacAddress = arp->resolveL3Address(header->getDestAddr(), ie);
    }
    else{
        header->setDestAddr( L3Address() );
        EV_WARN << "Packet sent with no destination";
    }
    header->setId(sequenceNumber++);
    header->setLength(packet->getDataLength() + header->getChunkLength());
    auto protocolTag = packet->findTag<PacketProtocolTag>();
    if(protocolTag != nullptr){
        header->setProtocol(protocolTag->getProtocol());
    }
    else{
        header->setProtocol(&Protocol::manet);
    }
    header->setSrcAddr(nodeAddress);
    header->setTtl(initialTTL);
    header->setVersion(IpProtocolId::IP_PROT_MANET);
    packet->insertAtFront(header);
    ExpectedCost initialCost = 65535;
    if(expectedCostTable.find(header->getDestAddr())!=expectedCostTable.end()){
        initialCost = expectedCostTable.at(header->getDestAddr());
    }
    setDownControlInfo(packet, outboundMacAddress, 65535);
}

void OpportunisticRpl::setDownControlInfo(Packet* const packet, const MacAddress& macMulticast, const ExpectedCost& expectedCost) {
    packet->addTagIfAbsent<MacAddressReq>()->setDestAddress(macMulticast);
    packet->addTagIfAbsent<ExpectedCostReq>()->setExpectedCost(expectedCost);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&OpportunisticRouting);
    packet->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(&OpportunisticRouting);
}

void OpportunisticRpl::decapsulate(Packet* const packet)
{
    auto networkHeader = packet->popAtFront<OpportunisticRoutingHeader>();
    auto payloadLength = networkHeader->getLength() - networkHeader->getChunkLength(); // TODO: Remove header length magic number
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

void OpportunisticRpl::queuePacket(Packet* const packet) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    // If packet lifetime not expired and there is space in queue
    if(waitingPacket == nullptr && header->getTtl()>0){
        // queue packet
        waitingPacket = packet;
        // TODO: Allow immediate send once WuMAC Layer problem
        // of radio mode switching is solved
        // If forwarding delay timer expired, reset
        if(!nextForwardTimer->isScheduled()){
            // send packet after scheduled timer
            scheduleAt(simTime()+forwardingSpacing, nextForwardTimer);
            waitingPacket = packet;
        }
    }
    else{
        //drop packet
        EV_INFO << "ORPL at" << simTime() << ": dropping packet at " << nodeAddress << " to " << header->getDestAddr() << endl;
        delete packet;
    }
}

void OpportunisticRpl::handleSelfMessage(cMessage* const msg) {
    if(msg == nextForwardTimer){
        //Resend the message in the queue
        if(waitingPacket != nullptr){
            sendDown(waitingPacket);
            waitingPacket = nullptr;
            scheduleAt(simTime()+forwardingSpacing, nextForwardTimer);
        }
    }
}

void OpportunisticRpl::finish() {
    cancelAndDelete(nextForwardTimer);
}

void OpportunisticRpl::handleStartOperation(LifecycleOperation *op) {
}

void OpportunisticRpl::handleStopOperation(LifecycleOperation *op) {
}

void OpportunisticRpl::handleCrashOperation(LifecycleOperation *op) {
}

bool OpportunisticRpl::queryAcceptPacket(const MacAddress& destination,
        const ExpectedCost& currentExpectedCost) {
    L3Address l3dest = arp->getL3AddressFor(destination);
    L3Address modPathAddr = l3dest.toModulePath();
    if(l3dest==nodeAddress){
        // Mac layer should probably perform this check anyway
        return true;
    }
    else if(expectedCostTable.find(modPathAddr)!=expectedCostTable.end()){
        ExpectedCost newCost = expectedCostTable.at(modPathAddr);
        if(newCost < currentExpectedCost){
            return true;
        }
    }
    // Insufficient progress or unknown destination so don't accept
    return false;

}
