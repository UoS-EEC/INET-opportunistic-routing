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

Define_Module(ORWMac);

void ORWMac::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // create timer messages
        ackBackoffTimer = new cMessage("ack wait timer");
        receiveTimeout = new cMessage("wake-up wait timer");

        //load parameters
        transmissionStartMinEnergy = J(par("transmissionStartMinEnergy"));

        // link surrounding modules
        const char* energyStoragePath = par("energyStorage");
        cModule* storageModule = getModuleByPath(energyStoragePath);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);


        // Verification
        phyMtu = B(0); // TODO: verify using dataRadio datarate and mac durations
    }
}

void ORWMac::configureInterfaceEntry() {
    // generate a link-layer address to be used as interface token for IPv6
    auto lengthPrototype = makeShared<WakeUpDatagram>();
    const B interfaceMtu = phyMtu-B(lengthPrototype->getChunkLength());
    ASSERT2(interfaceMtu >= B(80), "The interface MTU available to the net layer is too small (under 80 bytes)");
    interfaceEntry->setMtu(interfaceMtu.get());
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setPointToPoint(false);
}

void ORWMac::cancelAllTimers() {
    cancelEvent(ackBackoffTimer);
    cancelEvent(receiveTimeout);
}

void ORWMac::deleteAllTimers() {
    delete ackBackoffTimer;
    delete receiveTimeout;
}

ORWMac::~ORWMac() {
    cancelAllTimers();
    deleteAllTimers();
}

bool ORWMac::transmissionStartEnergyCheck() const
{
    return energyStorage->getResidualEnergyCapacity() >= transmissionStartMinEnergy;
}

void ORWMac::setupTransmission() {
    //Cancel transmission timers
    //Reset progress counters
    txInProgressForwarders = 0;

    if(currentTxFrame!=nullptr){
        PacketDropDetails details;
        details.setReason(PacketDropReason::QUEUE_OVERFLOW);
        dropCurrentTxFrame(details);
    }
    popTxQueue();
    if(datagramLocalOutHook(currentTxFrame)!=INetfilter::IHook::Result::ACCEPT){
        throw cRuntimeError("Unhandled rejection of packet at transmission setup");
    }
}
