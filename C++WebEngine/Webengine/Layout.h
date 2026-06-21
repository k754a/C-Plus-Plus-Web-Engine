#pragma once
#ifndef  GET_LAYOUT
#define GET_LAYOUT

#include <string> 
#include <iostream>
#include <vector>



//ok, lets first define our strucutre
//currently we want to have a start tag, a text, and a end tag
//we say enum just to tell the complier that we wont be changing this
//this is saying class tokentype( start (like <p>), text (like "this is a test"), and end (like <p>))
//i added a VOID tag, and COMMENT, just for the dom tree
// 
//make sure we can import the same datatype into our DOM
//this is moved here, for cleanness
enum class NODETYPE { START, TEXT, END };
struct Node {
	NODETYPE tag; //our tag (START, TEXT, END)
	std::string tagValue; //the cleaned value, like "a" 
	std::vector<Node*> children; //make this node so that we can have depth, like stuff inside the tag
	Node* Parent = nullptr; // so we can track the parent

};


//pull the tokendata
int LayoutTree(Node* node);

#endif // ! Parser
