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

#include "networklayer/ORPLHello.h"

#include "inet/common/IProtocolRegistrationListener.h"
#include "OpportunisticRoutingHeader_m.h"
#include "common/EqDCTag_m.h"
#include "common/oppDefs.h"
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/networklayer/contract/IL3AddressType.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/ModuleAccess.h>

using namespace inet;
using namespace oppostack;
Define_Module(ORPLHello);

std::vector<const Protocol *> ORPLHello::allocatedProtocols;

ORPLHello::~ORPLHello()
{
    cancelAndDelete(timer);
}

void ORPLHello::initialize(int const stage)
{
    ApplicationBase::initialize(stage);
    if(stage == INITSTAGE_LOCAL){
        int protocolId = par("protocol");
        if (protocolId < 143 || protocolId > 254)
            throw cRuntimeError("invalid protocol id %d, accepts only between 143 and 254", protocolId);
        protocol = ProtocolGroup::ipprotocol.findProtocol(protocolId);
        if (!protocol) {
            std::string name = "prot_" + std::to_string(protocolId);
            protocol = new Protocol(name.c_str(), name.c_str());
            allocatedProtocols.push_back(protocol);
            ProtocolGroup::ipprotocol.addProtocol(protocolId, protocol);
        }
        numPackets = par("numPackets");
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");

        packetLengthPar = &par("packetLength");
        sendIntervalPar = &par("sendInterval");

        timer = new cMessage("sendTimer");

        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        sentMessageQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("destinationRecord"));
        minTransmissionProbability = par("intermittentPacketRate");
        ASSERT(minTransmissionProbability > 0 && minTransmissionProbability < 1);
        packetSourceModule = getCModuleFromPar(par("packetSignalSourceModule"), this);
    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        packetSourceModule->subscribe(packetSentToLowerSignal, this);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        registerService(*protocol, nullptr, gate("ipIn"));
        registerProtocol(*protocol, gate("ipOut"), nullptr);
    }


}

void ORPLHello::startApp()
{
    if (isEnabled() && onOffCycles == 0)
        scheduleNextPacket(-1);
    else if (isEnabled())
        scheduleNextPacket(simTime());
}



void ORPLHello::handleMessageWhenUp(cMessage* msg)
{
    if(msg == timer){
        if (msg->getKind() == START) {
            destAddresses.clear();
            const char *destAddrs = par("destAddresses");
            cStringTokenizer tokenizer(destAddrs);
            const char *token;
            while ((token = tokenizer.nextToken()) != nullptr) {
                L3Address result;
                L3AddressResolver().tryResolve(token, result);
                if (result.isUnspecified())
                    EV_ERROR << "cannot resolve destination address: " << token << endl;
                else
                    destAddresses.push_back(result);
            }
        }

        if (!destAddresses.empty()) {
            sendHelloBroadcast(destAddresses[0]);
            if(isEnabled())
                scheduleNextPacket(simTime());
        }
    }
    else
        processPacket(check_and_cast<Packet *>(msg));
}

void ORPLHello::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[40];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void ORPLHello::scheduleNextPacket(simtime_t previous)
{
    simtime_t next;
    if (previous == -1) {
        next = simTime() <= startTime ? startTime : simTime();
        timer->setKind(START);
    }
    else {
        next = previous + *sendIntervalPar;
        timer->setKind(NEXT);
    }
    if (stopTime < SIMTIME_ZERO || next < stopTime)
        scheduleAt(next, timer);
}

void ORPLHello::cancelNextPacket()
{
    cancelEvent(timer);
}

bool ORPLHello::isEnabled()
{
    return numPackets == -1 || numSent < numPackets;
}

void ORPLHello::receiveSignal(cComponent* source, omnetpp::simsignal_t signalID, cObject* msg, cObject* details)
{
    if(signalID == packetSentToLowerSignal){
        // Extract destination to new header
        Packet* sentPacket = check_and_cast<Packet*>(msg);
        auto sentHeader = sentPacket->peekAtFront<OpportunisticRoutingHeader>();

        auto packetRecord = makeShared<OpportunisticRoutingHeader>();
        packetRecord->setId(onOffCycles); // Reuse ID for what cycle packet transmitted in
        //TODO: Make plural when relevant
        packetRecord->setDestAddr(sentHeader->getDestAddr());
        auto pkt = new Packet("PacketLog");
        pkt->insertAtFront(packetRecord);

        sentMessageQueue->pushPacket(pkt);
    }
}

