#include "NetEntity.h"
#include "Node.h"
#include "Packet_m.h"

#include <omnetpp.h>
#include "NetSender.h"

NetEntity::NetEntity(Node *node) : m_Node(node), m_NodeId(node->GetNodeId())
{
    m_NextSendTime = 0;
}

NetEntity::~NetEntity()
{
    for (auto ctx : m_TransmissionContexts)
    {
        delete ctx;
    }
}

void NetEntity::ReceivePacket(Packet *packet, int *recvParity)
{
    if (recvParity)
    {
        *recvParity = CalculateParity(packet->getPayload());
    }

    if (packet->getFrameType() == FRAME_TYPE_DATA)
    {
        // decode payload
        DecodePacket(packet);
    }
}

void NetEntity::SendPacket(TransmissionContext *ctx, PTransmissionCallback onPostProcess, PTransmissionCallback onPreProcess)
{
    auto packet = ctx->packet;
    auto data = ctx->data;

    if (packet->getFrameType() == FRAME_TYPE_DATA)
    {
        // escape payload
        EncodePacket(packet);

        // calculate parity
        packet->setParity(CalculateParity(packet->getPayload()));

        if (data)
        {
            // log flags
            NODE_LOG("Sending packet with flags: modification=%d, loss=%d, duplication=%d, delay=%d",
                     data->flags.modification, data->flags.loss, data->flags.duplication, data->flags.delay);

            // check for modification
            if (data->flags.modification)
            {
                NODE_LOG("Modifying packet");

                // modify packet
                auto newPayload = _STD string(packet->getPayload());
                int idx = m_Node->intuniformexcl(0, newPayload.size());
                int bit = m_Node->intuniformexcl(1, 8);
                newPayload[idx] ^= 1 << bit;

                // set modified bit index
                ctx->modifiedBitIdx = idx * 8 + bit;

                NODE_LOG("Modified packet at idx=%d, bit=%d, old=%s, new=%s", idx, bit, packet->getPayload(), newPayload.c_str());

                packet->setPayload(newPayload.c_str());
            }
        }
    }

    auto postProcessed = [this, ctx, packet, onPostProcess, data]()
    {
        NODE_LOG("Sending packet seqNum=%d, ackNum=%d, payload=%s", packet->getSeqNum(), packet->getAckNum(), packet->getPayload());

        // execute onSent callback, typically start timer
        if (onPostProcess)
        {
            (*onPostProcess)(ctx);
            delete onPostProcess;
        }

        if ((data && data->flags.loss) || ctx->ackLost)
        {
            // log after delay
            NODE_LOG("Packet lost");
            return;
        }

        // calc delay
        auto delay = CalculateDelay(data);
        NODE_LOG("Sending packet with channel delay %ld", delay);
        m_Node->sendDelayed(packet, simtime_t(delay, SIMTIME_MS), "port$o");

        // duplicated packet?
        if (data && data->flags.duplication)
        {
            auto dup = packet->dup();
            auto dupDelay = m_Node->GetParams()->duplicationDelay * 1000;
            delay += dupDelay;

            NODE_LOG("Duplicating packet with delay %ld", delay);

            m_Node->sendDelayed(dup, simtime_t(delay, SIMTIME_MS), "port$o");

            // log after delay
            auto dupLog = [this, ctx]()
            {
                ((NetSender*)this)->SysLogTransmission(ctx, 0);
            };

            ExecuteScheduled(dupDelay, dupLog);
        }
    };

    // execute post process callback, and get delay of pre-process as well
    long preprocessDelay;
    ExecuteScheduled(GetAndUpdateProcessingDelay(&preprocessDelay), postProcessed);

    // execute pre-process callback
    if (onPreProcess)
    {
        auto preProcessed = [onPreProcess, ctx]()
        {
            (*onPreProcess)(ctx);
            delete onPreProcess;
        };

        if (preprocessDelay > 0)
        {
            ExecuteScheduled(preprocessDelay, preProcessed);
        }
        else
        {
            preProcessed();
        }
    }
}

bool NetEntity::Probability(const char *param)
{
    char predParamName[200];
    sprintf(predParamName, "%sPred", param);

    return m_Node->par(predParamName).doubleValue() <= m_Node->par(param).doubleValue();
}

long NetEntity::GetSimTime()
{
    return m_Node->getSimulation()->getSimTime().inUnit(SIMTIME_MS);
}

float NetEntity::GetSimTimeF()
{
    return GetSimTime() / 1000.0f;
}

TransmissionContext *NetEntity::CreateTransmissionContext(Packet *packet, NodeMessageData *data)
{
    auto ctx = new TransmissionContext;
    ctx->packet = packet;
    ctx->data = data;
    ctx->modifiedBitIdx = -1;
    ctx->nextDuplicateType = data && data->flags.duplication ? 1 : 0;
    ctx->ackLost = false;

    m_TransmissionContexts.push_back(ctx);

    return ctx;
}

int NetEntity::CalculateParity(const char *payload)
{
    int parity = 0;
    for (int i = 0, len = strlen(payload); i < len; i++)
    {
        parity ^= payload[i];
    }

    return parity;
}

long NetEntity::GetAndUpdateProcessingDelay(long *preprocessDelay)
{
    auto curTime = GetSimTime();
    if (m_NextSendTime <= curTime)
    {
        m_NextSendTime = curTime;
    }

    m_NextSendTime += m_Node->GetParams()->processingTime * 1000;
    auto delay = m_NextSendTime - curTime;

    if (preprocessDelay)
    {
        *preprocessDelay = _STD max(delay - 500, 0L);
    }

    return delay;
}

long NetEntity::CalculateDelay(NodeMessageData *data)
{
    auto delay = 0L;

    // transmission delay
    delay += m_Node->GetParams()->transmissionDelay * 1000;

    // error delay
    if (data && data->flags.delay)
    {
        delay += m_Node->GetParams()->errorDelay * 1000;
    }

    return delay;
}

void NetEntity::ExecuteScheduled(long delay, _STD function<void()> func)
{
    auto msg = new cMessage("scheduled");
    msg->setKind(MSG_KIND_SCHEDULED);
    msg->setContextPointer(new _STD function<void()>(func));

    m_Node->scheduleAt(simTime() + simtime_t(delay, SIMTIME_MS), msg);
}

void NetEntity::EncodePacket(Packet *packet)
{
    // flag = $
    // esc = /

    auto payload = _STD string(packet->getPayload());
    for (int i = 0; i < payload.size(); i++)
    {
        if (payload[i] == '$' || payload[i] == '/')
        {
            payload.insert(i, 1, '/');
            i++;
        }
    }

    payload = "$" + payload + "$";
    packet->setPayload(payload.c_str());
}

void NetEntity::DecodePacket(Packet *packet)
{
    auto payload = _STD string(packet->getPayload());

    // remove escaping
    for (int i = 0; i < payload.size() - 1; i++)
    {
        if (payload[i] == '/')
        {
            if (payload[i + 1] == '$' || payload[i + 1] == '/')
            {
                payload.erase(i, 1);
                //i++;
            }
        }
    }

    payload = payload.substr(1, payload.size() - 2);
    packet->setPayload(payload.c_str());
}

void NetEntity::ReceiveTimerEvent(NodeMessageData *data)
{
    NODE_LOG("Timer event received for message %d at t=%ld", data->id, GetSimTime());
}