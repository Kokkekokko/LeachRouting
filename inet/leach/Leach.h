//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __INET4_5_LEACH_H_
#define __INET4_5_LEACH_H_

#include <omnetpp.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "inet/common/INETDefs.h"
#include "inet/routing/base/RoutingProtocolBase.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/routing/leach/LeachPkts_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

using namespace omnetpp;

namespace inet {

/**
 * TODO - Generated class
 */
class INET_API Leach : public RoutingProtocolBase
{
    private:
        struct ForwardEntry {
            cMessage *event = nullptr;
            Packet *hello = nullptr;

            ForwardEntry() {}
            ~ForwardEntry();
    };

    bool isForwardHello = false;
    cMessage *event = nullptr;
    cMessage *ackEvent = nullptr;
    cMessage *schEvent = nullptr;
    cPar *broadcastDelay = nullptr;
    std::list<ForwardEntry *> *forwardList = nullptr;
    NetworkInterface *interface802154ptr = nullptr;
    int interfaceId = -1;
    unsigned int sequencenumber = 0;
    simtime_t routeLifetime;
    cModule *host = nullptr;

  protected:
    simtime_t helloInterval;
    ModuleRefByPar<IInterfaceTable> ift;
    ModuleRefByPar<IIpv4RoutingTable> rt;
    simtime_t roundDuration;
    simtime_t initialDuration;

    int numNodes = 0;
    double clusterHeadPercentage = 0.0;
    int packetSize;

  public:
    Leach();
    ~Leach();
    enum LeachState{
        CLUSTER_HEAD,
        NON_CLUSTER_HEAD
    };

    LeachState leachState;
    double TDMADelayCounter;

    struct NodeMemoryObject{
      Ipv4Address nodeAddress;
      Ipv4Address clusterHeadAddress;
      double energy;
    };

    struct TDMAScheduleEntry{
      Ipv4Address nodeAddress;
      double TDMAdelay;
    };

    std::vector<NodeMemoryObject> nodeMemory;
    std::vector<TDMAScheduleEntry> nodeClusterHeadMemory;
    std::vector<TDMAScheduleEntry> extractedTDMASchedule;
    bool wasCH;
    bool sentAck;
    bool sentSch;
    int nodeRound;
    

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;

    virtual void handleStartOperation(LifecycleOperation *operation) override { start(); }
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    void handleSelfMessage(cMessage *msg);
    void processMessage(cMessage *msg);
    void addToNodeMemory(Ipv4Address nodeAddress, Ipv4Address clusterHeadAddress, double energy);
    void addToNodeClusterHeadMemory(Ipv4Address nodeAddress);
    bool isCHAddedInMemory(Ipv4Address clusterHeadAddress);
    bool isNodeAddedInClusterHeadMemory(Ipv4Address nodeAddress);

    void sendAckToCH(Ipv4Address nodeAddress);
    void sendSchToNCH(Ipv4Address selfAddr);
    void sendDataToCH(Ipv4Address nodeAddress, Ipv4Address clusterHeadAddress, double TDMAslot);
    void sendDataToBS(Ipv4Address CHAddr);

    Ipv4Address getIdealCH(Ipv4Address nodeAddress);

    void start();
    void stop();
    enum SelfMsgKinds{
        SELF = 1,
        DATA2CH,
        DATA2BS,
    };

    double generateThresholdValue(int round);
};

} //namespace

#endif
