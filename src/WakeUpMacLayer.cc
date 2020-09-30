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

#include "WakeUpMacLayer.h"

Define_Module(WakeUpMacLayer);

void WakeUpMacLayer::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        //Register the Wake-up radio gates
        wakeUpRadioInGateId = findGate("wakeUpRadioIn");
        wakeUpRadioOutGateId = findGate("wakeUpRadioOut");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {

    }
}

void WakeUpMacLayer::handleLowerPacket(Packet *packet) {
    // Process packet from the radio
}

void WakeUpMacLayer::handleLowerCommand(cMessage *msg) {
    // Process command from the radio (or wake-up radio)
}

void WakeUpMacLayer::isLowerMessage(cMessage *msg) {
    // Check if message comes from lower gate or wake-up radio
    return MacProtocolMessage::isLowerMessage(cMessage *msg)
              || message->getArrivalGateId() == wakeUpRadioInGateId;
}
