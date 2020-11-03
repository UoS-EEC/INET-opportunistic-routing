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
#include "OpportunisticRoutingHeader_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "OpportunisticRpl.h"

Define_Module(OpportunisticRpl);

void OpportunisticRpl::initialize(int stage) {
    NetworkProtocolBase::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        nextForwardTimer = new cMessage("Forwarding Backoff Timer");
        // This is a crude measure to stop the Mac buffer becoming overloaded
        forwardingSpacing = SimTime(30, SIMTIME_MS);

        interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        // routingTable = getModuleFromPar<NextHopRoutingTable>(par("routingTableModule"), this);
        arp = getModuleFromPar<IArp>(par("arpModule"), this);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        auto ie = interfaceTable->findFirstNonLoopbackInterface();
        if (ie != nullptr)
            nodeAddress = ie->getNetworkAddress();
        else
            throw cRuntimeError("No non-loopback interface found!");

        // Initialize progress table
        L3Address hubAddress(par("hubAddress"));
        Progress hubProgress(par("hubProgress"));
        if(hubAddress.getType() == L3Address::AddressType::IPv4){
            progressTable.insert(std::pair<L3Address, Progress>(hubAddress, hubProgress));
        }
    }
}

void OpportunisticRpl::handleUpperPacket(Packet *packet) {

}

void OpportunisticRpl::handleLowerPacket(Packet *packet) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    if(header->getDestAddr()==nodeAddress){
        decapsulate(packet);
        sendUp(packet);
    }
    else if(progressTable.find(header->getDestAddr())!=progressTable.end()){
        // Route to a node in the routing table
        // Packet not destined for this node
        // Decrease TTL, calculate progress and Forward.
        // Possible "trim" required
        // packet->trim();
        auto mutableHeader = packet->removeAtFront<OpportunisticRoutingHeader>();
        auto newTtl = mutableHeader->getTtl()-1;
        mutableHeader->setTtl(newTtl);
        packet->insertAtFront(mutableHeader);
        Progress currentProgress = progressTable.at(header->getDestAddr());
        setDownControlInfo(packet, MacAddress::STP_MULTICAST_ADDRESS, currentProgress);
    }
}

void OpportunisticRpl::encapsulate(Packet *packet) {
    auto header = makeShared<OpportunisticRoutingHeader>();

    // TODO: Populate Header fields
    // TODO: remove tiny ttl when routing implemented
    header->setTtl(3);
    packet->insertAtFront(header);

    setDownControlInfo(packet, MacAddress::STP_MULTICAST_ADDRESS, 65535);
}

void OpportunisticRpl::setDownControlInfo(Packet* packet, MacAddress macMulticast, Progress progress) {
    packet->getTags().addTagIfAbsent<MacAddressReq>()->setDestAddress(macMulticast);
    packet->getTags().addTagIfAbsent<PacketProtocolTag>()->setProtocol(&OpportunisticRouting);
    packet->getTags().addTagIfAbsent<DispatchProtocolInd>()->setProtocol(&OpportunisticRouting);
}

void OpportunisticRpl::decapsulate(Packet *packet)
{
    auto networkHeader = packet->popAtFront<OpportunisticRoutingHeader>();
    auto payloadLength = networkHeader->getLength()-16; // TODO: Remove header length magic number
    if (packet->getDataLength() < B(payloadLength) ) {
        throw cRuntimeError("Data error: illegal payload length");     //FIXME packet drop
    }
    if (packet->getDataLength() > B(payloadLength) )
        packet->setBackOffset(packet->getFrontOffset() + B(payloadLength));
    auto payloadProtocol = networkHeader->getProtocol();
    packet->getTags().addTagIfAbsent<NetworkProtocolInd>()->setProtocol(&getProtocol());
    packet->getTags().addTagIfAbsent<NetworkProtocolInd>()->setNetworkProtocolHeader(networkHeader);
    packet->getTags().addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->getTags().addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
    packet->getTags().addTagIfAbsent<L3AddressInd>()->setSrcAddress(networkHeader->getSrcAddr());
}

void OpportunisticRpl::queuePacket(Packet *packet) {
    auto header = packet->peekAtFront<OpportunisticRoutingHeader>();
    if(header->getTtl()>0){
        if(nextForwardTimer->getArrivalTime()>simTime()){
            // timer is scheduled so queue packet instead
            if(waitingPacket == nullptr){
                waitingPacket = packet;
                scheduleAt(simTime()+forwardingSpacing, nextForwardTimer);
            }
            else{
                EV << "Dropping packet as queue of 1 is full" << endl;
            }
        }
        else{
            // send packet and schedule timer
            sendDown(packet);
        }
    }
    else{
        //drop packet
        EV << "ORPL at" << simTime() << ": dropping packet at " << nodeAddress << " to " << header->getDestAddr() << endl;
        delete packet;
    }
}

void OpportunisticRpl::handleSelfMessage(cMessage *msg) {
    if(msg == nextForwardTimer){
        //Resend the message in the queue
        queuePacket(waitingPacket);
        waitingPacket = nullptr;
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
