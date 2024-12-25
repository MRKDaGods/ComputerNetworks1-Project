#include "Node.h"
#include "NetSender.h"
#include "NetReceiver.h"

#include <fstream>

Define_Module(Node);

Node::Node()
{
    m_NetEntity = 0;
}

Node::~Node()
{
    for (auto &msg : m_Messages)
    {
        delete msg;
    }
}

void Node::ReadParams()
{
    NODE_LOG("Reading params");

    m_Params.windowSize = par(PARAM_WINDOW_SIZE).intValue();
    m_Params.timeoutInterval = par(PARAM_TIMEOUT).doubleValue();
    m_Params.processingTime = par(PARAM_PROCESSING_TIME).doubleValue();
    m_Params.transmissionDelay = par(PARAM_TRANSMISSION_DELAY).doubleValue();
    m_Params.errorDelay = par(PARAM_ERROR_DELAY).doubleValue();
    m_Params.duplicationDelay = par(PARAM_DUPLICATION_DELAY).doubleValue();
    m_Params.lossRate = par(PARAM_LOSS_RATE).doubleValue();

    NODE_LOG("Read params: WS=%d, TO=%f, PT=%f, TD=%f, ED=%f, DD=%f, LP=%f",
             m_Params.windowSize,
             m_Params.timeoutInterval,
             m_Params.processingTime,
             m_Params.transmissionDelay,
             m_Params.errorDelay,
             m_Params.duplicationDelay,
             m_Params.lossRate);
}

bool Node::InitializeMessages()
{
    NODE_LOG("Initializing messages");

    // inputX.txt
    char inputFilename[11];
    sprintf(inputFilename, "input%d.txt", m_NodeId);

    NODE_LOG("Reading messages from %s", inputFilename);

    _STD ifstream input(inputFilename);
    if (!input.is_open())
    {
        EV << "Failed to open " << inputFilename << endl;
        return false;
    }

    // read messages
    int msgId = 0;

    _STD string line;
    while (getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }

        _STD stringstream ss(line);

        // flags: XXXX
        char flags[5];
        ss >> flags;

        // read message
        auto message = line.substr((uint32_t)ss.tellg() + 1);
        
        // alloc data
        auto data = new NodeMessageData;
        data->message = message;
        data->id = msgId++;
        data->flags = {flags[0] == '1', flags[1] == '1', flags[2] == '1', flags[3] == '1'};

        m_Messages.push_back(data);
    }

    NODE_LOG("Read %d messages", m_Messages.size());

    input.close();
    return true;
}

void Node::initialize()
{
    // get node id
    m_NodeId = par("ID").intValue();
    NODE_LOG("Initializing");

    // read params
    ReadParams();

    // init messages
    InitializeMessages();
}

void Node::handleMessage(cMessage *msg)
{
    switch (msg->getKind())
    {
    case MSG_KIND_START:
        // check if message was sent by coord
        NODE_LOG("Received start message, initializing net entity as sender");

        // init net entity
        m_NetEntity = new NetSender(this);
        return;

    case MSG_KIND_PACKET:
        // if we received a packet and our NetEntity is still not initialized
        // that means we are a receiver
        if (m_NetEntity == 0)
        {
            NODE_LOG("Received packet, initializing net entity as receiver");

            // init net entity
            m_NetEntity = new NetReceiver(this);
        }

        // now forward the packet to the NetEntity
        m_NetEntity->ReceivePacket((Packet *)msg);
        break;

    case MSG_KIND_TIMER:
        // forward timer event to NetEntity
        m_NetEntity->ReceiveTimerEvent((NodeMessageData*)msg->getContextPointer());
        break;

    case MSG_KIND_SCHEDULED:
        // execute post-processed function
        (*(_STD function<void()> *)msg->getContextPointer())();
        break;
    }
}

int Node::GetNodeId() const
{
    return m_NodeId;
}

const NodeParams *Node::GetParams() const
{
    return &m_Params;
}

const _STD vector<NodeMessageData*> &Node::GetMessages() const
{
    return m_Messages;
}