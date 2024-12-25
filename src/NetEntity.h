#pragma once

#include <functional>
#include <vector>

#include "Common.h"

class Node;
class Packet;
struct NodeMessageData;
struct TransmissionContext;

typedef _STD function<void(TransmissionContext*)> TransmissionCallback, *PTransmissionCallback;

enum NET_ENTITY_TYPE
{
    NET_ENTITY_TYPE_SENDER,
    NET_ENTITY_TYPE_RECEIVER
};

struct TransmissionContext
{
    Packet* packet;
    NodeMessageData* data;

    int modifiedBitIdx;
    int nextDuplicateType;
    bool ackLost;
};

class NetEntity
{
private:
    long m_NextSendTime;
    _STD vector<TransmissionContext*> m_TransmissionContexts;

    void EncodePacket(Packet *packet);
    void DecodePacket(Packet *packet);
    int CalculateParity(const char *payload);
    long GetAndUpdateProcessingDelay(long* preprocessDelay = 0);
    long CalculateDelay(NodeMessageData *data);
    void ExecuteScheduled(long delay, _STD function<void()> func);

protected:
    Node *const m_Node;
    const int m_NodeId;

    virtual void SendPacket(TransmissionContext* ctx, PTransmissionCallback onPostProcess = 0, PTransmissionCallback onPreProcess = 0);
    bool Probability(const char *param);
    long GetSimTime(); // in ms
    float GetSimTimeF(); // in s
    TransmissionContext* CreateTransmissionContext(Packet* packet, NodeMessageData* data = 0);

public:
    NetEntity(Node *node);
    ~NetEntity();

    virtual void ReceivePacket(Packet *packet, int *recvParity = 0);
    virtual void ReceiveTimerEvent(NodeMessageData *data);
    virtual int GetType() = 0;
};

#define MAKE_PACKET(name, frameType, seqNum, payload, parity, ackNum) \
    auto name = new Packet(0, MSG_KIND_PACKET);                       \
    name->setFrameType(frameType);                                    \
    name->setSeqNum(seqNum);                                          \
    name->setPayload(payload);                                        \
    name->setParity(parity);                                          \
    name->setAckNum(ackNum);