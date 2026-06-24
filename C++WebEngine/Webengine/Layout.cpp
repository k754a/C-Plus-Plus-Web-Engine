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













std::vector<Layout> layoutList; //list to store the layout

void GenerateLayoutTree(Node* node, int& currentYpos, int fontsize)
{
	
	if (node->tag == NODETYPE::START)
	{
		CSSRule* id = FindID(node->tagValue);

		//check if its not null
		if (id != nullptr)
		{
			std::string fs = FindProperty(id, "font-size"); //check each one for font size

			if (!fs.empty())
			{
				//ok now we set our font size
				//fonts are marked like 16px, so we need to get the numbers before the nums

				fontsize = std::stoi(fs) * 2; //we * by 2, cause it would be super small
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
			else if (node->tagValue == "div") {

				currentYpos += 5;
			}
		}
		
	}
	

	if (node->tag == NODETYPE::TEXT)
	{
		//make a var that resets every run
		Layout layouttree;
		layouttree.x = 10; 
		layouttree.y = currentYpos;


		layouttree.node = node;

		//when i code css, this will get replaced merging the data.




		layouttree.fontSize = fontsize;

	

		//send back the node to our layout list, to save
		layoutList.push_back(layouttree);
		std::cout << layouttree.fontSize << std::endl;

		currentYpos += (fontsize + 10);

	}
	

	for (Node* child : node->children)
	{
		GenerateLayoutTree(child, currentYpos, fontsize);
	}
	
}

int LayoutTree(Node* node)
{
	layoutList.clear(); //we need to do this, or we will have errors

	int currentY = 40;
	int startingfontsize = 14;
	//ok first we assing the node val to our new layout list
	//now we set it to add the node
	GenerateLayoutTree(node, currentY, startingfontsize);

	std::cout << "Layout Compleate." << std::endl;
	
	IMPORT(layoutList);
	return 0;
}



