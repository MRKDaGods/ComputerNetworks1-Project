#pragma once

#include "Common.h"
#include "NetEntity.h"
#include "Node.h"

#include <string>
#include <vector>

struct WindowPacketData
{
    int seqNum;
    NodeMessageData* data;
    
    bool read; // have we read this packet?
    bool sent; // have we sent this packet?
    bool acked; // have we received an ack for this packet?
    void* timer;
};

class NetSender : public NetEntity
{
private:
    _STD vector<WindowPacketData> m_Window;
    int m_WindowBase;
    int m_NextSeqNum;

    void SendWindow(bool force = false);
    Packet* CreateOutgoingPacket(NodeMessageData* data);
    void ConstructWindow();
    void LogWindow();
    void StartTimer(WindowPacketData *wnd);
    void CancelTimer(void *&timer);

protected:
    void SendPacket(TransmissionContext* ctx, PTransmissionCallback onPostProcess = 0, PTransmissionCallback onPreProcess = 0) override;

public:
    NetSender(Node *node);
    void ReceivePacket(Packet *packet, int* recvParity = 0) override;
    void ReceiveTimerEvent(NodeMessageData *data) override;
    void SysLogTransmission(TransmissionContext* ctx, WindowPacketData* wnd);
    int GetType() override;
};
