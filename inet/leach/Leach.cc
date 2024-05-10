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

#include "Leach.h"

#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace inet {

double randNo;
double threshold;

Ipv4Address idealClusterHead;

Register_Enum(inet::Leach, (Leach::CLUSTER_HEAD, Leach::NON_CLUSTER_HEAD))
Define_Module(Leach);

Leach::ForwardEntry::~ForwardEntry(){
    if (this->event != nullptr) delete this->event;
    if (this->hello != nullptr) delete this->hello;
}

Leach::Leach(){

}

Leach::~Leach(){
    stop();
    delete event;
}

void Leach::initialize(int stage){
    EV << "Initializing state" << endl;
    EV << "Stage: " << stage << endl;
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        EV << "Initializing Leach" << endl;
        host = getContainingNode(this);
        ift.reference(this, "interfaceTableModule", true);
        
        clusterHeadPercentage = par("clusterHeadPercentage");
        EV << "Cluster head percentage: " << clusterHeadPercentage << endl;
        numNodes = par("numNodes");
        
        helloInterval = par("helloInterval");
        event = new cMessage("event");
        ackEvent = new cMessage("ackEvent");
        schEvent = new cMessage("schEvent");

        WATCH(threshold);

        roundDuration = par("roundDuration");
        initialDuration = par("initialDuration");
        packetSize = par("packetSize");
        EV << "Round duration: " << roundDuration << endl;
        nodeRound = 0;
        TDMADelayCounter = 1;
        wasCH = false;
        sentAck = false;
        sentSch = false;
        leachState = NON_CLUSTER_HEAD;
        
        std::vector<NodeMemoryObject> nodeMemory(numNodes);
        std::vector<TDMAScheduleEntry> nodeClusterHeadMemory(numNodes);
        std::vector<TDMAScheduleEntry> extractedTDMASchedule(numNodes);
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS)
    {
        registerService(Protocol::manet, gate("ipOut"), gate("ipIn"));
        registerProtocol(Protocol::manet, gate("ipOut"), gate("ipIn"));
    }
    

}

void Leach::handleMessageWhenUp(cMessage *msg){
    Ipv4Address selfAddr = (interface802154ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress());
    EV << "Message received" << endl;
    if (msg->isSelfMessage())
    {
        if (msg->getKind() == SELF)
        {
            EV << "Self message received" << endl;
            double randNo = dblrand(0);
            EV << "Random number: " << randNo << endl;
            EV << "Round: " << nodeRound << endl;
            EV << "wasCH: " << wasCH << endl;
            threshold = generateThresholdValue(nodeRound);
            EV << "Threshold value: " << threshold << endl;

            if(randNo < threshold && wasCH == false){
                leachState = CLUSTER_HEAD;
                wasCH = true;
                EV << "Node " << host->getFullName() << " is a cluster head" << endl;
                handleSelfMessage(msg);
            }

            EV << "Leach state: " << leachState << endl;
            nodeRound += 1;
            
            int intervalLength = 1.0 / clusterHeadPercentage;
            if (fmod(nodeRound, intervalLength) == 0){
                EV << "Remove cluster head" << endl;
                wasCH = false;

                nodeRound = 0;
            }
            
            event->setKind(4);
            scheduleAt(simTime() + roundDuration - 2, event);
        }
        else if (msg->getKind() == ACK && leachState == NON_CLUSTER_HEAD){
            EV << "End initial duration" << endl;
            sentAck = false;
            sendAckToCH(selfAddr);
        }
        else if (msg->getKind() == SCH && leachState == CLUSTER_HEAD){
            EV << "End wait time duration" << endl;
            sentSch = false;
            sendSchToNCH(selfAddr);
        }
        else if (msg->getKind() == 4){
            EV << "End round duration" << endl;
            leachState = NON_CLUSTER_HEAD;
            EV << "Leach state: " << leachState << endl;
            nodeMemory.clear();
            nodeClusterHeadMemory.clear();
            extractedTDMASchedule.clear();
            sentAck = false;
            sentSch = false;
            TDMADelayCounter = 1;
            event->setKind(SELF);
            scheduleAt(simTime() + 2, event);
        }
    }
    else if (check_and_cast<Packet *>(msg)->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::manet)
    {
        EV << "Packet received" << endl;
        processMessage(msg);
    }
    else
    {
        delete msg;
    }
    
}

