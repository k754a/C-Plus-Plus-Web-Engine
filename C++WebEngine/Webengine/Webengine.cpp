// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



//we include the parser for the using a file
#include "Parser.h"
#include <thread>
#include "GUI.h"

#include <regex>
#include <iostream>
#include "ConnectSocket.h" //pulls our validate string global libary
#include <fstream>  //for loading local files
#include <sstream> // for the buffer
//this checks if a url is valid or not.
const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
bool USELOCALFILE = FALSE; //use internet, set this to false.



//hide the cmd on relase config
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")







std::atomic<bool> running = true; //we connect to change the bool in GUI.h
int main()
{
    //this is used for a higher performace gain, it removes some of the checks for std::cout
    std::ios_base::sync_with_stdio(false);
    //this stops automatic flushing.
    std::cin.tie(NULL);

    SetConsoleOutputCP(CP_UTF8); //we do this for ╨á╤â



    //before we do anything, we need to load our screen.
    
    StartWinSock();


    //ConnectSocketHTTPS();

    if (USELOCALFILE)
    {
        //load the file using fsstream
        std::ifstream file("TestDoc.html");

        if (!file.is_open()) {
            std::cout << "Could not open local file." << std::endl;
            return -1;
        }
        //we dump the file into a buffer

        std::stringstream buffer;

        //load it in 
        buffer << file.rdbuf();

        //now we load the full file into a temp var
        //we use the buffer and convert it into a string
        std::string fileinfo = buffer.str();

        //now we do something diffrent, we just inject it right into the parser to have the same effect
        Parser(fileinfo);

    }

    //THERE IS NOW A SETTING SO THAT YOU CAN TEST IT WITH JUST HTML FILES DIRECTLY!
    //this will be off in build versions tho.
   

    GUIRENDER();



    //kill the thread
    running = false;
   // GUITHREAD.join(); //join the main thread up with this, to end it nicely
    //if we dont do this, we get errors lol

    EndWinSock();
    
    return(0); 
}

