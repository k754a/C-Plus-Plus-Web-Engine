//THIS IS THE DOM TREE.

//ok this is my plan, currently we just have a list of tokens, they are assigned with things like END tags, START tags, and TEXT, if we see an new <> we bump out a space on the tree
//then if we see a </> we bump in a space, because we already sort when we parse we all set for that.
#include <iostream>
#include "DOMTree.h"
#include <string>
#include "Layout.h"


std::vector<CSSToken> globalCSS;

int CSSDOM(std::vector<CSSToken> tokens) { //this returns an int, and takes in our special CSSToken class.
	//ok we have the CSSDOM
	//we don't need to do anything lol, so ill just pass it off
	globalCSS = tokens;

	return 0;



}











#include <string>


//ok, i figured out its something to do like \t \n or \r, its because our font tries to render them, but cannot!
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
			//if we dont have a \n \r or \t, we can flip it to false, prepearing for the next new line!
			result += c;
			inWhitespace = false;
		}
	}

	//lets trim the extras, to keep it clean, check if the result is basicly just whitespace, and remove it
	if (!result.empty() && result.front() == ' ') result.erase(0, 1);
	if (!result.empty() && result.back() == ' ') result.pop_back();

	return result;
}










int DOM(std::vector<HTMLToken> tokens)
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
		HTMLToken currentToken = tokens[i];


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
					

					TempNode->href = rawToken.substr(startpos, endpos - startpos); //grab between start and len of start - end

				}
			}

			size_t srcPos = rawToken.find("src=\"");
			if (srcPos != std::string::npos) //if we do
			{
				size_t startpos = srcPos + 5; //skip the src= part
				size_t endpos = rawToken.find("\"", startpos); //find the end
				if (endpos != std::string::npos)
				{


					TempNode->src = rawToken.substr(startpos, endpos - startpos); //grab between start and len of start - end

			
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

			//these 2 assign the HTMLToken value (our Modifyed token) And the type (NODETYPE::START)
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
			TempNode->tagValue = CollapseWhitespace(currentToken.value); //set it to our current token, this does not need any refinement.
			TempNode->Parent = Save;


			//set our parent to the current dir
			

			//make sure to write it
			Save->children.push_back(TempNode);

			//we don't do the Save = TempNode, cause we don't wanna move the dir forward on the end of values.

			//DEBUG
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
				//td::cout << "End of Tag, Back one!" << std::endl;
			}
		}








	}

	std::cout << std::endl << "DOM TREE" << std::endl;
	//PrintDomTree(Root, 0); //to improve performace


	//ok send it to the layout
	LayoutTree(Root);
	
	return 0;
}




