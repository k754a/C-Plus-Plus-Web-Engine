// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <regex>
#include <iostream>
#include "ConnectSocket.h" //pulls our validate string global libary

//this checks if a url is valid or not.
const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

int main()
{
    StartWinSock();
    std::string input = "";
    
    //CHANGED WITH AI: Infinite loop to keep the engine running for continuous use
    while (true) {
        //First, we need to prompt the user, what URL are they attempting to connect too?
        std::cout << "\n\nType the URL to open it: ";

        //pulls the input, then creates a new line for cleaness
        if (!(std::cin >> input)) {
            // Disconnected or EOF
            break;
        }
        std::cout << std::endl;

        //check if this is a valid url
        //note, this will assume that this is a valid url, if it follows the design scheme, but it may not be, so we then do a network test on the server (attempt to check its status)
       
        if (std::regex_match(input, httpPattern)) {
            std::cout << "http url!" << std::endl;
            ConnectSocketHTTP(input);
            //now that we understand its a valid url, lets attempt a socket connect.
            //[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
        }


        if (std::regex_match(input, httpsPattern)) {
            std::cout << "https url!" << std::endl;
          
            ConnectSocketHTTPS(input);
            //now that we understand its a valid url, lets attempt a socket connect.
            //[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
        }

       
    }
    
    EndWinSock();
    return(0); 
}

