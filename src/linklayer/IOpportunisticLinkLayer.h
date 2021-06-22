/*
 * IOpportunisticLinkLayer.h
 *
 *  Created on: 21 Jun 2021
 *      Author: Edward
 */

#ifndef LINKLAYER_IOPPORTUNISTICLINKLAYER_H_
#define LINKLAYER_IOPPORTUNISTICLINKLAYER_H_

#include <inet/networklayer/contract/INetfilter.h>

namespace oppostack {

// Interface for reactive/opportunistic link layer protocol
// Pre-routing hook is queried with any incoming Packet to test opportunistic acceptance
// All other hooks must accept
class IOpportunisticLinkLayer : public inet::NetfilterBase
{
public:
    /**
     * Neighbor Update signals definitions
     */
    static omnetpp::simsignal_t expectedEncounterSignal;
    static omnetpp::simsignal_t coincidentalEncounterSignal;
    static omnetpp::simsignal_t listenForEncountersEndedSignal;
protected:
    // NetFilter functions:
    // @brief called before a inet::Packetarriving from the network is accepted/acked
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet* datagram);
    // @brief datagramForwardHook not implemented since MAC does not forward -> see datagramLocalOutHook
    // called before a inet::Packetarriving from the network is delivered via the network
    // @brief called before a inet::Packetis delivered via the network
    virtual inet::INetfilter::IHook::Result datagramPostRoutingHook(inet::Packet* datagram);
    // @brief called before a inet::Packetarriving from the network is delivered locally
    virtual inet::INetfilter::IHook::Result datagramLocalInHook(inet::Packet* datagram);
    // @brief called before a inet::Packetarriving locally is delivered to the network
    virtual inet::INetfilter::IHook::Result datagramLocalOutHook(inet::Packet* datagram);

    // @brief reinjecting datagram means nothing at mac layer currently
    virtual void reinjectQueuedDatagram(const inet::Packet* datagram) override{};
    // @brief dropQueuedDatagram cannot drop due to forced const argument
    virtual void dropQueuedDatagram(const inet::Packet* datagram) override{};
};

} /* namespace oppostack */

#endif /* LINKLAYER_IOPPORTUNISTICLINKLAYER_H_ */
