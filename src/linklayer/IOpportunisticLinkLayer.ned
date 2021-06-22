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

moduleinterface IOpportunisticLinkLayer
{
    parameters:
        // Weighted Encounters signals and with attached L2Addr and EDC/EquivalentTx(EqTx) details
        @signal[coincidentalEncounter](type=double);
        @signal[expectedEncounter](type=double);
        @signal[listenForEncountersEnded](type=double);
        @statistic[coincidentalEncounter](title="Weighted coincidental neighbor encounter"; record=vector,count);
        @statistic[expectedEncounter](title="Weighted expected neighbor encounter"; record=vector,count);
        @statistic[listenForEncountersEnded](title="Ended expected encounters listening period"; record=vector,count);
}