// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



//we include the parser for the using a file
#include "Parser.h"



#include <regex>
#include <iostream>
#include "ConnectSocket.h" //pulls our validate string global libary
#include <fstream>  //for loading local files
#include <sstream> // for the buffer
//this checks if a url is valid or not.
const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
bool USELOCALFILE = FALSE; //use internet, set this to false.


int main()
{
    //this is used for a higher performace gain, it removes some of the checks for std::cout
    std::ios_base::sync_with_stdio(false);
    //this stops automatic flushing.
    std::cin.tie(NULL);


    //THERE IS NOW A SETTING SO THAT YOU CAN TEST IT WITH JUST HTML FILES DIRECTLY!
    //this will be off in build versions tho.
    if (!USELOCALFILE)
    {
        StartWinSock();
        std::string input = "";
        //First, we need to prompt the user, what URL are they attempting to connect too?
        std::cout << "Type the URL to open it: "; //CAN NOW BE HTTPS!!!!

        //pulls the input, then creates a new line for cleaness
        std::cin >> input; std::cout << std::endl;


        //these 2 blocks autodetect if its a https or http.

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
            std::wstring temp(input.begin(), input.end());
            ConnectSocketHTTPS(temp);
            //now that we understand its a valid url, lets attempt a socket connect.
            //[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
        }
        
        EndWinSock();
    }
    else {
        //load the file using fsstream
        std::ifstream file("TestDoc.html");

        if (!file.is_open()) {
            std::cout << "Could not open local file." << std::endl;
            return -1;
        }
        //we dump the file into a buffer
        
        std::stringstream buffer ;

        //load it in 
        buffer << file.rdbuf();

        //now we load the full file into a temp var
        //we use the buffer and convert it into a string
        std::string fileinfo = buffer.str();

        //now we do something diffrent, we just inject it right into the parser to have the same effect
        Parser(fileinfo);


    }

    
    return(0); 
}

