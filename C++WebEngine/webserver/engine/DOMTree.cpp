//THIS IS THE DOM TREE.

#include <iostream>
#include "DOMTree.h"
#include "Layout.h"
#include <string>
#include <cctype>

//this is super slow, so i dont use it lol, but its a good way to see it
void PrintDomTree(Node* node, int debth = 0)
{
        //check if we have anything, we break;
        if (node == nullptr) return ;
        std::string indent = ""; //to start

        //for the len of debth, increase the spacing
        for (int i = 0; i < debth; i++)
        {
                indent = indent + "│  ";
        }

        //now we check the current node, and see what type it is
        if (node->tag == NODETYPE::START)
        {
                //ok we have text, lets print somthin
                std::cout << indent << "├── [" << node->tagValue << "]" << std::endl;
        }
        if (node->tag == NODETYPE::TEXT)
        {
                std::cout << indent << "└──── \" " << node->tagValue << " \"" << std::endl;
        }
        for (int i = 0; i < node->children.size(); i++)
        {
                PrintDomTree(node->children[i], debth + 1);
        }
}

std::vector<CSSRule> globalCSS;

int CSSDOM(std::vector<CSSRule> tokens) {
        globalCSS = tokens;
        return 0;
}


std::string CollapseWhitespace(const std::string& input)
{
        //result string
        std::string result = "";

        //check if its whitespace
        bool inWhitespace = false;

        for (char c : input)
        {
                // Treat newlines, carriage returns, tabs, and spaces all as whitespace
                if (c == '\n' || c == '\r' || c == '\t' || c == ' ')
                {
                        if (!inWhitespace)
                        {
                                result += ' '; // Replace the whole block of whitespace with one space
                                inWhitespace = true; //say that we have whitespace!
                        }
                }
                else
                {
                        result += c;
                        inWhitespace = false;
                }
        }

        //lets trim the extras, to keep it clean
        if (!result.empty() && result.front() == ' ') result.erase(0, 1);
        if (!result.empty() && result.back() == ' ') result.pop_back();

        return result;
}


int DOM(std::vector<Token> tokens)
{


        std::cout << "-------------------" << std::endl;

        //now lets make a node that helps out with the loading.
        //we create a new starting tag, and this will be the tag that updates, and adds to our save node
        Node* Root = new Node(); //this is the main node, because we set SAVE to fill the same area in ram as root, letting us save data.
        Root->tagValue = "ROOT";
        Root->tag = NODETYPE::START;
        //we set the parent to 0, because this is the base.
        Root->Parent = nullptr;


        //ok, lets make our main tag that will save everything we do
        Node* Save = Root; //we set it to our root, as thats our first point.

        //now lets make the loop
        //this loops for each token item in our Vector!
        for (int i = 0; i < tokens.size(); i++)
        {
                //lets set a val at the top, so we dont need to check so many times
                Token currentToken = tokens[i];


                //lets do the START first.
                if (currentToken.type == TokenType::START) //check if its a start token
                {


                        //ok now this is what we need to do.
                        Node* TempNode = new Node(); //first lets make a temp node.
                        //then we need to filter our stuff, currently we have a lot of junk we dont want, like <> and some other stuff the tokenizer didnt catch


                        std::string rawToken = currentToken.value;

                        size_t hrefPos = rawToken.find("href=\"");
                        if (hrefPos != std::string::npos) //if we do
                        {
                                size_t startpos = hrefPos + 6; //skip the href= part
                                size_t endpos = rawToken.find("\"", startpos); //find the end
                                if (endpos != std::string::npos)
                                {
                                        TempNode->href = rawToken.substr(startpos, endpos - startpos);
                                }
                        }

                        size_t srcPos = rawToken.find("src=\"");
                        if (srcPos != std::string::npos) //if we do
                        {
                                size_t startpos = srcPos + 5; //skip the src= part
                                size_t endpos = rawToken.find("\"", startpos); //find the end
                                if (endpos != std::string::npos)
                                {
                                        TempNode->src = rawToken.substr(startpos, endpos - startpos);
                                }
                        }




                        size_t space = rawToken.find(' '); //find if we have a " "
                        if (space != std::string::npos) //if we do
                        {
                                rawToken = rawToken.substr(0, space);
                        }

                        //find the '>'
                        space = rawToken.find('>');
                        if (space != std::string::npos) //if we do
                        {
                                rawToken = rawToken.substr(0, space); //remove it
                        }
                        //find the '<'
                        space = rawToken.find('<');
                        if (space != std::string::npos) //if we do
                        {
                                rawToken = rawToken.substr(space + 1, rawToken.length()); //remove it
                        }

                        std::string MToken = rawToken;

                        //-------------------------------------------------------------------------------------


                        //now that we have simplifyed this token, we need to acctualy assign it and stuff.

                        //these 2 assign the Token value (our Modifyed token) And the type (NODETYPE::START)
                        TempNode->tagValue = MToken;
                        TempNode->tag = NODETYPE::START;

                        //ok, now we need to set our parent
                        //this lets us link back to the save, creating a tree.
                        TempNode->Parent = Save;

                        //ok now with all our values assigned, lets send it to the save node.
                        Save->children.push_back(TempNode);



                        //finaly lets update our Current with our temp
                        //this says, move the main dir forward, like how we did it for the Root.
                        Save = TempNode;


                }


                //Lets do our next check, for Text
                if(currentToken.type == TokenType::TEXT)
                {
                        //ok, basicly the same, however we dont move up our main dir, because this is text, not a dir

                        Node* TempNode = new Node(); //make the new node.
                        TempNode->tag = NODETYPE::TEXT;
                        
                        TempNode->tagValue = CollapseWhitespace(currentToken.value);
                        TempNode->Parent = Save;


                        //make sure to write it
                        Save->children.push_back(TempNode);

                }

                //handle end tokens
                if (currentToken.type == TokenType::END)
                {
                        //ok, we want to handle these tokens, by if we get them, we go back a directory.
                        //first we make sure we dont have an issue rq
                        //prevents an error, making sure the back dir arnt nullptr
                        if (Save != nullptr && Save->Parent != nullptr)
                        {
                                //move back one level
                                Save = Save->Parent;
                        }
                }
        }

        std::cout << std::endl << "DOM TREE" << std::endl

        LayoutTree(Root);

        return 0;
}
