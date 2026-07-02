#ifndef  GET_PARSE
#define GET_PARSE

#include <string> // for stoi
#include <iostream>

//pull the data we downloaded
int Parser(std::string input);

// CHANGED WITH AI: Ported from the Windows engine — pulls the <title> tag.
std::string pullTITLE(std::string htmldata);

#endif // ! Parser
