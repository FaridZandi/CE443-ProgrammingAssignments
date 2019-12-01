#include <iostream>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char const *argv[]) {
    int PORT;

    PORT = std::stoi(argv[1]);

    std::string command;

    bool connected = false;
    int global_sock = -1;
    int listener_sock = -1;

    struct sockaddr_in serv_addr;

    while (true) {
        std::cin >> command;

        if (command == "connect") {
            int dest_port;
            std::cin >> dest_port;

            if (connected) {
                std::cout << "invalid command" << std::endl;
            } else {
                if ((global_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    global_sock = -1;
                    return 0;
                }

                memset(&serv_addr, '0', sizeof(serv_addr));

                struct sockaddr_in client_addr;

                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(dest_port);

                client_addr.sin_family = AF_INET;
                client_addr.sin_addr.s_addr = INADDR_ANY;
                client_addr.sin_port = htons(PORT + 1);

                int opt = 1;
                // Forcefully attaching socket to the port 8080
                if (setsockopt(global_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
                    return 0;
                }


                if (bind(global_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                    return 0;
                }

                // Convert IPv4 and IPv6 addresses from text to binary form
                if (inet_pton(AF_INET, "192.168.205.246", &serv_addr.sin_addr) <= 0) {
                    return 0;
                }

                if (connect(global_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                    std::cout << "invalid command" << std::endl;
                    continue;
                }

                std::cout << "connected to " << dest_port << std::endl;

                connected = true;
            }
        } else if (command == "send") {
            std::string message;
            std::cin.ignore(1);
            std::getline(std::cin, message);
            message += "\n";

            if (!connected) {
                std::cout << "invalid command" << std::endl;
            } else {
                send(global_sock, message.c_str(), message.length(), 0);
            }
        } else if (command == "exit") {
            if (!connected) {
                std::cout << "invalid command" << std::endl;
            } else {
                if (close(global_sock) < 0) {
//                    std::cout << "couldn't close the socket." << std::endl;
                };
                global_sock = -1;
                connected = false;
            }
        } else if (command == "listen") {
            if (connected) {
                std::cout << "invalid command" << std::endl;
            } else {
                int valread;
                char buffer[1024] = {0};
                struct sockaddr_in address;
                int addrlen = sizeof(address);

                if ((listener_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
                    return 0;
                }

                int opt = 1;
                if (setsockopt(listener_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
                    return 0;
                }

                address.sin_family = AF_INET;
                address.sin_addr.s_addr = INADDR_ANY;
                address.sin_port = htons(PORT);

                // Forcefully attaching socket to the port 8080
                if (bind(listener_sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
                    return 0;
                }

                if (listen(listener_sock, 3) < 0) {
                    return 0;
                }

                if ((global_sock = accept(listener_sock, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
                    return 0;
                }

                std::cout << "connected to " << ntohs(address.sin_port) << std::endl;
                connected = true;
            }
        } else if (command == "recv") {
            if(!connected){
                std::cout << "invalid command" << std::endl;
            } else {
                char buffer[1025] = {0};
                int valread;

                bool finished = false;
                bool notEmpty = false;
                bool firstPass = true;

                while(!finished){
                    if ((valread = recv(global_sock, buffer, 1024, 0)) > 0) {

                        if(firstPass && valread != 0){
                            notEmpty = true;
                            std::cout << "recv ";
                        }

                        for (int j = 0; j < valread; ++j) {
                            if(buffer[j] == '\n'){
                                finished = true;
                                buffer[j] = '\0';
                                break;
                            }
                        }

                        buffer[1024] = '\0';

                        std::cout << buffer;

                        for (int i = 0; i < valread; ++i) {
                            buffer[i] = '\0';
                        }

                        firstPass = false;
                    }else{
                        finished = true;
                    }
                }
                if(notEmpty){
                    std::cout << std::endl;
                }
            }
        } else {
            std::cout << "invalid command" << std::endl;
        }
    }
}

