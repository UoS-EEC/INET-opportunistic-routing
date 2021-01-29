/*
 * Units.h
 *
 *  Created on: 18 Dec 2020
 *      Author: Edward
 */

#ifndef UNITS_H_
#define UNITS_H_

#include "inet/common/Units.h"
namespace oppostack{
    namespace units{
        struct DC; // Duty cycle
    }
    typedef inet::units::value<double, units::DC> EqDC; // Equivalent dutycycles = 1/10th of ExpectedCost
    namespace units{
        typedef inet::units::scale<DC, 10> ExpectedCost;
    }
    typedef inet::units::value<int, units::ExpectedCost> ExpectedCost;

}// namespace oppostack

namespace inet::units::internal{
    // Specialized conversion implementation for conversion
    // between integer ExpectedCost and double EqDC
    template<>
    struct convert<oppostack::units::ExpectedCost, oppostack::units::DC>
    {
        static double fn(const int& v)
        {
            return (double)v/10.0;
        }
    };
} // namespace inet::units::internal
#endif /* UNITS_H_ */
