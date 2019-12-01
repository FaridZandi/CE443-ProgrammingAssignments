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

#ifndef _NAT_M_H_
#define _NAT_M_H_

#include <map>
#include <bitset>
#include "simpleMachine.h"
#include "sm.h"
#include <climits>

class NatMachine : public SimpleMachine {
    std::map<std::pair<uint32, uint16>, std::pair<uint32, uint16 >>* outbound_sessions;

    std::map<std::pair<uint32, uint16>, std::pair<uint32, uint16 >>* inbound_sessions;

    std::bitset<USHRT_MAX>* blocked_ports;

public:
    NatMachine(SimulatedMachine *, Interface *iface);

    virtual ~NatMachine();

    virtual void initialize();

    virtual void run();

    virtual void processFrame(Frame frame, int ifaceIndex);

    virtual uint16 getDataLength(byte data_type, uint16 message_length) override;

    void process_command(std::string command);

    void block_port_range(uint16 min, uint16 max);

    void reset_nat(uint16 port);
};

#endif

