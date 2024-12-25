#include "NetSender.h"
#include "Node.h"
#include "Packet_m.h"
#include "SysLogger.h"

#include <omnetpp.h>
#include <bitset>

NetSender::NetSender(Node *node) : NetEntity(node)
{
    NODE_LOG("NetSender constructed");

    // init window
    ConstructWindow();

    // send current window
    SendWindow();
}

void NetSender::ReceivePacket(Packet *packet, int *recvParity)
{
    NetEntity::ReceivePacket(packet, recvParity);

    // check if we received an ack/nack
    auto frameType = packet->getFrameType();
    if (frameType != FRAME_TYPE_ACK && frameType != FRAME_TYPE_NACK)
    {
        NODE_LOG("ERROR Received packet with invalid frame type %d", frameType);
        return;
    }

    NODE_LOG("Received %s at t=%ld", packet->getFrameType() == FRAME_TYPE_ACK ? "ACK" : "NACK", GetSimTime());

    // are we going to advance window?
    if (frameType == FRAME_TYPE_ACK)
    {
        // mark acked and cancel timer
        int ackNum = packet->getAckNum();

        auto &wnd = m_Window[ackNum];
        wnd.acked = true;
        CancelTimer(wnd.timer);

        // advance window if needed
        if (ackNum == m_WindowBase)
        {
            NODE_LOG("Advancing window base to %d", ++ackNum);

            // should we terminate?
            bool fullyAcked = true;
            for (auto &wnd : m_Window)
            {
                if (!wnd.acked)
                {
                    fullyAcked = false;
                    break;
                }
            }

            if (fullyAcked)
            {
                NODE_LOG("All messages acked, terminating");
                m_Node->endSimulation();
                return;
            }

            m_WindowBase = _STD min(ackNum, (int)m_Window.size() - 1);

            // log window
            LogWindow();

            // send window
            SendWindow();
        }
    }
    else
    {
        // syslog
        SysLog("At : %.2f, Node : %d, [%s] NACK for seq_number : %d",
               GetSimTimeF(), m_NodeId, "received", packet->getSeqNum());
    }
}

void NetSender::ReceiveTimerEvent(NodeMessageData *data)
{
    if (data == 0)
    {
        NODE_LOG("ERROR NodeMessageData null at sender");
        return;
    }

    NetEntity::ReceiveTimerEvent(data);

    // check if already acked or out of window
    auto &wnd = m_Window[data->id];
    if (wnd.acked ||
        data->id < m_WindowBase ||
        data->id >= m_WindowBase + m_Node->GetParams()->windowSize)
    {
        NODE_LOG("Timer event received for message %d, but already acked or out of window", data->id);
        return;
    }

    // syslog
    SysLog("Time out event at time : %.2f, Node : %d, for frame with seq_num = %d",
           GetSimTimeF(), m_NodeId, wnd.seqNum);

    // remove all errors from timedout packet
    data->flags = {false, false, false, false};

    // resend window
    SendWindow(true);
}

int NetSender::GetType()
{
    return NET_ENTITY_TYPE_SENDER;
}

void NetSender::SendWindow(bool force)
{
    auto windowSize = m_Node->GetParams()->windowSize;
    int endIdx = _STD min(m_WindowBase + windowSize, (int)m_Window.size());
    auto end = m_Window.begin() + endIdx;

    NODE_LOG("Sending window, WS=%d WB=%d END=%d", windowSize, m_WindowBase, endIdx);

    for (auto it = m_Window.begin() + m_WindowBase; it != end; it++)
    {
        NODE_LOG("WND: id=%d", it->data->id);

        if (!force && it->sent)
        {
            NODE_LOG("WND: already sent");
            continue;
        }

        // mark as sent
        it->sent = true;
        it->acked = false;

        // cancel timer
        CancelTimer(it->timer);

        // send packet
        SendPacket(CreateTransmissionContext(CreateOutgoingPacket(it->data), it->data));
    }
}

Packet *NetSender::CreateOutgoingPacket(NodeMessageData *data)
{
    auto &wnd = m_Window[data->id];
    MAKE_PACKET(pkt, FRAME_TYPE_DATA, wnd.seqNum, data->message.c_str(), -1, data->id);
    return pkt;
}

