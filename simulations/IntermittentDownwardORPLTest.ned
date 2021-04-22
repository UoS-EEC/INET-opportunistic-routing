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

package oppostack.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import oppostack.node.ORPLLayerNode;
import oppostack.node.IntermittentORPLNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;

network IntermittentDownwardORPLTest extends IntermittentDenseORPLTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        radioMedium.mediumLimitCache.maxTransmissionDuration = 20ms;
        @display("bgb=78.790436,65.78085;scale=4");
    submodules:
        branched1: IntermittentORPLNode {
            @display("p=48.14748,32.471558");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched2: IntermittentORPLNode {
            @display("p=49.76484,36.950394");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched3: IntermittentORPLNode {
            @display("p=54.990147,33.591267");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched4: IntermittentORPLNode {
            @display("p=57.976036,39.0654");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        branched5: IntermittentORPLNode {
            @display("p=61.335163,35.333035");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 150ExpectedCost;
            packetGenerator.destAddresses = "";
            packetGenerator.sendInterval = 5s;
        }
        transmittingCrossBranch: IntermittentORPLNode {
            @display("p=62.952522,40.682755");
            generic.*.hubAddress = "receivingWakeUpNode(modulepath)";
            generic.*.hubExpectedCost = 200ExpectedCost;
            packetGenerator.destAddresses = "node5(modulepath) receivingWakeUpNode(modulepath)";
            packetGenerator.startTime = 0.5s;
            packetGenerator.sendInterval = 40s;
            energyGenerator.powerGeneration = 40uW;
        }
}
