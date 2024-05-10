// designate the NED package of this folder
package test.simulations;

import inet.node.leachNode.leachNode;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.apsk.packetlevel.ApskScalarRadioMedium;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.visualizer.common.IntegratedMultiVisualizer;
//
// TODO documentation
//
network test
{
    parameters:
        int numNodes;
        @display("bgb=400,400");
    submodules:
        configurator: Ipv4NetworkConfigurator { // network autoconfiguration
            @display("p=30.000002,69.230774");
        }
        radioMedium: ApskScalarRadioMedium {
            @display("p=30.000002,109.230774");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=30.000002,22.307693");
        }
        leachNode[numNodes]: leachNode;
        BaseStation: leachNode {
            @display("p=200,200;i=misc/houses");
        }
        
    connections:
}
@license(LGPL);
