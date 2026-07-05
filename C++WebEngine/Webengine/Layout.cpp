//THIS WILL HANDLE THE LAYOUT OF THE ENGINE
//it will assign things like the positions of elements, and assign things like x and y

//currently because we have no css stuff, lets just increase the y (0 is starting the higher the y, lower it is)
#include <string>
#include <vector>
#include "Layout.h"
#include "GUI.h"
#include "DOMTree.h"
//lets bild a basic loop and stuff.

//first we assign the node from the dom tree






//looks through all our CSS rules to find the one that matches our tag in our tree
//basicy we look through everything, and we pass in "H1" we match it and return the proprites
CSSToken* FindID(std::string input)
{
	//loop and check through each element
	for (int i = 0; i < globalCSS.size(); i++)
	{
		if (globalCSS[i].id == input) //if the id we looping through is = to it
		{
			return &globalCSS[i]; //we need the & or we get an error
		}
	}

	return nullptr; //i forgot this lol, and it just crashed
}

//now we want to get proprites
//we will loop throught the properties, and match
std::string FindProperty(CSSToken* rule, std::string propertyName)
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
	//we didnt find it :(
	return "";
}


//see if a node has absolute in the css
//if it does we return true!
//we need the x and y
bool IsAbsolute(Node* node, int& outX, int& outY)
{
	//i first want to look through the nodes tag, to see if it has it
	//we check that it has css values, if not, we return false

	CSSToken* id = FindID(node->tagValue);
	if (id == nullptr)
	{
		return false;
	}

	//now letes get the display part, if the element dont have this, we skip

	std::string position = FindProperty(id, "position");
	if (position.empty())
	{
		return false;
	}

	//rm the spaces, (same code ive been using in a bunch of other areas lol

	while (!position.empty() && std::isspace((unsigned char)position.front()))
		position.erase(position.begin());

	//if its flex, we return true
	if (position.find("absolute") != std::string::npos)
	{
		//grab the left and top
		std::string leftStr = FindProperty(id, "left");
		std::string topStr = FindProperty(id, "top");

		while (!leftStr.empty() && std::isspace((unsigned char)leftStr.front()))
			leftStr.erase(leftStr.begin());
		if (!leftStr.empty() && std::isdigit(leftStr[0])) {
			outX = std::stoi(leftStr);
		}

		while (!topStr.empty() && std::isspace((unsigned char)topStr.front()))
			topStr.erase(topStr.begin());
		if (!topStr.empty() && std::isdigit(topStr[0])) {
			outY = std::stoi(topStr);
		}





		return true;

	}

	return false;
}

//see if a node has flex in the css
//if it does we return true!
bool IsFlex(Node* node)
{
	//i first want to look through the nodes tag, to see if it has it
	//we check that it has css values, if not, we return false

	CSSToken* id = FindID(node->tagValue);
	if (id == nullptr)
	{
		return false;
	}

	//now letes get the display part, if the element dont have this, we skip

	std::string display = FindProperty(id, "display");
	if (display.empty())
	{
		return false;
	}

	//rm the spaces, (same code ive been using in a bunch of other areas lol

	while (!display.empty() && std::isspace((unsigned char)display.front()))
		display.erase(display.begin());

	//if its flex, we return true
	return display.find("flex") != std::string::npos;
}


























//for bg and text colors!
struct RGB { int r, g, b;  };

//got this online! 
RGB hexToRgb(unsigned int hexValue)
{
	RGB color;
	color.r = (hexValue >> 16) & 0xFF; // Extract the first 2 hex digits
	color.g = (hexValue >> 8) & 0xFF;  // Extract the middle 2 hex digits
	color.b = hexValue & 0xFF;         // Extract the last 2 hex digits
	return color;
}

RGB ParseHexColor(std::string hex)
{
	//css can somtimes have a lot of spaces
	// #bbbbbb
	//the problem is, this would cause a huge crash, as the code didnt know what to do.
	//now, the code will work!
	//this removes the front spaces
	while (!hex.empty() && std::isspace((unsigned char)hex.front()))
		hex.erase(hex.begin());

	//this removes the back.
	while (!hex.empty() && std::isspace((unsigned char)hex.back()))
		hex.pop_back();

	//remove spaces or #
	if (!hex.empty() && hex[0] == '#')
	{
		hex = hex.substr(1);
	}

	//expand shortand (#eee) to #eeeeee

	if (hex.size() == 3)
	{
		hex = std::string() + hex[0] + hex[0]
			+ hex[1] + hex[1]
			+ hex[2] + hex[2];
	}

	//handle errors
	if (hex.empty() || hex.size() != 6)
	{
		std::cout << "Found a invalid hex size, going back to black." << std::endl;
		RGB error = { 0,0,0, };
		return error;
	}

	//prevent a crash
	try {
		unsigned int hexValue = std::stoul(hex, nullptr, 16);
		return hexToRgb(hexValue);
	}
	catch (...) //any error
	{
		//just return our error thing
		RGB error = { 0,0,0, };
		return error;
	}



	//convert to our rgb vals!
	
}







std::vector<Layout> layoutList; //list to store the layout



