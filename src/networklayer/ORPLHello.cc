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

        timer = new cMessage("Hello retransmission timer");

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
        // Populate the queue with dummy packets to begin with
        auto packetRecord = makeShared<OpportunisticRoutingHeader>();
        packetRecord->setId(onOffCycles); // Reuse ID for what cycle packet transmitted in
        //TODO: Make plural when relevant
        destAddresses.clear();
        //const char *destAddrs = par("destAddresses");
        //cStringTokenizer tokenizer(destAddrs);
        //const char *token;
        /*while ((token = tokenizer.nextToken()) != nullptr) {
            L3Address result;
            L3AddressResolver().tryResolve(token, result);
            if (result.isUnspecified())
                EV_ERROR << "cannot resolve destination address: " << token << endl;
            else
                destAddresses.push_back(result);
        }*/
        //Taken from IpvxTrafGen
        const char *token = par("destAddresses");
        L3AddressResolver().tryResolve(token, helloDestination, L3AddressResolver::ADDR_MODULEPATH);
        if (helloDestination.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;

        packetRecord->setDestAddr(helloDestination);
        auto pkt = new Packet("PacketLog");
        pkt->insertAtFront(packetRecord);

        const int queueLimit = sentMessageQueue->getMaxNumPackets();
        for(int i=0; i<queueLimit; i++){
            sentMessageQueue->pushPacket(pkt->dup());
        }
        delete pkt;
    }


}

ORPLHello::~ORPLHello()
{
    cancelAndDelete(timer);
}

void ORPLHello::handleStartOperation(inet::LifecycleOperation* op)
{

    if(isEnabled())
        rescheduleTransmissionTimer();
    onOffCycles++;

    // Fails if queue is empty, but it should never be empty
    const int firstRecordedCycle = sentMessageQueue->getPacket(0)->peekAtFront<OpportunisticRoutingHeader>()->getId();
    const int numCycles = onOffCycles - firstRecordedCycle;
    ASSERT(numCycles>0);
    //TODO: Make plural when relevant
    // by looping through helloDestinations
    int helloDestinationCount =0;
    const int queueSize = sentMessageQueue->getNumPackets();
    for(int i=0; i<queueSize; i++){
        const Packet* recordedMessage = sentMessageQueue->getPacket(i);
        const L3Address destAddr = recordedMessage->peekAtFront<OpportunisticRoutingHeader>()->getDestAddr();
        if(destAddr == helloDestination){
            helloDestinationCount++;
        }
    }
    const double transmissionRate = (double)helloDestinationCount/(numCycles);
    if(transmissionRate<minTransmissionProbability){
        sendHelloBroadcast(helloDestination);
    }
}

void ORPLHello::handleStopOperation(inet::LifecycleOperation* op)
{
    handleCrashOperation(op);
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

void ORPLHello::handleCrashOperation(inet::LifecycleOperation* op)
{
    cancelNextPacket();
    onOffCycles++;
}

void ORPLHello::handleSelfMessage(cMessage* msg)
{
    if(msg == timer){
        sendHelloBroadcast(helloDestination);
        if(isEnabled())
            rescheduleTransmissionTimer();
    }
}

void ORPLHello::sendHelloBroadcast(L3Address destination)
{

    auto pkt = new Packet("Hello Broadcast");
    pkt->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(25.5)); // Set to accept any forwarder

    //TODO: Make plural when relevant (with chooseDestAddr() from IpvxTraffGen() )
    const L3Address destAddr = helloDestination;

    const IL3AddressType *addressType = destAddr.getAddressType();
    pkt->addTag<PacketProtocolTag>()->setProtocol(protocol);
    pkt->addTag<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    pkt->addTag<L3AddressReq>()->setDestAddress(destAddr);
    pkt->addTag<EqDCBroadcast>();

    EV_INFO << "Sending hello broadcast";
    send(pkt,"lowerLayerOut");
}

void ORPLHello::handleMessageWhenUp(cMessage* message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
}

void ORPLHello::rescheduleTransmissionTimer()
{
    scheduleAt(simTime() + uniform(0.9,1)* ((simtime_t)*sendIntervalPar), timer);
}

void ORPLHello::cancelNextPacket()
{
    cancelEvent(timer);
}

bool ORPLHello::isEnabled()
{
    return numPackets == -1 || numSent < numPackets;
}
