package projbgddd;

simple Node
{
    parameters:
        int ID = default(0);
        int WS = default(5);
        double TO = default(10.0);
        double PT = default(0.5);
        double TD = default(1.0);
        double ED = default(4.0);
        double DD = default(0.1);
        double LP = default(0.1);

        // loss probability prediction
        volatile double LPPred = uniform(0, 1);

        // normal predictor
        volatile double BasePred = uniform(0, 1);
        
    gates:
        input coordPort;
    	inout port;
}
