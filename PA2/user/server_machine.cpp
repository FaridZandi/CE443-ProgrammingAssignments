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
#include <iomanip>

using namespace std;

ServerMachine::ServerMachine(SimulatedMachine *simulatedMachine, Interface *iface) :
        SimpleMachine(simulatedMachine, iface) {
    // The machine instantiated.
    // Interfaces are not valid at this point.
}

ServerMachine::~ServerMachine() {
    // destructor...
}

void ServerMachine::initialize() {
    // TODO: Initialize your program here; interfaces are valid now.

    for (int i = 0; i < 32; ++i) {
        peers_data[i].valid = false;
    }

    current_id_index = 1;
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

    std::cerr << "frame received with length " << frame.length << std::endl;
    printFrame(frame);

    frame_header *received_frame_header = (frame_header *) frame.data;

    if (received_frame_header->iph.protocol != 17 or !checkIPChecksum(frame)) {
        std::cout << "invalid packet, dropped" << std::endl;
        return;
    }

    if (ntohl(received_frame_header->iph.daddr) != server_ip) {
        routeFrame(frame);
    } else {
        DataType frame_data_type = getFrameDataType(frame);

        std::cerr << "frame type " << frame_data_type << std::endl;
        switch (frame_data_type) {
            case REQUEST_ASSIGNING_ID:
                handle_request_assigning_id(frame);
                break;
            case REQUEST_GETTING_IP:
                handle_request_getting_ip(frame);
                break;
            case REQUEST_UPDATING_INFO:
                handle_request_updating_info(frame);
                break;
            case STATUS:
                handle_status(frame);
                break;
            default:
                break;
        }
    }

}

uint16 ServerMachine::getDataLength(byte data_type, uint16 message_length) {
    switch (data_type) {
        case 0:
            return 0;
        case 1:
            return 12;
        case 2:
            return 4;
        case 7:
            return 6;
        default:
            return 0;
    }
}

/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ServerMachine::run() {
}

void ServerMachine::handle_request_assigning_id(Frame frame) {

    frame_header *received_frame_header = (frame_header *) frame.data;
    packet_data *packetData = (packet_data *) (frame.data + header_length);

    peers_data[current_id_index].local_ip = ntohl(packetData->local_ip);
    peers_data[current_id_index].local_port = ntohs(packetData->local_port);
    peers_data[current_id_index].public_ip = ntohl(received_frame_header->iph.saddr);
    peers_data[current_id_index].public_port = ntohs(received_frame_header->udph.source_port);
    peers_data[current_id_index].valid = true;

    std::cout << "new id " << current_id_index << " assigned to "
              << ip_int_to_string(peers_data[current_id_index].public_ip) << ":"
              << peers_data[current_id_index].public_port << std::endl;

    int new_id = current_id_index;

    current_id_index++;

    uint32 dest_ip = ntohl(received_frame_header->iph.saddr);
    uint16 dest_port = ntohs(received_frame_header->udph.source_port);

    send_response_assigning_id(new_id, dest_ip, dest_port);
}

void ServerMachine::send_response_assigning_id(int id, uint32 ip, uint16 port) {
    int iface_index = findDestinationInterface(ip);

    Frame frame = frameFactory(iface_index, server_port, port, RESPONSE_ASSIGNING_ID, 0, ip, (byte) id);

    frame_header *sent_frame = (frame_header *) frame.data;
    sent_frame->iph.saddr = htonl(server_ip);
    setIPChecksum(frame);

    sendFrame(frame, iface_index);
}

