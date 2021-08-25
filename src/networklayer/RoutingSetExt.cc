/* Copyright (c) 2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later */

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
        throw omnetpp::cRuntimeError("Higher proportions should be handled by inverting bitmap");
    }
}
