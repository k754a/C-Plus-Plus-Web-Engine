#pragma once
#ifndef  GET_DOMTREE
#define GET_DOMTREE

#include <string> //For std::string
#include <vector> //For std::vector



enum class TokenType { START, TEXT, END}; //Create a class, with the 3 defining points START, TEXT, END.
struct HTMLToken { TokenType type; std::string value;}; //Create a struct, that holds our defining points, and a value.

int DOM(std::vector<HTMLToken> tokens); //define our class, it returns an 'int' and takes in our custom HTMLToken class.


struct CSSToken { std::string id; std::vector<std::string> properties; }; //create a struct that holds our CSSToken
extern std::vector<CSSToken> globalCSS; //create an external vector class, a list that holds the CSSToken struct, allowing us to define many tokens in a list. This is used in multiple classes.
extern bool cssUpdated;
extern std::vector<CSSToken>* activeCSS;

int CSSDOM(std::vector<CSSToken> tokens); //define our class, it returns an 'int' and takes in our custom CSSToken class.


#endif // ! Parser
