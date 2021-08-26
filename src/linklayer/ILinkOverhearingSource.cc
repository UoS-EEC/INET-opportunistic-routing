/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

#include "ILinkOverhearingSource.h"

namespace oppostack {
using namespace omnetpp;
/**
 * Neighbor Update signals
 * Sent when information overheard from neighbors
 */
simsignal_t ILinkOverhearingSource::expectedEncounterSignal = cComponent::registerSignal("expectedEncounter");
simsignal_t ILinkOverhearingSource::coincidentalEncounterSignal = cComponent::registerSignal("coincidentalEncounter");
simsignal_t ILinkOverhearingSource::listenForEncountersEndedSignal = cComponent::registerSignal("listenForEncountersEnded");

} /* namespace oppostack */