void NetSender::ConstructWindow()
{
    NODE_LOG("Constructing window");

    m_Window.clear();
    m_WindowBase = m_NextSeqNum = 0;

    for (auto &msg : m_Node->GetMessages())
    {
        WindowPacketData data;
        data.seqNum = m_NextSeqNum++;
        data.read = false;
        data.data = msg;
        data.sent = data.acked = false;
        data.timer = 0;

        m_Window.push_back(data);

        // wrap around
        if (m_NextSeqNum == m_Node->GetParams()->windowSize)
        {
            m_NextSeqNum = 0;
        }
    }

    NODE_LOG("Window constructed, size=%d", m_Window.size());

    LogWindow();
}

void NetSender::SendPacket(TransmissionContext *ctx, PTransmissionCallback onPostProcess, PTransmissionCallback onPreProcess)
{
    auto packet = ctx->packet;
    auto data = ctx->data;

    if (data == 0)
    {
        NODE_LOG("ERROR NodeMessageData null at sender");
        return;
    }

    NODE_LOG("Sending packet seqNum=%d, ackNum=%d, payload=%s", packet->getSeqNum(), packet->getAckNum(), packet->getPayload());

    // start timer after processing is done
    auto onPostProcessCallback = [this, data](TransmissionContext *ctx)
    {
        // start timer
        auto wnd = &m_Window[data->id];
        StartTimer(wnd);

        // log transmission
        SysLogTransmission(ctx, wnd);
    };

    auto onPreProcessCallback = [this, data](TransmissionContext *ctx)
    {
        auto wnd = &m_Window[data->id];

        if (!wnd->read)
        {
            wnd->read = true;

            // syslog
            SysLog("At : %.2f, Node : %d, Introducing channel error with code = %d%d%d%d",
                   GetSimTimeF(), m_NodeId, data->flags.modification, data->flags.loss, data->flags.duplication, data->flags.delay);
        }
    };

    NetEntity::SendPacket(ctx, new TransmissionCallback(onPostProcessCallback), new TransmissionCallback(onPreProcessCallback));
}

void NetSender::SysLogTransmission(TransmissionContext *ctx, WindowPacketData *wnd)
{
    auto packet = ctx->packet;
    auto data = ctx->data;

    _STD bitset<4> parityBits(packet->getParity());
    auto parityStr = parityBits.to_string();

    if (wnd == 0)
    {
        wnd = &m_Window[data->id];
    }

    // syslog
    SysLog("At : %.2f, Node : %d, [%s] frame with seq_num : %d and payload = %s and\ntrailer = %s, Modified = %d, Lost = %s, Duplicate = %d, Delay = %.2f",
           GetSimTimeF(), m_NodeId, "sent", wnd->seqNum, packet->getPayload(), parityStr.c_str(),
           ctx->modifiedBitIdx, data->flags.loss ? "YES" : "NO", ctx->nextDuplicateType++,
           data->flags.delay ? m_Node->GetParams()->errorDelay : 0.0);
}

void NetSender::LogWindow()
{
    NODE_LOG("Window state:");
    for (size_t i = 0; i < m_Window.size(); i++)
    {
        bool inWindow = (i >= m_WindowBase && i < m_WindowBase + m_Node->GetParams()->windowSize);
        NODE_LOG("[%c] Seq=%d Msg=%s",
                 inWindow ? '*' : ' ',
                 m_Window[i].seqNum,
                 m_Window[i].data->message.c_str());
    }
}

void NetSender::StartTimer(WindowPacketData *wnd)
{
    if (wnd->timer)
    {
        // cancel previous timer
        CancelTimer(wnd->timer);
    }

    NODE_LOG("Starting timer at t=%ld for message %d", GetSimTime(), wnd->data->id);

    auto timer = new cMessage("timer");
    timer->setKind(MSG_KIND_TIMER);
    timer->setContextPointer(wnd->data);

    auto delay = m_Node->GetParams()->timeoutInterval * 1000;
    m_Node->scheduleAt(simTime() + simtime_t(delay, SIMTIME_MS), timer);

    wnd->timer = timer;
}

void NetSender::CancelTimer(void *&timer)
{
    if (!timer)
    {
        return;
    }

    auto msg = ((omnetpp::cMessage *)timer);
    NODE_LOG("Cancelling timer at t=%ld for node %d", GetSimTime(), ((NodeMessageData *)msg->getContextPointer())->id);

    // cancel timer
    m_Node->cancelEvent(msg);

    // bye timer
    timer = 0;
}
