/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_ORPLROUTINGTABLE_H_
#define NETWORKLAYER_ORPLROUTINGTABLE_H_

#include "ORWRoutingTable.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/contract/IRoute.h"
#include "OpportunisticRoutingHeader_m.h"

namespace oppostack {

class ORPLRoutingTable : public ORWRoutingTable
{
private:
    // Merged set of downward neighbors routing set.
    // Each occurrence from neighbors increments interactionsTotal
    // Included in get(Num)Route(s) if recentInteractionProb > 0
    // Periodically the recentInteractionProb is updated using
    // interactionsTotal
    NeighbourRecords routingSetTable;

    const Ptr<const OpportunisticRoutingHeader> getOpportunisticRoutingHeaderFromPacket(const cObject* msg, EqDC& indicatedMinCostToSink);
    int countDownwardNodes(const EqDC ownEqDCEstimate) const;

protected:
    void initialize(int stage) override;
    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, cObject* msg, cObject *details) override;
public:
    // Like interfaces from IRoutingTable
    // Get number of routes to nodes in routing set
    // Does not remove duplicate from immediate neighborTable and routingSetTable
    virtual int getNumRoutes();
    // Get specific route info for each node
    // Useful for checking if in range or via another node
    // TODO: replace with IRoute*
    virtual std::pair<const inet::L3Address ,int > getRoute(int k);

    virtual void activateWarmUpRoutingData() override;
    EqDC calculateUpwardsCost(const inet::L3Address destination) const override;
    EqDC calculateDownwardsCost(const inet::L3Address& destination) const;

    inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;

    simsignal_t static downwardSetSizeSignal;
    void addToDownwardsWarmupSet(const inet::L3Address destination, const EqDC minimumEqDC);
private:
    void printRoutingTable();
    virtual void finish() override;
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ORPLROUTINGTABLE_H_ */
