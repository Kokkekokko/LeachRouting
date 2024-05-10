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

#ifndef __INET4_5_LEACHBS_H_
#define __INET4_5_LEACHBS_H_

#include <omnetpp.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "Leach.h"

namespace inet {

class INET_API LeachBS : public RoutingProtocolBase{
    private:
        int bsPktReceived;

    public:
        LeachBS();
        ~LeachBS();

        int interfaceId = -1;
        NetworkInterface *interface802154ptr = nullptr;
        unsigned int sequencenumber = 0;
        cModule *host = nullptr;

    protected:
        ModuleRefByPar<IInterfaceTable> ift;
        ModuleRefByPar<IIpv4RoutingTable> rt;
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(cMessage *msg) override;

        virtual void handleStartOperation(LifecycleOperation *operation) override { start(); }
        virtual void handleStopOperation(LifecycleOperation *operation) override { stop(); }
        virtual void handleCrashOperation(LifecycleOperation *operation) override  { stop(); }
        void start();
        void stop();
        void finish() override;
};

}

#endif