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

#include "sm.h"

#include "interface.h"
#include "frame.h"
#include "sr_protocol.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <unistd.h>
#include <queue>

using namespace std;


SimulatedMachine::SimulatedMachine(const ClientFramework *cf, int count) :
        Machine(cf, count) {
    // The machine instantiated.
    // Interfaces are not valid at this point.

}

SimulatedMachine::~SimulatedMachine() {
    // destructor...
}


void SimulatedMachine::interface_handler(int i) {

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        time_t now = std::time(0);

        thread_locks[i].lock();

        if (connect_retry_timer_active[i]) {
            if (now > connect_retry_time_left[i]) { // connect retry timer timeout.

                switch (interface_bgp_states[i]) {
                    case IDLE_STATE: {

                        break;
                    }
                    case CONNECT_STATE: {

                        interface_bgp_states[i] = ACTIVE_STATE;
                        printStateChangeMessage(CONNECT_STATE, ACTIVE_STATE, i);

                        connect_retry_time_left[i] = std::time(0) + CONNECT_RETRY_TIME;
                        connect_retry_timer_active[i] = true;

                        break;
                    }
                    case ACTIVE_STATE: {
                        byte packet[54];
                        memset(packet, 0, 54);

                        uint32 dest_ip = interface_infos[i].peer_ip;
                        uint32 src_ip = iface[i].getIp();
                        fill_ethernet_ip_header(packet, dest_ip, src_ip, 20);

                        sr_tcp *tcp_hdr = (sr_tcp *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

                        tcp_hdr->tcp_doff = 5;
                        tcp_hdr->tcp_flags = TCP_SYN;

                        sendFrame(Frame(54, packet), i);

                        interface_bgp_states[i] = CONNECT_STATE;
                        printStateChangeMessage(ACTIVE_STATE, CONNECT_STATE, i);

                        connect_retry_time_left[i] = std::time(0) + CONNECT_RETRY_TIME;
                        connect_retry_timer_active[i] = true;

                        break;
                    }

                    default:
                        break;
                }

            }
        }


        if (hold_timer_active[i]) {
            if (now > hold_timer_time_left[i]) {
                switch (interface_bgp_states[i]) {
                    case OPEN_STATE:
                    case OPEN_CONFIRM_STATE: {

                        byte packet[54];
                        memset(packet, 0, 54);

                        uint32 dest_ip = interface_infos[i].peer_ip;
                        uint32 src_ip = iface[i].getIp();
                        fill_ethernet_ip_header(packet, dest_ip, src_ip, 20);

                        bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

                        memset(&(*bgpHeader->marker), 255, 16);
                        bgpHeader->type = NOTIFICATION;
                        bgpHeader->length = htons(20);

                        packet[54] = HOLD_TIME_EXPIRED;

                        sendFrame(Frame(54, packet), i);

                        hold_timer_active[i] = false;

                        printStateChangeMessage(interface_bgp_states[i], IDLE_STATE, i);
                        interface_bgp_states[i] = IDLE_STATE;

                        break;
                    }

                    default:
                        break;
                }
            }
        }

        thread_locks[i].unlock();
    }
}

void SimulatedMachine::initialize() {
    // filling the needed info about this node and it's neighbours.
    as_num = extract_as_num();
    fill_interface_info(as_num);

    // initializing all states and setting all to IDLE.
    interface_bgp_states = new bgp_state[getCountOfInterfaces()];
    for (int i = 0; i < getCountOfInterfaces(); ++i) {
        interface_bgp_states[i] = IDLE_STATE;
    }

    //initializing all timers.
    connect_retry_timer_active = new bool[getCountOfInterfaces()];
    connect_retry_time_left = new time_t[getCountOfInterfaces()];
    hold_timer_active = new bool[getCountOfInterfaces()];
    hold_timer_time_left = new time_t[getCountOfInterfaces()];
    keep_alive_timer_active = new bool[getCountOfInterfaces()];
    keep_alive_time_left = new time_t[getCountOfInterfaces()];
    for (int k = 0; k < getCountOfInterfaces(); ++k) {
        connect_retry_timer_active[k] = false;
        hold_timer_active[k] = false;
        keep_alive_timer_active[k] = false;
    }

    // starting a handler thread for each interface,
    // and a lock for synchronization.
    thread_locks = new std::mutex[getCountOfInterfaces()];
    interface_handler_threads = new thread *[getCountOfInterfaces()];
    for (int j = 0; j < getCountOfInterfaces(); ++j) {
        interface_handler_threads[j] = new thread(&SimulatedMachine::interface_handler, this, j);
    }
}

