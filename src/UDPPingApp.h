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

#ifndef __INET_UDPPINGAPP_H
#define __INET_UDPPINGAPP_H

#include <vector>
#include <map>
#include <omnetpp.h>

//#include "NotificationBoard.h"

#include "UDPBasicApp.h"

typedef std::map<long int,simtime_t> PacketRegisterVector;

/**
 * UDP Ping application. See NED for more info.
 */
class UDPPingApp : public UDPBasicApp, public cIListener
{
  private:
    cOutVector*             cOutCounter;
    cOutVector*             cOutRTT;
    long int                bytes_sent;
    long int                bytes_received;
    long int                counter;

    static simsignal_t      packet_sent_signal;

    PacketRegisterVector    prv;

    cModule*                destination_module;

  private:
    virtual cModule* getDestinationModule();

  protected:
    virtual cPacket *createPacket();
    virtual void processPacket(cPacket *msg);
    virtual void sendPacket();

  protected:
    virtual int numInitStages() const { return 5; }
    virtual void initialize(int stage);
    virtual void finish();

    // usable IListerner methods

    virtual void receiveSignal(cComponent *src, simsignal_t id, long value);
    // not usable IListerner methods
    virtual void receiveSignal(cComponent *src, simsignal_t id, cObject *obj) { error("receiveSingal: object not supported"); }
    virtual void receiveSignal(cComponent *src, simsignal_t id, long unsigned int value) { error("receiveSingal: long unsigned int supported"); }
    virtual void receiveSignal(cComponent *src, simsignal_t id, double value) { error("receiveSingal: double not supported"); }
    virtual void receiveSignal(cComponent *src, simsignal_t id, const SimTime& st) { error("receiveSingal: simtime_t not supported"); }
    virtual void receiveSignal(cComponent *src, simsignal_t id, const char* value) { error("receiveSingal: const char* not supported"); }
    virtual void receiveSignal(cComponent *src, simsignal_t id, bool value) { error("receiveSingal: bool not supported"); }
};

#endif
