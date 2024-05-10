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

#include "LeachBS.h"

namespace inet {

Define_Module(LeachBS);

    LeachBS::LeachBS() {
        
    }

    LeachBS::~LeachBS() {
        
    }

    void LeachBS::initialize(int stage) {
        RoutingProtocolBase::initialize(stage);
        if (stage == INITSTAGE_LOCAL) {
            bsPktReceived = 0;
            ift.reference(this, "interfaceTableModule", true);
            host = getContainingNode(this);
        }
        else if(stage == INITSTAGE_ROUTING_PROTOCOLS) {
            registerService(Protocol::manet, gate("ipOut"), gate("ipIn"));
            registerProtocol(Protocol::manet, gate("ipOut"), gate("ipIn"));
        }
    }

    void LeachBS::start(){
        EV << "LeachBS::start()" << endl;
        int num_802154 = 0;
        NetworkInterface *ie;
        NetworkInterface *i_face;

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
    }

    void LeachBS::stop(){
        EV << "LeachBS::stop()" << endl;
    }

    void LeachBS::handleMessageWhenUp(cMessage *msg){
        Ipv4Address nodeAddress = (interface802154ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress());
        if (msg->isSelfMessage()){
            EV << "LeachBS::handleMessageWhenUp() - self message" << endl;
            delete msg;
        }
        else if (check_and_cast<Packet *>(msg)->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::manet){
            auto receivedCtrlPkt = staticPtrCast<LeachControlPkt>(check_and_cast<Packet *>(msg)->peekData<LeachControlPkt>()->dupShared());
            Packet * receivedPkt = check_and_cast<Packet *>(msg);
            auto& leachControlPkt = receivedPkt->popAtFront<LeachControlPkt>();

            auto packetType = leachControlPkt->getPacketType();

            if (msg->arrivedOn("ipIn")){
                if (packetType == 5){
                    bsPktReceived++;
                    EV << "Data packet received by BS. now " << bsPktReceived << endl;
                }
                else{
                    delete msg;
                }
            }
            else{
                delete msg;
            }
        }
        else{
            delete msg;
        }
    }

    void LeachBS::finish(){
        EV << "LeachBS::finish()" << endl;
        EV << "Total data packets received by BS: " << bsPktReceived << endl;

        recordScalar("#bsPktReceived", bsPktReceived);
    }

} // namespace inet