//THIS WILL HANDLE THE LAYOUT OF THE ENGINE
//it will assign things like the positions of elements, and assign things like x and y



#include "Layout.h"
#include "GUI.h"
#include "DOMTree.h"
#include "Profiler.h"







#pragma region CSS Handle Code





//THIS CODE SECTION HANDLES THE SUBCOMPONETS FOR RENDERING THE CSS, THINGS LIKE FINIDING PROPERTIES, ID's, AND RGB.
//=====================================================================================================================

//FindID, and FindProperty find the id's and properties in our css tree.
//IsAbsolute, and IsFlex find if css elements have custom positioning.
//hexToRgb, and ParseHexColor, handle converting hexvalue's into RGB.


//THIS CODE HAS BEEN CREATED BY ME, YOU CAN FREELY USE IT HOWEVER YOU WANT. :)

//=====================================================================================================================






//looks through all our CSS rules to find the one that matches our tag in our tree

//=======Find ID======\\

CSSToken* FindID(const static std::string input) //FIND-ID returns in our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), and takes in a std::string
{
	//loop and check through each element, for the size of globalCSS
	for (int i = 0; i < globalCSS.size(); i++)
	{
		if (globalCSS[i].id == input) //if the current id = to the input
		{
			return &globalCSS[i]; //we return the id, and the vector string, holding the properties
		}
	}

	return nullptr; //if we cannot find it
} //END OF FIND-ID


//=======Find Property======\\

std::string FindProperty(const static CSSToken* rule, const static std::string propertyName) //FindProperty returns a std::string, and takes in our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), and a string
{
	for (int i = 0; i < rule->properties.size(); i++)
	{
		std::string current = rule->properties[i]; //grab the current property

		//we need to do this, as if we just do current = propertyName, it wont work lol
		size_t pos = current.find(propertyName);
		if (pos == 0) // our pos of the propertyName is the same
		{
			size_t colon = current.find(":");
			if (colon != std::string::npos) //if we found it
			{
				return current.substr(colon + 1); //return the data past
			}
		}
	}
	//we did'nt find it :(
	return "";
}//END OF FIND-PROPERTY


//=======IS ABSOLUTE======\\

//function to check if a node is IsAbsolute.
bool IsAbsolute(const static Node* node, int& outX, int& outY) //IsAbsolute returns a bool, and takes in our custom Node class, our outX int, and our outY int
{
	//i first want to look through the nodes tag, to see if it has it
	//we check that it has css values, if not, we return false

	//using our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), we attempt to find our ID, (with the funct above)
	CSSToken* id = FindID(node->tagValue);
	if (id == nullptr) //if we cannot find the "id" of the CSSToken
	{
		return false; //fail case.
	}

	

	std::string position = FindProperty(id, "position"); //using our FindProperty funct, we attempt to find the word "position" for example -> " position: static;" we use our id, of the tagValue (the inside value, like h1 {'example'}
	if (position.empty()) //if we cannot find the "position" text of the id
	{
		return false; //fail case.
	}

	
	//remove the spaces, check if position contains something, and the front pos, contains a space.
	while (!position.empty() && std::isspace((unsigned char)position.front()))
	{
		position.erase(position.begin()); //then we remove " " from the pos.
	}
		

	//now we check, if the 'position' value exists, in the tag.
	if (position.find("absolute") != std::string::npos)
	{
		//now, we attempt to find the "left" and the "top", we use are custom search funct, FindProperty, and attempt to find the words.
		std::string leftStr = FindProperty(id, "left");
		std::string topStr = FindProperty(id, "top");

		//remove the spaces, check if leftStr contains something, and the front pos, contains a space.
		while (!leftStr.empty() && std::isspace((unsigned char)leftStr.front()))
		{
			leftStr.erase(leftStr.begin());
		}
			
		//if the lefStr contains something, and the first bit after left is a number, ex.58, we set the outX to the digit, so 58, not just the 5 we check for.
		if (!leftStr.empty() && std::isdigit(leftStr[0])) {
			outX = std::stoi(leftStr);
		}

		//remove the spaces, check if topStr contains something, and the front pos, contains a space.
		while (!topStr.empty() && std::isspace((unsigned char)topStr.front()))
		{
			topStr.erase(topStr.begin()); //then we remove " " from the pos.
		}
			
		//if the topStr contains something, and the first bit after left is a number, ex.58, we set the outX to the digit, so 58, not just the 5 we check for.
		if (!topStr.empty() && std::isdigit(topStr[0])) 
		{
			outY = std::stoi(topStr); //then we remove " " from the pos.
		}



		return true; //we have determined that the value is absolute, so we return true

	}

	return false; //we have determined that our value is NOT absolute, so we return false.

}// END OF ISABSOLUTE


