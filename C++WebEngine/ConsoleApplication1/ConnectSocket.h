#pragma once
//define class and include a guard to make sure its not used multiple times
#ifndef  GET_ValidateString
#define GET_ValidateString

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string> // for stoi
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> //for pton

//pull the validate string
int ConnectSocket(std::string input);
int EndWinSock();
int StartWinSock();


#endif // ! ValidateString
