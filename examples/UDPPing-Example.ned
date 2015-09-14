//
// Copyright (C) 2012 Juan Carlos Maureira
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.nodes.ethernet.EtherSwitch;
import inet.nodes.inet.StandardHost;
import inet.util.ThruputMeteringChannel;

network UDPPingExample
{
    parameters:
        int numHosts;
        @display("bgb=225,343");
    types:
        channel ethernetline extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        srv: StandardHost {
            @display("p=111,297");
        }
        switch: EtherSwitch {
            parameters:
                @display("p=100,215");
        }
        cli[numHosts]: StandardHost {
            @display("p=111,91");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xml("<config><interface hosts='srv' address='192.168.1.1' netmask='255.255.255.0' /></config>");
                @display("p=30,31");
        }
    connections:
        srv.ethg++ <--> ethernetline <--> switch.ethg++;
        for i=0..numHosts-1 {
            cli[i].ethg++ <--> ethernetline <--> switch.ethg++;
        }
}

