#pragma once
//define class and include a guard to make sure its not used multiple times
#ifndef  GET_ValidateString
#define GET_ValidateString

//CHANGED WITH AI: Added cross-platform support for linux server port
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h> //for pton
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include <string> // for stoi
#include <iostream>

//pull the validate string
int ConnectSocket(std::string input);
int EndWinSock();
int StartWinSock();


#endif // ! ValidateString
