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

        // After receiving wake-up, recheck the data before sending ack and accepting
        bool recheckDataPacketEqDC = default(true);
        // If transmitter receives ack from the final dest, and recheckDataPacketEqDC is
        // performed then don't resend data, as only the final dest will accept EqDC=0
        bool skipDirectTxFinalAck = default(false);
    gates:
        input wakeUpRadioIn;
        output wakeUpRadioOut;
    submodules:
        monitor: WuMacEnergyMonitor {
            @display("p=100,200");
        }

}
