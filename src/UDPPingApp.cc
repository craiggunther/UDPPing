//
// Copyright (C) 2012 Juan-Carlos Maureira
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <omnetpp.h>
#include "UDPPingApp.h"
#include "UDPPingMsg_m.h"
#include "UDPControlInfo_m.h"
#include "IPAddressResolver.h"
#include "IInterfaceTable.h"
#include "InterfaceEntry.h"
#include "IPv4InterfaceData.h"

Define_Module(UDPPingApp);

cModule* UDPPingApp::getDestinationModule() {

    EV << "Searching for destination module" << endl;

    cTopology* topo = new cTopology("udpPingApp");
    // extract topology
    topo->extractByProperty("node");

    EV << "cTopology found " << topo->getNumNodes() << " nodes\n";

    cModule* destModule = NULL;

    for (int i = 0; i < topo->getNumNodes(); i++) {
        if ( topo->getNode(i) == topo->getNodeFor(getParentModule()) ) continue; // skip ourselves

        cModule* mod = topo->getNode(i)->getModule();
        bool is_ip_node = IPAddressResolver().findInterfaceTableOf(mod)!=NULL;

        if (is_ip_node) {
            IInterfaceTable *ift = IPAddressResolver().interfaceTableOf(mod);
            // search the destination ip
            if (ift!=NULL) {
                for (int k=0; k<ift->getNumInterfaces(); k++) {
                    InterfaceEntry *ie = ift->getInterface(k);

                    for(int j=0;j<this->destAddresses.size();j++) {

                        if (!ie->isLoopback() & (ie->ipv4Data()->getIPAddress() == this->destAddresses[j].get4()) ) {
                            // found!!
                            EV << "Module " << mod << " IP address: " << ie->ipv4Data()->getIPAddress() << endl;
                            destModule = topo->getNode(i)->getModule();
                            break;
                        }
                    }

                    if (destModule) {
                        break;
                    }
                }
            }
            if (destModule) {
                break;
            }
        }
    }

    if (destModule) {
        EV << "Destination Module : " << destModule->getFullName() << endl;
    }

    return destModule;
}


void UDPPingApp::initialize(int stage)
{
    // because of IPAddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    if (stage!=3)
        return;

    counter = 0;
    numSent = 0;
    numReceived = 0;
    WATCH(numSent);
    WATCH(numReceived);

    localPort = par("localPort");
    destPort = par("destPort");

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;
    while ((token = tokenizer.nextToken())!=NULL)
        destAddresses.push_back(IPAddressResolver().resolve(token));

    if (destAddresses.empty())
        return;

    bindToPort(localPort);

    this->cOutCounter = NULL;

    // create the output vector for counter arrival registration when selected
    if ((bool)par("registerArrivalCounter") == true) {
        this->cOutCounter = new cOutVector("Counter Arrival");
    }

    // watch the bytes received variable
    bytes_received = 0;
    WATCH(bytes_received);

    // schedule the first packet at "startingTime"
    cMessage *timer = new cMessage("sendTimer");
    scheduleAt((double)par("startingTime"), timer);

    // Register the signal to notify possible receivers about the on-the-way packet
    packet_sent_signal = registerSignal("PacketSent");

    // subscribe the on-the-way packet signal

    this->destination_module = this->getDestinationModule();

    if (this->destination_module!=NULL) {
        this->destination_module->subscribe("PacketSent",this);
    } else {
        EV << "Could not determine destination module at initialization time, trying later" << endl;
    }
}

void UDPPingApp::finish()
{
    recordScalar("Bytes Received",this->bytes_received,"Bytes");
}

cPacket *UDPPingApp::createPacket()
{
    char msgName[32];
    sprintf(msgName,"UDPPing-%d", counter++);

    UDPPingMsg *message = new UDPPingMsg(msgName);
    message->setByteLength(par("messageLength").longValue());
    message->setCounter(counter);

    if (this->destination_module == NULL) {
        this->destination_module = this->getDestinationModule();

        if (this->destination_module!=NULL) {
            this->destination_module->subscribe("PacketSent",this);
        } else {
            EV << "Could not determine destination module at initialization time, trying later" << endl;
        }
    }

    emit(this->packet_sent_signal,counter);

    return message;
}

void UDPPingApp::processPacket(cPacket *msg)
{
    if (msg->getKind() == UDP_I_ERROR)
    {
        delete msg;
        return;
    }

    UDPPingMsg *packet = check_and_cast<UDPPingMsg *>(msg);

    if (packet->getIsRequest())
    {
        UDPControlInfo *controlInfo = check_and_cast<UDPControlInfo *>(packet->getControlInfo());

        // swap src and dest
        IPvXAddress srcAddr = controlInfo->getSrcAddr();
        int srcPort = controlInfo->getSrcPort();
        controlInfo->setSrcAddr(controlInfo->getDestAddr());
        controlInfo->setSrcPort(controlInfo->getDestPort());
        controlInfo->setDestAddr(srcAddr);
        controlInfo->setDestPort(srcPort);

        // register the arrival counter when selected
        if (this->cOutCounter!=NULL) {
            this->cOutCounter->record(packet->getCounter());
        }

        // register the amount of bytes received
        this->bytes_received += packet->getByteLength();

        if (this->prv.find(packet->getCounter())) {

        packet->setIsRequest(false);
        send(packet, "udpOut");
    }
    else
    {
        simtime_t rtt = simTime() - packet->getCreationTime();
        EV << "RTT: " << rtt << "\n";
        delete msg;
    }
    numReceived++;
}

void UDPPingApp::receiveSignal(cComponent *src, simsignal_t id, long value) {
    if (id == this->packet_sent_signal) {
        EV << "[" << this->getFullPath() << "] packet sent signal received from " << src->getFullPath() << ". Sequence Number : " << value << endl;
        this->prv.insert(std::make_pair(value,simTime()));
    }
}

void UDPPingApp::sendPacket() {
    UDPBasicApp::sendPacket();
}

