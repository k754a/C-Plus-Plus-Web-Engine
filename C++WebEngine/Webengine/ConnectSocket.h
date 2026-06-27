#pragma once
//define class and include a guard to make sure its not used multiple times
#ifndef  GET_ValidateString
#define GET_ValidateString

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string> // for stoi
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> //for pton
#include <vector>

//pull the validate string
int ConnectSocketHTTP(std::string input);
int ConnectSocketHTTPS(std::wstring input);
std::vector<unsigned char> DownloadBytes(std::string url);
int EndWinSock();
int StartWinSock();


#endif // ! ValidateString
