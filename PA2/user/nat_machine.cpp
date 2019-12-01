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

#include "nat_machine.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct

using namespace std;

NatMachine::NatMachine(SimulatedMachine *simulatedMachine, Interface *iface) :
        SimpleMachine(simulatedMachine, iface) {
    // The machine instantiated.
    // Interfaces are not valid at this point.
}

NatMachine::~NatMachine() {
    // destructor...
}

uint16 NatMachine::getDataLength(byte data_type, uint16 message_length) {
    return 0;
}

void NatMachine::initialize() {
    // TODO: Initialize your program here; interfaces are valid now.
    inbound_sessions = new std::map<std::pair<uint32, uint16>, std::pair<uint32, uint16 >>;

    outbound_sessions = new std::map<std::pair<uint32, uint16>, std::pair<uint32, uint16 >>;

    blocked_ports = new std::bitset<USHRT_MAX>;

    blocked_ports->reset();
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
void NatMachine::processFrame(Frame frame, int ifaceIndex) {
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

        uint16 source_port = ntohs(received_frame_header->udph.source_port);
        uint16 dest_port = ntohs(received_frame_header->udph.dest_port);
        uint32 source_ip = ntohl(received_frame_header->iph.saddr);
        uint32 dest_ip = ntohl(received_frame_header->iph.daddr);

        if(ifaceIndex == 0){ // inbound
            if(outbound_sessions->find(make_pair(dest_ip, dest_port)) != outbound_sessions->end()){
                auto result = (*outbound_sessions)[make_pair(dest_ip, dest_port)];

                if(result.first == source_ip and result.second == source_port){

                } else {
                    std::cout << "outer packet dropped" << std::endl;
                }
            } else {
                std::cout << "outer packet dropped" << std::endl;
            }
        } else { // outbound

        }
    }
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void NatMachine::run() {
    std::string command;

    while (true) {
        std::getline(std::cin, command);
        process_command(command);
    }
}

void NatMachine::process_command(std::string command) {
    std::vector<std::string> splitted = split(command, ' ');

    if (splitted[0] == "block" and splitted[1] == "port" and splitted[2] == "range") {
        uint16 port_min = (uint16) std::stoi(splitted[3]);
        uint16 port_max = (uint16) std::stoi(splitted[4]);

        block_port_range(port_min, port_max);
    } else if (splitted[0] == "reset" and splitted[1] == "network" and splitted[2] == "settings") {
        std::cout << "please enter the base start number for port." << std::endl;
        uint16 base_port;
        std::cin >> base_port;
        reset_nat(base_port);
    } else {
        std::cout << "invalid command" << std::endl;
    }
}

void NatMachine::block_port_range(uint16 min, uint16 max) {
    for (int i = min; i < max; ++i) {
        (*blocked_ports)[i] = true;
    }
}

void NatMachine::reset_nat(uint16 port) {

}

