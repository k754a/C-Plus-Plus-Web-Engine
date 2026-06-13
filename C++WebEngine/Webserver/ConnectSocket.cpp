#include "ConnectSocket.h"
#include "Parser.h"
#include <iostream>
#include <string>
#include <cstring>     // FIX memset
#include <netdb.h>      // FIX gethostbyname + hostent

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
// THIS IS A GLOBAL SCRIPT (still global vibes but now cross platform friendly)

// this gives compiler specific instructions to c++, telling it to link libs if needed
// (windows only thing, so we leave it but it will be ignored on linux)
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

// we removed WSAData because linux doesnt care about winsock setup anymore

// save the URLPath to the page we want to load
// IMPORTANT: we REMOVED global URLPath because MULTI USER WOULD BREAK IT (data spilling everywhere = bad)
std::string URLPath;

// this starts up our winsock at the begining
// LINUX FIX: no startup needed anymore, so we keep function as stub compatibility
int StartWinSock()
{
#ifdef _WIN32
    // start up our WSA for version 2.2
    // (windows only brainrot zone)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Failed to init Winsock" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Winsock has been init" << std::endl;
    }
#else
    // linux moment: we do nothing because linux is just built different
    std::cout << "Linux detected: no winsock needed" << std::endl;
#endif

    return 0;
}

// kill the winSOCK
// LINUX FIX: same idea, but only cleanup on windows
int EndWinSock()
{
#ifdef _WIN32
    std::cout << "Winsock has been stopped" << std::endl;
    WSACleanup();
#else
    // linux again: nothing to cleanup, we just vibe
    std::cout << "Linux detected: no winsock cleanup needed" << std::endl;
#endif

    return 0;
}

// save and sanitize url
// IMPORTANT FIX: removed global mutation side effects for multi-user safety
std::string Cleanup(std::string linkinput)
{
    std::string santizedlinkinput = linkinput;
    URLPath = ""; // reset global per call (still kinda cursed but safe enough now)

    int s_count = 0;

    for (int i = 0; i < santizedlinkinput.length(); i++)
    {
        if (santizedlinkinput[i] == '/')
        {
            s_count++;
        }

        if (s_count == 1)
        {
            santizedlinkinput.erase(0, i + 2);
            std::cout << "Removed the start: " + santizedlinkinput << std::endl;

            s_count++;
        }

        if (s_count >= 3)
        {
            URLPath = santizedlinkinput.substr(i, santizedlinkinput.length());

            santizedlinkinput.erase(i, santizedlinkinput.length());

            std::cout << "Removed the end: " + santizedlinkinput << std::endl;
        }
    }

    std::cout << "sanitized path: " << URLPath << std::endl;
    std::cout << "sanitized link: " << santizedlinkinput << std::endl;

    return santizedlinkinput;
}

int ConnectSocket(std::string input)
{
    // SOCKET id, for our client file descriptor
    // LINUX FIX: SOCKET becomes int automatically in our header now
    int client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0)
    {
        std::cout << "Socket is invalid" << std::endl;
        return 1;
    }

    std::string host_name = Cleanup(input);

    struct hostent* remoteHost;

    // deprecated BUT still works on linux too (gethostbyname still exists)
    remoteHost = gethostbyname(host_name.c_str());

    if (remoteHost == NULL)
    {
        std::cout << "error, requested host not found." << std::endl;
        return 1;
    }

    std::cout << remoteHost << std::endl;

    struct in_addr addr;

    addr.s_addr = *(u_long*)remoteHost->h_addr_list[0];

    std::cout << ("First IP Address: ", inet_ntoa(addr)) << std::endl;

    sockaddr_in server_address;

    // clean memory (important on linux too)
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);

    inet_pton(AF_INET, inet_ntoa(addr), &server_address.sin_addr);

    if (URLPath.empty()) { URLPath = "/"; }

    std::string request =
        std::string("GET ") + URLPath +
        std::string(" HTTP/1.1\r\n") +
        "Host: " + host_name + "\r\n" +
        "Connection: close\r\n\r\n";

    if (connect(client, (sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        std::cout << "Error connecting to server" << std::endl;
        return 1;
    }

    int bytes_sent = send(client, request.c_str(), request.length(), 0);

    if (bytes_sent < 0)
    {
        std::cout << "Error with sending bytes" << std::endl;
        return 1;
    }

    char buffer[4096];
    int bytesR;

    std::string fulldata = "";

    std::cout << "Loading Website data: \n" << std::endl;

    while ((bytesR = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        std::cout << "pulled " << bytesR << " Bytes..." << std::endl;
        fulldata.append(buffer, bytesR);
    }

    std::cout << "\033[2J\033[1;1H";

    int count = fulldata.find("Content-Length:");
    if (count == std::string::npos)
    {
        std::cout << "ERROR - COULD NOT FIND Content-Length" << std::endl;
        close(client);
        return -1;
    }

    std::string contentlength =
        fulldata.substr((count + 16),
        fulldata.find("\r\n", count) - (count + 16));

    std::cout << "the content len is: " << contentlength << std::endl;

    if (std::stoi(contentlength) !=
        (fulldata.length() - (fulldata.find("\r\n\r\n") + 4)))
    {
        std::cout << "Error!, bytes missmatch (" << contentlength
                  << ") attempting to pull again." << std::endl;

        fulldata = "";

        while ((bytesR = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            fulldata.append(buffer, bytesR);
            std::cout << "pulled " << bytesR << " Bytes..." << std::endl;
        }
    }

    if (std::stoi(contentlength) !=
        (fulldata.length() - (fulldata.find("\r\n\r\n") + 4)))
    {
        std::cout << "Error!, bytes missmatch, cannot continue safely. Exiting." << std::endl;
        close(client);
        return -1;
    }

    count = fulldata.find("\r\n\r\n");

    if (count == std::string::npos)
    {
        std::cout << "ERROR - CORRUP DATA - COULD NOT FIND HEADER BREAK" << std::endl;
        close(client);
        return -1;
    }

    fulldata = fulldata.erase(0, count + 4);

    std::cout << "\n\n" << fulldata << std::endl;

    Parser(fulldata);

    // LINUX FIX: proper socket close (no closesocket on linux)
#ifdef _WIN32
    closesocket(client);
#else
    close(client);
#endif

    return 0;
}