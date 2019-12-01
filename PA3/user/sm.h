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

#ifndef _S_M_H_
#define _S_M_H_

#include "machine.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <frame.h>
#include <thread>
#include <map>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <mutex>

using namespace std;
using namespace std::chrono;

enum BGP_TYPE{
	OPEN = 1,
	UPDATE = 2,
	NOTIFICATION = 3,
	KEEPALIVE = 4
};

enum NOTIFICATION_ERROR_CODE{
	RESERVED = 0,
	MESSAGE_HEADER_ERROR = 1,
	OPEN_MESSAGE_ERROR = 2,
	UDPATE_MESSAGE_ERROR = 3,
	HOLD_TIME_EXPIRED = 4,
	FINITE_STATE_MACHINE_ERROR = 5,
	CEASE = 6
};

enum bgp_state{
    IDLE_STATE = 0,
    CONNECT_STATE = 1,
    ACTIVE_STATE = 2,
    OPEN_STATE = 3,
    OPEN_CONFIRM_STATE = 4,
    ESTABLISHED_STATE = 5,
};

enum peering_mode{
    CUSTOMER,
    PROVIDER,
    PEER
};

struct prefix_info{
    uint32 subnet;
    short mask;
};

struct prefix_route_info{
    int priority;
    int interface_index;
    std::vector<int> path_vector;
};

struct interface_info{
    peering_mode mode;
    uint32 peer_ip;
};

struct bgp_header{
    byte marker[16];
    uint16 length;
    byte type;
} __attribute__ ((packed));


struct bgp_open_message{
    byte version;
    uint16 my_autonomous_system;
    uint16 hold_time;
    uint32 bgp_identifier;
    byte opt_param_length;
} __attribute__ ((packed));


class SimulatedMachine : public Machine {
	uint32 as_num;

    std::thread ** interface_handler_threads;
    bgp_state * interface_bgp_states;
    interface_info * interface_infos;

    std::mutex* thread_locks;

    bool * connect_retry_timer_active;
	time_t * connect_retry_time_left;

    bool * hold_timer_active;
	time_t * hold_timer_time_left;

    bool * keep_alive_timer_active;
	time_t * keep_alive_time_left;

    std::vector<prefix_info> owned_prefixes;

    std::map<prefix_info, std::vector<prefix_route_info> > learned_routes;

	static const int CONNECT_RETRY_TIME = 30;
	static const int HOLD_TIMER_TIME = 120;

    static const int CUSTOMER_LEARNED_PRIORITY = 100;
    static const int PEER_LEARNED_PRIORITY = 90;
    static const int PROVIDER_LEARNED_PRIORITY = 80;

public:
	SimulatedMachine (const ClientFramework *cf, int count);
	virtual ~SimulatedMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame(Frame frame, int ifaceIndex);
	static void parseArguments (int argc, char *argv[]);

    void interface_handler(int interface_index);

	void process_command(string command);

	void startConnection(int iface_index);

    void advertiseAll();

    void printRoutesToPrefix(uint32 subnet, int mask);

    void updateInterfacePriority(int interface_index, int priority);

    void hijackPrefix(uint32 subnet, int mask);

    void withdraw(uint32 subnet, int mask);

    void handle_tcp_syn_frame(Frame frame, int iface_index);

    void handle_open_frame(Frame frame, int iface_index);

    void handle_update_frame(Frame frame, int iface_index);

    void handle_notification_frame(Frame frame, int iface_index);

    void handle_keepalive_frame(Frame frame, int iface_index);

    void fill_ethernet_ip_header(byte *frame_date, uint32 dest_ip, uint32 src_ip, int interface_index);

    uint32 extract_as_num();

    void fill_interface_info(uint32 as_num);

    std::string getBGPStateName(bgp_state bgps);

    static prefix_info prefix_string_to_int(std::string string_prefix);

    static std::string prefix_int_to_string(prefix_info int_prefix);

    std::vector<string> split(string s, char delim);

	void printStateChangeMessage(bgp_state initial_state, bgp_state dest_state, int interface_index);
};

#endif /* sm.h */

