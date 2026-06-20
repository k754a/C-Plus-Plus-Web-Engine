//THIS IS THE DOM TREE.

//ok this is my plan, currently we just have a list of tokens, they are assigned with things like END tags, START tags, and TEXT, if we see an new <> we bump out a space on the tree
//then if we see a </> we bump in a space, because we already sort when we parse we all set for that.
#include <iostream>
#include "DOMTree.h"
#include <string>

enum class NODETYPE { START, TEXT, END };
struct Node {
	NODETYPE tag; //our tag (START, TEXT, END)
	std::string tagValue; //the cleaned value, like "a" 
	std::vector<Node*> children; //make this node so that we can have depth, like stuff inside the tag
	Node* Parent = nullptr; // so we can track the parent

};

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





	//ok finaly, we advance the amount of children sizes, and what this does, is it runs and routs through all the paths
	//and these branches stop when we make it to the end (nullptr). this is just a more complex way to do it.
	for (int i = 0; i < node->children.size(); i++)
	{
		PrintDomTree(node->children[i], debth + 1);
	}



		
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


			//DEBUG
			std::cout << rawToken << std::endl;
		}


		//Lets do our next check, for Text
		if(currentToken.type == TokenType::TEXT)
		{
			//ok, basicly the same, however we dont move up our main dir, because this is text, not a dir

			Node* TempNode = new Node(); //make the new node.
			TempNode->tag = NODETYPE::TEXT;
			TempNode->tagValue = currentToken.value; //set it to our current token, this does not need any refinement.
			TempNode->Parent = Save;


			//set our parent to the current dir
			

			//make sure to write it
			Save->children.push_back(TempNode);

			//we dont do the Save = TempNode, cause we dont wanna move the dir forward on the end of values.

			//DEBUG
			std::cout << currentToken.value << std::endl;

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
				std::cout << "End of Tag, Back one!" << std::endl;
			}
		}








	}
	std::cout << std::endl << "DOM TREE" << std::endl;
	PrintDomTree(Root, 0);
	
	return 0;
}




