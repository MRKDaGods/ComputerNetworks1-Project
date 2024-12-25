#ifndef __PROJBGDDD_NODE_H_
#define __PROJBGDDD_NODE_H_

#include "Common.h"
#include "NetEntity.h"

#include <omnetpp.h>
#include <vector>
#include <string>

using namespace omnetpp;

#define NODE_LOG(msg, ...)                             \
  {                                                    \
    char buf[300];                                     \
    char final_buf[320];                               \
    sprintf(buf, msg, ##__VA_ARGS__);                  \
    sprintf(final_buf, "[Node %d] %s", m_NodeId, buf); \
    EV << final_buf << endl;                           \
  }

struct NodeParams
{
  int windowSize;
  double timeoutInterval;
  double processingTime;
  double transmissionDelay;
  double errorDelay;
  double duplicationDelay;
  double lossRate;
};

struct NodeMessageData
{
  _STD string message;
  int id;

  struct
  {
    bool modification : 1;
    bool loss : 1;
    bool duplication : 1;
    bool delay : 1;
  } flags;
};

class Node : public cSimpleModule
{
private:
  int m_NodeId;
  NodeParams m_Params;
  _STD vector<NodeMessageData*> m_Messages;
  NetEntity *m_NetEntity;

  void ReadParams();
  bool InitializeMessages();

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;

public:
  Node();
  ~Node();
  int GetNodeId() const;
  const NodeParams* GetParams() const;
  const _STD vector<NodeMessageData*>& GetMessages() const;
};

#endif
