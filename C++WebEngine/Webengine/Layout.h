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
//this is moved here, for cleanness
enum class NODETYPE { START, TEXT, END };
struct Node {
	NODETYPE tag; //our tag (START, TEXT, END)
	std::string tagValue; //the cleaned value, like "a" 
	std::vector<Node*> children; //make this node so that we can have depth, like stuff inside the tag
	Node* Parent = nullptr; // so we can track the parent

	std::string href = ""; //we have non links "" clickable ones are filled in!
	std::string src = ""; //tell if we have images (cause links and images ids are kinda the same)


    //ok, updates to the node, adding things like w & h, so we can put in flexboxes!

    int measuredWidth = 0;
    int measuredHeight = 0;
    bool measureded = false;  //so we dont measure more than once!
    bool isFlexContainer = false; //handle <divs> 

};


struct Layout
{
	Node* node;

	int x; int y; //the x and y pos

	int width; int hight; //handle the width and hight of the text

	int fontSize; 

	SDL_Texture* textTex = nullptr; //we are gonna prerender the text, so we dont gotta do it on the fly.

	SDL_Color textColor; // txt colors

	//we set the 0,0,0,0 to keep it transparent

	SDL_Color bgColor = { 0, 0, 0, 0 }; //bg color

	bool hasBg = false; //start off false

	std::string href = ""; //we have non links "" clickable ones are filled in!



	//images
	SDL_Texture* imageTex = nullptr; 

	//this will speed up stuff
	bool isImage = false;


	//because we attempt to pull multiple times, we gotta make sure we dont get stuck in a loop
	bool imageAttempted = false;


	//we add our loaded surfaces to this, to save time
    std::atomic<SDL_Surface*> pendingSurface{ nullptr };


   

    //we make copies so that SDL_Surface can use this lol
    Layout() = default;

    // 2. Custom Copy Constructor
    Layout(const Layout& other)
    {
        node = other.node;
        x = other.x;
        y = other.y;
        width = other.width;
        hight = other.hight;
        fontSize = other.fontSize;
        textTex = other.textTex;
        textColor = other.textColor;
        bgColor = other.bgColor;
    
         href = other.href; 
         hasBg = other.hasBg;
        isImage = other.isImage;
        imageTex = other.imageTex;
        imageAttempted = other.imageAttempted;

        pendingSurface.store(other.pendingSurface.load());
    }

    // 3. Custom Copy Assignment Operator
    Layout& operator=(const Layout& other)
    {
        if (this != &other)
        {
            node = other.node;
            x = other.x;
            y = other.y;
            width = other.width;
            hight = other.hight;
            fontSize = other.fontSize;
            textTex = other.textTex;
            textColor = other.textColor;
            bgColor = other.bgColor;
             href = other.href;
             hasBg = other.hasBg;
            isImage = other.isImage;
            imageTex = other.imageTex;
            imageAttempted = other.imageAttempted;

          
            pendingSurface.store(other.pendingSurface.load());
        }
        return *this;
    }
};




//pull the tokendata
int LayoutTree(Node* node);

#endif // ! Parser
