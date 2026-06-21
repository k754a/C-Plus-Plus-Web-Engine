#pragma once
#ifndef  GET_LAYOUT
#define GET_LAYOUT

#include <string> 
#include <iostream>
#include <vector>
#include <SDL3/SDL.h> //include the SDL3 lib
#include <SDL3_ttf/SDL_ttf.h>



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


struct Layout
{
	Node* node;

	int x; int y; //the x and y pos

	int width; int hight; //handle the width and hight of the text

	int fontSize; 

	SDL_Texture* textTex; //we are gonna prerender the text, so we dont gotta do it on the fly.
};
//pull the tokendata
int LayoutTree(Node* node);

#endif // ! Parser
