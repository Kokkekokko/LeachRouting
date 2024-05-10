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

package inet.node.leachNode;

import inet.node.inet.AdhocHost;
import inet.routing.contract.IManetRouting;


module leachNode extends AdhocHost
{
    parameters:
        @display("i=misc/sensor2");
        @figure[submodules];
        numWlanInterfaces = default(1);
        energyStorage.typename = default("IdealEpEnergyStorage");
        wlan[*].typename = default("Ieee802154NarrowbandInterface");
        wlan[*].radio.energyConsumer.typename = default("SensorStateBasedEpEnergyConsumer");
    submodules:
        routing: <default("")> like IManetRouting if typename != ""{
            @display("p=1043.8909,206.69386");
        }
    connections:
        routing.ipOut --> tn.in++ if exists(routing);
        routing.ipIn <-- tn.out++ if exists(routing);
}