//=======IS FLEX======\\

//this to check if a node is Flex
bool IsFlex(const static Node* node) //IsFlex returns a bool, we take in a const static custom Node* class we have.
{
	
	//i first want to look through the nodes tag, to see if it has it
	//we check that it has css values, if not, we return false

	//using our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), we attempt to find our ID, (with the funct above)
	CSSToken* id = FindID(node->tagValue);
	if (id == nullptr)
	{
		return false;
	}

	
	//using our FindProperty funct, we attempt to find the word "display" for example -> " display: static;" we use our id, of the tagValue (the inside value, like h1 {'example'}
	std::string display = FindProperty(id, "display");
	if (display.empty())
	{
		return false; //fail case
	}

	
	//remove the spaces, check if position contains something, and the front pos, contains a space.
	while (!display.empty() && std::isspace((unsigned char)display.front()))
	{
		display.erase(display.begin()); //then we remove " " from the pos.
	}
		

	//now, we attempt to find "flex" in the display id, and we return true our false depending on the output (if we find it or not)
	return display.find("flex") != std::string::npos;

} //END-OF-ISABSOLUTE



//--------------------------------------------------------------------------------------------------



struct RGB { int r, g, b;  }; //Create a custom "RGB" struct, holding 3 ints, r, g, b.

//=======HEX TO RGB======\\

RGB hexToRgb(const static unsigned int hexValue) //This funct returns our custom RGB class ({ int r, g, b;  }), and takes in a single int, holding a converted hexValue, for example 3A5 -> 933 (we convert the 3A5 using ParseHexColor)
{
	RGB color; //Create a temp RGB var struct.
	color.r = (hexValue >> 16) & 0xFF; // Extract the first 2 hex digits
	color.g = (hexValue >> 8) & 0xFF;  // Extract the middle 2 hex digits
	color.b = hexValue & 0xFF;         // Extract the last 2 hex digits

	if (darkmode) //if darkmode is enabled
	{
		//convert r -> (255 -r)
		color.r = (255 - color.r);
		//convert g -> (255 -g)
		color.g = (255 - color.g);
		//convert b -> (255 -b)
		color.b = (255 - color.b);
	}
	return color; //return the RGB, r, g, b
} // END OF HEX-TO-RGB

//=======PARSE HEX COLOR======\\

RGB ParseHexColor(std::string hex) // This funct returns our custom RGB class ({ int r, g, b;  }), and takes in a string, holding in a hex value, for example '#3498DB'
{
	//removes spaces from the front, for example ' #bbbbbb' -> '#bbbbbb', we repeat while the hex is not empty, and there is a space in the front.
	while (!hex.empty() && std::isspace((unsigned char)hex.front()))
		hex.erase(hex.begin());

	//removes spaces from the back, for example '#bbbbbb ' -> '#bbbbbb', we repeat while the hex is not empty, and there is a space in the back.
	while (!hex.empty() && std::isspace((unsigned char)hex.back()))
		hex.pop_back();

	//if our "Hex" contains a value, and the first part of it is '#', we rm it.
	if (!hex.empty() && hex[0] == '#')
	{
		hex = hex.substr(1); //create a new 'substring' of the hex, without '#'
	}

	

	if (hex.size() == 3) //check if our hex size, is '3'
	{
		//this expands our string, from #bbb -> #bbbbbb
		hex = std::string() + hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
	}

	//if our hex size is empty, or our hex.size() != 6 chars
	if (hex.empty() || hex.size() != 6)
	{
		//if we fulfill the conditions above.
		RGB error = { 0,0,0, }; //we create a temp RGB, setting the color to black

		return error; //return this 'black' color.
	}

	//then, we attempt to run the piece inside the 'try' condition
	try {
		unsigned int hexValue = std::stoul(hex, nullptr, 16); //converts our hex string, into an int.

		return hexToRgb(hexValue); //return our final hexValue, however, we send it to our "hexToRgb" value, to get the RGB for 'RGB' return condition.
	}
	catch (...) //if we have an error, any error
	{
		//we want to prevent crashes, so we return black.
		RGB error = { 0,0,0, }; //we create a temp RGB, setting the color to black
		return error; //return this 'black' color.
	}


	
} //END OF PARSE-HEX-COLOR



