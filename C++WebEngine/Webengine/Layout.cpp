//THIS WILL HANDLE THE LAYOUT OF THE ENGINE
//it will assign things like the positions of elements, and assign things like x and y

//currently because we have no css stuff, lets just increase the y (0 is starting the higher the y, lower it is)
#include <string>
#include <vector>
#include "Layout.h"
#include "GUI.h"
//lets bild a basic loop and stuff.

//first we assign the node from the dom tree




std::vector<Layout> layoutList; //list to store the layout

void GenerateLayoutTree(Node* node, int& currentYpos, int fontsize)
{
	
	if (node->tag == NODETYPE::START)
	{
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
	int currentY = 0;
	int startingfontsize = 14;
	//ok first we assing the node val to our new layout list
	//now we set it to add the node
	GenerateLayoutTree(node, currentY, startingfontsize);

	std::cout << "Layout Compleate." << std::endl;
	
	IMPORT(layoutList);
	return 0;
}



