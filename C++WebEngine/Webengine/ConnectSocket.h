#pragma once
//define class and include a guard to make sure its not used multiple times
#ifndef  GET_CONNECTSOCKET
#define GET_CONNECTSOCKET


#include <string> //for using std::string classes
#include <iostream>  //FOR STD::
#include <vector> //For using std::vector classes
#include <fstream> //for loading images localy

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS //AVOID ERRORS WITH USING OLD STUFF FOR HTTP

	#include <ws2tcpip.h> //for pton
	#include <wininet.h> //for all connections to https servers.
#endif // _Win32



//pull the validate string
int ConnectSocketHTTPS(std::wstring input); //DEFINE OUR HTTPS FUNCT (USES A WSTRING)
std::vector<unsigned char> DownloadImages(std::string url, bool usingLocal); //DEFINE OUR DOWNLOADING IMAGES FUNCT (A VECTOR CONTATING THE BYTES)

#endif // ! GET_CONNECTSOCKET
