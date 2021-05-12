//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
cplusplus {{
#include "Units.h"
#include "inet/common/Units.h"
}}

namespace oppostack;

class ExpectedCost
{
    @existingClass;
    @opaque;
    @toString(.str());
}
class EqDC
{
    @existingClass;
    @opaque;
    @toString(.str());
}

class inet::J    { @existingClass; @primitive; @fromString(inet::J(string2double($)));    @toString(unit2string($)); @defaultValue(inet::J(NaN)); }
