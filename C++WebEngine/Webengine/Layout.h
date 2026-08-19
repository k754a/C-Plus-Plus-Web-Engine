#pragma once
#ifndef  GET_LAYOUT
#define GET_LAYOUT

#include <string> 
#include <vector>
#include <SDL3/SDL.h> //include the SDL3 lib
#include <SDL3_ttf/SDL_ttf.h>

inline bool darkmode = true; //setting to handle "dark mode" -> we take the rgb for example 0, 0, 0, and subtract 255 by each one, so then it becomes -> 255, 255, 255, we do this for everything.     


enum class NODETYPE { START, TEXT, END }; //NODETYPE holds 1 value out of 3 types (a fancy bool)

struct Node {
	NODETYPE tag; //our tag (START, TEXT, END)
	std::string tagValue; //the value of the tag, like <'a'>
	std::vector<Node*> children; //Node contains our children, and there properties, recalling this function

	Node* Parent = nullptr; //our parent (starts unassigned)

	std::string href = ""; //If we contain a link
	std::string src = ""; //If we contain an image

    int measuredWidth = 0; //hold the measured Width
    int measuredHeight = 0; //hold the measured Height
    bool measured = false;  //so we don't measure more than once!
    bool isFlexContainer = false; //determine if we are in a div
	std::string className = ""; //holds css class names, like class="example-class"
	std::string idName = ""; //holds the id name, like id="example-id"
};


struct Layout
{
	Node* node = nullptr;

	int x = 0 ; int y = 0; //the x and y pos

	int width = 0; int height = 0; //handle the width and height of the text

	int fontSize = 0; 

	SDL_Texture* textTex = nullptr; //we are gonna prerender the text, so we don't gotta do it on the fly.

	SDL_Color textColor = { 0, 0, 0, 255 }; // txt colors

	//we set the 0,0,0,0 to keep it transparent

	SDL_Color bgColor = { 0, 0, 0, 0 }; //bg color

	bool hasBg = false; //start off false

	std::string href; //we have non links "" clickable ones are filled in!



	//images
	SDL_Texture* imageTex = nullptr; 

	//this will speed up stuff
	bool isImage = false;

    int loadGen = 0; //set when the layout is created.
	//because we attempt to pull multiple times, we gotta make sure we don't get stuck in a loop
	bool imageAttempted = false;


	//we add our loaded surfaces to this, to save time
    SDL_Surface* pendingSurface = nullptr ;

    //for text
    SDL_Surface* pendingTextSurface = nullptr ;

    bool textAttempted = false;

};

//pull the tokendata
int LayoutTree(Node* node);

#endif // ! Parser
