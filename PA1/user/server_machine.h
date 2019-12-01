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

#ifndef _SRV_M_H_
#define _SRV_M_H_

#include "simpleMachine.h"
#include "sm.h"
#include <set>
#include <map>

struct ip_pool_entry {
    bool is_allocated;
    bool is_waiting_for_accept;
    byte mac[6];
    unsigned int ttl;
};

class ServerMachine : public SimpleMachine {
        std::map<unsigned int, ip_pool_entry> *ip_pool;

public:
    ServerMachine(SimulatedMachine *, Interface *iface);

    virtual ~ServerMachine();

    virtual void initialize();

    virtual void run();

    virtual void processFrame(Frame frame, int ifaceIndex);

    void process_command(std::string command);

    void add_to_pool(std::string prefix_and_mask);

    void advance_time(unsigned int time);

    unsigned int get_one_ip(byte* mac, unsigned int ttl);

    void print_all_ips();

    void handle_dhcp_discover(int interface, unsigned int time, byte *mac);

    void send_dhcp_offer(int interface, unsigned int ip, unsigned int time, byte *mac);

    void handle_hdcp_request(int interface, unsigned int ip, unsigned int time, byte *mac);

    void send_dhcp_ack(int interface, unsigned int ip, unsigned int time, byte *mac);

    void handle_hdcp_ack(unsigned int ip, byte *mac);

    void handle_dhcp_release(unsigned int ip);

    void send_dhcp_timeout(unsigned int ip, byte *mac);

    void handle_request_extend(int interface, unsigned int ip, unsigned int time, byte *mac);

    void send_response_extend(int interface, unsigned int ip, unsigned int time, byte *mac);
};

#endif

