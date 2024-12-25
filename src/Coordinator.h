#ifndef __PROJBGDDD_COORDINATOR_H_
#define __PROJBGDDD_COORDINATOR_H_

#include <omnetpp.h>

using namespace omnetpp;

class Coordinator : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif
