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

#ifndef _CLI_M_H_
#define _CLI_M_H_

#include "simpleMachine.h"
#include "sm.h"

class ClientMachine : public SimpleMachine {

//    all of ips for this node.
    std::vector<unsigned int> * ips;

//     all of the offered ips with their ttl.
    std::vector<std::pair<unsigned int, unsigned int> > * offers;

//     to know which ip to invalidate when receiving response_extend frame.
    unsigned int extend_request_ip;

public:
    ClientMachine(SimulatedMachine *, Interface *iface);

    virtual ~ClientMachine();

    virtual void initialize();

    virtual void run();

    virtual void processFrame(Frame frame, int ifaceIndex);

    void process_command(std::string command);

    void send_dhcp_discover(unsigned int time);

    void handle_hdcp_offer(unsigned int ip, unsigned int time);

    void send_dhcp_request(unsigned int ip, unsigned int time);

    void handle_dhcp_ack(unsigned int ip, unsigned int time);

    void send_dhcp_release(unsigned int ip);

    void handle_dhcp_timeout(unsigned int ip);

    void send_request_extend(unsigned int ip, unsigned int time);

    void handle_response_extend(unsigned int ip, unsigned int time);

    bool is_frame_mine(Frame frame);

    void print_all_ips();
};

#endif

