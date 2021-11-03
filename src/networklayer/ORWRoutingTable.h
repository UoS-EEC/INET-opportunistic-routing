/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_ORWROUTINGTABLE_H_
#define NETWORKLAYER_ORWROUTINGTABLE_H_

#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/contract/IArp.h>
#include <inet/networklayer/contract/IInterfaceTable.h>
#include <inet/networklayer/contract/INetfilter.h>
#include <inet/common/Units.h>

#include "RoutingTableBase.h"

namespace oppostack{


class ORWRoutingTable : public RoutingTableBase, public omnetpp::cListener{
public:
    virtual void initialize(int stage) override;
protected:
    inet::IArp *arp{nullptr};

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
    virtual void calculateInteractionProbability();
    static omnetpp::simsignal_t updatedEqDCValueSignal;
    static omnetpp::simsignal_t vagueNeighborsSignal;
    static omnetpp::simsignal_t sureNeighborsSignal;
    void increaseInteractionDenominator();
    EqDC calculateCostToRoot() const;
    virtual void activateWarmUpRoutingData();
    virtual inet::Hz estAdvertismentRate() override;

public:
    using RoutingTableBase::calculateUpwardsCost;
    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination) const override;
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;
};

} //namespace oppostack

#endif /* NETWORKLAYER_ORWROUTINGTABLE_H_ */
