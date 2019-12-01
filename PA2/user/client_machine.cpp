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
#include <iomanip>

using namespace std;

ClientMachine::ClientMachine(SimulatedMachine *simulatedMachine, Interface *iface) :
        SimpleMachine(simulatedMachine, iface) {
    // The machine instantiated.
    // Interfaces are not valid at this point.
    peers_data = new std::map<int, peer_data2>();
}

ClientMachine::~ClientMachine() {
    delete peers_data;
}

void ClientMachine::initialize() {
    // TODO: Initialize your program here; interfaces are valid now.

    local_port = 0;
    my_id = -1;

    last_reached_id = -1;
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
    std::cerr << "frame received with length " << frame.length << std::endl;
    printFrame(frame);

    frame_header *received_frame_header = (frame_header *) frame.data;
    packet_data *packetData = (packet_data *) (frame.data + header_length);

    if (received_frame_header->iph.protocol != 17 or !checkIPChecksum(frame)) {
        std::cout << "invalid packet, dropped" << std::endl;
        return;
    }

    if (!isFrameMine(frame)) {
        routeFrame(frame);
    } else {
        DataType frame_data_type = getFrameDataType(frame);

        switch (frame_data_type) {
            case RESPONSE_ASSIGNING_ID: {// maybe it was drop! why should they have had the same data type ?!!
                uint32 id = received_frame_header->data_type_id & 31U;
                if (id == 0) { // assuming server never assigns the id = 0
                    handle_drop_frame(frame);
                } else {
                    handle_response_assigning_id_frame(frame);
                }
            }
                break;
            case RESPONSE_GETTING_IP:
                handle_response_getting_ip_frame(frame);
                break;
            case REQUEST_LOCAL_SESSION: // or a bunch of other types!!
                if (((char *) (&packetData->local_ip))[1] == 'i') { // if ping !!
                    handle_request_session_frame(frame);
                } else { // if pong !!
                    handle_response_session_frame(frame);
                }
                break;
            case MESSAGE:
                handle_message_frame(frame);
                break;
            case NAT_UPDATED:
                handle_nat_updated_frame(frame);
                break;
            case STATUS_RESPOND:
                handle_status_respond(frame);
                break;
            default:
                break;
        }
    }
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ClientMachine::run() {
    std::string command;

    while (true) {
        std::getline(std::cin, command);
        process_command(command);
    }
}

void ClientMachine::process_command(std::string command) {
//    std::cout << "entered command is : " << command << std::endl;

    std::vector<std::string> splitted = split(command, ' ');

    if (splitted[0] == "make" and splitted[1] == "a" and splitted[2] == "connection" and
        splitted[3] == "to" and splitted[4] == "server" and splitted[5] == "on" and
        splitted[6] == "port") {
        uint16 port = (uint16) std::stoi(splitted[7]);
        send_request_assigning_id(port);
    } else if (splitted[0] == "get" and splitted[1] == "info" and splitted[2] == "of") {
        int id = std::stoi(splitted[3]);
        send_request_getting_ip(id);
    } else if (splitted[0] == "make" and splitted[1] == "a" and splitted[2] == "local" and
               splitted[3] == "session" and splitted[4] == "to") {
        int id = std::stoi(splitted[5]);
        send_request_local_session(id);
    } else if (splitted[0] == "make" and splitted[1] == "a" and splitted[2] == "public" and
               splitted[3] == "session" and splitted[4] == "to") {
        int id = std::stoi(splitted[5]);
        send_request_public_session(id);
    } else if (splitted[0] == "send" and splitted[1] == "msg" and splitted[2] == "to") {
        unsigned long colon_index = splitted[3].find(':');
        int id = std::stoi(splitted[3].substr(0, colon_index));
        colon_index = command.find(':');
        std::string message = command.substr(colon_index + 1);
        send_message(id, message);
    } else if (splitted[0] == "status") {
        send_status();
    } else {
        std::cout << "invalid command" << std::endl;
    }
}

void ClientMachine::send_request_assigning_id(uint16 port) {
    if (my_id != -1) {
        std::cout << "you already have an id, ignored" << std::endl;
        return;
    }
    local_port = port;

    int iface_index = findDestinationInterface(server_ip);

    Frame frame = frameFactory(iface_index, port, server_port, REQUEST_ASSIGNING_ID, 0, server_ip, 0);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    packetData->local_port = htons(port);
    packetData->local_ip = htonl(iface[0].getIp());

    sendFrame(frame, iface_index);
}

void ClientMachine::handle_drop_frame(Frame frame) {

    frame_header *received_frame_header = (frame_header *) frame.data;

    uint16 nat_port = ntohs(received_frame_header->udph.source_port);

    if (nat_port == 1234) {
        local_port += 100;
        std::cout << "connection to server failed, retrying on port " << local_port << std::endl;

        int iface_index = findDestinationInterface(server_ip);
        Frame sent_frame = frameFactory(iface_index, local_port, server_port, REQUEST_ASSIGNING_ID, 0, server_ip, 0);

        packet_data *packetData = (packet_data *) (sent_frame.data + header_length);

        packetData->local_port = htons(local_port);
        packetData->local_ip = htonl(iface[0].getIp());

        sendFrame(sent_frame, iface_index);
    } else if (nat_port == 4321) {
        std::cout << "connection lost, perhaps " << last_reached_id << "'s info has changed, ask server for updates" << std::endl;
    }
}

void ClientMachine::handle_response_assigning_id_frame(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;

    my_id = received_frame_header->data_type_id & 31U;

    std::cout << "Now My ID is " << my_id << std::endl;
}

void ClientMachine::send_request_getting_ip(int id) {
    int iface_index = findDestinationInterface(server_ip);

    Frame frame = frameFactory(iface_index, local_port, server_port, REQUEST_GETTING_IP, 0, server_ip, (byte) id);

    sendFrame(frame, iface_index);
}

void ClientMachine::handle_response_getting_ip_frame(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;
    packet_data *packetData = (packet_data *) (frame.data + header_length);

    uint32 local_ip = ntohl(packetData->local_ip);
    uint16 local_port = ntohs(packetData->local_port);
    uint32 public_ip = ntohl(packetData->public_ip);
    uint16 public_port = ntohs(packetData->public_port);

    int requested_id = received_frame_header->data_type_id & 31U;

    std::cout << "packet with (" << requested_id << ", " << ip_int_to_string(local_ip) << ", " << local_port << ", "
              << ip_int_to_string(public_ip) << ", " << public_port << ") received" << std::endl;

    (*peers_data)[requested_id].public_port = public_port;
    (*peers_data)[requested_id].public_ip = public_ip;
    (*peers_data)[requested_id].local_ip = local_ip;
    (*peers_data)[requested_id].local_port = local_port;
}

void ClientMachine::send_request_local_session(int id) {
    if (peers_data->find(id) == peers_data->end()) {
        std::cout << "info of node " << id << " was not received" << std::endl;
        return;
    }

    uint16 dest_port = (*peers_data)[id].local_port;
    uint32 dest_ip = (*peers_data)[id].local_ip;
    (*peers_data)[id].connected_locally = true;
    (*peers_data)[id].ping_sent = true;
    (*peers_data)[id].pong_received = false;

    int iface_index = findDestinationInterface(dest_ip);

    Frame frame = frameFactory(iface_index, local_port, dest_port, REQUEST_LOCAL_SESSION, 0, dest_ip, (byte) my_id);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    memcpy(&(packetData->local_ip), "ping", 4); // lol !

    last_reached_id = id;
    sendFrame(frame, iface_index);
}

void ClientMachine::send_request_public_session(int id) {
    if (peers_data->find(id) == peers_data->end()) {
        std::cout << "info of node " << id << " was not received" << std::endl;
        return;
    }

    uint16 dest_port = (*peers_data)[id].public_port;
    uint32 dest_ip = (*peers_data)[id].public_ip;

    (*peers_data)[id].connected_locally = false;
    (*peers_data)[id].ping_sent = true;
    (*peers_data)[id].pong_received = false;

    int iface_index = findDestinationInterface(dest_ip);

    Frame frame = frameFactory(iface_index, local_port, dest_port, REQUEST_PUBLIC_SESSION, 0, dest_ip, (byte) my_id);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    memcpy(&(packetData->local_ip), "ping", 4); // lol !

    last_reached_id = id;
    sendFrame(frame, iface_index);
}

void ClientMachine::handle_request_session_frame(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;

    uint32 received_ip = ntohl(received_frame_header->iph.saddr);
    uint16 received_port = ntohs(received_frame_header->udph.source_port);
    uint32 sender_id = received_frame_header->data_type_id & 31U;

    if (peers_data->find(sender_id) == peers_data->end()) {
        // don't know if this is ok.
        // it's ethically wrong anyway :)

        (*peers_data)[sender_id].local_port = received_port;
        (*peers_data)[sender_id].public_port = received_port;
        (*peers_data)[sender_id].public_ip = received_ip;
        (*peers_data)[sender_id].local_ip = received_ip;
        (*peers_data)[sender_id].connected_locally = true;
        (*peers_data)[sender_id].pong_received = true;
        (*peers_data)[sender_id].ping_sent = true;

        std::cout << "Connected to " << sender_id << std::endl;

        int iface_index = findDestinationInterface(received_ip);

        Frame sent_frame = frameFactory(iface_index, local_port, received_port, REQUEST_LOCAL_SESSION,
                                        0, received_ip, (byte) my_id);

        packet_data *packetData = (packet_data *) (sent_frame.data + header_length);

        memcpy(&(packetData->local_ip), "pong", 4); // lol !

        sendFrame(sent_frame, iface_index);

        // end unethical section :))
    } else {
        bool received_locally;

        received_locally = (*peers_data)[sender_id].local_ip == received_ip and
                           (*peers_data)[sender_id].local_port == received_port;

        (*peers_data)[sender_id].connected_locally = received_locally;
        (*peers_data)[sender_id].pong_received = true;
        (*peers_data)[sender_id].ping_sent = true;

        std::cout << "Connected to " << sender_id << std::endl;

        int iface_index = findDestinationInterface(received_ip);

        Frame sent_frame = frameFactory(iface_index, local_port, received_port, REQUEST_LOCAL_SESSION,
                                        0, received_ip, (byte) my_id);

        packet_data *packetData = (packet_data *) (sent_frame.data + header_length);

        memcpy(&(packetData->local_ip), "pong", 4); // lol !

        sendFrame(sent_frame, iface_index);
    }
}

void ClientMachine::handle_response_session_frame(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;

    uint32 sender_id = received_frame_header->data_type_id & 31U;

    if (peers_data->find(sender_id) == peers_data->end()) {
        return;
    }

    if (not((*peers_data)[sender_id].pong_received)) {
        (*peers_data)[sender_id].pong_received = true;

        std::cout << "Connected to " << sender_id << std::endl;
    }
}

void ClientMachine::send_message(int id, std::string message) {
    uint16 dest_port;
    uint32 dest_ip;


    if (not((*peers_data)[id].ping_sent) or not((*peers_data)[id].pong_received)) {
        std::cout << "please make a session first" << std::endl;
        return;
    }

    if ((*peers_data)[id].connected_locally) {
        dest_port = (*peers_data)[id].local_port;
        dest_ip = (*peers_data)[id].local_ip;
    } else {
        dest_port = (*peers_data)[id].public_port;
        dest_ip = (*peers_data)[id].public_ip;
    }

    int iface_index = findDestinationInterface(dest_ip);

    Frame frame = frameFactory(iface_index, local_port, dest_port, MESSAGE, (byte) message.length(), dest_ip,
                               (byte) my_id);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    memcpy(packetData, message.c_str(), message.length());

    last_reached_id = id;
    sendFrame(frame, iface_index);
}

void ClientMachine::handle_nat_updated_frame(Frame frame) {
    send_request_updating_info(local_port);
}

void ClientMachine::send_status() {
    int iface_index = findDestinationInterface(server_ip);

    Frame frame = frameFactory(iface_index, local_port, server_port, STATUS, 0, server_ip, 0);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    packetData->local_port = htons(local_port);
    packetData->local_ip = htonl(iface[0].getIp());

    sendFrame(frame, iface_index);
}

void ClientMachine::handle_status_respond(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;

    uint32 flag = received_frame_header->data_type_id & 31U;

    if (flag == 1) {
        std::cout << "direct" << std::endl;
    } else {
        std::cout << "indirect" << std::endl;
    }
}

void ClientMachine::send_request_updating_info(uint16 port) {
    int iface_index = findDestinationInterface(server_ip);

    Frame frame = frameFactory(iface_index, port, server_port, REQUEST_UPDATING_INFO, 0, server_ip, 0);

    packet_data *packetData = (packet_data *) (frame.data + header_length);

    packetData->local_port = htons(port);
    packetData->local_ip = htonl(iface[0].getIp());

    sendFrame(frame, iface_index);
}

uint16 ClientMachine::getDataLength(byte data_type, uint16 message_length) {
    switch (data_type) {
        case 0:
            return 6;
        case 1:
            return 0;
        case 2:
            return 4;
        case 3:
            return message_length;
        case 5:
            return 6;
        case 6:
            return 7;
        default:
            return 0;
    }
}

void ClientMachine::handle_message_frame(Frame frame) {
    frame_header *received_frame_header = (frame_header *) frame.data;
    char *packetData = (char *) (frame.data + header_length);

    uint32 sender_id = received_frame_header->data_type_id & 31U;
    uint32 message_length = frame.length - header_length;

    std::cout << "received msg from " << sender_id << ":";

    for (uint32 i = 0; i < message_length; ++i) {
        std::cout << *(packetData + i);
    }
    std::cout << std::endl;
}

