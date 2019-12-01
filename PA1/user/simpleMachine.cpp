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


#include "simpleMachine.h"
#include "interface.h"

SimpleMachine::SimpleMachine(const SimulatedMachine *simulatedMachine, Interface *iface) {
    this->simulatedMachine = simulatedMachine;
    this->iface = iface;
}

SimpleMachine::~SimpleMachine() {
}

int SimpleMachine::getCountOfInterfaces() const {
    return simulatedMachine->getCountOfInterfaces();
}

void SimpleMachine::printInterfacesInformation() const {
    simulatedMachine->printInterfacesInformation();
}

const std::string SimpleMachine::getCustomInformation() {
    return simulatedMachine->getCustomInformation();
}

bool SimpleMachine::sendFrame(Frame frame, int ifaceIndex) {
    return simulatedMachine->sendFrame(frame, ifaceIndex);
}

std::vector<std::string> SimpleMachine::split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

void SimpleMachine::broadcast(Frame frame, int exception, bool set_sender) {
    int iface_num = getCountOfInterfaces();

    for (int i = 0; i < iface_num; i++) {
        if (i != exception) {
            if(set_sender) set_sender_mac(frame, i);

            sendFrame(frame, i);
        }
    }
}

unsigned int SimpleMachine::ip_string_to_int(std::string string_ip) {
    std::stringstream s(string_ip);
    char ch;
    unsigned int a, b, c, d;
    s >> a >> ch >> b >> ch >> c >> ch >> d;
    return (a << 24) + (b << 16) + (c << 8) + d;
}

std::string SimpleMachine::ip_int_to_string(unsigned int int_ip) {
    std::stringstream ss;
    ss << int_ip / (1 << 24) << ".";
    int_ip %= (1 << 24);
    ss << int_ip / (1 << 16) << ".";
    int_ip %= (1 << 16);
    ss << int_ip / (1 << 8) << ".";
    int_ip %= (1 << 8);
    ss << int_ip;
    return ss.str();
}

byte *SimpleMachine::int_to_byte_network_order(unsigned int value) {
    value = htonl(value);
    byte *result = new byte[4];
    memcpy(result, &value, 4);
    return result;
}

unsigned int SimpleMachine::byte_network_order_to_int(byte *value) {
    unsigned int answer;

    answer = (unsigned int) value[0];
    answer += ((unsigned int) value[1]) << 8;
    answer += ((unsigned int) value[2]) << 16;
    answer += ((unsigned int) value[3]) << 24;

    return ntohl(answer);
}

Frame SimpleMachine::populate_frame(byte data_type, byte *mac,
                                    unsigned int ip, unsigned int time) {

    unsigned int frame_size = sizeof(ethernet_header) + sizeof(data_frame);
    byte *frame_data = new byte[frame_size];


    ethernet_header *eth = (ethernet_header *) frame_data;

    memset(eth->dst, 255, 6); // broadcast
    memset(eth->src, 0, 6); // to be set later
    eth->type = 0;


    data_frame *data = (data_frame *) (frame_data + sizeof(ethernet_header));

    byte *ip_bytes = int_to_byte_network_order(ip);
    byte *time_bytes = int_to_byte_network_order(time);

    memcpy(data->ip, ip_bytes, 4);
    memcpy(data->time, time_bytes, 4);
    memcpy(data->mac, mac, 6);
    data->type = data_type;

    delete[] ip_bytes;
    delete[] time_bytes;

    return Frame(frame_size, frame_data);
}

std::string SimpleMachine::mac_byte_to_string(byte *mac) {
    std::stringstream ss;
    for (int i = 0; i < 6; ++i) {
        ss << std::uppercase << std::hex << (unsigned int) mac[i];
//        if (i != 5) {
//            ss << ":";
//        }
    }
    return ss.str();
}

void SimpleMachine::set_sender_mac(Frame &frame, int interface) {

    ethernet_header *eth = (ethernet_header *) frame.data;

    memcpy(eth->src, iface[interface].mac, 6); // to be set later

}