void Leach::handleSelfMessage(cMessage *msg){
    EV << "Handling self message" << endl;
    if(msg == event){
        EV << "Event received" << endl;
        if(event->getKind() == SELF){
            EV << "Event kind is SELF" << endl;
            auto ctrlPkts = makeShared<LeachControlPkt>();
            EV << "Control packet created" << endl;

            ctrlPkts->setPacketType(CH);
            EV << "Packet type set" << endl;
            Ipv4Address source = (interface802154ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress());
            ctrlPkts->setChunkLength(b(128));
            ctrlPkts->setSrcAddress(source);
            EV << "Source address set" << endl;

            auto packet = new Packet("LeachControlPkt", ctrlPkts);
            auto addressReq = packet->addTag<L3AddressReq>();
            addressReq->setDestAddress(Ipv4Address(255, 255, 255, 255));
            addressReq->setSrcAddress(source);
            packet->addTag<InterfaceReq>()->setInterfaceId(interface802154ptr->getInterfaceId());
            packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
            packet->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            EV << "Packet created" << endl;

            send(packet, "ipOut");
            packet = nullptr;
            ctrlPkts = nullptr;

            EV << "Packet sent" << endl;
        }
    }
    else
    {
        EV << "Deleting message" << endl;
        delete msg;
        EV << "Message deleted" << endl;
    }
}

void Leach::processMessage(cMessage *msg){
    EV << "Processing message" << endl;
    Ipv4Address selfAddr = (interface802154ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress());
    auto recievedCtrlPkt = staticPtrCast<LeachControlPkt>(check_and_cast<Packet *>(msg)->peekData<LeachControlPkt>()->dupShared());
    EV << "Control packet received" << endl;

    Packet * receivedPkt = check_and_cast<Packet *>(msg);
    auto& leachControlPkt = receivedPkt->popAtFront<LeachControlPkt>();
    auto packetType = leachControlPkt->getPacketType();
    EV << "Packet type: " << packetType << endl;

    if(msg->arrivedOn("ipIn")){
        EV << "Packet arrived on ipIn" << endl;
        if(packetType == 1 && leachState == NON_CLUSTER_HEAD){
            EV << "Packet type is 1" << endl;
            Ipv4Address CHaddr = recievedCtrlPkt->getSrcAddress();
            
            auto signalPowerInd = receivedPkt->getTag<SignalPowerInd>();
            double rxPower = signalPowerInd->getPower().get();
            EV << "Received power: " << rxPower << endl;

            addToNodeMemory(selfAddr, CHaddr, rxPower);
            EV << "sentAck is " << sentAck << endl;
            if (sentAck == false)
            {
                ackEvent->setKind(ACK);
                sentAck = true;
                EV << "sentAck is " << sentAck << endl;
                scheduleAt(simTime() + initialDuration + dblrand(0), ackEvent);
                
            }
        }
        else if (packetType == 2 && leachState == CLUSTER_HEAD){
            EV << "Packet type is 2" << endl;
            Ipv4Address nodeAddr = recievedCtrlPkt->getSrcAddress();
            addToNodeClusterHeadMemory(nodeAddr);
            EV << "sentSch is " << sentSch << endl;
            if(sentSch == false){
                schEvent->setKind(SCH);
                sentSch = true;
                EV << "sentSch is " << sentSch << endl;
                scheduleAt(simTime() + 2, schEvent);
            }
        }
        else if (packetType == 3 && leachState == NON_CLUSTER_HEAD){
            EV << "Packet type is 3" << endl;
            Ipv4Address CHaddr = recievedCtrlPkt->getSrcAddress();
            
            int scheduleArraySize = recievedCtrlPkt->getScheduleArraySize();

            for(int i = 0; i < scheduleArraySize; i++){
                ScheduleEntry tempScheduleEntry = recievedCtrlPkt->getSchedule(i);
                TDMAScheduleEntry extractedTDMAScheduleEntry;
                extractedTDMAScheduleEntry.nodeAddress = tempScheduleEntry.getNodeAddress();
                extractedTDMAScheduleEntry.TDMAdelay = tempScheduleEntry.getTDMAdelay();
                extractedTDMASchedule.push_back(extractedTDMAScheduleEntry);
            }

            double receivedTDMADelay = -1;
            for(auto &node : extractedTDMASchedule){
                if(node.nodeAddress == selfAddr){
                    receivedTDMADelay = node.TDMAdelay;
                    break;
                }
            }

            if (receivedTDMADelay > -1)
            {
                sendDataToCH(selfAddr, CHaddr, receivedTDMADelay);
            }
            
        }
        else if (packetType == 4 && leachState == CLUSTER_HEAD){
            EV << "Packet type is 4" << endl;
            Ipv4Address nodeAddr = recievedCtrlPkt->getSrcAddress();
            sendDataToBS(selfAddr);
        }
        else if (packetType == 5){
            EV << "Packet type is 5" << endl;
            delete msg;
        }
    }
    else{
        throw cRuntimeError("Unknown message received on unknown gate %s", msg->getArrivalGate()->getFullName());
    }
}

