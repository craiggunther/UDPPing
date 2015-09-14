//
// Copyright (C) 2005 Andras Babos
// Copyright (C) 2014 Juan Carlos Maureira
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
#include "IPvXAddressResolver.h"
#include "IInterfaceTable.h"
#include "InterfaceEntry.h"

#include "IPv4InterfaceData.h"

Define_Module(UDPPingApp);

simsignal_t UDPPingApp::packet_sent_signal = registerSignal("PacketSent");

cModule* UDPPingApp::getDestinationModule() {

    ev << "Searching for destination module in " << this->getFullPath() << endl;

    cTopology* topo = new cTopology("udpPingApp");
    // extract topology
    topo->extractByProperty("node");

    ev << "cTopology found " << topo->getNumNodes() << " nodes\n";

    cModule* destModule = NULL;

    for (int i = 0; i < topo->getNumNodes(); i++) {
        if ( topo->getNode(i) == topo->getNodeFor(getParentModule()) ) continue; // skip ourselves

        cModule* mod = topo->getNode(i)->getModule();
        bool is_ip_node = IPvXAddressResolver().findInterfaceTableOf(mod)!=NULL;

        if (is_ip_node) {
            IInterfaceTable *ift = IPvXAddressResolver().interfaceTableOf(mod);
            // search the destination ip
            if (ift!=NULL) {
                for (int k=0; k<ift->getNumInterfaces(); k++) {
                    InterfaceEntry *ie = ift->getInterface(k);

                    for(unsigned int j=0;j<this->destAddresses.size();j++) {

                        if (!ie->isLoopback() & (ie->ipv4Data()!=NULL && ie->ipv4Data()->getIPAddress() == this->destAddresses[j].get4()) ) {
                            // found!!

                            for (cSubModIterator iter(*mod); !iter.end(); iter++)
                            {
                              if (iter()->getModuleType() == cModuleType::get("udpping.UDPPingApp")) {
                                  destModule = iter();
                                  break;
                              }
                            }

                            ev << "Module " << mod << " dest " << destModule <<" IP address: " << ie->ipv4Data()->getIPAddress() << endl;
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
        ev << "Destination Module : " << destModule->getFullName() << endl;
    }

    return destModule;
}


void UDPPingApp::initialize(int stage) {
    UDPBasicApp::initialize(stage);

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
        destAddresses.push_back(IPvXAddressResolver().resolve(token));

    if (destAddresses.empty())
        return;

    this->cOutCounter = NULL;

    // create the output vector for counter arrival registration when selected
    if ((bool)par("registerArrivalCounter") == true) {
        this->cOutCounter = new cOutVector("Counter Arrival");
    }

    // create the vector to track the rtt of packets
    this->cOutRTT= new cOutVector("RTT");

    // watch the bytes received variable
    bytes_received = 0;
    WATCH(bytes_received);


    // subscribe the on-the-way packet signal
    this->destination_module = this->getDestinationModule();

    if (this->destination_module!=NULL) {
        ev << "Subscribing PacketSent signal in " << this->destination_module << endl;

        this->destination_module->subscribe("PacketSent",this);

    } else {
        ev << "Could not determine destination module at initialization time, trying later" << endl;
    }
}

void UDPPingApp::finish()
{
    recordScalar("Bytes Received",this->bytes_received,"Bytes");

    // record a vector with the lost packets
    cOutVector packet_loss_vector("Packet Loss");

    for(PacketRegisterVector::iterator it=this->prv.begin();it!=this->prv.end();it++) {
        packet_loss_vector.recordWithTimestamp((*it).second,(*it).first);
    }
}

cPacket *UDPPingApp::createPacket()
{
    char msgName[32];
    sprintf(msgName,"UDPPing-%ld", counter++);

    UDPPingMsg *message = new UDPPingMsg(msgName);
    message->setByteLength(par("messageLength").longValue());
    message->setCounter(counter);

    if (this->destination_module == NULL) {
        this->destination_module = this->getDestinationModule();

        if (this->destination_module!=NULL) {
            this->destination_module->subscribe("PacketSent",this);
        } else {
            ev << "Could not determine destination module at initialization time, trying later" << endl;
        }
    }

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

        UDPDataIndication *controlInfo = check_and_cast<UDPDataIndication *>(packet->removeControlInfo());

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

        if (this->prv.find(packet->getCounter())!=this->prv.end()) {
            this->prv.erase(packet->getCounter());
        }

        packet->setIsRequest(false);
        socket.sendTo(packet, srcAddr, srcPort);

        numSent++;
    }
    else
    {
        simtime_t rtt = simTime() - packet->getCreationTime();
        ev << "RTT: " << rtt << "\n";
        this->cOutRTT->record(rtt);
        delete msg;
    }
    numReceived++;
}

void UDPPingApp::receiveSignal(cComponent *src, simsignal_t id, long value) {

    ev << "signal arrived from " << src << endl;

    if (id == packet_sent_signal) {
        ev << "[" << this->getFullPath() << "] packet sent signal received from " << src->getFullPath() << ". Sequence Number : " << value << endl;
        this->prv.insert(std::make_pair(value,simTime()));

    }
}

void UDPPingApp::sendPacket() {

    cPacket* pkt = this->createPacket();

    IPvXAddress destAddr = chooseDestAddr();

    ev << "Sending Packet " << pkt << endl;

    emit(packet_sent_signal,counter);

    //emit(sentPkSignal, pkt);

    this->socket.sendTo(pkt, destAddr, destPort);
    numSent++;

}

