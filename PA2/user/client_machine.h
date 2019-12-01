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

#include <map>
#include "simpleMachine.h"
#include "sm.h"


struct peer_data2 {
    uint32 local_ip;
    uint16 local_port;
    uint32 public_ip;
    uint16 public_port;

    bool ping_sent;
    bool pong_received;

    bool connected_locally;
};

class ClientMachine : public SimpleMachine {

    static const uint32 server_ip = 16843009;

    static const uint16 server_port = 1234;

    uint16 local_port;

    int my_id;

    std::map<int, peer_data2> *peers_data;

    int last_reached_id;

public:
    ClientMachine(SimulatedMachine *, Interface *iface);

    virtual ~ClientMachine();

    virtual void initialize();

    virtual void run();

    virtual void processFrame(Frame frame, int ifaceIndex);

    void process_command(std::string command);

    void send_request_assigning_id(uint16 port);

    void handle_drop_frame(Frame frame);

    void handle_response_assigning_id_frame(Frame frame);

    void send_request_getting_ip(int id);

    void handle_response_getting_ip_frame(Frame frame);

    void send_request_local_session(int id);

    void send_request_public_session(int id);

    void handle_request_session_frame(Frame frame);

    void handle_response_session_frame(Frame frame);

    void send_message(int id, std::string message);

    void handle_message_frame(Frame frame);

    void handle_nat_updated_frame(Frame frame);

    void send_status();

    void handle_status_respond(Frame frame);

    void send_request_updating_info(uint16 port);

    uint16 getDataLength(byte data_type, uint16 message_length);
};

#endif

