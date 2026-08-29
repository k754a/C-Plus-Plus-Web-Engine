#pragma once
#ifndef  GET_LAYOUT
#define GET_LAYOUT

#include <string>
#include <iostream>
#include <vector>
#include <atomic>

// ok, lets first define our strucutre
// this is saying class tokentype( start (like <p>), text (like "this is a test"), and end (like <p>))
enum class NODETYPE { START, TEXT, END };

//make a class to hold it ig
struct WebColor {
    unsigned char r = 0, g = 0, b = 0, a = 255;
};

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
    bool measured = false;  //so we dont measure more than once!
};

struct Layout
{
    Node* node = nullptr;

    int x = 0; int y = 0; //the x and y pos
    int width = 0; int hight = 0; //handle the width and hight of the text

    int fontSize = 0;

    WebColor textColor; // txt colors

    //we set the 0,0,0,0 to keep it transparent
    WebColor bgColor = { 0, 0, 0, 0 }; //bg color

    bool hasBg = false; //start off false

    std::string href = ""; //we have non links "" clickable ones are filled in!

    //images
    std::string imageSrc = "";
    bool isImage = false;
};

//pull the tokendata
int LayoutTree(Node* node);

#endif // ! Parser
