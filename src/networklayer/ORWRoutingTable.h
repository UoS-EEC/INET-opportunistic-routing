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

#ifndef NETWORKLAYER_ORWROUTINGTABLE_H_
#define NETWORKLAYER_ORWROUTINGTABLE_H_

#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/contract/IArp.h>
#include <inet/networklayer/contract/IInterfaceTable.h>
#include <inet/networklayer/contract/INetfilter.h>

#include "common/Units.h"

namespace oppostack{


class ORWRoutingTable : public omnetpp::cSimpleModule, public inet::cListener, public inet::NetfilterBase::HookBase
{
public:
    ORWRoutingTable():
        arp(nullptr){};
    virtual void initialize(int stage) override;
protected:
    inet::IArp *arp;
    inet::IInterfaceTable *interfaceTable;
    inet::L3Address::AddressType addressType = inet::L3Address::NONE;


    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, double weight, cObject *details) override;
    void updateEncounters(const inet::L3Address address, const oppostack::EqDC cost, const double weight);
    class NeighborEntry{
    public:
        oppostack::EqDC lastEqDC = oppostack::EqDC(25.5);
        double recentInteractionProb = 0;
        double interactionsTotal = 0;
    };
    inet::L3Address rootAddress;
    typedef std::map<inet::L3Address, NeighborEntry> NeighbourRecords;
    NeighbourRecords encountersTable;
    int encountersCount = 0;
    int probCalcEncountersThreshold = 20;
    int probCalcEncountersThresholdMax = 40;
    int interactionDenominator = 0;
    oppostack::EqDC forwardingCostW = oppostack::EqDC(0.1);
    virtual void calculateInteractionProbability();
    void configureInterface(inet::InterfaceEntry *ie);
    static omnetpp::simsignal_t updatedEqDCValueSignal;
    static omnetpp::simsignal_t vagueNeighborsSignal;
    static omnetpp::simsignal_t sureNeighborsSignal;
    void increaseInteractionDenominator();
    EqDC calculateCostToRoot() const;
    virtual void activateWarmUpRoutingData();

public:
    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination, oppostack::EqDC& nextHopEqDC) const;
    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination) const;
    inet::L3Address getRouterIdAsGeneric();
    // Hook to accept incoming requests
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet*) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramPostRoutingHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramLocalInHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramLocalOutHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    EqDC getForwardingCost(){return forwardingCostW;}
};

} //namespace oppostack

#endif /* NETWORKLAYER_ORWROUTINGTABLE_H_ */
