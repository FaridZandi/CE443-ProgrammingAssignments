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

#include "cf.h"

bool runCF (ClientFramework &cf);

int main (int argc, char *argv[]) {
//    unsigned int farid = SimpleMachine::ip_string_to_int("192.168.2.3");
//
//    std::cout << farid << std::endl;
//
//    byte* farid_bytes = SimpleMachine::ip_int_to_byte_network_order(farid);
//
//    unsigned int navid = SimpleMachine::ip_byte_network_order_to_int(farid_bytes);
//
//    std::cout << (unsigned int) farid_bytes[0] << std::endl;
//    std::cout << (unsigned int) farid_bytes[1] << std::endl;
//    std::cout << (unsigned int) farid_bytes[2] << std::endl;
//    std::cout << (unsigned int) farid_bytes[3] << std::endl;
//
//    std::cout << SimpleMachine::ip_int_to_string(navid);

    ClientFramework::init (argc, argv);

	bool res = runCF (*ClientFramework::getInstance ());

	ClientFramework::destroy ();

	if (res) {
		return 0;
	} else {
		return -1;
	}
}

bool runCF (ClientFramework &cf) {
	cf.printArguments ();

	return cf.connectToServer ()
		&& cf.doInitialNegotiations ()
		&& cf.startSimulation ();
}

