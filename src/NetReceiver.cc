#include "NetReceiver.h"
#include "Node.h"
#include "Packet_m.h"
#include "SysLogger.h"

NetReceiver::NetReceiver(Node *node) : NetEntity(node)
{
    NODE_LOG("NetReceiver constructed");

    m_LastSeqNum = -1;
}

void NetReceiver::ReceivePacket(Packet *packet, int *recvParity)
{
    // calc parity before decoding
    int newParity;
    NetEntity::ReceivePacket(packet, &newParity);

    NODE_LOG("Received packet content=%s parity=%d at t=%ld", packet->getPayload(), packet->getParity(), GetSimTime());

    // syslog
    SysLog("At : %.2f Node : %d Received packet with seqNum : %d, payload = %s",
           GetSimTimeF(), m_NodeId, packet->getSeqNum(), packet->getPayload());

    // check parity
    bool error = packet->getParity() != newParity;
    if (error)
    {
        NODE_LOG("Parity check failed");
    }

    // do we actually send?
    bool lost = int(m_Node->uniform(0, 100)) < (int)m_Node->GetParams()->lossRate;  // Probability(PARAM_LOSS_RATE);
    auto onPostProcessCallback = [this, error, packet, lost](TransmissionContext *ctx)
    {
        int prevSeqNum = packet->getSeqNum() - 1;
        if (prevSeqNum == -1)
        {
            prevSeqNum = m_Node->GetParams()->windowSize - 1;
        }

        if (m_LastSeqNum == -1 || m_LastSeqNum == prevSeqNum)
        {
            if (!error)
            {
                m_LastSeqNum = packet->getSeqNum();
            }

            // syslog
            SysLog("At time : %.2f Node : %d Sending %s with number : %d, loss : %s",
                   GetSimTimeF(), m_NodeId, error ? "NACK" : "ACK", packet->getSeqNum(), lost ? "YES" : "NO");
        }
    };

    // send ack/nack
    if (!lost)
    {
        NODE_LOG("Sending %s", error ? "NACK" : "ACK");
    }

    MAKE_PACKET(ack, error ? FRAME_TYPE_NACK : FRAME_TYPE_ACK, packet->getSeqNum(), "", -1, packet->getAckNum());

    auto ctx = CreateTransmissionContext(ack);
    ctx->ackLost = lost;

    SendPacket(ctx, new TransmissionCallback(onPostProcessCallback));
}

int NetReceiver::GetType()
{
    return NET_ENTITY_TYPE_RECEIVER;
}