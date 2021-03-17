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
#include <inet/common/ModuleAccess.h>

using namespace inet;
using namespace oppostack;
Define_Module(ORPLHello);

void ORPLHello::initialize(int const stage)
{
    IpvxTrafGen::initialize(stage);

    if(stage == INITSTAGE_LOCAL){
        sentMessageQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("destinationRecord"));
        minTransmissionProbability = par("intermittentPacketRate");
        ASSERT(minTransmissionProbability > 0 && minTransmissionProbability < 1);
        packetSourceModule = getCModuleFromPar(par("packetSignalSourceModule"), this);
    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        packetSourceModule->subscribe(packetSentToLowerSignal, this);
    }


}

L3Address ORPLHello::chooseDestAddr()
{
    return quietestDestination().second;
}


void ORPLHello::sendPacket()
{
    char msgName[] = "Hello Broadcast";

    auto pkt = new Packet(msgName);
    pkt->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(25.5)); // Set to accept any forwarder

    if(destAddresses.size()>5)
        EV_ERROR << "ORPLHello is not designed to handle large numbers of destinations";
    else if(destAddresses.size()>3)
        EV_ERROR << "ORPLHello may not behave properly with more than 3 destinations";

    const L3Address destAddr = chooseDestAddr();
    ASSERT(destAddr == destAddresses[0]);
    const IL3AddressType *addressType = destAddr.getAddressType();
    pkt->addTag<PacketProtocolTag>()->setProtocol(protocol);
    pkt->addTag<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    pkt->addTag<L3AddressReq>()->setDestAddress(destAddr);
    pkt->addTag<EqDCBroadcast>();

    EV_INFO << "Sending hello broadcast";
    emit(packetSentSignal, pkt);
    send(pkt,"ipOut");
    numSent++;
}

void ORPLHello::handleStartOperation(inet::LifecycleOperation* op)
{
    onOffCycles++;

    if(numSent == 0){
        startApp();
        return;
    }

    scheduleNextPacket(simTime());
    const int queueSize = sentMessageQueue->getNumPackets();
    int numCycles = 0;
    if(queueSize>0){
        const int firstRecordedCycle = sentMessageQueue->getPacket(0)->peekAtFront<OpportunisticRoutingHeader>()->getId();
        numCycles = onOffCycles - firstRecordedCycle;
        const int transmissionsExpected = minTransmissionProbability*(numCycles);
        if(quietestDestination().first<=transmissionsExpected){
            sendPacket();
        }
    }
}

std::pair<int, inet::L3Address> ORPLHello::quietestDestination() const
{
    const int k = 0;
    ASSERT(!destAddresses.empty());
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