void Leach::start(){
    int num_802154 = 0;
    NetworkInterface *ie;
    NetworkInterface *i_face;
    broadcastDelay = &par("broadcastDelay");
    for(int i = 0; i < ift->getNumInterfaces(); i++){
        ie = ift->getInterface(i);
        if (ie->isWireless()){
            i_face = ie;
            num_802154++;
        }
    }

    if (num_802154 == 1){
        interface802154ptr = i_face;
        interfaceId = i_face->getInterfaceId();
    } else {
        throw cRuntimeError("Leach must have exactly one 802.15.4 interface, now have %d", num_802154);
    }

    CHK(interface802154ptr->getProtocolDataForUpdate<Ipv4InterfaceData>())->joinMulticastGroup(Ipv4Address::LL_MANET_ROUTERS);
    EV<< "Leach started" << endl;
    EV<< "Module Name : " << host->getFullName() << endl;

    event->setKind(SELF);
    scheduleAt(simTime() + uniform(0.0, par("maxVariance").doubleValue()), event);
    EV << "Event scheduled" << endl;
}

void Leach::handleStopOperation(LifecycleOperation *operation){
    cancelEvent(event);
}

void Leach::handleCrashOperation(LifecycleOperation *operation){
    cancelEvent(event);
}

void Leach::stop(){
    cancelEvent(event);
    nodeMemory.clear();
    nodeClusterHeadMemory.clear();
    extractedTDMASchedule.clear();

    TDMADelayCounter = 1;
}

double Leach::generateThresholdValue(int round){
    EV << "Generating threshold value" << endl;
    int intervalLength = 1.0 / clusterHeadPercentage;
    double threshold = (clusterHeadPercentage / (1 - (clusterHeadPercentage * (fmod(round, intervalLength)))));
    return threshold;
}

void Leach::addToNodeMemory(Ipv4Address nodeAddress, Ipv4Address clusterHeadAddress, double energy){
    if(!isCHAddedInMemory(clusterHeadAddress)){
        EV << "Adding to node memory" << endl;
        NodeMemoryObject node;
        node.nodeAddress = nodeAddress;
        node.clusterHeadAddress = clusterHeadAddress;
        node.energy = energy;
        nodeMemory.push_back(node);
        EV << "Added to node memory" << endl;
    }
}

void Leach::addToNodeClusterHeadMemory(Ipv4Address nodeAddress){
    if(!isNodeAddedInClusterHeadMemory(nodeAddress)){
        EV << "Adding to node cluster head memory" << endl;
        TDMAScheduleEntry scheduleEntry;
        scheduleEntry.nodeAddress = nodeAddress;
        scheduleEntry.TDMAdelay = TDMADelayCounter;
        nodeClusterHeadMemory.push_back(scheduleEntry);
        TDMADelayCounter += 1;
        EV << "Added to node cluster head memory" << endl;
    }
}

bool Leach::isCHAddedInMemory(Ipv4Address clusterHeadAddress){
    for(auto &node : nodeMemory){
        if(node.clusterHeadAddress == clusterHeadAddress){
            return true;
        }
    }
    return false;
}

bool Leach::isNodeAddedInClusterHeadMemory(Ipv4Address nodeAddress){
    for(auto &node : nodeClusterHeadMemory){
        if(node.nodeAddress == nodeAddress){
            return true;
        }
    }
    return false;
}


Ipv4Address Leach::getIdealCH(Ipv4Address nodeAddress){
    EV << "Getting ideal cluster head" << endl;
    double tempRxPower = 0.0;
    Ipv4Address idealClusterHead;
    for(auto &node : nodeMemory){
        if (node.nodeAddress == nodeAddress){
            if (node.energy > tempRxPower){
                tempRxPower = node.energy;
                idealClusterHead = node.clusterHeadAddress;
                continue;
            }
        }
        else{
            continue;
        }
    }
    return idealClusterHead;
}

