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

#include "ORPLRoutingTable.h"
#include "WakeUpMacLayer.h"
#include "EncounterDetails_m.h"
#include "inet/networklayer/nexthop/NextHopInterfaceData.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;
using namespace inet;
using namespace oppostack;

Define_Module(ORPLRoutingTable);
simsignal_t ORPLRoutingTable::updatedEqDCValueSignal = cComponent::registerSignal("updatedEqDCValue");

void ORPLRoutingTable::initialize(int stage){
    if(stage == INITSTAGE_LOCAL){
        cModule* encountersModule = getModuleFromPar<cModule>(par("encountersSourceModule"), this);
        encountersModule->subscribe(WakeUpMacLayer::coincidentalEncounterSignal, this);
        encountersModule->subscribe(WakeUpMacLayer::expectedEncounterSignal, this);
        encountersModule->subscribe(WakeUpMacLayer::listenForEncountersEndedSignal, this);

        interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        arp = getModuleFromPar<IArp>(par("arpModule"), this);

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
    }
    else if(stage == INITSTAGE_LINK_LAYER){
        for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i)
            configureInterface(interfaceTable->getInterface(i));
    }
    else if(stage == INITSTAGE_NETWORK_LAYER){
        const char* rootParameter = par("hubAddress");
        L3AddressResolver().tryResolve(rootParameter, rootAddress, L3AddressResolver::ADDR_MODULEPATH);
    }
}

void ORPLRoutingTable::receiveSignal(cComponent* source, simsignal_t signalID, double weight, cObject* details)
{
    if(signalID == WakeUpMacLayer::coincidentalEncounterSignal || signalID == WakeUpMacLayer::expectedEncounterSignal){
        oppostack::EncounterDetails* encounterMacDetails = check_and_cast<oppostack::EncounterDetails*>(details);
        const L3Address inboundMacAddress = arp->getL3AddressFor(encounterMacDetails->getEncountered());
        updateEncounters(inboundMacAddress, encounterMacDetails->getCurrentEqDC(), weight);
    }
    if(signalID == WakeUpMacLayer::listenForEncountersEndedSignal || signalID == WakeUpMacLayer::coincidentalEncounterSignal){
        increaseInteractionDenominator();
    }
}

void ORPLRoutingTable::updateEncounters(const L3Address address, const oppostack::EqDC cost, const double weight)
{
    // Update encounters table entry. Optionally adding if it doesn't exist
    const EqDC oldEqDC = calculateEqDC(rootAddress);
    if(cost!=encountersTable[address].lastEqDC){
        encountersTable[address].lastEqDC = cost;
        if(cost<oldEqDC){
            emit(updatedEqDCValueSignal, oldEqDC.get());
            emit(updatedEqDCValueSignal, calculateEqDC(rootAddress).get());
        }
    }
    encountersTable[address].interactionsTotal += weight;
    encountersCount++;
}

EqDC ORPLRoutingTable::calculateEqDC(const L3Address destination, EqDC& nextHopEqDC) const
{
    const InterfaceEntry* interface = interfaceTable->findFirstNonLoopbackInterface();
    if(interface->getNetworkAddress() == destination){
        return EqDC(0.0);
    }
    else if(destination != rootAddress){
        throw cRuntimeError("Routing error, unknown graph root");
    }

    typedef std::pair<EqDC, double> EncPair;
    std::vector<EncPair> neighborEncounterPairs;
    for(auto const& entry : encountersTable){
        // Copy pairs to sortable vector
        const EncPair
            encounterPair(entry.second.lastEqDC, entry.second.recentInteractionProb);
        neighborEncounterPairs.push_back(encounterPair);
    }
    // Sort neighborEncounterPairs increasing on EqDC
    std::sort(neighborEncounterPairs.begin(), neighborEncounterPairs.end(), [](const EncPair &left, const EncPair &right) {
        return left.first < right.first;
    });

    double probSum = 0.0;
    EqDC probProductSum = EqDC(0.0);
    EqDC estimatedCostLessW = EqDC(25.5);
    for(auto const& entry : neighborEncounterPairs){
        // Check if in forwarding set
        if(entry.first <= estimatedCostLessW){
            probSum += entry.second;
            probProductSum += entry.second * entry.first;
            if(probSum > 0){
                estimatedCostLessW = (EqDC(1.0) + probProductSum)/probSum;
            }
            else{
                estimatedCostLessW = EqDC(25.5);
            }
        }
        else{
            break;
        }
    }
    // Set initial values of EqDC to aid startup
    if(probSum == 0){
        estimatedCostLessW = ExpectedCost(par("hubExpectedCost"));
    }
    nextHopEqDC = ExpectedCost(estimatedCostLessW);
    // Limit resolution and add own routing cost before reporting.
    const EqDC estimatedCost = ExpectedCost(estimatedCostLessW + forwardingCostW);
    return estimatedCost;
}
EqDC ORPLRoutingTable::calculateEqDC(const inet::L3Address destination) const
{
    EqDC nextHopDummyVar = EqDC(0.0);
    return calculateEqDC(destination, nextHopDummyVar);
}

void ORPLRoutingTable::calculateInteractionProbability()
{
    for(auto & entry : encountersTable){
        const double new_prob = entry.second.interactionsTotal/interactionDenominator;
        entry.second.recentInteractionProb = new_prob;
        entry.second.interactionsTotal = 0;

    }
    emit(updatedEqDCValueSignal, calculateEqDC(rootAddress).get());
    interactionDenominator = 0;
}

void ORPLRoutingTable::configureInterface(inet::InterfaceEntry* ie)
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

void ORPLRoutingTable::increaseInteractionDenominator()
{
    interactionDenominator++;
    if(encountersCount > probCalcEncountersThreshold
            || interactionDenominator > 2*probCalcEncountersThreshold){// KLUDGE of interaction denominator until hello messages are implemented
        calculateInteractionProbability();
        encountersCount = 0;
    }
}
