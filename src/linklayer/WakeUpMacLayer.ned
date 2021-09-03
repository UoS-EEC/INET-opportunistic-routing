// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;

// Communicates using two radios (IRadio), one for wake-up one for data. 
// Two radio model is an abstraction of using wake-up receiver and data radio to 
// send the wake-up. 
// 
// Adds a wake-up radio to the ORWMac to wake-up potential receivers
// wake-up only sent once for each ack negotiation period

module WakeUpMacLayer extends ORWMac
{
    parameters:
        @class(WakeUpMacLayer);
        // Assumes MAC layer is within an interface
        string wakeUpRadioModule = default("^.wakeUpRadio");   // The path to the Wake up radio
        double txWakeUpWaitDuration @unit(s) = default(0.0006 s); // Must be bigger than radio OFF -> idle
        double wuApproveResponseLimit @unit(s) = default(1000ns);

        // After receiving wake-up, check the data before sending ack and accepting
        bool checkDataPacketEqDC = default(true);
        // If transmitter receives ack from the final dest, and checkDataPacketEqDC is
        // performed then don't resend data, as only the final dest will accept EqDC=0
        bool skipDirectTxFinalAck = default(false);
    gates:
        input wakeUpRadioIn;
        output wakeUpRadioOut;
    submodules:
        monitor: MacEnergyMonitor {
            @display("p=100,200");
        }

}
