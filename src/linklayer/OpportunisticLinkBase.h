/*
 * OpportunisticLinkBase.h
 *
 *  Created on: 21 Jun 2021
 *      Author: Edward
 */

#ifndef LINKLAYER_OPPORTUNISTICLINKBASE_H_
#define LINKLAYER_OPPORTUNISTICLINKBASE_H_

#include <inet/networklayer/contract/INetfilter.h>

namespace oppostack {

// Base for reactive/opportunistic link layer protocol
// Pre-routing hook is queried with any incoming Packet to test opportunistic acceptance
// All other hooks must accept
class OpportunisticLinkBase : public inet::NetfilterBase
{
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

#endif /* LINKLAYER_OPPORTUNISTICLINKBASE_H_ */
