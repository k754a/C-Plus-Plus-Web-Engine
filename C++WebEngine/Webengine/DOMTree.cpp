//THIS IS THE DOM TREE.

#include "DOMTree.h" //allow the classes to be global, and import the globalCSS class.
#include "Layout.h" //pull the Layout for libs, and "node"
#include "Profiler.h" //DEBUG

std::vector<CSSToken> globalCSS; //pull our global css class, and assign it (so that any other instances in layout.cpp, works.)



//======COLLAPSE WHITESPACE======\\

std::string RemoveNewLineChars(const std::string& input) //this takes a const std::string, and returns a string.
{
	std::string result = ""; result.reserve(input.size()); //string to hold our final result, reserve data to be faster

	bool inWhitespace = false;  //create a bool, to check if we are currently in a whitespace.

	for (unsigned char c : input) //loop through each char in our 'input'
	{
		//treat newlines, carriage returns, tabs, and spaces all as whitespace
		if (c == '\n' || c == '\r' || c == '\t' || c == ' ')
		{
			//if we are not inside whitespace currently, and that means we prob have a ' '
			if (!inWhitespace)
			{
				//dont do anything if the result is blank
				if (!result.empty())
					result.push_back(' '); //if its not blank, we add a new space, like "EXAMPLE" -> "EXAMPLE "
			}

			inWhitespace = true; //we are in whitespace, so now we start to skip chars, for example "EXAM    PLE" -> "EXAM  PLE" -> "EXAM PLE" -> "EXAMPLE"
		}
		else
		{

			result.push_back(c); //we dont have whitespace, so we can add to our result so we go "E"  -> "X" ...
			inWhitespace = false; //we found a normal char, we are not in whitespace
		}
	}

	//check if the final char is a space, if it is, we remove it.
	if (!result.empty() && result.back() == ' ')
		result.pop_back(); //remove it.

	return result; //return our final cleaned string
} //END OF REMOVE NEW LINE CHARS






//======CSSDOM======\\

int CSSDOM(std::vector<CSSToken> tokens) { //this takes in our custom CSSToken class ({ std::string id; std::vector<std::string> properties; }), and returns an int
	globalCSS = tokens; //assign the globalCSS to the current token list, in parser.cpp.

	return 0; //done, return 0;
}



//======DOM======\\

int DOM(const std::vector<HTMLToken> tokens) //this takes a custom HTMLTOKEN vector ({ TokenType type; std::string value; }), and returns a 'int' 
{
	PROFILE("DOM"); //PROFILE THE DOM

	
	Node* Root = new Node(); //Create our starting node, this holds a lot of data, and we will add to it, as we index.
	Root->tagValue = "ROOT"; //For the starting node, we set the starting one's id to "ROOT"
	Root->tag = NODETYPE::START; //Set the NODETYPE to start, as this is the start of the tree.
	
	Root->Parent = nullptr; //we set the parent too null, as its the base, and does not contain a parent


	
	Node* Temp = Root; //this "Temp" Node, will get the current window, and send it back the ROOT.

	
	for (size_t i = 0; i < tokens.size(); i++) //go through the size of the tokenList
	{
		
		const HTMLToken& currentToken = tokens[i]; //index the current token at the start, for performance.


		//check if its a "START" tag.
		if (currentToken.type == TokenType::START) //check if its a start token
		{


			
			Node* TempNode = new Node(); //Temp node, to temporally assign values.
			

			std::string_view rawToken = currentToken.value;  //get the current token value, and assign it to a string


			size_t hrefPos = rawToken.find("href=\""); //attempt to find 'href=\'

			if (hrefPos != std::string::npos) //if the herfPos condition is met (it returns a value)
			{
				size_t StartPos = hrefPos + 6; //set the startpos to start PAST, the herf.
				size_t EndPos = rawToken.find("\"", StartPos); //find the end part of our raw token. so our href= 'https://google.com'/
				if (EndPos != std::string::npos)
				{
					

					TempNode->href = rawToken.substr(StartPos, EndPos - StartPos); //grab the 'https://google.com' inside part

				}
			}


			size_t srcPos = rawToken.find("src=\""); //attempt to find 'src=\'
			if (srcPos != std::string::npos) //if the srcPos condition is met (it returns a value)
			{
				size_t StartPos = srcPos + 5; //set the StartPos to start PAST, the src.
				size_t EndPos = rawToken.find("\"", StartPos); //find the end part
				if (EndPos != std::string::npos)
				{


					TempNode->src = rawToken.substr(StartPos, EndPos - StartPos); //grab only the inside part, like image.png.

			
				}
			}

			size_t space = rawToken.find(' '); //find if we have a space
			if (space != std::string::npos) //if the condition is met
			{
				rawToken = rawToken.substr(0, space); //we remove the space.
			}
			space = rawToken.find('>'); //find if we have a '>'
			if (space != std::string::npos) //if the condition is met
			{
				rawToken = rawToken.substr(0, space); //remove it
			}
			space = rawToken.find('<'); //find if we have a '>'
			if (space != std::string::npos) //if the condition is met
			{
				rawToken = rawToken.substr(space + 1, rawToken.length()); //remove it
			}

			
			std::string_view MToken = rawToken; //temp var to hold our modified (cleaned) token.









		

			//-------------------------------------------------------------------------------------


			//now that we have simplified this token, we need to actually assign the values towards it

			//these 2 assign the HTMLToken value (our Modified token) And the type (NODETYPE::START)
			TempNode->tagValue = MToken;
			TempNode->tag = NODETYPE::START;

			//ok, now we need to set our parent
			//this lets us link back to the save, creating a tree.
			TempNode->Parent = Temp;

			//ok now with all our values assigned, lets send it to the save node.
			Temp->children.push_back(TempNode);



			//finally lets update our Current with our temp
			//this says, move the main dir forward, like how we did it for the Root.
			Temp = TempNode;
		}


		
		if(currentToken.type == TokenType::TEXT) //check if its TEXT or not.
		{
			//ok, basically the same, however we don't move up our main dir, because this is text, not a dir

			Node* TempNode = new Node(); //make a temp node to store our values
			TempNode->tag = NODETYPE::TEXT; //set the type to NODETYPE::TEXT;
			TempNode->tagValue = RemoveNewLineChars(currentToken.value); //set the tag value to our currentToken.value, but remove things like /n
			TempNode->Parent = Temp; //Set our parent to the Temp node


			//make sure to write it
			Temp->children.push_back(TempNode);

			//we don't do the Temp = TempNode, cause we don't wanna move the dir forward on the end of values.

		}

		//handle end tokens
		if (currentToken.type == TokenType::END)
		{
			//ok, we want to handle these tokens, by if we get them, we go back a directory.
			
			
			if (Temp != nullptr && Temp->Parent != nullptr) //check that our Temp dir is not null, and our parent for the tag is not null
			{
				//if we pass
				Temp = Temp->Parent; //we have passed, lets move our cursor up one.
				
			}
		}

	}


	LayoutTree(Root); //ok send it to the layout

	return 0;
}




