#pragma once

#include "NetEntity.h"

class NetReceiver : public NetEntity
{
private:
    int m_LastSeqNum;

public:
    NetReceiver(Node *node);
    void ReceivePacket(Packet *packet, int *recvParity = 0) override;
    int GetType() override;
};
