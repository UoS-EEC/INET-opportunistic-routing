/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ORWRoutingTable.h"

#include "common/EncounterDetails_m.h"
#include <inet/networklayer/nexthop/NextHopInterfaceData.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "../linklayer/ORWGram_m.h"
#include "linklayer/ILinkOverhearingSource.h"
#include "linklayer/IOpportunisticLinkLayer.h"
#include "common/EqDCTag_m.h"
#include "common/oppDefs.h"
#include "ORWHello.h"

using namespace omnetpp;
using namespace inet;
using namespace oppostack;

Define_Module(ORWRoutingTable);
simsignal_t ORWRoutingTable::updatedEqDCValueSignal = cComponent::registerSignal("updatedEqDCValue");
simsignal_t ORWRoutingTable::vagueNeighborsSignal = cComponent::registerSignal("vagueNeighbors");
simsignal_t ORWRoutingTable::sureNeighborsSignal = cComponent::registerSignal("sureNeighbors");

void ORWRoutingTable::initialize(int stage){
    RoutingTableBase::initialize(stage);
    if(stage == INITSTAGE_LOCAL){
        cModule* encountersModule = getCModuleFromPar(par("encountersSourceModule"), this);
        encountersModule->subscribe(ILinkOverhearingSource::coincidentalEncounterSignal, this);
        encountersModule->subscribe(ILinkOverhearingSource::expectedEncounterSignal, this);
        encountersModule->subscribe(ILinkOverhearingSource::listenForEncountersEndedSignal, this);

        arp = inet::getModuleFromPar<IArp>(par("arpModule"), this);

        probCalcEncountersThresholdMax = par("probCalcEncountersThresholdMax");
    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        const char* rootParameter = par("hubAddress");
        L3AddressResolver().tryResolve(rootParameter, rootAddress, L3AddressResolver::ADDR_MODULEPATH);

    }
}

void ORWRoutingTable::receiveSignal(cComponent* source, simsignal_t signalID, double weight, cObject* details)
{
    if(signalID == ILinkOverhearingSource::coincidentalEncounterSignal || signalID == ILinkOverhearingSource::expectedEncounterSignal){
        oppostack::EncounterDetails* encounterMacDetails = check_and_cast<oppostack::EncounterDetails*>(details);
        const L3Address inboundMacAddress = arp->getL3AddressFor(encounterMacDetails->getEncountered());
        updateEncounters(inboundMacAddress, encounterMacDetails->getCurrentEqDC(), weight);
    }
    if(signalID == ILinkOverhearingSource::listenForEncountersEndedSignal || signalID == ILinkOverhearingSource::coincidentalEncounterSignal){
        increaseInteractionDenominator();
    }
}

void ORWRoutingTable::updateEncounters(const L3Address address, const oppostack::EqDC cost, const double weight)
{
    // Update encounters table entry. Optionally adding if it doesn't exist
    const EqDC oldEqDC = calculateUpwardsCost(rootAddress);
    if(cost!=encountersTable[address].lastEqDC){
        encountersTable[address].lastEqDC = cost;
        if(cost<oldEqDC){
            emit(updatedEqDCValueSignal, oldEqDC.get());
            emit(updatedEqDCValueSignal, calculateUpwardsCost(rootAddress).get());
        }
    }
    encountersTable[address].interactionsTotal += weight;
    encountersCount++;
}

EqDC ORWRoutingTable::calculateCostToRoot() const
{
    typedef std::pair<EqDC, double> EncPair;
    std::vector<EncPair> neighborEncounterPairs;
    for (const auto& entry : encountersTable) {
        // Copy pairs to sortable vector
        const EncPair encounterPair(entry.second.lastEqDC, entry.second.recentInteractionProb);
        neighborEncounterPairs.push_back(encounterPair);
    }
    // Sort neighborEncounterPairs increasing on EqDC
    std::sort(neighborEncounterPairs.begin(), neighborEncounterPairs.end(), [](const EncPair &left, const EncPair &right) {
        return left.first < right.first;
        });
    double probSum = 0.0;
    EqDC probProductSum = EqDC(0.0);
    EqDC estimatedCostLessW = EqDC(25.5);
    for (const auto& entry : neighborEncounterPairs) {
        // Check if in forwarding set
        if (entry.first <= estimatedCostLessW) {
            probSum += entry.second;
            probProductSum += entry.second * entry.first;
            if (probSum > 0) {
                estimatedCostLessW = (EqDC(1.0) + probProductSum) / probSum;
            }
            else {
                estimatedCostLessW = EqDC(25.5);
            }
        }
        else {
            break;
        }
    }
    // Set initial values of EqDC to aid startup
    if (probSum == 0) {
        estimatedCostLessW = ExpectedCost(par("hubExpectedCost"));
    }
    return estimatedCostLessW;
}

