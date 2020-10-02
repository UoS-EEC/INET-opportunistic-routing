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

package wakeupmac.simulations;

import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import wakeupmac.WakeUpLinkLayerNode;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;


network WakeUpLinkTest
{
    parameters:
        visualizer.mediumVisualizer.displayCommunicationRanges = true;
        @display("bgb=443,341");
    submodules:
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=61,111");
        }
        receivingWakeUpNode: WakeUpLinkLayerNode {
            @display("p=146,57");
        }
        transmittingNode: AdhocHost {
            @display("p=150,147");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=61,186");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=61,250");
        }
}
