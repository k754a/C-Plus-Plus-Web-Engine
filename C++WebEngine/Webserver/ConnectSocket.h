
//MODIFYED FOR LINUX
#pragma once

#ifndef GET_ConnectSocket
#define GET_ConnectSocket

#include <string>
//now we have a check if we have winsock2.h, if we do we include it, otherwise we include the linux headers
//this isnt going to be in the main code, because i dont want to have to worry about multiple operating systems.
#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
//removed the start with and end win socket, because we dont need that no more.
int ConnectSocket(const std::string& input);

#endif