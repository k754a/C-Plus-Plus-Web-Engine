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
typedef unsigned long u_long;
#endif

#include <string> // for stoi
#include <iostream>
#include <vector>

//pull the validate string
int ConnectSocketHTTP(std::string input);
// CHANGED WITH AI: Windows used std::wstring for HTTPS; web port uses std::string (curl).
int ConnectSocketHTTPS(std::string input);
// CHANGED WITH AI: Ported DownloadBytes from the Windows engine (uses curl on Linux)
// so the web engine can fetch image bytes for the layout.
std::vector<unsigned char> DownloadBytes(std::string url);
int EndWinSock();
int StartWinSock();


#endif // ! ValidateString
