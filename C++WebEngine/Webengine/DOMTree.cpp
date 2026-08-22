//THIS IS THE DOM TREE.

#include "DOMTree.h" //allow the classes to be global, and import the globalCSS class.
#include "Layout.h" //pull the Layout for libs, and "node"
#include "Profiler.h" //DEBUG

std::vector<CSSToken> globalCSS; //pull our global css class, and assign it (so that any other instances in layout.cpp, works.)
bool cssUpdated = false; //set the cssUpdated to false

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
	cssUpdated = true;
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
			//TODO - any spaces screw it up, so i NEED to fix that.

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

			size_t classPos = rawToken.find("class=\""); //attempt to find the classPos
			if (classPos != std::string::npos) //if it exists
			{
				size_t StartPos = classPos + 7; //set the StartPos to start PAST, the class=\.
				size_t EndPos = rawToken.find("\"", StartPos); //find the end part
				if (EndPos != std::string::npos)
				{
					TempNode->className = rawToken.substr(StartPos, EndPos - StartPos); //grab only the inside part, like image.png.
				}
			}

			size_t idPos = rawToken.find("id=\""); //attempt to find the idPos
			if (idPos != std::string::npos) //if it exists
			{
				size_t StartPos = idPos + 4; //set the StartPos to start PAST, the class=\.
				size_t EndPos = rawToken.find("\"", StartPos); //find the end part
				if (EndPos != std::string::npos)
				{
					TempNode->idName = rawToken.substr(StartPos, EndPos - StartPos); //grab only the inside part, like image.png.
				}
			}

			size_t stylePos = rawToken.find("style=\""); //attempt to find the stylePos
			if (stylePos != std::string::npos) //if it exists
			{
				size_t start = stylePos + 7; 
				size_t end = rawToken.find("\"", start); //find the end part
				if (end != std::string::npos)
				{
					std::string rawStyle = std::string(rawToken.substr(start, (end - start)));
					TempNode->style = ""; //reset to be clean

					if (rawStyle.find("text-align:") != std::string::npos)
					{
						if (rawStyle.find("center") != std::string::npos) { TempNode->style += "center "; std::cout << "added center, "; }
						else if (rawStyle.find("left") != std::string::npos) { TempNode->style += "left "; std::cout << "added left, "; }
						else if (rawStyle.find("right") != std::string::npos) { TempNode->style += "right "; std::cout << "added right, "; }
						else if ("text-align:")
						{
							size_t verticalVal = rawStyle.find("text-align:"); //NEED TO BE like this, to prevent overalap.
							if (verticalVal != std::string::npos) {
								size_t verticalEnd = rawStyle.find_first_of("; ", verticalVal); //grab the end of the vertical link
								if (verticalEnd == std::string::npos) verticalEnd = rawStyle.size();
								std::string hexVal = rawStyle.substr(verticalVal, verticalEnd - verticalVal);
								TempNode->style += hexVal + " "; std::cout << " added text-align, " << hexVal;
							}
						}
					}

					if (rawStyle.find("vertical-align:") != std::string::npos)
					{
						if (rawStyle.find("top") != std::string::npos) { TempNode->style += "top "; std::cout << "added top, "; }
						else if (rawStyle.find("middle") != std::string::npos) { TempNode->style += "middle "; std::cout << "added middle, "; }
						else if (rawStyle.find("bottom") != std::string::npos) { TempNode->style += "bottom "; std::cout << "added bottom, "; }
						else if ("vertical-align:")
						{
							size_t verticalVal = rawStyle.find("vertical-align:"); //NEED TO BE like this, to prevent overalap.
							if (verticalVal != std::string::npos) {
								size_t verticalEnd = rawStyle.find_first_of("; ", verticalVal); //grab the end of the vertical link
								if (verticalEnd == std::string::npos) verticalEnd = rawStyle.size();
								std::string hexVal = rawStyle.substr(verticalVal, verticalEnd - verticalVal);
								TempNode->style += hexVal + " "; std::cout <<" added vertical-align, " << hexVal;
							}
						}
					}

					if (rawStyle.find("color:") != std::string::npos && rawStyle.find("color:") != rawStyle.find("background-color:")) //if we find the color term, and we cannot find the backgroundcolor
					{
						if (rawStyle.find("color:red") != std::string::npos) { TempNode->style += "textcolor:red "; std::cout << "added red color, "; } //if we find the color:red, we add to the style list, -> textcolor:red (we use textcolor to avoid issues)
						if (rawStyle.find("color:green") != std::string::npos) { TempNode->style += "textcolor:green "; std::cout << "added green color, "; } //if we find the color:green, we add to the style list, -> textcolor:green (we use textcolor to avoid issues)
						if (rawStyle.find("color:blue") != std::string::npos) { TempNode->style += "textcolor:blue "; std::cout << "added blue color, "; } //if we find the color:blue, we add to the style list, -> textcolor:blue (we use textcolor to avoid issues)


						//HANDLE RAW COLORS
						size_t rgbPos = rawStyle.find("color:rgb("); //NEED TO BE like this, to prevent overalap.
						if (rgbPos != std::string::npos){
							std::string rgbVal = rawStyle.substr(rgbPos);
							TempNode->style += "text" + rgbVal + " "; std::cout << "added rgb color, ";
						}
						
						size_t hexPos = rawStyle.find("color:#"); //NEED TO BE like this, to prevent overalap.
						if (hexPos != std::string::npos) {
							size_t hexEnd = rawStyle.find_first_of("; ", hexPos); //grab the end of the hex link
							if (hexEnd == std::string::npos) hexEnd = rawStyle.size(); 
							std::string hexVal = rawStyle.substr(hexPos, hexEnd - hexPos);
							TempNode->style += "text" + hexVal + " "; std::cout << "added hex color, ";
						}
						//handle the RGB, not just the color names
					}

					if (rawStyle.find("background-color:") != std::string::npos)
					{
						if (rawStyle.find("background-color:red") != std::string::npos) { TempNode->style += "background-color:red "; std::cout << "added bg red color, "; }
						if (rawStyle.find("background-color:green") != std::string::npos) { TempNode->style += "background-color:green "; std::cout << "added bg green color, "; }
						if (rawStyle.find("background-color:blue") != std::string::npos) { TempNode->style += "background-color:blue "; std::cout << "added bg blue color, "; }

						//HANDLE RAW COLORS
						size_t rgbPos = rawStyle.find("background-color:rgb("); //NEED TO BE like this, to prevent overalap.
						if (rgbPos != std::string::npos) {
							std::string rgbVal = rawStyle.substr(rgbPos);
							TempNode->style += rgbVal + " "; std::cout << "added bg rgb color, ";
						}

						size_t hexPos = rawStyle.find("background-color:#"); //NEED TO BE like this, to prevent overalap.
						if (hexPos != std::string::npos) {
							size_t hexEnd = rawStyle.find_first_of("; ", hexPos); //grab the end of the hex link
							if (hexEnd == std::string::npos) hexEnd = rawStyle.size();
							std::string hexVal = rawStyle.substr(hexPos, hexEnd - hexPos);
							TempNode->style += hexVal + " "; std::cout << "added bg hex color, ";
						}
						//handle the RGB, not just the color names
					}


					if (rawStyle.find("font-size:") != std::string::npos) //check for fontsize
					{
						size_t fontSize = rawStyle.find("font-size:"); //NEED TO BE like this, to prevent overalap.


						if (rawStyle.find("adjust") != std::string::npos) { TempNode->style += "font-size:adjust "; std::cout << "added font-size:adjust, "; }
						else if (rawStyle.find("fit") != std::string::npos) { TempNode->style += "font-size:fit "; std::cout << "added font-size:fit, "; }
						else if (rawStyle.find("font-size:") != std::string::npos) {
							size_t hexEnd = rawStyle.find_first_of("; ", fontSize); //grab the end of the fontSize link
							if (hexEnd == std::string::npos) hexEnd = rawStyle.size();
							std::string hexVal = rawStyle.substr(fontSize, hexEnd - fontSize);
							TempNode->style += hexVal + " "; std::cout << hexVal << " added font size, ";
						}
					}

					if (rawStyle.find("font-weight:") != std::string::npos)
					{
						if (rawStyle.find("font-weight:normal") != std::string::npos) { TempNode->style += "font-weight:normal "; std::cout << "added bg red color, "; }
						else if (rawStyle.find("font-weight:bold") != std::string::npos) { TempNode->style += "font-weight:bold "; std::cout << "added bg green color, "; }
						else if ("font-weight:")
						{
							size_t fontWeight = rawStyle.find("font-weight:"); //NEED TO BE like this, to prevent overalap.
							if (fontWeight != std::string::npos) {
								size_t hexEnd = rawStyle.find_first_of("; ", fontWeight); //grab the end of the fontWeight link
								if (hexEnd == std::string::npos) hexEnd = rawStyle.size();
								std::string hexVal = rawStyle.substr(fontWeight, hexEnd - fontWeight);
								TempNode->style += hexVal + " "; std::cout << hexVal << " added font Weight, ";
							}
						}

					}
					//TODO - More of these.
					//remove the spaces in the back
					if (!TempNode->style.empty() && TempNode->style.back() == ' ')
					{
						TempNode->style.pop_back(); //remove the space
					}

					std::cout << std::endl;
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
	activeCSS = &globalCSS;
	LayoutTree(Root); //ok send it to the layout

	return 0;
}