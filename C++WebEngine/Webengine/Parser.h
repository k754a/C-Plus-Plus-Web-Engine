#ifndef  GET_PARSE
#define GET_PARSE

#include <string> //For std::string class
#include <iostream> //FOR STD::
#include <algorithm> //for std::transform, and others
#include <vector> //For std::vector class
#include <unordered_set> //For std::unordered_set class
#include "Layout.h" //included for the node tree

int Parser(std::string input); //Returns an "int" while taking in an input.
void DeleteTree(Node* node); //Destorys the current NODE
#endif // ! Parser
	