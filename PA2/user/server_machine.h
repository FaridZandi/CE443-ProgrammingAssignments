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

struct peer_data{
    uint32 local_ip;
    uint16 local_port;
    uint32 public_ip;
    uint16 public_port;

    bool valid;
};

class ServerMachine : public SimpleMachine {
    peer_data peers_data[32];

    int current_id_index;

    static const uint32 server_ip = 16843009;

    static const uint16 server_port = 1234;


public:
    ServerMachine(SimulatedMachine *, Interface *iface);

    virtual ~ServerMachine();

    virtual void initialize();

    virtual void run();

    virtual void processFrame(Frame frame, int ifaceIndex);

    void handle_request_assigning_id(Frame frame);

    void send_response_assigning_id(int id, uint32 ip, uint16 port);

    void handle_request_getting_ip(Frame frame);

    void send_response_getting_ip(int dest_id, uint32 dest_ip, uint16 dest_port, uint32 local_ip, uint16 local_port,
                                  uint32 public_ip, uint16 public_port);

    void handle_request_updating_info(Frame frame);

    void handle_status(Frame frame);

    void send_respond_status(byte flag, uint32 ip, uint16 port);

    virtual uint16 getDataLength(byte data_type, uint16 message_length) override;
};

#endif