void Leach::sendAckToCH(Ipv4Address nodeAddress){
    EV << "Sending ACK to cluster head" << endl;
    auto ackPkt = makeShared<LeachAckPkt>();
    ackPkt->setPacketType(ACK);
    ackPkt->setChunkLength(b(128));
    ackPkt->setSrcAddress(nodeAddress);

    auto ackPacket = new Packet("LeachAckPkt", ackPkt);
    auto addressReq = ackPacket->addTag<L3AddressReq>();

    addressReq->setDestAddress(getIdealCH(nodeAddress));
    addressReq->setSrcAddress(nodeAddress);
    ackPacket->addTag<InterfaceReq>()->setInterfaceId(interface802154ptr->getInterfaceId());
    ackPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    ackPacket->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

    send(ackPacket, "ipOut");
    EV << "ACK sent" << endl;
}

void Leach::sendSchToNCH(Ipv4Address selfAddr){
    EV << "Sending schedule to non-cluster head" << endl;
    auto schPkt = makeShared<LeachSchPkt>();
    schPkt->setPacketType(SCH);
    schPkt->setChunkLength(b(128));
    schPkt->setSrcAddress(selfAddr);

    for(auto &node : nodeClusterHeadMemory){
        ScheduleEntry scheduleEntry;
        scheduleEntry.setNodeAddress(node.nodeAddress);
        scheduleEntry.setTDMAdelay(node.TDMAdelay);
        schPkt->insertSchedule(scheduleEntry);
    }

    auto schedulePacket = new Packet("LeachSchPkt", schPkt);
    auto scheduleReq = schedulePacket->addTag<L3AddressReq>();

    scheduleReq->setDestAddress(Ipv4Address(255, 255, 255, 255));
    scheduleReq->setSrcAddress(selfAddr);
    schedulePacket->addTag<InterfaceReq>()->setInterfaceId(interface802154ptr->getInterfaceId());
    schedulePacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    schedulePacket->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

    send(schedulePacket, "ipOut");
    EV << "Schedule sent" << endl;
}

void Leach::sendDataToCH(Ipv4Address nodeAddress, Ipv4Address clusterHeadAddress, double TDMAslot){
    EV << "Sending data to cluster head" << endl;
    auto dataPkt = makeShared<LeachDataPkt>();
    dataPkt->setPacketType(DATA);
    double temperature = (double) rand()/RAND_MAX;
    double humidity = (double) rand()/RAND_MAX;

    dataPkt->setChunkLength(B(packetSize));
    dataPkt->setTemperature(temperature);
    dataPkt->setHumidity(humidity);
    dataPkt->setSrcAddress(nodeAddress);
    
    auto dataPacket = new Packet("LeachDataPkt", dataPkt);
    auto addressReq = dataPacket->addTag<L3AddressReq>();

    addressReq->setDestAddress(getIdealCH(nodeAddress));
    addressReq->setSrcAddress(nodeAddress);
    dataPacket->addTag<InterfaceReq>()->setInterfaceId(interface802154ptr->getInterfaceId());
    dataPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    dataPacket->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

    sendDelayed(dataPacket, TDMAslot, "ipOut");
    EV << "Data sent" << endl;
    dataPacket = nullptr;
    dataPkt = nullptr;
}

void Leach::sendDataToBS(Ipv4Address CHAddr){
    EV << "Sending data to base station" << endl;
    auto bsPkt = makeShared<LeachBSPkt>();
    bsPkt->setPacketType(BS);

    bsPkt->setChunkLength(B(packetSize));
    bsPkt->setSrcAddress(CHAddr);
    
    auto bsPacket = new Packet("LeachBSPkt", bsPkt);
    auto addressReq = bsPacket->addTag<L3AddressReq>();

    addressReq->setDestAddress(Ipv4Address(10, 0, 0, 1));
    addressReq->setSrcAddress(CHAddr);
    bsPacket->addTag<InterfaceReq>()->setInterfaceId(interface802154ptr->getInterfaceId());
    bsPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    bsPacket->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

    send(bsPacket, "ipOut");
    EV << "Data sent from cluster head to base station" << endl;
    bsPacket = nullptr;
    bsPkt = nullptr;
    
}

} //namespace
