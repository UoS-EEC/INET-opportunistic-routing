/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include <inet/networklayer/nexthop/NextHopInterfaceData.h>
#include <inet/common/ModuleAccess.h>

#include "RoutingTableBase.h"
#include "linklayer/ILinkOverhearingSource.h"

using namespace omnetpp;
using namespace inet;
using namespace oppostack;

void RoutingTableBase::initialize(int stage) {
    if(stage == INITSTAGE_LOCAL){interfaceTable = inet::getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        const char *addressTypeString = par("addressType");
        if (!strcmp(addressTypeString, "mac"))
            addressType = L3Address::MAC;
        else if (!strcmp(addressTypeString, "modulepath"))
            addressType = L3Address::MODULEPATH;
        else if (!strcmp(addressTypeString, "moduleid"))
            addressType = L3Address::MODULEID;
        else
            throw cRuntimeError("Unknown address type");

        forwardingCostW = EqDC(par("forwardingCost"));
    }
    else if(stage == INITSTAGE_LINK_LAYER){
        for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i){
            auto interfaceI = interfaceTable->getInterface(i);
            configureInterface(interfaceI);
            INetfilter* wakeUpMacFilter = dynamic_cast<INetfilter*>(interfaceI->getSubmodule("mac"));
            if(wakeUpMacFilter)wakeUpMacFilter->registerHook(0, this);
        }
        if(netfilters.empty()){
            throw cRuntimeError("No suitable Wake Up Mac found under: %s.mac", this->getFullPath().c_str());
        }
    }
}

void RoutingTableBase::configureInterface(InterfaceEntry *ie) {
    int interfaceModuleId = ie->getId();
    // mac
    NextHopInterfaceData *d = ie->addProtocolData<NextHopInterfaceData>();
    d->setMetric(1);
    if (addressType == L3Address::MAC)
        d->setAddress(ie->getMacAddress());
    else if (ie && addressType == L3Address::MODULEPATH)
        d->setAddress(ModulePathAddress(interfaceModuleId));
    else if (ie && addressType == L3Address::MODULEID)
        d->setAddress(ModuleIdAddress(interfaceModuleId));
}

L3Address RoutingTableBase::getRouterIdAsGeneric() {
    Enter_Method_Silent("RoutingTableBase::getRouterIdAsGeneric()");
    return interfaceTable->findFirstNonLoopbackInterface()->getNetworkAddress();
}

EqDC RoutingTableBase::calculateUpwardsCost(const L3Address destination, EqDC& nextHopEqDC) const
{
    Enter_Method("RoutingTableBase::calculateUpwardsCost(address, ..)");

    const EqDC estimatedCost = calculateUpwardsCost(destination);
    nextHopEqDC = EqDC(25.5);
    // Limit resolution and add own routing cost before reporting.
    if(estimatedCost < EqDC(25.5)){
        nextHopEqDC = ExpectedCost(estimatedCost - forwardingCostW);
    }
    return estimatedCost;
}
