/*
 * OpportunisticLinkBase.cpp
 *
 *  Created on: 21 Jun 2021
 *      Author: Edward
 */

#include "IOpportunisticLinkLayer.h"

namespace oppostack {
using namespace inet;

/**
 * Neighbor Update signals
 * Sent when information overheard from neighbors
 */
simsignal_t IOpportunisticLinkLayer::expectedEncounterSignal = cComponent::registerSignal("expectedEncounter");
simsignal_t IOpportunisticLinkLayer::coincidentalEncounterSignal = cComponent::registerSignal("coincidentalEncounter");
simsignal_t IOpportunisticLinkLayer::listenForEncountersEndedSignal = cComponent::registerSignal("listenForEncountersEnded");

INetfilter::IHook::Result IOpportunisticLinkLayer::datagramPreRoutingHook(Packet *datagram)
{
    auto ret = INetfilter::IHook::Result::DROP;
    for (auto & elem : hooks) {
        INetfilter::IHook::Result r = elem.second->datagramPreRoutingHook(datagram);
        PacketDropDetails details;
        switch (r) {
            case INetfilter::IHook::ACCEPT:
                ret = r;
                break;    // continue iteration
            case INetfilter::IHook::DROP:
                return r;
            case INetfilter::IHook::QUEUE:
            case INetfilter::IHook::STOLEN:
            default:
                throw cRuntimeError("Unimplemented Hook::Result value: %d", (int)r);
        }
    }
    return ret;
}

INetfilter::IHook::Result IOpportunisticLinkLayer::datagramPostRoutingHook(Packet *datagram)
{
    for (auto & elem : hooks) {
        INetfilter::IHook::Result r = elem.second->datagramPostRoutingHook(datagram);
        switch (r) {
            case INetfilter::IHook::ACCEPT:
                break;    // continue iteration
            case INetfilter::IHook::DROP:
            case INetfilter::IHook::QUEUE:
            case INetfilter::IHook::STOLEN:
            default:
                throw cRuntimeError("Unimplemented Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result IOpportunisticLinkLayer::datagramLocalInHook(Packet *datagram)
{
    L3Address address;
    for (auto & elem : hooks) {
        INetfilter::IHook::Result r = elem.second->datagramLocalInHook(datagram);
        switch (r) {
            case INetfilter::IHook::ACCEPT:
                break;    // continue iteration
            case INetfilter::IHook::DROP:
            case INetfilter::IHook::QUEUE:
            case INetfilter::IHook::STOLEN:
            default:
                throw cRuntimeError("Unimplemented Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result IOpportunisticLinkLayer::datagramLocalOutHook(Packet *datagram)
{
    for (auto & elem : hooks) {
        INetfilter::IHook::Result r = elem.second->datagramLocalOutHook(datagram);
        switch (r) {
            case INetfilter::IHook::ACCEPT:
                break;    // continue iteration
            case INetfilter::IHook::DROP:
            case INetfilter::IHook::QUEUE:
            case INetfilter::IHook::STOLEN:
            default:
                throw cRuntimeError("Unimplemented Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}


} /* namespace oppostack */