EqDC ORWRoutingTable::calculateUpwardsCost(const inet::L3Address destination) const
{
    Enter_Method("ORWRoutingTable::calculateUpwardsCost(address)");
    const InterfaceEntry* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    else if(destination != rootAddress){
        throw cRuntimeError("Routing error, unknown graph root");
    }
    return ExpectedCost(std::min(calculateCostToRoot() + forwardingCostW, EqDC(25.5)));
}

void ORWRoutingTable::calculateInteractionProbability()
{
    int sureNeighbors = 0;
    int vagueNeighbors = 0;
    for(auto & entry : encountersTable){
        const double new_prob = entry.second.interactionsTotal/interactionDenominator;
        entry.second.recentInteractionProb = new_prob;
        switch((int) std::floor(entry.second.interactionsTotal)){
            case 0: break;
            case 1:
            case 2: vagueNeighbors++; break;
            default: sureNeighbors++;
        }
        entry.second.interactionsTotal = 0;

    }
    emit(updatedEqDCValueSignal, calculateUpwardsCost(rootAddress).get());
    emit(vagueNeighborsSignal, vagueNeighbors);
    emit(sureNeighborsSignal, sureNeighbors);
    interactionDenominator = 0;
}

void ORWRoutingTable::activateWarmUpRoutingData()
{
    probCalcEncountersThreshold = std::min(probCalcEncountersThreshold * 2, probCalcEncountersThresholdMax);
    calculateInteractionProbability();
    encountersCount = 0;
}

void ORWRoutingTable::increaseInteractionDenominator()
{
    interactionDenominator++;
    if(encountersCount > probCalcEncountersThreshold
            || interactionDenominator > 2*probCalcEncountersThreshold){// KLUDGE of interaction denominator until hello messages are implemented
        activateWarmUpRoutingData();
    }
}

Hz ORWRoutingTable::estAdvertismentRate() {
    auto helloModule = dynamic_cast<ORWHello*>(getModuleByPath("^.helloManager"));
    auto helloInterval = s( helloModule->par("sendInterval").doubleValueInUnit("s") );
    auto  helloRate = unit(1)/helloInterval;
    return helloRate;
}

INetfilter::IHook::Result ORWRoutingTable::datagramPreRoutingHook(Packet* datagram)
{
    auto header = datagram->peekAtFront<ORWGram>();
    const auto destAddr = header->getReceiverAddress();
    // TODO: Use IInterfaceTable::findInterfaceByAddress( L3Address(destAddr) )
    // If packet addressed directly to interface, then accept it with zero cost.
    EqDC upwardsCostToRoot = calculateUpwardsCost(rootAddress);
    datagram->addTagIfAbsent<EqDCInd>()->setEqDC(upwardsCostToRoot);
    for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i){
        auto interfaceEntry = interfaceTable->getInterface(i);
        if(destAddr == interfaceEntry->getMacAddress()){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(0.0));
            return IHook::Result::ACCEPT;
        }
    }
    const auto headerType = header->getType();
    if(headerType==ORWGramType::ORW_BEACON ||
            headerType==ORWGramType::ORW_DATA){
        auto costHeader = datagram->peekAtFront<ORWBeacon>();
        if(destAddr == MacAddress::BROADCAST_ADDRESS){
            datagram->addTagIfAbsent<EqDCReq>()->setEqDC(EqDC(25.5));
            //TODO: Check OpportunisticRoutingHeader for further forwarding confirmation
            return IHook::Result::ACCEPT;
        }
        else if(header->getUpwards() == true){
            if(upwardsCostToRoot<=costHeader->getMinExpectedCost()){
                datagram->addTagIfAbsent<EqDCReq>()->setEqDC(upwardsCostToRoot);
                //TODO: Check OpportunisticRoutingHeader for further forwarding confirmation
                return IHook::Result::ACCEPT;
            }

        }
    }
    return IHook::Result::DROP;
}
