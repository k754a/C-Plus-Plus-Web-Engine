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
CSSRule* FindID(std::string input)
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
std::string FindProperty(CSSRule* rule, std::string propertyName)
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

void GenerateLayoutTree(Node* node, int& currentXpos, int& currentYpos, int fontsize, SDL_Color textColor, SDL_Color bgColor, bool hasBg, std::string currentHref)
{
	
	if (node->tag == NODETYPE::START)
	{
		//if we have one
		if (!node->href.empty())
		{
			//std::cout << "LINK HREF FOUND" << std::endl;
			currentHref = node->href;



		}
		
		CSSRule* id = FindID(node->tagValue);

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
			else {
				//defualt size
				fontsize = 24;
			}
		}

		//this will let more stuff on one line, and will make the formating better
		//first we check if its a structure block tag (div), (p), (h1)
		// Block elements: push down to a new line
		// added way more elemnets adns tuff
		if (node->tagValue == "div" || node->tagValue == "p" ||
			node->tagValue == "h1" || node->tagValue == "h2" ||
			node->tagValue == "h3" || node->tagValue == "tr" ||
			node->tagValue == "li" || node->tagValue == "br")
		{
			currentYpos += fontsize + 6;
			currentXpos = 20;
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
			layoutList.push_back(imgLayout);


			//give it a alocated size
			int allocatedWidth = 200;
			int allocatedHeight = 150;
			imgLayout.width = allocatedWidth;
			imgLayout.hight = allocatedHeight; 

			layoutList.push_back(imgLayout);

			//move the cursor to not run into it
			currentXpos += allocatedWidth + 15;
			if (currentXpos > 1200) {
				currentXpos = 20;
				currentYpos += allocatedHeight + 15;
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

		int estimatedTextWidth = (int)(node->tagValue.size() * (fontsize * 0.45));
		int estimatedTextHeight = fontsize + 4;

		//estimate the size or whateveer and for img
		layouttree.width = estimatedTextWidth;
		layouttree.hight = estimatedTextHeight;

		//send back the node to our layout list, to save
		layoutList.push_back(layouttree);

		// Advance layout typing alignment cursor safely without overlapping
		currentXpos += estimatedTextWidth + 8;

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
		GenerateLayoutTree(child, currentXpos, currentYpos, fontsize, textColor, bgColor, hasBg, currentHref);
	}
	
}

int LayoutTree(Node* node)
{
	layoutList.clear(); //we need to do this, or we will have errors

	int currentY = 50;
	int currentX = 10;
	int startingfontsize = 14;
	//ok first we assing the node val to our new layout list
	//now we set it to add the node
	SDL_Color startingTextColor = { 0, 0, 0, 255 };

	SDL_Color startingBgColor = { 0, 0, 0, 0 };
	bool startingHasBg = false;
																													//current Href starts empty
	GenerateLayoutTree(node, currentX, currentY, startingfontsize, startingTextColor, startingBgColor, startingHasBg, "");

	std::cout << "Layout Compleate." << std::endl;
	
	IMPORT(layoutList);
	return 0;
}



