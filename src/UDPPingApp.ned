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

package udpping;

import inet.applications.udpapp.UDPApp;

//
// UDP Ping Application..
//
simple UDPPingApp like UDPApp
{
    parameters:
        int localPort = default(5000);
        int destPort = default(5000);
        volatile int messageLength = default(64B) @unit("B"); // length of messages to generate, int bytes
        volatile double messageFreq = default(1s) @unit("s"); // should usually be a random value, e.g. exponential(1)
		volatile int startingTime = default(5s) @unit("s"); // starting time 
        
        string destAddresses = default(""); // list of \IP addresses, separated by spaces
        
        bool registerArrivalCounter = default(false);
        
        @display("i=block/app");
    gates:
        input udpIn @labels(UDPControlInfo/up);
        output udpOut @labels(UDPControlInfo/down);
}
