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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.networklayer.contract.ipv4.Ipv4Address;

namespace inet;

//
// TODO generated message class
//
enum LeachPktType{
    CH = 1;
    ACK = 2;
    SCH = 3;
    DATA = 4;
    BS = 5;
}

class ScheduleEntry extends cObject{
    Ipv4Address nodeAddress;
    double TDMAdelay;
}

class LeachControlPkt extends FieldsChunk{
    LeachPktType packetType = static_cast<LeachPktType>(-1);
    Ipv4Address srcAddress;

    ScheduleEntry schedule[];
}

class LeachAckPkt extends LeachControlPkt{

}

class LeachSchPkt extends LeachControlPkt{

}

class LeachDataPkt extends LeachControlPkt{
    double temperature;
    double humidity;
}

class LeachBSPkt extends LeachControlPkt{
    Ipv4Address CHAddr;
}