//measures our node stuff here
void MeasureNodes(Node* node, int fontsize)
{
	//first check, if the node has been measured, dont do it again
	if (node->measureded) return;


	//same thing in the old generate layout tree.
	CSSToken* id = FindID(node->tagValue);

	//check if its not null
	if (id != nullptr)
	{
		std::string fs = FindProperty(id, "font-size"); //check each one for font size

		//before, we removed spaces, however we dont care no more, so we dont do that (because we are moving text)
		if (!fs.empty())
		{
			if (std::isdigit(fs[0])) //check to avoid a crash
			{
				fontsize = std::stoi(fs) * 2; //we * by 2, cause it would be super small
			}
		}
	}
	else if (node->tag == NODETYPE::START)
	{
		//give it that predefined
		if (node->tagValue == "h1")
		{
			fontsize = 96;
		}
		else if (node->tagValue == "p") {
			fontsize = 48;
		}
		else if (node->tagValue == "a") {
			fontsize = 36;
		}
		else if (node->tagValue == "span") {
			fontsize = 36;
		}
	}
	//because we can, ill just handle images real fast
	//check if its a start node, it has the value "img" and its not null/empty
	if (node->tag == NODETYPE::START && node->tagValue == "img" && !node->src.empty())
	{
		//now we just set the sizes
		//same as before
		node->measuredWidth = 200;
		node->measuredHeight = 150;
		node->measureded = true;
		return;
	}





	//ok handle it if its text
	if (node->tag == NODETYPE::TEXT)
	{
		//we want to pos the text
		//we set the width to the len of the text, * the fontsize, with a bit of adjusting!
		node->measuredWidth = (int)(node->tagValue.size() * (fontsize * 0.45));
		node->measuredHeight = fontsize + 4;
		node->measureded = true;
		return;
	}


	//ok, now we do what we came to do:
	//we understand that this node has children (like a div, body, a, ect)
	//we cant know our own size, till we know there sizes
	int totalH = 0; //holds our total hight size OVERALL
	int maxW = 0; //holds the widest child we've seen

	//for each child in our children
	for (Node* child : node->children)
	{
		//we first measure the child before we use its size

		MeasureNodes(child, fontsize);

		//now that we know its size, we stack it
		totalH += child->measuredHeight; //add to our total
		maxW = std::max(maxW, child->measuredWidth); //we do it like this, because the way we index, we will get the widest child at the end

	}

	//now that we have measured all our child ones, we now know our size!
	node->measuredWidth = maxW;
	node->measuredHeight = totalH;

	//make sure we mark its done
	node->measureded = true;


}


//this is the poistion part, i split them into functs, just to be a bit cleaner
//we are gonna adjust our pos, based on a bunch of things :)
void PositionNodes(Node* node, int& currentXpos, int& currentYpos, int fontsize, SDL_Color textColor, SDL_Color bgColor, bool hasBg, std::string currentHref, bool inFlex = false)
{

	//first we gotta make sure its an open tag, rather than something like text
	if (node->tag == NODETYPE::START)
	{
		

		//if we have one
		if (!node->href.empty())
		{
			//std::cout << "LINK HREF FOUND" << std::endl;
			currentHref = node->href;



		}

		CSSToken* id = FindID(node->tagValue);

		//check if its not null
		if (id != nullptr)
		{
			std::string fs = FindProperty(id, "font-size"); //check each one for font size

			if (!fs.empty())
			{
				//rm the spaces
				while (!fs.empty() && std::isspace((unsigned char)fs.front())) {
					fs.erase(fs.begin());
				}

				//std::cout << "Getting char size" << std::endl;
				//ok now we set our font size
				//fonts are marked like 16px, so we need to get the numbers before the nums
				if (std::isdigit(fs[0])) //check to avoid a crash
				{
					fontsize = std::stoi(fs) * 2; //we * by 2, cause it would be super small
				}
				//fontsize = std::stoi(fs) * 2; 
			}



			std::string bg = FindProperty(id, "background-color"); //check each one for font size
			if (bg.empty())
			{
				bg = FindProperty(id, "background"); //if it fails the first time, check for this
			}
			if (!bg.empty()) //if we get either.
			{
				std::cout << "Getting hex code" << std::endl;

				if (bg.find("var(") != std::string::npos)
				{
					//we found somthing like
					std::cout << "ERROR, CSS Defined with Var, Auto Skipping page color!" << std::endl;
					std::cout << "This is not a problem :)" << std::endl;

					return;
				}
				//ok now we set our font size
				//fonts are marked like 16px, so we need to get the numbers before the nums

				std::cout << "BG RAW: [" << bg << "]" << std::endl;

				RGB parsed = ParseHexColor(bg);
				//we do this, as there is other bg definitions in things like the <divs>, we only want the bg (currently) for the body.
				if (node->tagValue == "body")
				{


					backgroundColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 }; //we * by 2, cause it would be super small
				}
				else {
					bgColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 };
					hasBg = true;
				}

			}


			//now support text color
			std::string tc = FindProperty(id, "color");
			if (!tc.empty())
			{
				RGB parsed = ParseHexColor(tc);
				textColor = { (Uint8)parsed.r, (Uint8)parsed.g, (Uint8)parsed.b, 255 }; //we * by 2, cause it would be super small
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
			imgLayout.hight = node->measuredHeight;

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
		layouttree.hight = node->measuredHeight;

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
	layoutList.clear(); //we need to do this, or we will have errors

	int currentY = 120; //update this, to fix text clipping
	int currentX = 10;
	int startingfontsize = 14;


	MeasureNodes(node, startingfontsize);
	

	//ok first we assign the node val to our new layout list
	//now we set it to add the node
	SDL_Color startingTextColor = { 0, 0, 0, 255 };

	SDL_Color startingBgColor = { 0, 0, 0, 0 };
	bool startingHasBg = false;
																													//current Href starts empty
	PositionNodes(node, currentX, currentY, startingfontsize, startingTextColor, startingBgColor, startingHasBg, "");

	std::cout << "Layout Complete." << std::endl;
	
	IMPORT(layoutList);
	return 0;
}



