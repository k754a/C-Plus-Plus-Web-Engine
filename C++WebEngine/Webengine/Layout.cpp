//THIS WILL HANDLE THE LAYOUT OF THE ENGINE
//it will assign things like the positions of elements, and assign things like x and y

//currently because we have no css stuff, lets just increase the y (0 is starting the higher the y, lower it is)
#include <string>
#include <vector>
#include "Layout.h"
//lets bild a basic loop and stuff.

//first we assign the node from the dom tree



struct Layout
{
	Node* node;

	int x; int y; //the x and y pos

	int width; int hight; //handle the width and hight of the text
};

std::vector<Layout> layoutList; //list to store the layout

void GenerateLayoutTree(Node* node, int& currentYpos)
{
	//make a var that resets every run
	Layout layouttree;

	layouttree.x = 0; //set the x to 0
	layouttree.y = currentYpos;
	layouttree.hight = 12; //12 point font
	layouttree.width = 2000; //doesnt really matter rn, cause we dont got no boxes or nothin

	
	layouttree.node = node;

	//send back the node to our layout list, to save
	layoutList.push_back(layouttree);

	currentYpos += 20;

	

	for (Node* child : node->children)
	{
		GenerateLayoutTree(child, currentYpos);
	}
}

int LayoutTree(Node* node)
{
	int currentY = 0;
	//ok first we assing the node val to our new layout list
	//now we set it to add the node
	GenerateLayoutTree(node, currentY);

	std::cout << "Layout Compleate." << std::endl;
	
	return 0;
}



