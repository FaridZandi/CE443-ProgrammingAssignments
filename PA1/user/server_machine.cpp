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


#include "server_machine.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct

#include <cmath>
#include <algorithm>

using namespace std;

ServerMachine::ServerMachine(SimulatedMachine *simulatedMachine, Interface *iface) :
        SimpleMachine(simulatedMachine, iface){

    ip_pool = new map<unsigned int, ip_pool_entry>;
    // The machine instantiated.
    // Interfaces are not valid at this point.
}

ServerMachine::~ServerMachine() {
    delete ip_pool;
    // destructor...
}

void ServerMachine::initialize() {
    // TODO: Initialize your program here; interfaces are valid now.
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

void ServerMachine::processFrame(Frame frame, int ifaceIndex) {
//    std::cout << "Frame received at iface " << ifaceIndex <<
//         " with length " << frame.length << endl;

    data_frame *data = (data_frame *)(frame.data + sizeof(ethernet_header));

    unsigned int ip = SimpleMachine::byte_network_order_to_int(data->ip);
    unsigned int time = SimpleMachine::byte_network_order_to_int(data->time);
    byte* mac = data->mac;

    if (data->type == 0) {        // DHCPDISCOVER
        handle_dhcp_discover(ifaceIndex, time, mac);
    } else if (data->type == 2) { // DHCPREQUEST
        handle_hdcp_request(ifaceIndex,ip, time, mac);
    } else if (data->type == 3) { // DHCPACK
        handle_hdcp_ack(ip, mac);
    } else if (data->type == 4) { // DHCPRELEASE
        handle_dhcp_release(ip);
    } else if (data->type == 6) { // Request Extend
        handle_request_extend(ifaceIndex, ip, time, mac);
    } else {
        std::cout << "invalid packet, dropped" << std::endl;
    }
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ServerMachine::run() {
//    this->printInterfacesInformation();

    std::string command;

    while (true) {
        std::getline(std::cin, command);
        process_command(command);
    }
}


void ServerMachine::process_command(std::string command) {
//    std::cout << "entered command is: " << command << std::endl << std::endl;

    std::vector<std::string> splitted = split(command, ' ');

    if (splitted[0] == "add") {
        if (splitted[1] == "pool") {

            std::string prefix_and_mask = splitted[2];
            add_to_pool(prefix_and_mask);

        } else if (splitted[1] == "time") {

            std::string time_string = splitted[2];
            unsigned int time = (unsigned int) stoul(time_string);
            advance_time(time);

        } else {
            std::cout << "invalid command" << std::endl;
        }
    } else if (splitted[0] == "print" && splitted[1] == "pool") {
        print_all_ips();
    } else {
        std::cout << "invalid command" << std::endl;
    }
}

void ServerMachine::add_to_pool(std::string prefix_and_mask) {
    unsigned int slash_index = (int) prefix_and_mask.find_first_of('/');

    int mask = std::stoi(prefix_and_mask.substr(slash_index + 1));
    int count = (int) pow(2, 32 - mask);

    std::string prefix = prefix_and_mask.substr(0, slash_index);
    unsigned int base_ip = (ip_string_to_int(prefix) / (count)) * count;

    for (int i = 0; i < count; i++) {
        unsigned int ip = base_ip + i;

        if(ip_pool->find(ip) == ip_pool->end()){
            ip_pool_entry new_entry;

            new_entry.is_allocated = false;
            new_entry.is_waiting_for_accept = false;
            new_entry.ttl = 0;
            memset(new_entry.mac, 0, 6);

            ip_pool->insert(std::make_pair(ip, new_entry));
        }
    }
}

struct timeout_message{
    unsigned int time;
    unsigned int ip;
    byte* mac;
};

bool operator<(const timeout_message& a, const timeout_message& b){
    return a.time < b.time;
}

void ServerMachine::advance_time(unsigned int time) {
    std::vector<timeout_message> timeout_messages;

    std::map<unsigned int, ip_pool_entry>::iterator it;

    for(it = ip_pool->begin(); it != ip_pool->end(); it++){
        if(it->second.is_allocated){
            if(it->second.ttl <= time){
                timeout_messages.push_back(timeout_message{it->second.ttl, it->first, it->second.mac});
            } else {
                it->second.ttl -= time;
            }
        }
    }

    std::sort(timeout_messages.begin(), timeout_messages.end());

    for (int i = 0; i < (int) timeout_messages.size(); ++i) {
        send_dhcp_timeout(timeout_messages[i].ip, timeout_messages[i].mac);

        it = ip_pool->find(timeout_messages[i].ip);
        it->second.is_waiting_for_accept = false;
        it->second.is_allocated = false;
        it->second.ttl = 0;
        memset(it->second.mac, 0, 6);
    }

}

unsigned int ServerMachine::get_one_ip(byte* mac, unsigned int ttl) {
    for (std::map<unsigned int, ip_pool_entry>::iterator it = ip_pool->begin(); it != ip_pool->end(); it++) {
        if(! it->second.is_allocated)
        {
            it->second.is_allocated = true;
            it->second.is_waiting_for_accept = true;
            it->second.ttl = ttl;
            memcpy(it->second.mac, mac, 6);

            return it->first;
        }
    }
    // TODO: what to do if pool is empty?
    return 0;
}

void ServerMachine::print_all_ips() {
    for (std::map<unsigned int, ip_pool_entry>::iterator it = ip_pool->begin(); it != ip_pool->end(); it++) {
        if(! it->second.is_allocated)
        {
            std::cout << ip_int_to_string(it->first) << std::endl;
        }
    }
}

void ServerMachine::handle_dhcp_discover(int interface, unsigned int time, byte *mac) {
    unsigned int ip = get_one_ip(mac, time);

    std::cout << "offer " << ip_int_to_string(ip) << " to " << mac_byte_to_string(mac) << " for time " << time <<  std::endl;

    send_dhcp_offer(interface, ip, time, mac);
}

void ServerMachine::send_dhcp_offer(int interface, unsigned int ip, unsigned int time, byte *mac) {
    Frame frame = populate_frame(1, mac, ip, time);
    set_sender_mac(frame, interface);
    sendFrame(frame, interface);
    delete[] frame.data;
}

void ServerMachine::handle_hdcp_request(int interface, unsigned int ip, unsigned int time, byte *mac) {
    std::map<unsigned int, ip_pool_entry>::iterator result = ip_pool->find(ip);

    if(result != ip_pool->end()){
        result->second.is_waiting_for_accept = false;
        result->second.ttl = time;

        std::cout << "assign " << ip_int_to_string(ip) << " to " << mac_byte_to_string(mac) << " for " << time <<  std::endl;

        send_dhcp_ack(interface, ip, time, mac);
    }
}

void ServerMachine::send_dhcp_ack(int interface, unsigned int ip, unsigned int time, byte *mac) {
    Frame frame = populate_frame(3, mac, ip, time);
    set_sender_mac(frame, interface);
    sendFrame(frame, interface);
}

void ServerMachine::handle_hdcp_ack(unsigned int ip, byte *mac) {
    for(std::map<unsigned int, ip_pool_entry>::iterator it = ip_pool->begin(); it != ip_pool->end(); it++){
        bool matched = true;

        for (int i = 0; i < 6; ++i) {
            if(it->second.mac[i] != mac[i]){
                matched = false;
                break;
            }
        }

        if(matched){
            if(it->second.is_waiting_for_accept){
                it->second.is_waiting_for_accept = false;
                it->second.is_allocated = false;
                it->second.ttl = 0;
                memset(it->second.mac, 0, 6);

                std::cout << ip_int_to_string(it->first) << " back to pool" << std::endl;

                return;
            }
        }
    }
}

void ServerMachine::handle_dhcp_release(unsigned int ip) {
    std::map<unsigned int, ip_pool_entry>::iterator result = ip_pool->find(ip);

    if(result != ip_pool->end()){
        result->second.ttl = 0 ;
        result->second.is_allocated = false;
        result->second.is_waiting_for_accept = false;
        memset(result->second.mac, 0, 6);
    }
}

void ServerMachine::send_dhcp_timeout(unsigned int ip, byte *mac) {
//    std::cout << ip_int_to_string(ip) << " " << mac_byte_to_string(mac) << std::endl;
    Frame frame = populate_frame(5, mac, ip, 0);
    frame.length = 25;
    broadcast(frame);
    delete[] frame.data;
}

void ServerMachine::handle_request_extend(int interface, unsigned int ip, unsigned int time, byte *mac) {
    std::map<unsigned int, ip_pool_entry>::iterator result = ip_pool->find(ip);

    if(result != ip_pool->end()){
        unsigned int extended_ttl = result->second.ttl;

        if(time == 0){
            time = 10;
        }

        extended_ttl += time;

        result->second.ttl = 0;
        result->second.is_allocated = false;
        result->second.is_waiting_for_accept = false;
        memset(result->second.mac, 0, 6);

        unsigned int new_ip = get_one_ip(mac, extended_ttl);
        ip_pool->find(new_ip)->second.is_waiting_for_accept = false;

        send_response_extend(interface, new_ip, extended_ttl, mac);

        std::cout << "assign " << ip_int_to_string(new_ip) << " to " << mac_byte_to_string(mac) << " for " << extended_ttl <<  std::endl;
    }
}

void ServerMachine::send_response_extend(int interface, unsigned int ip, unsigned int time, byte *mac) {
    Frame frame = populate_frame(7, mac, ip , time);
    set_sender_mac(frame, interface);
    sendFrame(frame, interface);
    delete[] frame.data;
}
