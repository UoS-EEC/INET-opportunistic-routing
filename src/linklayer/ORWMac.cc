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

using namespace oppostack;
using namespace inet;

void ORWMac::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        ackBackoffTimer = new cMessage("ack wait timer");
        receiveTimeout = new cMessage("wake-up wait timer");
    }
}

void ORWMac::cancelAllTimers() {
    cancelEvent(ackBackoffTimer);
    cancelEvent(receiveTimeout);
}

void ORWMac::deleteAllTimers() {
    delete ackBackoffTimer;
    delete receiveTimeout;
}

ORWMac::~ORWMac() {{
    cancelAllTimers();
    deleteAllTimers();
}
}
