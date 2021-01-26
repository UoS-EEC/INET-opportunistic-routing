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

#ifndef ORPLROUTINGTABLE_H_
#define ORPLROUTINGTABLE_H_

#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "Units.h"

class ORPLRoutingTable : public omnetpp::cSimpleModule, public inet::cListener
{
public:
    ORPLRoutingTable():
        arp(nullptr){};
    virtual void initialize(int stage) override;
protected:
    inet::IArp *arp;
    inet::IInterfaceTable *interfaceTable;
    inet::L3Address::AddressType addressType = inet::L3Address::NONE;


    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, double weight, cObject *details) override;
    void updateEncounters(const inet::L3Address address, const orpl::EqDC cost, const double weight);
    class NeighborEntry{
    public:
        orpl::EqDC lastEqDC = orpl::EqDC(25.5);
        double recentInteractionProb = 0;
        double interactionsTotal = 0;
    };
    inet::L3Address rootAddress;
    typedef std::map<inet::L3Address, NeighborEntry> NeighbourRecords;
    NeighbourRecords encountersTable;
    int encountersCount = 0;
    int probCalcEncountersThreshold = 20;
    int interactionDenominator = 0;
    orpl::EqDC forwardingCostW = orpl::EqDC(0.1);
    void calculateInteractionProbability();
    void configureInterface(inet::InterfaceEntry *ie);
    static omnetpp::simsignal_t updatedEqDCValueSignal;
public:
    orpl::EqDC calculateEqDC(const inet::L3Address destination, orpl::EqDC& nextHopEqDC) const;
    orpl::EqDC calculateEqDC(const inet::L3Address destination) const;
    void increaseInteractionDenominator();
};

#endif /* ORPLROUTINGTABLE_H_ */