/**
 * This method is called from the main thread.
 * Also ownership of the data of the frame is not with you.
 * If you need it, make a copy for yourself.
 *
 * You can also send frames using:
 * <code>
 *     bool synchronized sendFrame (Frame frame, int ifaceIndex) const;
 * </code>
 * which accepts one frame and the interface index (counting from 0) and
 * sends the frame on that interface.
 * The Frame class used here, encapsulates any kind of network frame.
 * <code>
 *     class Frame {
 *     public:
 *       uint32 length;
 *       byte *data;
 *
 *       Frame (uint32 _length, byte *_data);
 *       virtual ~Frame ();
 *     };
 * </code>
 */
void SimulatedMachine::processFrame(Frame frame, int ifaceIndex) {
    cerr << "Frame received at iface " << ifaceIndex <<
         " with length " << frame.length << endl;
//    printFrame(frame);

    byte *ip_payload = frame.data + sizeof(ip) + sizeof(sr_ethernet_hdr);

    bool is_bgp = true;
    for (int i = 0; i < 16; ++i) {
        if (ip_payload[i] != 255) {
            is_bgp = false;
        }
    }

    if (is_bgp) {
        bgp_header *bgpHeader = (bgp_header *) ip_payload;
        switch (bgpHeader->type) {
            case OPEN: {
                handle_open_frame(frame, ifaceIndex);
                break;
            }
            case UPDATE: {
                handle_update_frame(frame, ifaceIndex);
                break;
            }
            case NOTIFICATION: {
                handle_notification_frame(frame, ifaceIndex);
                break;
            }
            case KEEPALIVE: {
                handle_keepalive_frame(frame, ifaceIndex);
                break;
            }
            default:
                break;
        }
    } else {
        handle_tcp_syn_frame(frame, ifaceIndex);
    }
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void SimulatedMachine::run() {
    std::string command;

    while (true) {
        std::getline(std::cin, command);
        process_command(command);
    }
}


/**
 * You could ignore this method if you are not interested on custom arguments.
 */
void SimulatedMachine::parseArguments(int argc, char *argv[]) {
    // TODO: parse arguments which are passed via --args
}

void SimulatedMachine::process_command(string command) {

//    std::cout << "entered command : " << command << std::endl;

    std::vector<std::string> splitted = split(command, ' ');

    if (splitted[0] == "start" and splitted[1] == "connection" and splitted[2] == "on" and splitted[3] == "interface") {
        int iface_index = std::stoi(splitted[4]);
        startConnection(iface_index);
    } else if (splitted[0] == "advertise" and splitted[1] == "all") {
        advertiseAll();
    } else if (splitted[0] == "print" and splitted[1] == "routes" and splitted[2] == "to") {
        prefix_info info = prefix_string_to_int(splitted[3]);
        printRoutesToPrefix(info.subnet, info.mask);
    } else if (splitted[0] == "priority" and splitted[1] == "of" and splitted[3] == "is") {
        int iface_index = std::stoi(splitted[2]);
        int priority = std::stoi(splitted[4]);
        updateInterfacePriority(iface_index, priority);
    } else if (splitted[0] == "hijack") {
        prefix_info info = prefix_string_to_int(splitted[1]);
        hijackPrefix(info.subnet, info.mask);
    } else if (splitted[0] == "withdraw") {
        prefix_info info = prefix_string_to_int(splitted[1]);
        withdraw(info.subnet, info.mask);
    } else {
        // now what.
    }
}

std::vector<string> SimulatedMachine::split(string s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

std::string SimulatedMachine::getBGPStateName(bgp_state bgps) {
    switch (bgps) {
        case IDLE_STATE:
            return "IDLE";
        case CONNECT_STATE:
            return "CONNECT";
        case ACTIVE_STATE:
            return "ACTIVESTATE";
        case OPEN_STATE:
            return "OPENSTATE";
        case OPEN_CONFIRM_STATE:
            return "OPENCONFIRM";
        case ESTABLISHED_STATE:
            return "ESTABLISHED";
        default:
            return "whaaawt?";
    }
}

std::string SimulatedMachine::prefix_int_to_string(prefix_info int_prefix) {
    std::stringstream ss;
    uint32 subnet = int_prefix.subnet;
    ss << subnet / (1 << 24) << ".";
    subnet %= (1 << 24);
    ss << subnet / (1 << 16) << ".";
    subnet %= (1 << 16);
    ss << subnet / (1 << 8) << ".";
    subnet %= (1 << 8);
    ss << subnet;

    ss << "/" << int_prefix.mask;
    return ss.str();
}

prefix_info SimulatedMachine::prefix_string_to_int(std::string string_prefix) {
    unsigned int slash_index = (int) string_prefix.find_first_of('/');
    short mask = (short) std::stoi(string_prefix.substr(slash_index + 1));

    std::string subnet_string = string_prefix.substr(0, slash_index);
    std::stringstream s(subnet_string);
    char ch;
    unsigned int a, b, c, d;
    s >> a >> ch >> b >> ch >> c >> ch >> d;
    uint32 subnet = (a << 24) + (b << 16) + (c << 8) + d;

    return prefix_info{subnet, mask};
}

void SimulatedMachine::startConnection(int iface_index) {
    if (interface_bgp_states[iface_index] != IDLE_STATE) {
        return;
    }

    byte packet[54];
    memset(packet, 0, 54);

    uint32 dest_ip = interface_infos[iface_index].peer_ip;
    uint32 src_ip = iface[iface_index].getIp();
    fill_ethernet_ip_header(packet, dest_ip, src_ip, 20);

    sr_tcp *tcp_hdr = (sr_tcp *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

    tcp_hdr->tcp_doff = 5;
    tcp_hdr->tcp_flags = TCP_SYN;

    sendFrame(Frame(54, packet), iface_index);

    thread_locks[iface_index].lock();

    interface_bgp_states[iface_index] = CONNECT_STATE;
    printStateChangeMessage(IDLE_STATE, CONNECT_STATE, iface_index);

    connect_retry_time_left[iface_index] = std::time(0) + CONNECT_RETRY_TIME;
    connect_retry_timer_active[iface_index] = true;

    thread_locks[iface_index].unlock();
}

uint32 SimulatedMachine::extract_as_num() {
    string custom_info = getCustomInformation();

    std::stringstream ss(custom_info);
    std::string line;

    // get second line.
    getline(ss, line, '\n');
    getline(ss, line, '\n');

    std::string as_num_string = line.substr(2);
    return (uint32) std::stoi(as_num_string);
}

void SimulatedMachine::fill_interface_info(uint32 as_num) {

    interface_infos = new interface_info[getCountOfInterfaces()];

    switch (as_num) {
        case 0: {
            interface_infos[0] = {PROVIDER, prefix_string_to_int("1.1.1.0/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("6.0.0.0/8"));

            break;
        }
        case 1: {
            interface_infos[0] = {CUSTOMER, prefix_string_to_int("6.6.6.0/32").subnet};
            interface_infos[1] = {PEER, prefix_string_to_int("2.2.2.0/32").subnet};
            interface_infos[2] = {PEER, prefix_string_to_int("3.3.3.0/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("1.0.0.0/8"));

            break;
        }
        case 2: {
            interface_infos[0] = {PEER, prefix_string_to_int("1.1.1.1/32").subnet};
            interface_infos[1] = {PEER, prefix_string_to_int("3.3.3.1/32").subnet};
            interface_infos[2] = {CUSTOMER, prefix_string_to_int("4.4.4.1/32").subnet};
            interface_infos[3] = {CUSTOMER, prefix_string_to_int("5.5.5.0/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("2.0.0.0/8"));
            owned_prefixes.push_back(prefix_string_to_int("22.0.0.0/8"));
            owned_prefixes.push_back(prefix_string_to_int("222.0.0.0/8"));

            break;
        }
        case 3: {
            interface_infos[0] = {PEER, prefix_string_to_int("1.1.1.2/32").subnet};
            interface_infos[1] = {PEER, prefix_string_to_int("2.2.2.1/32").subnet};
            interface_infos[2] = {CUSTOMER, prefix_string_to_int("4.4.4.0/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("3.0.0.0/8"));
            owned_prefixes.push_back(prefix_string_to_int("33.0.0.0/8"));

            break;
        }
        case 4: {
            interface_infos[0] = {PROVIDER, prefix_string_to_int("3.3.3.2/32").subnet};
            interface_infos[1] = {PROVIDER, prefix_string_to_int("2.2.2.2/32").subnet};
            interface_infos[2] = {CUSTOMER, prefix_string_to_int("5.5.5.1/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("4.0.0.0/8"));

            break;
        }
        case 5: {
            interface_infos[0] = {PROVIDER, prefix_string_to_int("2.2.2.3/32").subnet};
            interface_infos[1] = {PROVIDER, prefix_string_to_int("4.4.4.2/32").subnet};

            owned_prefixes.push_back(prefix_string_to_int("5.0.0.0/8"));

            break;
        }
        default:
            break;
    }
}

void SimulatedMachine::advertiseAll() {
    for (int i = 0; i < getCountOfInterfaces(); ++i) {
        if (interface_bgp_states[i] == ESTABLISHED_STATE) {
            std::vector<prefix_info> prefix_infos;
            std::vector<std::vector<int>> paths;

            peering_mode mode = interface_infos[i].mode;

            for (auto it = learned_routes.begin(); it != learned_routes.end(); it++) {

                prefix_info prefix = it->first;

                for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {

                    int this_path_interface_index = it2->interface_index;
                    peering_mode this_path_mode = interface_infos[this_path_interface_index].mode;

                    bool should_advertise = false;

                    if (mode == CUSTOMER) {
                        should_advertise = true;
                    } else if (mode == PROVIDER) {
                        if (this_path_mode != PROVIDER) {
                            should_advertise = true;
                        } else {
                            if (this_path_interface_index == i) {
                                should_advertise = true;
                            }
                        }
                    } else {
                        if (this_path_mode != PEER) {
                            should_advertise = true;
                        } else {
                            if (this_path_interface_index == i) {
                                should_advertise = true;
                            }
                        }
                    }

                    if (should_advertise) {
                        prefix_infos.push_back(prefix);

                        // add myself to the beginning of the path vector.
                        std::vector<int> this_path_vector = it2->path_vector;
                        paths.push_back(this_path_vector);
                    }
                }
            }

            for (auto it3 = owned_prefixes.begin(); it3 != owned_prefixes.end(); it3++) {
                std::vector<int> this_path_vector;
                this_path_vector.push_back(as_num);
                paths.push_back(this_path_vector);

                prefix_infos.push_back(*it3);
            }

            int update_message_length = 4 + (int) (paths.size()) * 7;

            for (auto it4 = paths.begin(); it4 != paths.end(); it4++) {
                update_message_length += (2 * it4->size());
            }

            uint16 packet_length = sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header) + update_message_length;
            uint16 ip_payload_length = sizeof(bgp_header) + update_message_length;

            byte *packet = new byte[packet_length];
            memset(packet, 0, (size_t) packet_length);

            uint32 dest_ip = interface_infos[i].peer_ip;
            uint32 src_ip = iface[i].getIp();
            fill_ethernet_ip_header(packet, dest_ip, src_ip, ip_payload_length);

            bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

            memset(&(bgpHeader->marker), 255, 16);
            bgpHeader->type = UPDATE;
            bgpHeader->length = htons(ip_payload_length);

            byte *update_message = (packet + sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header));

            *(uint16 *) (update_message) = 0;
            *(uint16 *) (update_message + 2) = htons((uint16) paths.size());

            int cursor = 4;
            for (auto it4 = paths.begin(); it4 != paths.end(); it4++) {
                *(uint16 *) (update_message + cursor) = htons((uint16) it4->size());
                cursor += 2;

                for (auto it5 = it4->begin(); it5 != it4->end(); it5++) {
                    *(uint16 *) (update_message + cursor) = htons((uint16) *it5);
                    cursor += 2;
                }
            }

            for (auto it4 = prefix_infos.begin(); it4 != prefix_infos.end(); it4++) {
                *(uint32 *) (update_message + cursor) = htonl(it4->subnet);
                cursor += 4;
                *(update_message + cursor) = (byte) it4->mask;
                cursor += 1;
            }

            sendFrame(Frame(packet_length, packet), i);
        }
    }
}

bool operator<(const prefix_info &T1, const prefix_info &T2) {
    if(T1.subnet < T2.subnet) {
        return true;
    } else if(T1.subnet == T2.subnet){
        return T1.mask < T2.mask;
    } else {
        return false;
    }
}

void SimulatedMachine::printRoutesToPrefix(uint32 subnet, int mask) {

    std::priority_queue<std::pair<double, std::vector<int>>> sorted_paths;

    prefix_info prefix{subnet, (short) mask};

    if (learned_routes.find(prefix) == learned_routes.end() or learned_routes[prefix].size() == 0) {
        std::cout << "no routes found for " << prefix_int_to_string(prefix) << std::endl;
        return;
    }

    auto paths = learned_routes[prefix];
    for (auto it = paths.begin(); it != paths.end(); it++) {

        int AS_num_after_neighbour = 0;

        if (it->path_vector.size() > 1) {
            AS_num_after_neighbour = it->path_vector[1];
        }

        double score = 1e9 * it->priority +
                       1e6 * (1000 - it->path_vector.size()) +
                       1e3 * (1000 - it->interface_index) +
                       1 * (1000 - AS_num_after_neighbour);

        sorted_paths.push(std::make_pair(score, it->path_vector));
    }

    while (not sorted_paths.empty()) {
        auto current_path = sorted_paths.top().second;

        for (auto it = current_path.begin(); it != current_path.end(); it++) {
            std::cout << *it << " ";
        }

        std::cout << prefix_int_to_string(prefix) << std::endl;

        sorted_paths.pop();
    }

}

void SimulatedMachine::updateInterfacePriority(int interface_index, int priority) {
    for (auto it = learned_routes.begin(); it != learned_routes.end(); it++) {
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            if(it2->interface_index == interface_index){
                it2->priority = priority;
            }
        }
    }
}

void SimulatedMachine::hijackPrefix(uint32 subnet, int mask) {

    for (int i = 0; i < getCountOfInterfaces(); ++i) {
        uint16 update_message_length = 2 + 2 + 2 + 2 + 5;

        uint16 packet_length = sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header) + update_message_length;
        uint16 ip_payload_length = sizeof(bgp_header) + update_message_length;

        byte *packet = new byte[packet_length];
        memset(packet, 0, (size_t) packet_length);

        uint32 dest_ip = interface_infos[i].peer_ip;
        uint32 src_ip = iface[i].getIp();
        fill_ethernet_ip_header(packet, dest_ip, src_ip, ip_payload_length);

        bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

        memset(&(bgpHeader->marker), 255, 16);
        bgpHeader->type = UPDATE;
        bgpHeader->length = htons(ip_payload_length);

        byte *update_message = (packet + sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header));

        *(uint16 *) (update_message) = 0;
        *(uint16 *) (update_message + 2) = htons(1);
        *(uint16 *) (update_message + 4) = htons(1);
        *(uint16 *) (update_message + 6) = htons((uint16) as_num);
        *(uint32 *) (update_message + 8) = htonl(subnet);
        *(update_message + 12) = (byte) mask;

        sendFrame(Frame(packet_length, packet), i);
    }
}

void SimulatedMachine::withdraw(uint32 subnet, int mask) {
    for (uint32 j = 0; j < owned_prefixes.size(); ++j) {
        if(owned_prefixes[j].subnet == subnet and owned_prefixes[j].mask == mask){
            owned_prefixes.erase(owned_prefixes.begin() + j);
            break;
        }
    }

    prefix_info withdrawn_prefix{subnet, (short) mask};
    learned_routes.erase(withdrawn_prefix);

    for (int i = 0; i < getCountOfInterfaces(); ++i) {
        uint16 update_message_length = 2 + 2 + 5;

        uint16 packet_length = sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header) + update_message_length;
        uint16 ip_payload_length = sizeof(bgp_header) + update_message_length;

        byte *packet = new byte[packet_length];
        memset(packet, 0, (size_t) packet_length);

        uint32 dest_ip = interface_infos[i].peer_ip;
        uint32 src_ip = iface[i].getIp();
        fill_ethernet_ip_header(packet, dest_ip, src_ip, ip_payload_length);

        bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

        memset(&(bgpHeader->marker), 255, 16);
        bgpHeader->type = UPDATE;
        bgpHeader->length = htons(ip_payload_length);

        byte *update_message = (packet + sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header));

        *(uint16 *) (update_message) = htons(1);
        *(uint32 *) (update_message + 2) = htonl(subnet);
        *(update_message + 6) = (byte) mask;
        *(uint16 *) (update_message + 7) = 0;

        sendFrame(Frame(packet_length, packet), i);
    }
}

void SimulatedMachine::handle_tcp_syn_frame(Frame frame, int iface_index) {
    if (interface_bgp_states[iface_index] != CONNECT_STATE and interface_bgp_states[iface_index] != ACTIVE_STATE) {
        return;
    }

    sr_tcp *received_tcp_header = (sr_tcp *) (frame.data + sizeof(ip) + sizeof(sr_ethernet_hdr));

    bool is_syn = ((received_tcp_header->tcp_flags & TCP_SYN) != 0);
    bool is_ack = ((received_tcp_header->tcp_flags & TCP_ACK) != 0);

    if (is_syn) { // send ack back
        byte packet[54];
        memset(packet, 0, 54);

        uint32 dest_ip = interface_infos[iface_index].peer_ip;
        uint32 src_ip = iface[iface_index].getIp();
        fill_ethernet_ip_header(packet, dest_ip, src_ip, 20);

        sr_tcp *tcp_hdr = (sr_tcp *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

        tcp_hdr->tcp_doff = 5;

        if (is_ack) {
            tcp_hdr->tcp_flags = TCP_ACK;
        } else {
            tcp_hdr->tcp_flags = TCP_ACK | TCP_SYN;
        }

        sendFrame(Frame(54, packet), iface_index);
    }

    if (is_ack) { // tcp handshake completed. send open message.

        byte packet[63];
        memset(packet, 0, 63);

        uint32 dest_ip = interface_infos[iface_index].peer_ip;
        uint32 src_ip = iface[iface_index].getIp();
        fill_ethernet_ip_header(packet, dest_ip, src_ip, 29);

        bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

        memset(&(bgpHeader->marker), 255, 16);
        bgpHeader->type = OPEN;
        bgpHeader->length = htons(29);

        bgp_open_message *bgpOpenMessage = (bgp_open_message *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip) +
                                                                 sizeof(bgp_header));

        bgpOpenMessage->version = 4;
        bgpOpenMessage->my_autonomous_system = htons((uint16) as_num);
        bgpOpenMessage->hold_time = 0;
        bgpOpenMessage->bgp_identifier = htonl(iface[iface_index].getIp());
        bgpOpenMessage->opt_param_length = 0;

        sendFrame(Frame(63, packet), iface_index);

        thread_locks[iface_index].lock();

        connect_retry_timer_active[iface_index] = false;

        hold_timer_time_left[iface_index] = std::time(0) + HOLD_TIMER_TIME;
        hold_timer_active[iface_index] = true;

        interface_bgp_states[iface_index] = OPEN_STATE;
        printStateChangeMessage(CONNECT_STATE, OPEN_STATE, iface_index);

        thread_locks[iface_index].unlock();
    }
}


void SimulatedMachine::handle_open_frame(Frame frame, int iface_index) {
    if (interface_bgp_states[iface_index] != OPEN_STATE) {
        return;
    }

    bgp_open_message *receivedBgpOpenMessage = (bgp_open_message *) (frame.data + sizeof(sr_ethernet_hdr) + sizeof(ip) +
                                                                     sizeof(bgp_header));


    bool is_bgp_ok = true;
    if (receivedBgpOpenMessage->opt_param_length != 0) { is_bgp_ok = false; }
    if (receivedBgpOpenMessage->version != 4) { is_bgp_ok = false; }
    if (receivedBgpOpenMessage->hold_time != 0) { is_bgp_ok = false; }

    if (not is_bgp_ok) {
        // send notification

        std::cerr << "open not ok" << std::endl; // TODO: but with what error code ?

        return;
    }

    byte packet[53];
    memset(packet, 0, 53);

    uint32 dest_ip = interface_infos[iface_index].peer_ip;
    uint32 src_ip = iface[iface_index].getIp();
    fill_ethernet_ip_header(packet, dest_ip, src_ip, 19);

    bgp_header *bgpHeader = (bgp_header *) (packet + sizeof(sr_ethernet_hdr) + sizeof(ip));

    memset(&(bgpHeader->marker), 255, 16);
    bgpHeader->type = KEEPALIVE;
    bgpHeader->length = htons(19);

    sendFrame(Frame(53, packet), iface_index);

    thread_locks[iface_index].lock();

    hold_timer_active[iface_index] = true;
    hold_timer_time_left[iface_index] = std::time(0) + HOLD_TIMER_TIME;

    interface_bgp_states[iface_index] = OPEN_CONFIRM_STATE;
    printStateChangeMessage(OPEN_STATE, OPEN_CONFIRM_STATE, iface_index);

    thread_locks[iface_index].unlock();
}

void SimulatedMachine::handle_update_frame(Frame frame, int iface_index) {
    byte *update_message = (frame.data + sizeof(sr_ethernet_hdr) + sizeof(ip) + sizeof(bgp_header));

    uint16 withdrawn_routes_length = ntohs(*(uint16 *) (update_message));
    uint16 total_path_attributes_length = ntohs(*(uint16 *) (update_message + 2 + 5 * withdrawn_routes_length));

    if(withdrawn_routes_length > 0){
        int cursor = 2;

        for (int i = 0; i < withdrawn_routes_length; ++i) {
            uint32 subnet = htonl(*(uint32 *) (update_message + cursor));
            cursor += 4;
            byte mask = *(update_message + 6);
            cursor += 1;

            prefix_info withdrawn_prefix{subnet, mask};

            auto paths = learned_routes[withdrawn_prefix];
            vector<prefix_route_info> new_paths;
            for (uint32 j = 0; j < paths.size(); ++j) {
                if(paths[j].interface_index != iface_index){
                    new_paths.push_back(paths[j]);
                }
            }
            learned_routes[withdrawn_prefix] = new_paths;
        }


        for (int j = 0; j < getCountOfInterfaces(); ++j) {
            if(j == iface_index){
                continue;
            }

            byte* packet = new byte[frame.length];
            memcpy(packet, frame.data, frame.length);

            uint32 dest_ip = interface_infos[j].peer_ip;
            uint32 src_ip = iface[j].getIp();

            int ip_payload_length = frame.length - sizeof(sr_ethernet_hdr) - sizeof(ip);

            fill_ethernet_ip_header(packet, dest_ip, src_ip, ip_payload_length);

            sendFrame(Frame(frame.length, packet), j);
        }

    } if(total_path_attributes_length > 0){

        std::vector<prefix_info> prefix_infos;
        std::vector<std::vector<int>> paths;

        uint16 cursor = (uint16) (2 + 5 * withdrawn_routes_length + 2);

        for (int i = 0; i < total_path_attributes_length; ++i) {
            uint16 path_length = ntohs(*(uint16 *) (update_message + cursor));
            cursor += 2;

            std::vector<int> path_vector;
            for (int j = 0; j < path_length; ++j) {
                uint16 current_as_num = ntohs(*(uint16 *) (update_message + cursor));
                cursor += 2;
                path_vector.push_back(current_as_num);
            }

            paths.push_back(path_vector);
        }

        for (int i = 0; i < total_path_attributes_length; ++i) {
            uint32 current_subnet = ntohl(*(uint32*) (update_message + cursor));
            cursor += 4;
            byte current_mask = *(update_message + cursor);
            cursor += 1;

            prefix_infos.push_back(prefix_info{current_subnet, current_mask});
        }

        for (uint32 k = 0; k < paths.size(); ++k) {
            int claimed_owner = paths[k][paths[k].size() - 1];

            if(learned_routes.find(prefix_infos[k]) == learned_routes.end()){
                continue;
            }

            std::vector<int> current_path = learned_routes[prefix_infos[k]][0].path_vector;

            int current_owner = current_path[current_path.size() - 1];

            if(current_owner != claimed_owner){
                std::cout << prefix_int_to_string(prefix_infos[k]) << " is hijacked!" << std::endl;

                // TODO: maybe we should only remove this one. But in current situations it would not neccessary.
                return;
            }
        }

        for (uint32 k = 0; k < prefix_infos.size(); ++k) {
            bool my_as_present = false;
            for(auto it = paths[k].begin(); it != paths[k].end(); it++){
                if( (uint32) *it == as_num ){
                    my_as_present = true;
                    break;
                }
            }
            if(my_as_present){ // skip this one.
                continue;
            }

            paths[k].insert(paths[k].begin(), as_num);


            bool path_present = false;
            for(auto it = learned_routes[prefix_infos[k]].begin(); it != learned_routes[prefix_infos[k]].end(); it++){
                if(it->path_vector == paths[k]){
                    path_present = true;
                    break;
                }
            }

            if(not path_present){
                int priority;

                switch(interface_infos[iface_index].mode){
                    case CUSTOMER:
                        priority = CUSTOMER_LEARNED_PRIORITY;
                        break;
                    case PROVIDER:
                        priority = PROVIDER_LEARNED_PRIORITY;
                        break;
                    case PEER:
                        priority = PEER_LEARNED_PRIORITY;
                        break;
                    default:
                        priority = 0;
                }

                learned_routes[prefix_infos[k]].push_back(prefix_route_info{
                    priority,
                    iface_index,
                    paths[k]
                });
            }

        }
    }
}

void SimulatedMachine::handle_notification_frame(Frame frame, int iface_index) {
    thread_locks[iface_index].lock();

    std::cout << "an error occurred. state changed from "
              << getBGPStateName(interface_bgp_states[iface_index])
              << " to "
              << getBGPStateName(IDLE_STATE)
              << " on interface "
              << iface_index << std::endl;

    interface_bgp_states[iface_index] = IDLE_STATE;

    thread_locks[iface_index].unlock();

    startConnection(iface_index);
}

void SimulatedMachine::handle_keepalive_frame(Frame frame, int iface_index) {
    if (interface_bgp_states[iface_index] != OPEN_CONFIRM_STATE) {
        return;
    }

    thread_locks[iface_index].lock();

    hold_timer_active[iface_index] = false;

    interface_bgp_states[iface_index] = ESTABLISHED_STATE;
    printStateChangeMessage(OPEN_CONFIRM_STATE, ESTABLISHED_STATE, iface_index);

    thread_locks[iface_index].unlock();
}

void SimulatedMachine::fill_ethernet_ip_header(byte *frame_date, uint32 dest_ip, uint32 src_ip,
                                               int ip_payload_length) {

    sr_ethernet_hdr *ethernet_hdr = (sr_ethernet_hdr *) frame_date;

    memset(ethernet_hdr->ether_dhost, 255, 6);
    memset(ethernet_hdr->ether_shost, 255, 6);
    ethernet_hdr->ether_type = htons(0x0800);

    ip *ip_hdr = (ip *) (frame_date + sizeof(sr_ethernet_hdr));

    ip_hdr->ip_ttl = 255;
    *(uint32_t *) (&(ip_hdr->ip_dst)) = htonl(dest_ip);
    *(uint32_t *) (&(ip_hdr->ip_src)) = htonl(src_ip);
    ip_hdr->ip_len = htons(ip_payload_length + sizeof(ip));
    ip_hdr->ip_hl = 5;
    ip_hdr->ip_v = 4;

}

void SimulatedMachine::printStateChangeMessage(bgp_state initial_state, bgp_state dest_state, int interface_index) {
    std::cout << "state changed from "
              << getBGPStateName(initial_state)
              << " to "
              << getBGPStateName(dest_state)
              << " on interface "
              << interface_index << std::endl;
}




