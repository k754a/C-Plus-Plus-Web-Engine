#include <SDL3/SDL_main.h> //NEEDED FOR RELEASE BUILD


#include "GUI.h" //FOR THE GUIRENDER
#include "ConnectSocket.h" //FOR THE START WIN SOCK
#include "Parser.h" //FOR THE PARSER


#include <iostream> //FOR STD::
#include <fstream>  //FOR THE MAIN.HTML READING
#include <sstream> //NEED SSTREAM FOR THE BUFFER

//======MAIN======\\

int main(int argc, char* argv[]) //we give the start of main, a starting int and char 
{
    
    std::ios_base::sync_with_stdio(false);  std::cin.tie(NULL); //these lines allow the terminal to run a bit faster in debug, saves some time
 
    SetConsoleOutputCP(CP_UTF8); //we do this for ╨á╤â and other weird chars, to render right

   

    //=======STARTUP=======\\

    
    LoadStarredPages(); //load our pages from starred_pages.STAR, and load them into the list
    UpdateHTML();  //we update the main.html, and add our STAR pages.


    //======LOAD-MAIN-MENU======\\

    std::ifstream file("main.html"); //open on start, in read mode

    if (!file.is_open()) { //if we cannot open the file
        std::cout << "Could not open main file." << std::endl; //send an error, as we need this for new tab

        return -1; //return an error code
    }
    

   
    std::stringstream buffer;  buffer << file.rdbuf(); //create the buffer, and write the main.html file data to it

    std::string fileinfo = buffer.str(); //convert that buffer, into a string for the parser

    Parser(fileinfo); //send the new tab to the parser for rendering
        

   
    //======RENDER-GUI======\\

    GUIRENDER(); //Run the GUI loop, this again, is a loops, so we wont continue until we close it

    //======DEBUG======\\

    std::cout << "Press enter to exit...";
    std::cin.get();

    
    return(0); //end

} //END OF MAIN

