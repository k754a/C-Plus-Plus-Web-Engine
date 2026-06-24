#pragma once
#ifndef  GET_DOMTREE
#define GET_DOMTREE

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
enum class TokenType { START, TEXT, END, VOID };
struct Token { TokenType type; std::string value; };


struct CSSRule {
	std::string id; //save the id, like h1
	std::vector<std::string> properties;
};

extern std::vector<CSSRule> globalCSS; //turns out i can use  this! and it makes it like a billion times easier that what i was doing before lol 

//pull the tokendata
int DOM(std::vector<Token> tokens);


int CSSDOM(std::vector<CSSRule> tokens);


#endif // ! Parser
