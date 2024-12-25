package projbgddd;

import ned.IdealChannel;

@license(LGPL);

network Network
{
    @display("bgb=742,358");
    submodules:
        coordinator: Coordinator {
            @display("p=335,258");
        }
        node0: Node {                
            @display("p=145,146");
            
            // params
            ID = 0;
        }
        node1: Node {
            @display("p=514,146");
            
            // params
            ID = 1;
        }
    connections:
        node0.port <--> IdealChannel <--> node1.port;
        coordinator.p0 --> IdealChannel --> node0.coordPort;
        coordinator.p1 --> IdealChannel --> node1.coordPort;
}

