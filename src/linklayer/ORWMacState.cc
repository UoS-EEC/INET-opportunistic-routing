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
#include "common/EqDCTag_m.h"

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

ORWMac::State ORWMac::stateReceiveEnter()
{
    rxAckRound = 0;
    stateReceiveEnterAck();
    return State::RECEIVE;
}

void ORWMac::stateReceiveEnterDataWait()
{
    rxState = RxState::DATA_WAIT;
    scheduleAt(simTime() + dataListeningDuration, receiveTimeout);
}

void ORWMac::stateReceiveExitDataWait()
{
    // Cancel delivery timer until next round
    cancelEvent(receiveTimeout);
}

void ORWMac::stateReceiveEnterAck()
{
    // Continue to contend for packet
    rxAckRound++;
    activeBackoff = new CSMATxRemainderReciprocalBackoff(this, dataRadio,
            ackTxWaitDuration, minimumContentionWindow);
    activeBackoff->delayCarrierSense(uniform(0, initialContentionDuration));
    rxState = RxState::ACK;
}

void ORWMac::stateReceiveExitAck()
{
    delete activeBackoff;
    activeBackoff = nullptr;
    rxState = RxState::IDLE;
}

void ORWMac::stateReceiveEnterFinishDropReceived(const inet::PacketDropReason reason)
{
    PacketDropDetails details;
    details.setReason(reason);
    dropCurrentRxFrame(details);
    stateReceiveEnterFinish();
}

void ORWMac::stateReceiveEnterFinish()
{
    // return to receive mode (via receive finish) when ack transmitted
    // For follow up packet
    rxState = RxState::FINISH;
    scheduleAt(simTime(), receiveTimeout);
}


void ORWMac::stateReceiveDataWaitProcessDataReceived(cMessage * const msg) {
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<ORWGram>();
    Packet* storedFrame = check_and_cast_nullable<Packet*>(currentRxFrame);
    if(incomingMacData->getType()==ORW_DATA && currentRxFrame == nullptr){
        stateReceiveExitDataWait();
        // Store the new received packet
        currentRxFrame = incomingFrame;

        // Check using packet data that accepting wake-up is still correct
        INetfilter::IHook::Result preRoutingResponse = datagramPreRoutingHook(incomingFrame);
        if(preRoutingResponse != IHook::ACCEPT){
            // New information in the data packet means do not accept data packet
            stateReceiveEnterFinishDropReceived(PacketDropReason::OTHER_PACKET_DROP);
        }
        else{
            //TODO: Make Opportunistic Acceptance Decision
            auto acceptThresholdTag = incomingFrame->findTag<EqDCReq>();
            EqDC newAcceptThreshold = acceptThresholdTag!=nullptr ? acceptThresholdTag->getEqDC() : EqDC(25.5);
            // If better EqDC threshold, update
            if(newAcceptThreshold < acceptDataEqDCThreshold){
                acceptDataEqDCThreshold = newAcceptThreshold;
            }

            // Limit collisions with exponentially decreasing backoff, ack takes about 1ms anyway,
            // with minimum contention window derived from the Rx->tx turnaround time
            stateReceiveEnterAck();
        }
    }
    // Compare the received data to stored data, discard it new data
    else if(incomingMacData->getType()==ORW_DATA/* && currentRxFrame != nullptr*/
            && storedFrame->peekAtFront<ORWGram>()->getTransmitterAddress() == incomingMacData->getTransmitterAddress() ){
        // Delete the existing currentRxFrame
        delete currentRxFrame;

        stateReceiveExitDataWait();
        // Store the new received packet
        currentRxFrame = incomingFrame;


        // TODO: Make Opportunistic Contention Decision
        //        Containing Make Opportunistic Acceptance Decision
        // Check if the retransmitted packet still accepted
        // Packet will change if transmitter receives ack from the final destination
        // to improve the chances that only the data destination responds.
        if(recheckDataPacketEqDC && datagramPreRoutingHook(incomingFrame) != INetfilter::IHook::ACCEPT){
            // New information in the data packet means do not accept data packet
            stateReceiveEnterFinishDropReceived(PacketDropReason::OTHER_PACKET_DROP);
        }
        else{
            // Begin random relay contention
            double relayDiceRoll = uniform(0,1);
            bool destinationAckPersistance = recheckDataPacketEqDC && (acceptDataEqDCThreshold == EqDC(0.0) );
            if(destinationAckPersistance && skipDirectTxFinalAck){
                // Received Direct Tx and so stop extra ack
                // Send immediate wuTimeout to trigger MacEvent::WU_TIMEOUT
                stateReceiveEnterFinish();
            }
            else if(destinationAckPersistance ||
                    relayDiceRoll<candiateRelayContentionProbability){
                // Continue to contend for packet
                stateReceiveEnterAck();
            }
            else{
                // Drop out of forwarder contention
                stateReceiveEnterFinishDropReceived(PacketDropReason::DUPLICATE_DETECTED);
                EV_DEBUG  << "Detected other relay so discarding packet" << endl;
            }
        }
    }
    else{
        // Delete unknown message
        EV_DEBUG << "Discard interfering data transmission" << endl;
        delete incomingFrame;
    }
}

void ORWMac::stateReceiveAckProcessDataReceived(cMessage* msg)
{
    Packet* incomingFrame = check_and_cast<Packet*>(msg);
    auto incomingMacData = incomingFrame->peekAtFront<ORWGram>();
    Packet* storedFrame = check_and_cast_nullable<Packet*>(currentRxFrame);
    if(incomingMacData->getType()==ORW_DATA/* && currentRxFrame != nullptr*/
            && storedFrame->peekAtFront<ORWGram>()->getTransmitterAddress() == incomingMacData->getTransmitterAddress() ){
        stateReceiveExitAck();
        stateReceiveEnterFinishDropReceived(PacketDropReason::DUPLICATE_DETECTED);
    }
    delete msg;
}
