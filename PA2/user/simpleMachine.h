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

#ifndef _SI_M_H_
#define _SI_M_H_

#include "sm.h"
#include "../base/frame.h"
#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct
#include <vector>


struct ethernet_header {
    byte dst[6];
    byte src[6];
    uint16 type;
} __attribute__ ((packed));

struct udp_header {
    uint16 source_port;
    uint16 dest_port;
    uint16 length;
    uint16 checksum;
}__attribute__ ((packed));

struct frame_header {
    ethernet_header eth;
    iphdr iph;
    udp_header udph;
    byte data_type_id;
}__attribute__ ((packed));

struct packet_data{
    uint32 local_ip;
    uint16 local_port;
    uint32 public_ip;
    uint16 public_port;
}__attribute__ ((packed));;


enum DataType {
    REQUEST_ASSIGNING_ID = 0,
    RESPONSE_ASSIGNING_ID = 0,
    DROP = 0,
    REQUEST_GETTING_IP = 1,
    RESPONSE_GETTING_IP = 1,
    REQUEST_LOCAL_SESSION = 2,
    RESPONSE_LOCAL_SESSION = 2,
    REQUEST_PUBLIC_SESSION = 2,
    RESPONSE_PUBLIC_SESSION = 2,
    MESSAGE = 3,
    NAT_UPDATED = 4,
    REQUEST_UPDATING_INFO = 5,
    STATUS = 6,
    STATUS_RESPOND = 7,
};

class SimpleMachine {
private:
    const SimulatedMachine *simulatedMachine;

protected:
    Interface *iface;

    uint32 header_length = sizeof(ethernet_header) + sizeof(iphdr) + sizeof(udp_header) + sizeof(byte);

public:
    SimpleMachine(const SimulatedMachine *simulatedMachine, Interface *iface);

    virtual ~SimpleMachine();

    virtual void initialize() =0;

    virtual void run() = 0;

    virtual void processFrame(Frame frame, int ifaceIndex) = 0;

    int getCountOfInterfaces() const;

    void printInterfacesInformation() const;

    const std::string getCustomInformation();

    bool sendFrame(Frame frame, int ifaceIndex);

    // will build a frame and set the values in the headers.
    // will leave the checksums to be zero. those fields will be set after adding the data.
    Frame frameFactory(int iface_index, uint16 source_port, uint16_t dest_port, byte data_type,
                       byte message_length, uint32_t dest_ip, byte id);

    // will determine the length of the data based on the data type and the [possible] message length.
    virtual uint16 getDataLength(byte data_type, uint16 message_length) = 0;

    // will calculate and set the checksum in the ip header.
    static void setIPChecksum(Frame &frame);

    static bool checkIPChecksum(Frame &frame);

    // will decrement the ip ttl of this frame.
    static bool decrementTTL(Frame &frame);

    // determines if the this frame was destined to this node or not.
    bool isFrameMine(Frame &frame);

    // will route the frame to correct interface based on their ip and mask.
    // if no matches are found will be sent out on interface 0 to outer world.
    void routeFrame(Frame &frame);

    int findDestinationInterface(uint32 ip);

    void sendOutFrame(Frame &frame);

    // will split the the input string to a vector to string.
    static std::vector<std::string> split(const std::string &s, char delim);

    static DataType getFrameDataType(Frame& frame);

    std::string ip_int_to_string(uint32 ip);

    // just for testing. no logic. nothing.
    void printFrame(Frame& frame);
};

#endif /* sm.h */

