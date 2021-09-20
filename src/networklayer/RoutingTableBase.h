/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_ROUTINGTABLEBASE_H_
#define NETWORKLAYER_ROUTINGTABLEBASE_H_

#include <inet/networklayer/contract/INetfilter.h>
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>

#include "common/Units.h"

namespace oppostack {

class RoutingTableBase: public omnetpp::cSimpleModule,
        public omnetpp::cListener,
        public inet::NetfilterBase::HookBase {
public:
    virtual void initialize(int stage) override;
protected:
    inet::IInterfaceTable *interfaceTable{nullptr};
    inet::L3Address::AddressType addressType{inet::L3Address::AddressType::NONE};

    oppostack::EqDC forwardingCostW = oppostack::EqDC(0.1);

    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, double weight, cObject *details) override = 0;
    void configureInterface(inet::InterfaceEntry *ie);
public:
    inet::L3Address getRouterIdAsGeneric();
    // Hook to accept incoming requests
    using inet::NetfilterBase::HookBase::datagramPreRoutingHook;
//    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet*) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramPostRoutingHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramLocalInHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    virtual inet::INetfilter::IHook::Result datagramLocalOutHook(inet::Packet *datagram) override{return IHook::Result::ACCEPT;};
    EqDC getForwardingCost(){return forwardingCostW;}
};

} /* namespace oppostack */

#endif /* NETWORKLAYER_ROUTINGTABLEBASE_H_ */
