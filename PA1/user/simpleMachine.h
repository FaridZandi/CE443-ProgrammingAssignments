//                   In the name of GOD
/**
 * Partov is a simulation engine, supporting emulation as well,
 * making it possible to create virtual networks.
 *  
 * Copyright © 2009-2015 Behnam Momeni.
 * 
 * This file is part of the Partov.
 * 
 * Partov is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Partov is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Partov.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */

#ifndef _SI_M_H_
#define _SI_M_H_

#include "sm.h"
#include "../base/frame.h"
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <netinet/ip.h>


struct ethernet_header {
    byte dst[6];
    byte src[6];
    uint16 type;
} __attribute__ ((packed));

struct data_frame {
    byte type;
    byte mac[6];
    byte ip[4];
    byte time[4];
} __attribute__ ((packed));


class SimpleMachine {
private:
    const SimulatedMachine *simulatedMachine;

protected:
    Interface *iface;

public:
    SimpleMachine(const SimulatedMachine *simulatedMachine, Interface *iface);

    virtual ~SimpleMachine();

    virtual void initialize() =0;

    virtual void run() = 0;

    virtual void processFrame(Frame frame, int ifaceIndex) = 0;

    int getCountOfInterfaces() const;

    void printInterfacesInformation() const;

    const std::string getCustomInformation();

    bool sendFrame(Frame frame, int ifaceIndex);

    void broadcast(Frame frame, int exception = -1, bool set_sender = true);

    static std::vector<std::string> split(const std::string &s, char delim);

    static unsigned int ip_string_to_int(std::string string_ip);

    static std::string ip_int_to_string(unsigned int int_ip);

    static byte *int_to_byte_network_order(unsigned int value);

    static unsigned int byte_network_order_to_int(byte *value);

    static std::string mac_byte_to_string(byte* mac);

    Frame populate_frame(byte data_type, byte *mac,
                         unsigned int ip, unsigned int time);

    void set_sender_mac(Frame& frame, int interface);
};

#endif /* sm.h */

