#pragma once
#ifndef  GET_DOMTREE
#define GET_DOMTREE

#include <string>
#include <iostream>
#include <vector>

//ok, lets first define our strucutre
//currently we want to have a start tag, a text, and a end tag
//this is saying class tokentype( start (like <p>), text (like "this is a test"), and end (like <p>))
//i added a VOID tag, and COMMENT, just for the dom tree
enum class TokenType { START, TEXT, END, VOID };
struct Token { TokenType type; std::string value; };


// can now parse and apply <style> rules (font-size, color, background, flex, absolute).
struct CSSRule {
    std::string id; //save the id, like h1
    std::vector<std::string> properties;
};

extern std::vector<CSSRule> globalCSS;

//pull the tokendata
int DOM(std::vector<Token> tokens);

int CSSDOM(std::vector<CSSRule> tokens);


#endif // ! Parser
