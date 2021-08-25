/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 * getCModuleFromPar modified from inet/common/ModuleAccess.h getModuleFromPar<T>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */


#include "oppDefs.h"
omnetpp::cModule *oppostack::getCModuleFromPar(omnetpp::cPar& par, const omnetpp::cModule *from, bool required)
{
    const char *path = par;
    omnetpp::cModule *mod = from->getModuleByPath(path);
    if (!mod) {
        if (required)
            throw omnetpp::cRuntimeError("Module not found on path '%s' defined by par '%s'", path, par.getFullPath().c_str());
        else
            return nullptr;
    }
    return mod;
}

