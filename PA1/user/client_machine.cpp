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

#include "client_machine.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct
#include <algorithm>

using namespace std;

ClientMachine::ClientMachine(SimulatedMachine *simulatedMachine, Interface *iface) :
        SimpleMachine(simulatedMachine, iface) {

    ips = new std::vector<unsigned int>;

    offers = new std::vector<pair<unsigned int, unsigned int> >;

    // The machine instantiated.
    // Interfaces are not valid at this point.
}

ClientMachine::~ClientMachine() {

    delete ips;
    delete offers;

    // destructor...
}

void ClientMachine::initialize() {
    // TODO: Initialize your program here; interfaces are valid now.
    ips->clear();
    offers->clear();
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
void ClientMachine::processFrame(Frame frame, int ifaceIndex) {
//    std::cout << "Frame received at iface " << ifaceIndex <<
//              " with length " << frame.length << endl;

    if (!is_frame_mine(frame)) {
        broadcast(frame, ifaceIndex, false);
    } else {
        data_frame *data = (data_frame *) (frame.data + sizeof(ethernet_header));

        unsigned int ip = SimpleMachine::byte_network_order_to_int(data->ip);
        unsigned int time = SimpleMachine::byte_network_order_to_int(data->time);

        if (data->type == 1) {        // DHCPOFFER
            handle_hdcp_offer(ip, time);
        } else if (data->type == 3) { // DHCPACK
            handle_dhcp_ack(ip, time);
            broadcast(frame, ifaceIndex, false);
        } else if (data->type == 5) { // DHCPTIMEOUT
            handle_dhcp_timeout(ip);
        } else if (data->type == 7) { // Response Extend
            handle_response_extend(ip, time);
        } else {
            std::cout << "invalid packet, dropped" << std::endl;
        }
    }
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ClientMachine::run() {
//    this->printInterfacesInformation();

    std::string command;

    while (true) {
        std::getline(std::cin, command);
        process_command(command);
    }
}

void ClientMachine::process_command(std::string command) {
//    std::cout << "entered command is: " << command << std::endl << std::endl;

    std::vector<std::string> splitted = split(command, ' ');

    if (splitted[0] == "get" && splitted[1] == "ip" && splitted[2] == "for" && splitted[3] == "time") {

        std::string time_string = splitted[4];
        unsigned int time = (unsigned int) std::stoul(time_string);
        send_dhcp_discover(time);

    } else if (splitted[0] == "accept" && splitted[1] == "offer:" && splitted[3] == "for" && splitted[4] == "time") {

        std::string ip_string = splitted[2];
        std::string time_string = splitted[5];

        unsigned int ip = ip_string_to_int(ip_string);
        unsigned int time = (unsigned int) std::stoul(time_string);

        send_dhcp_request(ip, time);

    } else if (splitted[0] == "release") {

        std::string ip_string = splitted[1];
        unsigned int ip = ip_string_to_int(ip_string);

        send_dhcp_release(ip);

    } else if (splitted[0] == "extend" && splitted[1] == "lease" && splitted[3] == "for" && splitted[4] == "time") {
        std::string ip_string = splitted[2];
        std::string time_string = splitted[5];

        unsigned int ip = ip_string_to_int(ip_string);
        unsigned int time = (unsigned int)std::stoul(time_string);

        send_request_extend(ip, time);
    } else if (splitted[0] == "print" && splitted[1] == "ip") {
        print_all_ips();
    } else {
        std::cout << "invalid command" << std::endl;

    }
}

void ClientMachine::send_dhcp_discover(unsigned int time) {
    Frame frame = populate_frame(0, iface[0].mac, 0, time);
    broadcast(frame);
    delete[] frame.data;
}

bool ClientMachine::is_frame_mine(Frame frame) {
    data_frame *data = (data_frame *) (frame.data + sizeof(ethernet_header));

    for (int i = 0; i < 6; ++i) {
        if (data->mac[i] != iface[0].mac[i]) {
            return false;
        }
    }

    return true;
}

void ClientMachine::handle_hdcp_offer(unsigned int ip, unsigned int time) {
    std::cout << "new offer: " << ip_int_to_string(ip) << " for time " << time << std::endl;

    offers->push_back(std::make_pair(ip, time));
}

void ClientMachine::send_dhcp_request(unsigned int ip, unsigned int time) {

    for (int i = 0; i < (int) offers->size(); ++i) {
        if ((*offers)[i].first == ip) {
            if (time <= (*offers)[i].second) {
                Frame frame = populate_frame(2, iface[0].mac, ip, time);
                broadcast(frame);
                delete[] frame.data;
                return;
            }
        }
    }
    std::cout << "invalid offer" << std::endl;
}

void ClientMachine::handle_dhcp_ack(unsigned int ip, unsigned int time) {
    std::cout << "now my ip is: " << ip_int_to_string(ip) << " for time " << time << std::endl;
    offers->clear();
    ips->push_back(ip);
}

void ClientMachine::send_dhcp_release(unsigned int ip) {
    for (int i = 0; i < (int) ips->size(); ++i) {
        if ((*ips)[i] == ip) {
            ips->erase(ips->begin() + i);
            std::cout << "ip released" << std::endl;

            Frame frame = populate_frame(4, iface[0].mac, ip, 0);
            broadcast(frame);
            delete[] frame.data;

            return;
        }
    }
}

void ClientMachine::print_all_ips() {
    std::vector<unsigned int> sorted_ips = *ips;

    std::sort(sorted_ips.begin(), sorted_ips.end());

    for (int i = 0; i < (int) sorted_ips.size(); ++i) {
        std::cout << ip_int_to_string(sorted_ips[i]) << std::endl;
    }

//    std::cout << "my offers are: (" << offers->size() << ")" << std::endl;
//
//    for (int i = 0; i < (int) offers->size(); ++i) {
//        std::cout << ip_int_to_string((*offers)[i].first) << " " << (*offers)[i].second << std::endl;
//    }

}

void ClientMachine::handle_dhcp_timeout(unsigned int ip) {
    for (int i = 0; i < (int) ips->size(); ++i) {
        if ((*ips)[i] == ip) {
            ips->erase(ips->begin() + i);
            std::cout << "ip released" << std::endl;
            return;
        }
    }
}

void ClientMachine::send_request_extend(unsigned int ip, unsigned int time) {
    extend_request_ip = ip;

    Frame frame = populate_frame(6, iface[0].mac, ip, time);
    broadcast(frame);
    delete[] frame.data;
}

void ClientMachine::handle_response_extend(unsigned int ip, unsigned int time) {
    // remove the old ip
    for (int i = 0; i < (int) ips->size(); ++i) {
        if((*ips)[i] == extend_request_ip){
            ips->erase(ips->begin() + i);
            break;
        }
    }

    // add the new ip
    ips->push_back(ip);

    std::cout << "now my ip is: " << ip_int_to_string(ip) << " for time " << time << std::endl;
}
