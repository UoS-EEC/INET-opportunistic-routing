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

//
// TODO auto-generated type
//
module ORWMac extends MacProtocolBase like IMacProtocol, IOpportunisticLinkLayer
{
    parameters:
        @class(ORWMac);
        // Assumes MAC layer is within an interface
        string dataRadioModule = default("^.dataRadio");   // The path to the Radio module
        string energyStorage = default("^.^.energyStorage");
        
        double transmissionStartMinEnergy @unit(J) = 0.004345J; // Don't start transmitting unless there is at least this in storage
        double radioTurnaroundTime @unit(s) = default(50us);
        double dataListeningDuration @unit(s) = default(0.0085 s); // How long to listen after a wake-up
        double ackWaitDuration @unit(s) = default(0.0024 s); // Must be bigger than radio Rx -> Tx
        
}
