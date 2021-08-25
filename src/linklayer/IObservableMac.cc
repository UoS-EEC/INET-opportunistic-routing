/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "IObservableMac.h"

namespace oppostack {
using namespace omnetpp;
/**
 * Mac monitoring signals
 */
simsignal_t IObservableMac::receptionStartedSignal = cComponent::registerSignal("receptionStarted");
simsignal_t IObservableMac::receptionEndedSignal = cComponent::registerSignal("receptionEnded");
simsignal_t IObservableMac::receptionDroppedSignal = cComponent::registerSignal("receptionDropped");
simsignal_t IObservableMac::transmissionStartedSignal = cComponent::registerSignal("transmissionStarted");
simsignal_t IObservableMac::transmissionEndedSignal = cComponent::registerSignal("transmissionEnded");

} /* namespace oppostack */