void ORPLHello::sendHelloBroadcast(L3Address destination)
{

    auto pkt = new Packet("Hello Broadcast");
    pkt->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(25.5)); // Set to accept any forwarder

    //TODO: Make plural when relevant (with chooseDestAddr() from IpvxTraffGen() )
    const L3Address destAddr = destination;

    const IL3AddressType *addressType = destAddr.getAddressType();
    pkt->addTag<PacketProtocolTag>()->setProtocol(protocol);
    pkt->addTag<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    pkt->addTag<L3AddressReq>()->setDestAddress(destAddr);
    pkt->addTag<EqDCBroadcast>();

    EV_INFO << "Sending hello broadcast";
    emit(packetSentSignal, pkt);
    send(pkt,"ipOut");
}

void ORPLHello::printPacket(Packet *msg)
{
    L3Address src, dest;
    int protocol = -1;
    auto *ctrl = msg->getControlInfo();
    if (ctrl != nullptr) {
        protocol = ProtocolGroup::ipprotocol.getProtocolNumber(msg->getTag<PacketProtocolTag>()->getProtocol());
    }
    L3AddressTagBase *addresses = msg->findTag<L3AddressReq>();
    if (addresses == nullptr)
        addresses = msg->findTag<L3AddressInd>();
    if (addresses != nullptr) {
        src = addresses->getSrcAddress();
        dest = addresses->getDestAddress();
    }

    EV_INFO << msg << endl;
    EV_INFO << "Payload length: " << msg->getByteLength() << " bytes" << endl;

    if (protocol != -1)
        EV_INFO << "src: " << src << "  dest: " << dest << "  protocol=" << protocol << "\n";
}

void ORPLHello::processPacket(Packet *msg)
{
    emit(packetReceivedSignal, msg);
    EV_INFO << "Received packet: ";
    printPacket(msg);
    delete msg;
    numReceived++;
}

void ORPLHello::handleStartOperation(inet::LifecycleOperation* op)
{

    startApp();
    onOffCycles++;

    for(auto &helloDest : destAddresses){
        // by looping through helloDestinations
        int helloDestinationCount =0;
        const int queueSize = sentMessageQueue->getNumPackets();
        for(int i=0; i<queueSize; i++){
            const Packet* recordedMessage = sentMessageQueue->getPacket(i);
            const L3Address destAddr = recordedMessage->peekAtFront<OpportunisticRoutingHeader>()->getDestAddr();
            if(destAddr == helloDest){
                helloDestinationCount++;
            }
        }
        auto res = quietestDestination();
        if(res.second!=helloDest || res.first!=helloDestinationCount){
            throw cRuntimeError("Unsuitable replacement function");
        }
        const int firstRecordedCycle = sentMessageQueue->getPacket(0)->peekAtFront<OpportunisticRoutingHeader>()->getId();
        const int numCycles = onOffCycles - firstRecordedCycle;

        const int transmissionsExpected = minTransmissionProbability*(numCycles);
        if(helloDestinationCount<=transmissionsExpected){
            sendHelloBroadcast(helloDest);
        }
    }
}

void ORPLHello::handleStopOperation(inet::LifecycleOperation* op)
{
    handleCrashOperation(op);
}

void ORPLHello::handleCrashOperation(inet::LifecycleOperation* op)
{
    cancelNextPacket();
    onOffCycles++;
}

std::pair<int, inet::L3Address> ORPLHello::quietestDestination() const
{
    const int k = 0;
    inet::L3Address quietestAddress = destAddresses[k];

    const int queueSize = sentMessageQueue->getNumPackets();
    int quietestCount = queueSize;
    // Not efficient with large numbers of destinations but there should be <=3 anyway!
    for(auto &helloDest : destAddresses){
        int destinationCount =0;
        for(int i=0; i<queueSize; i++){
            const Packet* recordedMessage = sentMessageQueue->getPacket(i);
            const L3Address destAddr = recordedMessage->peekAtFront<OpportunisticRoutingHeader>()->getDestAddr();
            if(destAddr == helloDest){
                destinationCount++;
            }
        }
        if(destinationCount<quietestCount){
            quietestCount = destinationCount;
            quietestAddress = helloDest;
        }
    }
    return std::pair<int, inet::L3Address>(quietestCount, quietestAddress);
}
