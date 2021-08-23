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
#include "ORWMac.h"
#include <inet/physicallayer/contract/packetlevel/IRadio.h>

using namespace inet;
using physicallayer::IRadio;
using namespace oppostack;

ORWMac::State ORWMac::stateListeningEnterAlreadyListening(){
    if(currentTxFrame || not txQueue->isEmpty()){
        // Data is waiting in the tx queue
        // Schedule replenishment timer if insufficient stored energy
        if(!transmissionStartEnergyCheck())
            scheduleAt(simTime() + replenishmentCheckRate, replenishmentTimer);
        return State::AWAIT_TRANSMIT;
    }
    else{
        return State::DATA_IDLE;
    }
}


ORWMac::State ORWMac::stateListeningEnter(){
    dataRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    return stateListeningEnterAlreadyListening();
}
