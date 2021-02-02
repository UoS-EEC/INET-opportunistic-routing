/*
 * oppDefs.cc
 *
 * getCModuleFromPar modified from inet/common/ModuleAccess.h getModuleFromPar<T>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
 */


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

