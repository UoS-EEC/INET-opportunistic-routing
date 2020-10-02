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

#include "inet/common/ModuleAccess.h"
#include "WakeUpMacLayer.h"
using namespace inet;
using namespace physicallayer;

Define_Module(WakeUpMacLayer);

void WakeUpMacLayer::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        //Register the Wake-up radio gates
        wakeUpRadioInGateId = findGate("wakeUpRadioIn");
        wakeUpRadioOutGateId = findGate("wakeUpRadioOut");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("dataRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        dataRadio = check_and_cast<IRadio *>(radioModule);

        radioModule = getModuleFromPar<cModule>(par("wakeUpRadioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        wakeUpRadio = check_and_cast<IRadio *>(radioModule);
        wakeUpRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    }
}

void WakeUpMacLayer::handleLowerPacket(Packet *packet) {
    // Process packet from the radio
}

void WakeUpMacLayer::handleLowerCommand(cMessage *msg) {
    // Process command from the wake-up radio or delegate handler
    if (msg->getArrivalGateId() == wakeUpRadioInGateId){
        EV_DEBUG << "Received  wake-up command" << endl;
    }
    else{
        MacProtocolBase::handleLowerCommand(msg);
    }
}

void WakeUpMacLayer::receiveSignal(cComponent *source, simsignal_t signalID,
        intval_t value, cObject *details) {
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        // Handle both the data transmission ending and the wake-up transmission ending.
        // They should never happen at the same time, so one variable is enough
        // to manage both
        IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
            stepMacSM(EV_TODO, new cMessage("Transmission over"));
        }
        transmissionState = newRadioTransmissionState;
    }
}

void WakeUpMacLayer::handleUpperPacket(Packet *packet) {
}

void WakeUpMacLayer::handleSelfMessage(cMessage *msg) {
}

bool WakeUpMacLayer::isLowerMessage(cMessage *msg) {
    // Check if message comes from lower gate or wake-up radio
    return MacProtocolBase::isLowerMessage(msg)
              || msg->getArrivalGateId() == wakeUpRadioInGateId;
}

void WakeUpMacLayer::configureInterfaceEntry() {
}

void WakeUpMacLayer::stepMacSM(t_mac_event event, cMessage *msg) {
    EV_DEBUG << "Wake-up MAC received unknown event";
}

void WakeUpMacLayer::handleStartOperation(LifecycleOperation *operation) {
}

void WakeUpMacLayer::handleStopOperation(LifecycleOperation *operation) {
}

void WakeUpMacLayer::handleCrashOperation(LifecycleOperation *operation) {
}
