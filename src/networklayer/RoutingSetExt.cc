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

#include "RoutingSetExt_m.h"
#include <cstdlib>
using namespace oppostack;

short RoutingSetExt::getLength() const{
    const int minNodeCount = 0;
    const int maxNodeCount = 512;
    const int halfRange = (maxNodeCount - minNodeCount)/2 + minNodeCount;
    const int entriesLength = getEntryArraySize();
    const int entriesLengthDistFromMaxMin = halfRange - std::abs(entriesLength - halfRange);

    //Aproximation of compression possible using Huffman Coding on statistically sparse bit array
    if(entriesLength < 0.0325*maxNodeCount){
        // 15% compression ratio
        return std::ceil(maxNodeCount/8*0.15);
    }
    else if(entriesLength < 0.065*maxNodeCount){
        // 20% compression ratio
        return std::ceil(maxNodeCount/8*0.2);
    }
    else if(entriesLength < 0.25*maxNodeCount){
        // 50% compression ratio
        return std::ceil(maxNodeCount/8*0.5);
    }
    else if(entriesLength <= 0.5*maxNodeCount){
        // Do not compress at all
        return std::ceil(maxNodeCount/8*1.0);
    }
    else{
        omnetpp::cRuntimeError("Higher proportions should be handled by inverting bitmap");
    }
}
