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

#include "ORWRoutingTable.h"

#include "common/EncounterDetails_m.h"
#include <inet/networklayer/nexthop/NextHopInterfaceData.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "linklayer/IOpportunisticLinkLayer.h"
#include "linklayer/WakeUpGram_m.h"
#include "common/oppDefs.h"
#include "common/EqDCTag_m.h"

using namespace omnetpp;
using namespace inet;
using namespace oppostack;

Define_Module(ORWRoutingTable);
simsignal_t ORWRoutingTable::updatedEqDCValueSignal = cComponent::registerSignal("updatedEqDCValue");
simsignal_t ORWRoutingTable::vagueNeighborsSignal = cComponent::registerSignal("vagueNeighbors");
simsignal_t ORWRoutingTable::sureNeighborsSignal = cComponent::registerSignal("sureNeighbors");

void ORWRoutingTable::initialize(int stage){
    if(stage == INITSTAGE_LOCAL){
        cModule* encountersModule = getCModuleFromPar(par("encountersSourceModule"), this);
        encountersModule->subscribe(IOpportunisticLinkLayer::coincidentalEncounterSignal, this);
        encountersModule->subscribe(IOpportunisticLinkLayer::expectedEncounterSignal, this);
        encountersModule->subscribe(IOpportunisticLinkLayer::listenForEncountersEndedSignal, this);

        interfaceTable = inet::getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        arp = inet::getModuleFromPar<IArp>(par("arpModule"), this);

        const char *addressTypeString = par("addressType");
        if (!strcmp(addressTypeString, "mac"))
            addressType = L3Address::MAC;
        else if (!strcmp(addressTypeString, "modulepath"))
            addressType = L3Address::MODULEPATH;
        else if (!strcmp(addressTypeString, "moduleid"))
            addressType = L3Address::MODULEID;
        else
            throw cRuntimeError("Unknown address type");

        forwardingCostW = EqDC(par("forwardingCost"));

        probCalcEncountersThresholdMax = par("probCalcEncountersThresholdMax");
    }
    else if(stage == INITSTAGE_LINK_LAYER){
        for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i){
            auto interfaceI = interfaceTable->getInterface(i);
            configureInterface(interfaceI);
            INetfilter* wakeUpMacFilter = dynamic_cast<INetfilter*>(interfaceI->getSubmodule("mac"));
            if(wakeUpMacFilter)wakeUpMacFilter->registerHook(0, this);
        }
        if(netfilters.empty()){
            throw cRuntimeError("No suitable Wake Up Mac found under: %s.mac", this->getFullPath().c_str());
        }
    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        const char* rootParameter = par("hubAddress");
        L3AddressResolver().tryResolve(rootParameter, rootAddress, L3AddressResolver::ADDR_MODULEPATH);

    }
}

void ORWRoutingTable::receiveSignal(cComponent* source, simsignal_t signalID, double weight, cObject* details)
{
    if(signalID == IOpportunisticLinkLayer::coincidentalEncounterSignal || signalID == IOpportunisticLinkLayer::expectedEncounterSignal){
        oppostack::EncounterDetails* encounterMacDetails = check_and_cast<oppostack::EncounterDetails*>(details);
        const L3Address inboundMacAddress = arp->getL3AddressFor(encounterMacDetails->getEncountered());
        updateEncounters(inboundMacAddress, encounterMacDetails->getCurrentEqDC(), weight);
    }
    if(signalID == IOpportunisticLinkLayer::listenForEncountersEndedSignal || signalID == IOpportunisticLinkLayer::coincidentalEncounterSignal){
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


EqDC ORWRoutingTable::calculateUpwardsCost(const L3Address destination, EqDC& nextHopEqDC) const
{
    Enter_Method("ORWRoutingTable::calculateUpwardsCost(address, ..)");

    const EqDC estimatedCost = calculateUpwardsCost(destination);
    nextHopEqDC = EqDC(25.5);
    // Limit resolution and add own routing cost before reporting.
    if(estimatedCost < EqDC(25.5)){
        nextHopEqDC = ExpectedCost(estimatedCost - forwardingCostW);
    }
    return estimatedCost;
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

void ORWRoutingTable::configureInterface(inet::InterfaceEntry* ie)
{
    int interfaceModuleId = ie->getId();
    // mac
    NextHopInterfaceData *d = ie->addProtocolData<NextHopInterfaceData>();
    d->setMetric(1);
    if (addressType == L3Address::MAC)
        d->setAddress(ie->getMacAddress());
    else if (ie && addressType == L3Address::MODULEPATH)
        d->setAddress(ModulePathAddress(interfaceModuleId));
    else if (ie && addressType == L3Address::MODULEID)
        d->setAddress(ModuleIdAddress(interfaceModuleId));
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

INetfilter::IHook::Result ORWRoutingTable::datagramPreRoutingHook(Packet* datagram)
{
    auto header = datagram->peekAtFront<WakeUpGram>();
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
    if(headerType==WakeUpGramType::WU_BEACON ||
            headerType==WakeUpGramType::WU_DATA){
        auto costHeader = datagram->peekAtFront<WakeUpBeacon>();
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

inet::L3Address ORWRoutingTable::getRouterIdAsGeneric()
{
    Enter_Method_Silent("ORWRoutingTable::getRouterIdAsGeneric()");
    // TODO: Cleaner way to get L3 Address?
    return interfaceTable->findFirstNonLoopbackInterface()->getNetworkAddress();
}
