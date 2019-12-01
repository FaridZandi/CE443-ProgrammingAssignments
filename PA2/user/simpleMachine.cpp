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


#include <sstream>
#include <iomanip>
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
    // TODO : remove this !
    printFrame(frame);
    return simulatedMachine->sendFrame(frame, ifaceIndex);
}

Frame SimpleMachine::frameFactory(int iface_index, uint16 source_port, uint16_t dest_port, byte data_type,
                                  byte message_length, uint32_t dest_ip, byte id) {

    uint16 data_length = sizeof(byte) + getDataLength(data_type, message_length);

    uint16 upd_length = sizeof(udp_header) + data_length;
    uint16 ip_length = sizeof(iphdr) + upd_length;
    uint32 frame_length = sizeof(ethernet_header) + ip_length;

    frame_header *header = (frame_header *) (new byte[frame_length]);

    // ethernet header
    memcpy(header->eth.src, iface[iface_index].mac, 6);
    memset(header->eth.dst, 255, 6);
    header->eth.type = htons(0x0800);

    // ip header
    header->iph.version = 4;
    header->iph.ihl = 5;
    header->iph.tos = 0;
    header->iph.tot_len = htons(ip_length);
    header->iph.id = 0;
    header->iph.frag_off = 0;
    header->iph.ttl = 64;
    header->iph.protocol = 17; // for udp
    header->iph.check = 0;
    header->iph.saddr = htonl(iface[0].getIp());
    header->iph.daddr = htonl(dest_ip);

    // udp header
    header->udph.source_port = htons(source_port);
    header->udph.dest_port = htons(dest_port);
    header->udph.length = htons(upd_length);
    header->udph.checksum = 0;

    // data header
    header->data_type_id = data_type << 5;
    header->data_type_id |= id;

    Frame frame(frame_length, (byte *) header);

    setIPChecksum(frame);

    return frame;
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

void SimpleMachine::setIPChecksum(Frame &frame) {
    frame_header *data = (frame_header *) frame.data;
    data->iph.check = 0;

    unsigned long sum = 0;

    int i = 0;
    for (uint16 *p = (uint16 *) (&(data->iph)); i < 10; i++, p++) {
        sum += ntohs(*p);
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    data->iph.check = htons((uint16) ~sum);
}

bool SimpleMachine::decrementTTL(Frame &frame) {
    frame_header *data = (frame_header *) frame.data;
    data->iph.ttl --;
    return data->iph.ttl != 0;
}

void SimpleMachine::routeFrame(Frame &frame) {

    if(!decrementTTL(frame)){
        return;
    }

    setIPChecksum(frame);

    byte *new_frame_data = new byte[frame.length];
    memcpy(new_frame_data, frame.data, frame.length);
    Frame new_frame(frame.length, new_frame_data);

    frame_header *sent_frame_header = (frame_header *) new_frame.data;
    uint32 dest_ip = ntohl(sent_frame_header->iph.daddr);

    int iface_index = findDestinationInterface(dest_ip);

    sendFrame(new_frame, iface_index);
}

bool SimpleMachine::isFrameMine(Frame &frame) {
    frame_header *sent_frame_header = (frame_header *) frame.data;
    uint32 dest_ip = ntohl(sent_frame_header->iph.daddr);
    return iface[0].getIp() == dest_ip;
}

DataType SimpleMachine::getFrameDataType(Frame &frame) {
    frame_header *header = (frame_header *) frame.data;
    return static_cast<DataType>(header->data_type_id >> 5);
}

int SimpleMachine::findDestinationInterface(uint32 ip) {
    for (int i = 1; i < getCountOfInterfaces(); ++i) {
        if ((ip & iface[i].getMask()) == (iface[i].getIp() & iface[i].getMask())) {
            return i;
        }
    }
    return 0;
}

void SimpleMachine::sendOutFrame(Frame &frame) {
    for (int i = 1; i < getCountOfInterfaces(); ++i) {
        frame_header *sent_frame_header = (frame_header *) frame.data;

        if ((sent_frame_header->iph.daddr & iface[i].getMask()) == (iface[i].getIp() & iface[i].getMask())) {
            sendFrame(frame, i);
            return;
        }
    }
    sendFrame(frame, 0);
}

std::string SimpleMachine::ip_int_to_string(uint32 ip) {
    std::stringstream ss;
    ss << ip / (1 << 24) << ".";
    ip %= (1 << 24);
    ss << ip / (1 << 16) << ".";
    ip %= (1 << 16);
    ss << ip / (1 << 8) << ".";
    ip %= (1 << 8);
    ss << ip;
    return ss.str();
}

bool SimpleMachine::checkIPChecksum(Frame &frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;
    unsigned long sum = 0;
    int i = 0;
    for (uint16 *p = (uint16 *) (&(received_frame_header->iph)); i < 10; i++, p++) {
        sum += ntohs(*p);
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    uint32 checksum = (uint16) ~sum;

    return checksum == 0;
}

void SimpleMachine::printFrame(Frame &frame) {
    std::cerr << "frame with length " << frame.length << std::endl;
    for (uint32 i = 0; i < frame.length; ++i) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(*(frame.data + i));
    }
    std::cerr << std::dec;
    std::cerr << std::endl;
}

