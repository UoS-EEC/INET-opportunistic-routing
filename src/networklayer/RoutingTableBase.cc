/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include <inet/networklayer/nexthop/NextHopInterfaceData.h>
#include <inet/common/ModuleAccess.h>
#include <inet/power/management/SimpleEpEnergyManagement.h>
#include <inet/power/contract/IEpEnergyGenerator.h>
#include <inet/physicallayer/wireless/common/energyconsumer/StateBasedEpEnergyConsumer.h>
#include <math.h>

#include "RoutingTableBase.h"
#include "linklayer/ORWMacInterface.h"
#include "linklayer/ILinkOverhearingSource.h"

using namespace omnetpp;
using namespace inet;
using namespace oppostack;
using namespace inet::power;
using namespace inet::physicallayer;

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

void RoutingTableBase::configureInterface(NetworkInterface *ie) {
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

Hz RoutingTableBase::estAdvertismentRate() {
    return Hz{0};
}

EqDC RoutingTableBase::estimateEqDC(const Hz expectedLoad, const unit hopsToSink){
    auto energyManager = check_and_cast<SimpleEpEnergyManagement*>(getModuleByPath("^.^.energyManagement"));
    auto energyGenerator = check_and_cast<IEpEnergyGenerator*>(getModuleByPath("^.^.energyGenerator"));
    auto P_EH = energyGenerator->getPowerGeneration();
    const StateBasedEpEnergyConsumer* radioConsumer{nullptr};
    for (int j = 0; j < interfaceTable->getNumInterfaces(); j++) {
        const auto interfaceJ = interfaceTable->getInterface(j);
        if (interfaceJ->isWireless()
                && dynamic_cast<const ORWMacInterface*>(interfaceJ)) {
            auto radio = check_and_cast<const cModule*>(dynamic_cast<const ORWMacInterface*>(interfaceJ)->getInitiationRadio());
            radioConsumer = check_and_cast<const StateBasedEpEnergyConsumer*>(
                    radio->getModuleByPath(".energyConsumer")
                );
        }
    }

    const W P_Li{radioConsumer->par("receiverIdlePowerConsumption")};
    const W P_Tx{radioConsumer->par("transmitterTransmittingPowerConsumption")};
    const J ETx = P_Tx*b(8*8)/kbps{50};
    const J Ebudget = J(energyManager->par("nodeStartCapacity")) - J(energyManager->par("nodeShutdownCapacity"));
    ASSERT(Ebudget > J(1e-6) && Ebudget < J(1.0));
    const int iterationsLimit = 100;
    const unit deviationMin{0.01};
    Hz advRate = estAdvertismentRate();
    s TOn{100};
    const s TOff = Ebudget/P_EH;
    unit deviation{1.0};
    for(int i=0; i<iterationsLimit; i++){
        const J TOnNumerator{Ebudget + (P_EH-P_Li)*TOn};
        const W TOnDenominator{(Hz(0.05) + expectedLoad + advRate)*ETx};
        const s computedTOn{TOnNumerator/TOnDenominator};
        const s nextTOn{ computedTOn<s(1000)? computedTOn : s(1000) };
        const unit nextDeviation{ std::abs(unit((nextTOn-TOn)/TOn).get()) };
        if(nextDeviation < deviationMin && deviation < deviationMin){
            deviation = nextDeviation;
            TOn = nextTOn;
            EV_DEBUG << "TOnEstimate converged in " << i << "iterations" << endl;
            break;
        }
        deviation = nextDeviation;
        TOn = nextTOn;
    }
    const unit computedDC = TOn/(TOn+TOff);
    const unit DC_est = computedDC<unit(1)? computedDC : unit(1);
    const ExpectedCost EqDC_initial{ 100.0*sqrt( hopsToSink.get() )*( 1.0 + 0.05/std::sqrt(DC_est.get()) )-60.0 };
    cPar& expectedCostPar = this->par("hubExpectedCost");
    const cValue newExpectedCost = cValue(EqDC_initial.get(), "ExpectedCost");
    expectedCostPar.setValue(newExpectedCost);
    return EqDC_initial;
}
