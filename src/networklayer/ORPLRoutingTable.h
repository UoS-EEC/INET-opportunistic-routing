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
    void initialize(int stage);
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
    EqDC calculateDownwardsCost(const inet::L3Address& destination);

    inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;

    simsignal_t static downwardSetSizeSignal;
    void addToDownwardsWarmupSet(const inet::L3Address destination, const EqDC minimumEqDC);
private:
    void printRoutingTable();
    virtual void finish();
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ORPLROUTINGTABLE_H_ */
