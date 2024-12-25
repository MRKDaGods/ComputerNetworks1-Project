#include "Coordinator.h"
#include "Common.h"
#include "SysLogger.h"

#include <fstream>
#include <string>

Define_Module(Coordinator);

void Coordinator::initialize()
{
    EV << "Initializing coordinator, CWD = " << getcwd(0, 0) << endl;

    // delete logs
    SysDeleteLogs();

    // read config
    _STD ifstream config("coordinator.txt");
    if (!config.is_open())
    {
        EV << "Failed to open coordinator.txt" << endl;
        return;
    }

    // params
    int nodeId;
    double startTime;

    config >> nodeId >> startTime;

    EV << "[Config] Node ID: " << nodeId
       << ", Start time: " << startTime << endl;

    config.close();

    // schedule start time
    EV << "Sending start message to node " << nodeId << " at time " << startTime << endl;

    // p0 or p1
    char outPort[3];
    sprintf(outPort, "p%d", nodeId);

    // send start message
    auto startMsg = new cMessage("start");
    startMsg->setKind(MSG_KIND_START);
    sendDelayed(startMsg, startTime, outPort);

    EV << "Coordinator initialized, sent start message to port " << outPort << endl;
}

void Coordinator::handleMessage(cMessage *msg)
{
    delete msg;
}
