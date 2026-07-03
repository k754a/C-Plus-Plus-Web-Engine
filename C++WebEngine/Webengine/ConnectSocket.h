#pragma once
//define class and include a guard to make sure its not used multiple times
#ifndef  GET_CONNECTSOCKET
#define GET_CONNECTSOCKET

#define _WINSOCK_DEPRECATED_NO_WARNINGS //AVOID ERRORS WITH USING OLD STUFF FOR HTTP
#include <string> //FOR STD::
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> //for pton
#include <vector>
#include <windows.h>
#include <wininet.h>

//pull the validate string
int ConnectSocketHTTPS(std::wstring input); //DEFINE OUR HTTPS FUNCT (USES A WSTRING)
std::vector<unsigned char> DownloadImages(std::string url); //DEFINE OUR DOWNLOADING IMAGES FUNCT (A VECTOR CONTATING THE BYTES)
int StartWinSock(); //FUNCT TO HANDLE THE START OF WIN SOCK
int EndWinSock(); //FUNCT TO HANDLE THE END OF WIN SOCK

#endif // ! GET_CONNECTSOCKET
