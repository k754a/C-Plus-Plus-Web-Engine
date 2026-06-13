//NOTE THIS HAS BEEN MODIFIED WITH AI
//this is because i didnt want to learn a bunch of things i wont need lol, and want to keep up development.



#include <iostream>
#include <regex>
#include <thread>
#include <vector>
#include <string>
#include <mutex>

#include "ConnectSocket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

const std::regex pattern(
    "((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)"
);

std::mutex coutMutex;

// handles each client separately
void handleClient(int clientSocket)
{
    char buffer[1024];

    while (true)
    {
        int bytes = read(clientSocket, buffer, sizeof(buffer) - 1);

        if (bytes <= 0)
            break;

        buffer[bytes] = '\0';
        std::string input(buffer);

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Client sent: " << input << std::endl;
        }

        if (std::regex_match(input, pattern))
        {
            std::string response = "valid url\n";
            send(clientSocket, response.c_str(), response.size(), 0);

            // your backend call
            ConnectSocket(input);
        }
        else
        {
            std::string response = "invalid url\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    close(clientSocket);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    std::cout << "Server running on port 8080...\n";

    std::vector<std::thread> threads;

    while (true)
    {
        int clientSocket = accept(server_fd, nullptr, nullptr);

        if (clientSocket >= 0)
        {
            threads.emplace_back(handleClient, clientSocket);
        }
    }

    close(server_fd);
    return 0;
}