#pragma endregion //holds our css code


#pragma region Nodes Handling Code

std::vector<Layout> layoutList; //create a custom 'vector', holding our custom Layout struct. This is a global list to hold it all


//=======Measure Nodes======\\

//Measure Nodes calculates layout dimensions of elements, and accounts for font sizes and images.
void MeasureNodes(Node* node, int fontsize) //Measure nodes is a void, returning nothing, it takes in our custom 'Node*' struct, and a int.
{
	
	if (node->measured) return; //first we pull the 'measured' val from our 'node' struct we import, if it's true, we don't want to handle it again (to save performance), so we end it early.

	//I first want to look through the nodes tag, to see if it has a tag value tagValue
	//we check that it has a tagValue, if not, we return false

	//using our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), we attempt to find our tagValue, using find ID, (with the funct above)
	CSSToken* id = FindID(node->tagValue);

	
	if (id != nullptr) //insure that the id we have attempted to find, exists, and is NOT null.
	{
		std::string fs = FindProperty(id, "font-size"); //now we create a temp string, using our 'FindProperty', ({const static CSSToken* rule, const static std::string propertyName}), we put in our 'id', and what we want to find, the result is added to 'fs'
		if (!fs.empty() && std::isdigit(fs[0])) //check that the font size property is NOT blank, and contains a number.
		{
			fontsize = std::stoi(fs) * 2; //we convert the number from a string "32" -> 32, then * by 2, to make sure its big enough.
		}
	}
	else if (node->tag == NODETYPE::START && !node->tagValue.empty()) //If we cannot find our "id" of the text, but it is a START node, and the tagValue is NOT null
	{

		//we use predetermined points, using the nodes tag value.
		if (node->tagValue == "h1") { fontsize = 96; }
		else if (node->tagValue == "p") { fontsize = 48; }
		else if (node->tagValue == "a") { fontsize = 36; }
		else if (node->tagValue == "span") { fontsize = 36; }
	}
	

	//HANDLE IMAGES
	if (node->tag == NODETYPE::START && node->tagValue == "img" && !node->src.empty()) //check if its a START node, the tagValue contains "img", and the link of the img is NOT empty.
	{
		///now we set some temp sizes
		node->measuredWidth = 200; //W = 200
		node->measuredHeight = 150; //H = 200
		node->measured = true; //set the measured flag to 'true" to insure we do NOT measure it again.
		return; //done.
	}




	//HANDLE TEXT
	if (node->tag == NODETYPE::TEXT)
	{
		//we want to pos the text
		//we set the width to the len of the text, * the font size, with a bit of adjusting!
		node->measuredWidth = (int)(node->tagValue.size() * (fontsize * 0.45)); //we set the width of the text to the len of the text * (fontsize * 0.45)
		node->measuredHeight = fontsize + 4; //the height is the fontsize + a bit of buffer '4'
		node->measured = true; //set the measured flag to 'true" to insure we do NOT measure it again.
		return; //done.
	}


	//ok, now we do what we came to do:
	//we understand that this node has children (like a div, body, a, ect)
	//we cant know our own size, till we know there sizes
	int totalH = 0; //holds our total height size OVERALL
	int maxW = 0; //holds the widest child we've seen

	//loop through each child in our node.
	//this only runs once, as the returns get it before this runs again.
	for (Node* child : node->children)
	{
		
		//we first measure the child before we use its size
		MeasureNodes(child, fontsize); //we run this funct again, and we measure and get the size of our child

		
		//because elements like divs and tables can take up room, we can space text out based on how many there are!

		totalH += child->measuredHeight; //now that we know its size, we stack it
		maxW = std::max(maxW, child->measuredWidth);  //we update the max width, to match the width of the widest child.

	}

	//now that we have measured all our child nodes, we now know our size!
	node->measuredWidth = maxW; //set the measuredWidth = to our maxWidth
	node->measuredHeight = totalH; // set the measuredWidth = to our totalHight

	
	node->measured = true; //set the measured flag to 'true" to insure we do NOT measure it again.


} //END OF MEASURE-NODES