void ServerMachine::handle_request_getting_ip(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;

    uint32 requesting_ip = ntohl(received_frame_header->iph.saddr);
    uint16 requesting_port = ntohs(received_frame_header->udph.source_port);
    int requesting_id = -1;

    for (int i = 0; i < 32; ++i) {
        if (peers_data[i].public_port == requesting_port and peers_data[i].public_ip == requesting_ip) {
            requesting_id = i;
            break;
        }
    }
    if (requesting_id == -1) {
        std::cout << "id not exist, dropped" << std::endl;
        return;
    }

    int requested_id = received_frame_header->data_type_id & 31U;
    if (!peers_data[requested_id].valid) {
        std::cout << "id not exist, dropped" << std::endl;
        return;
    }

    uint32 local_ip = peers_data[requested_id].local_ip;
    uint16 local_port = peers_data[requested_id].local_port;
    uint32 public_ip = peers_data[requested_id].public_ip;
    uint16 public_port = peers_data[requested_id].public_port;

    std::cout << requesting_id << " wants info of node " << requested_id << std::endl;

    send_response_getting_ip(requested_id, requesting_ip, requesting_port, local_ip, local_port, public_ip,
                             public_port);
}

void ServerMachine::send_response_getting_ip(int dest_id, uint32 dest_ip, uint16 dest_port, uint32 local_ip,
                                             uint16 local_port, uint32 public_ip, uint16 public_port) {

    int iface_index = findDestinationInterface(dest_ip);

    Frame frame = frameFactory(iface_index, server_port, dest_port, RESPONSE_GETTING_IP, 0, dest_ip, (byte) dest_id);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    packetData->local_port = htons(local_port);
    packetData->local_ip = htonl(local_ip);
    packetData->public_port = htons(public_port);
    packetData->public_ip = htonl(public_ip);

    frame_header *sent_frame = (frame_header *) frame.data;
    sent_frame->iph.saddr = htonl(server_ip);
    setIPChecksum(frame);

    sendFrame(frame, iface_index);

}

void ServerMachine::handle_request_updating_info(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;
    packet_data *packetData = (packet_data *) (frame.data + header_length);

    uint32 public_ip = ntohl(received_frame_header->iph.saddr);
    uint16 public_port = ntohs(received_frame_header->udph.source_port);
    uint32 local_ip = ntohl(packetData->local_ip);
    uint16 local_port = ntohs(packetData->local_port);

    int requesting_id = -1;

    for (int i = 0; i < 32; ++i) {
        // we're sure the public and local ip of any node are fixed.
        // not sure however if this will suffice to uniquely choose the id.
        if (peers_data[i].public_ip == public_ip and peers_data[i].local_ip == local_ip) {
            requesting_id = i;
            break;
        }
    }

    if (requesting_id == -1) {
        return;
    }

    peers_data[requesting_id].local_port = local_port;
    peers_data[requesting_id].public_port = public_port;

    std::cout << "id " << requesting_id << " infos updated to "
              << ip_int_to_string(public_ip) << ":" << public_port << std::endl;
}

void ServerMachine::handle_status(Frame frame) {

    frame_header *received_frame_header = (frame_header *) frame.data;
    packet_data *packetData = (packet_data *) (frame.data + header_length);

    uint32 local_ip = ntohl(packetData->local_ip);
    uint16 local_port = ntohs(packetData->local_port);
    uint32 public_ip = ntohl(received_frame_header->iph.saddr);
    uint16 public_port = ntohs(received_frame_header->udph.source_port);

    byte flag;

    if (local_ip == public_ip and local_port == public_port) {
        flag = 1;
    } else {
        flag = 0;
    }

    uint32 dest_ip = public_ip;
    uint16 dest_port = public_port;

    send_respond_status(flag, dest_ip, dest_port);
}

void ServerMachine::send_respond_status(byte flag, uint32 ip, uint16 port) {
    int iface_index = findDestinationInterface(ip);

    Frame frame = frameFactory(iface_index, server_port, port, STATUS_RESPOND, 0, ip, (byte) flag);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    packetData->local_ip = 0;
    packetData->local_port = 0;

    frame_header *sent_frame = (frame_header *) frame.data;
    sent_frame->iph.saddr = htonl(server_ip);
    setIPChecksum(frame);

    sendFrame(frame, iface_index);
}
