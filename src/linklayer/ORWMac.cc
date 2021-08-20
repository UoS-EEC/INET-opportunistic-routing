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
#include <inet/physicallayer/base/packetlevel/FlatTransmitterBase.h>
#include <inet/physicallayer/contract/packetlevel/IRadioMedium.h>
#include <inet/physicallayer/backgroundnoise/IsotropicScalarBackgroundNoise.h>
#include <inet/physicallayer/base/packetlevel/FlatReceiverBase.h>
#include <inet/common/ModuleAccess.h>

using namespace oppostack;
using namespace inet;
using inet::physicallayer::IRadio;
using physicallayer::FlatTransmitterBase;

Define_Module(ORWMac);

void ORWMac::initialize(int stage) {
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // create timer messages
        ackBackoffTimer = new cMessage("ack wait timer");
        receiveTimeout = new cMessage("wake-up wait timer");

        //load parameters
        transmissionStartMinEnergy = J(par("transmissionStartMinEnergy"));
        dataListeningDuration = par("dataListeningDuration");
        ackWaitDuration = par("ackWaitDuration");
        initialContentionDuration = ackWaitDuration/3;

        // link direct module interfaces
        const char* energyStoragePath = par("energyStorage");
        cModule* storageModule = getModuleByPath(energyStoragePath);
        energyStorage = check_and_cast<inet::power::IEpEnergyStorage*>(storageModule);

        const char *radioModulePath = par("dataRadioModule");
        cModule *radioModule = getModuleByPath(radioModulePath);
        dataRadio = check_and_cast<IRadio *>(radioModule);

        // Parameter validation

        /*
         * Warning of noise in radio medium exceeding energy detection threshold
         */
        auto medium = getModuleFromPar<physicallayer::IRadioMedium>(radioModule->par("radioMediumModule"), radioModule);
        auto noiseModel = medium->getBackgroundNoise();
        auto scalarNoiseModel = check_and_cast_nullable<const physicallayer::IsotropicScalarBackgroundNoise*>(noiseModel);
        auto dataReceiverModel = check_and_cast_nullable<const physicallayer::FlatReceiverBase*>(dataRadio->getReceiver());
        if(scalarNoiseModel && dataReceiverModel)
            if(scalarNoiseModel->getPower() > dataReceiverModel->getEnergyDetection())
                throw cRuntimeError("Background noise power is greater than data radio energy detection threshold. Radio will always be \"Busy\"");

        /*
         * Calculation of invalid parameter combinations at the receiver
         * - MAC Layer requires tight timing during a communication negotiation
         * - Incorrect timing increases the collision and duplication probability
         * After the wake-up contention to receive and forward occurs
         * The physical MTU is limited by the time spent dataListening after sending an ACK
         * If an ACK is sent at the start of the ACK period, the data may not be fully
         * received until remainder of ack period + 1/5(ack period)
         */
        auto lengthPrototype = makeShared<WakeUpDatagram>();
        const b ackBits = b(lengthPrototype->getChunkLength());
        auto dataTransmitter = check_and_cast<const FlatTransmitterBase *>(dataRadio->getTransmitter());
        const bps bitrate = dataTransmitter->getBitrate();
        const int maxAckCount = std::floor(ackWaitDuration.dbl()*bitrate.get()/ackBits.get());
        ASSERT2(maxAckCount > requiredForwarders + 2, "Ack wait duration is too small for multiple forwarders.");
        ASSERT(maxAckCount < 20);
        const double remainingAckProportion = (double)(maxAckCount-1)/(double)(maxAckCount);
        ASSERT(remainingAckProportion < 1 && remainingAckProportion > 0);
        // 0.2*ackWaitDuration currently Hardcoded into TX_DATA state
        const b phyMaxBits = b( std::floor( (dataListeningDuration.dbl() - (remainingAckProportion+0.2)*ackWaitDuration.dbl())*bitrate.get() ) );
        phyMtu = B(phyMaxBits.get()/8); // Integer division implicitly (and correctly) rounds down

        /*
         * Calculation of Ack contention parameters
         * - The entire ack message must be sent within the ackWaitDuration.
         *   So must start by ackWaitDuration - ackDuration
         * - If the ack wait duration is too close to the "turnaround" (Rx->Tx)
         *   of the data radio, then collision probability is high,
         *   for 4 responding nodes, expect 2 must be uninterrupted
         *   So collision prob must be under 25%
         * - If the radio is still contending for an ack near the end of the
         *   ackWaitDuration, set a minimumContentionWindow so contention
         *   probability is < 50%
         */
        const simtime_t turnaroundTime = par("radioTurnaroundTime");
        const simtime_t ackDuration = SimTime(ackBits.get()/bitrate.get());
        ackTxWaitDuration = ackWaitDuration - ackDuration;
        const double initialCollisionProbability = 1 - std::exp(-4.0*turnaroundTime.dbl()/initialContentionDuration.dbl());
        minimumContentionWindow = 2.0/std::log(2)*turnaroundTime;
        ASSERT(initialContentionDuration > minimumContentionWindow);
        ASSERT(initialCollisionProbability < 0.25);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // Register signals for handling radio mode changes
        cModule* const dataCMod = check_and_cast<cModule*>(dataRadio);
        dataCMod->subscribe(IRadio::radioModeChangedSignal, this);
        dataCMod->subscribe(IRadio::transmissionStateChangedSignal, this);
        dataCMod->subscribe(IRadio::receptionStateChangedSignal, this);

        // Initial state handled by handleStartOperation()

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
