/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * getCModuleFromPar modified from inet/common/ModuleAccess.h getModuleFromPar<T>
 */

#ifndef COMMON_OPPDEFS_H_
#define COMMON_OPPDEFS_H_

#include <omnetpp.h>

namespace oppostack{

omnetpp::cModule* getCModuleFromPar(omnetpp::cPar& par, const omnetpp::cModule *from, bool required = true);

} //namespace oppostack

#endif /* COMMON_OPPDEFS_H_ */
