/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#ifndef NETWORKLAYER_ROUTINGTABLEBASE_H_
#define NETWORKLAYER_ROUTINGTABLEBASE_H_

#include <inet/networklayer/contract/INetfilter.h>
#include <inet/networklayer/contract/IInterfaceTable.h>
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>

#include "common/Units.h"

namespace oppostack {

class RoutingTableBase: public omnetpp::cSimpleModule,
        public inet::NetfilterBase::HookBase {
protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    inet::IInterfaceTable *interfaceTable{nullptr};
    inet::L3Address::AddressType addressType{inet::L3Address::AddressType::NONE};

    oppostack::EqDC forwardingCostW = oppostack::EqDC(0.1);

    void configureInterface(inet::NetworkInterface *ie);
    virtual inet::Hz estAdvertismentRate();

public:
    inet::L3Address getRouterIdAsGeneric();

    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination, oppostack::EqDC& nextHopEqDC) const;
    virtual oppostack::EqDC calculateUpwardsCost(const inet::L3Address destination) const = 0;
    virtual EqDC estimateEqDC(const inet::Hz expectedLoad, const inet::unit hopsToSink);

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