//=======Position Nodes======\\


//This code handles positioning our nodes, and handling/assigning/creating the layout tree.
void PositionNodes(Node* node, int& currentXpos, int& currentYpos, int fontsize, SDL_Color textColor, SDL_Color bgColor, bool hasBg, std::string currentHref, bool inFlex = false) //this code returns nothing, and takes in 3 ints, our custom Node* class, 2 SDL_COLOR's, 2 Bools, and a String.
{

	
	if (node->tag == NODETYPE::START) //check if the current node's value is a NODETYPE::START tag.
	{
		

		//check the nodes, 'href' value to see if it contains it.
		if (!node->href.empty())
		{	
			currentHref = node->href; //if so, we sent the temp currentHref to the current node's one.
		}

		//I first want to look through the nodes tag, to see if it has a tag value tagValue
		//we check that it has a tagValue, if not, we return false

		//using our custom CSSTOKEN class ({ std::string id; std::vector<std::string> properties; }), we attempt to find our tagValue, using find ID, (with the funct above)
		CSSToken* id = FindID(node->tagValue);

		
		if (id != nullptr) //insure we get a value from id, and that it contains a value.
		{
			std::string fs = FindProperty(id, "font-size"); //now we create a temp string, using our 'FindProperty', ({const static CSSToken* rule, const static std::string propertyName}), we put in our 'id', and what we want to find, the result is added to 'fs'

			if (!fs.empty()) //make sure the string we just made, contains a value.
			{
				//loop through, and remove the spaces, going through each char in the string
				while (!fs.empty() && std::isspace((unsigned char)fs.front())) {
					fs.erase(fs.begin());
				}

				
				if (std::isdigit(fs[0])) //check if the first char is a number, we do this to avoid errors for the next part.
				{
					fontsize = std::stoi(fs) * 2;//we convert the number from a string "32" -> 32, then * by 2, to make sure its big enough.
				}
			}



			std::string bg = FindProperty(id, "background-color"); //using our 'FindProperty', ({const static CSSToken* rule, const static std::string propertyName}), we put in our 'id', and we are trying to find our background-color, then the result is added to 'bg'
			if (bg.empty()) { bg = FindProperty(id, "background"); } // FALLBACK using our 'FindProperty', ({const static CSSToken* rule, const static std::string propertyName}), we put in our 'id', and we are trying to find our background, then the result is added to 'bg'
			
			if (!bg.empty()) //now we check if we got a background value, and continue if we have one
			{
				std::cout << "Getting BG hex code" << std::endl; //DEBUG

				//if we find that the css of the bg is defined with a var, we skip, as we dont want an issue.
				if (bg.find("var(") != std::string::npos) {
					std::cout << "ERROR, CSS Defined with Var, Auto Skipping page color!" << "\r" << "This is not a problem :)" << std::endl; return; //end.
				}
			

				std::cout << "BG RAW: [" << bg << "]" << std::endl; //DEBUG

				RGB parsed = ParseHexColor(bg); //get the RGB value, by parsing our current hex color, and setting it to RGB

				
				if (node->tagValue == "body") //how check if the tag value is defined as "body"
				{


					backgroundColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 }; //we set our background color
				}
				else {
					
					bgColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 }; //we set our background color
					hasBg = true;
				}

			}


			//now support text color
			std::string tc = FindProperty(id, "color");
			if (!tc.empty())
			{
				RGB parsed = ParseHexColor(tc);

				textColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 }; //we * by 2, cause it would be super small
				//std::cout << "DEBUG" << " TEXT COLOR IS " << textColor.r << " " << textColor.g << " " << textColor.b << " " << std::endl; //DEBUG
			}


		}
		else {
			//give it that predefined
			if (node->tagValue == "h1") {
				fontsize = 96;
			}
			else if (node->tagValue == "h2") {
				fontsize = 72;
			}
			else if (node->tagValue == "h3") {
				fontsize = 60;
			}
			else if (node->tagValue == "h4" || node->tagValue == "p" || node->tagValue == "li" || node->tagValue == "blockquote") {
				
				fontsize = 48;
			}
			else if (node->tagValue == "h5") {
				fontsize = 36;
			}
			else if (node->tagValue == "h6") {
				fontsize = 28;
			}
			else if (node->tagValue == "a" || node->tagValue == "span") {
				// Links and spans
				fontsize = 36;
			}
			else if (node->tagValue == "big") {
				fontsize = 64;
			}
			else if (node->tagValue == "small" || node->tagValue == "sub" || node->tagValue == "sup") {
				
				fontsize = 24;
			}
			else if (node->tagValue == "code" || node->tagValue == "pre") {
				
				fontsize = 36;
			}
			else {
				// Default size for unrecognized tags or raw text nodes
				fontsize = 24;
			}
		}



		//we are gonna handle our flex stuff here.
		//first check if its a flex node
		if (IsFlex(node))
		{
			std::cout << "Found FLEX tag! " << node->tagValue << std::endl; //DEBUG

			currentYpos += fontsize + 6;

			//this will kinda be a template for other things like flex, span, ect
			int flexX = currentXpos;
			int flexY = currentYpos;
			int tallestChild = 0; //we handle the loop again, for each child as we need to handle it

			//go through each child, as the flex affects them!

			for (Node* child : node->children)
			{
				//this helps prevent y drift!
				int childX = flexX;
				int childY = flexY;

				//change the pos of each child at the flex pos
				PositionNodes(child, childX, childY, fontsize, textColor, bgColor, hasBg, currentHref, true);


				//forgot to update this! it wont work wihout
				tallestChild = std::max(tallestChild, child->measuredHeight);


				//we should move the cursor a bit
				flexX += child->measuredWidth + 8;

				//handle if it gets too big
				if (flexX > 2600)
				{
					flexX = currentXpos; //reset x 
					flexY += tallestChild + 8; //we move the y down
					tallestChild = 0; //reset for the new row
				}

			}

			//after we place the children, lets update the cursor to avoid overlapping

			currentYpos = flexY + tallestChild + 8; //move it down!

			//reset the x
			currentYpos += tallestChild + 20;
			currentXpos = 20;


			return;

		}

		//handle absolute pos
		int absX = 0;
		int absY = 0;

		//ok, if we get true from this
		if (IsAbsolute(node, absX, absY))
		{
			std::cout << "Found ABSOLUTE tag! " << node->tagValue << std::endl; //DEBUG

			//save where the normal flow of the page was, like the pos beforehand
			int savedX = currentXpos;
			int savedY = currentYpos;

			//update the cursor to the pos of the css
			currentXpos = absX;
			currentYpos = absY;

			//render 
			for (Node* child : node->children)
			{
				PositionNodes(child, currentXpos, currentYpos, fontsize, textColor, bgColor, hasBg, currentHref, false);
			}

			//update the current x and y pos back, as we need them

			currentXpos = savedX;
			currentYpos = savedY;

			return;
		}













		//this will let more stuff on one line, and will make the formating better
		//first we check if its a structure block tag (div), (p), (h1)
		// Block elements: push down to a new line
		// added way more elemnets adns tuff

		if (!inFlex)
		{

			if (node->tagValue == "div" || node->tagValue == "p" ||
				node->tagValue == "h1" || node->tagValue == "h2" ||
				node->tagValue == "h3" || node->tagValue == "tr" ||
				node->tagValue == "li" || node->tagValue == "br")
			{
				currentYpos += fontsize + 6;
				currentXpos = 20;
			}
		}
		// table cells
		if (node->tagValue == "td" || node->tagValue == "th")
		{
			//gives collums and stuff
			currentXpos = ((currentXpos / 200) + 1) * 200;
		}




		//check if its an image
		//check if its a img and its not empty
		if (node->tagValue == "img" && !node->src.empty())
		{
			//create a image 
			Layout imgLayout;
			imgLayout.node = node;
			imgLayout.x = currentXpos;
			imgLayout.y = currentYpos;
			imgLayout.isImage = true;    //set the img to true

			imgLayout.imageAttempted = false;
			imgLayout.imageTex = nullptr;   //we dont know the texture yet
			imgLayout.fontSize = 0; //no font
			imgLayout.textColor = { 0,0,0,255 }; //dont matter
			imgLayout.href = currentHref;
			//layoutList.push_back(imgLayout); //put 2 in, oops!

			imgLayout.href = currentHref;
			
			imgLayout.width = node->measuredWidth;
			imgLayout.height = node->measuredHeight;

			layoutList.push_back(imgLayout);

			//move the cursor to not run into it
			currentXpos += node->measuredWidth + 15;

			if (currentXpos > 1200) {
				currentXpos = 20;
				currentYpos += node->measuredHeight + 15;
			}


		}









	}


	if (node->tag == NODETYPE::TEXT)
	{
		//make a var that resets every run
		Layout layouttree;


		layouttree.x = currentXpos;
		layouttree.y = currentYpos;

		if (node->tagValue == "tr")
		{
			currentYpos += fontsize + 6;
			currentXpos = 20; // reset columns for this new row
		}

		layouttree.node = node;

		//when i code css, this will get replaced merging the data.

		

		layouttree.textColor = textColor;

		if (!currentHref.empty())
		{
			layouttree.textColor = { 0, 50, 255, 255 }; //blue
		}
		layouttree.fontSize = fontsize;

		layouttree.bgColor = bgColor;

		layouttree.hasBg = hasBg;


		layouttree.href = currentHref;



		//std::cout << layouttree.fontSize << std::endl;

		layouttree.width = node->measuredWidth;
		layouttree.height = node->measuredHeight;

		//send back the node to our layout list, to save
		layoutList.push_back(layouttree);

		
		currentXpos += node->measuredWidth + 8;

		// If we've gone too wide, wrap to next line
		currentXpos += (int)(node->tagValue.size() * (fontsize / 2)) + 8;

		if (currentXpos > 1400)
		{
			currentXpos = 20;
			currentYpos += fontsize + 4;
		}












	}


	for (Node* child : node->children)
	{
		PositionNodes(child, currentXpos, currentYpos, fontsize, textColor, bgColor, hasBg, currentHref, false);
	}



}



int LayoutTree(Node* node)
{

	std::cout << "darkmode -> " << darkmode << std::endl; //DEBUG
	PROFILE("LAYOUTTREE"); //PROFILE THE LAYOUTTREE

	layoutList.clear(); //we need to do this, or we will have errors

	int currentY = 120; //update this, to fix text clipping
	int currentX = 10;
	int startingfontsize = (int)(14 * zoomAmount);


	MeasureNodes(node, startingfontsize);
	

	//ok first we assign the node val to our new layout list
	//now we set it to add the node
	SDL_Color startingTextColor;
	SDL_Color startingBgColor;

	if (darkmode) //if dark mode
	{
		 startingTextColor = { 255, 255, 255, 255 };

		 startingBgColor = { 255, 255, 255, 0 };
	}
	else { //else
		 startingTextColor = { 0, 0, 0, 255 };

		 startingBgColor = { 0, 0, 0, 0 };
	}

	bool startingHasBg = false;
																													//current Href starts empty
	PositionNodes(node, currentX, currentY, startingfontsize, startingTextColor, startingBgColor, startingHasBg, "");

	std::cout << "Layout Complete." << std::endl;
	
	IMPORT(layoutList, node);
	
	return 0;
}


#pragma endregion //holds our node handling code
