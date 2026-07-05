#pragma once
#ifndef  GET_GUI
#define GET_GUI


#include <iostream>
#include <atomic>
#include "Layout.h" //need for the structs


extern SDL_Color backgroundColor; // default white, but this needs to be global.


//tab structure

struct Tab //tab struct
{
	std::string url = ""; //save the url
	std::string title = ""; //save the title
	std::vector<Layout> layout = {}; //the layout of the current tab (save this into ram)
	int scrollpos = 0; //save the scroll pos for each tab
	int maxscroll = 3000; //the max scroll for the tab
	std::vector<std::string> history = {}; //give each tab its own history (currently we have it for the main one)
	int historypos = -1; //the pos for history/

	//hold the tabs search history!
	std::vector <std::string> SearchHistory = {};

	//and current pos, we look at the current selected tab before adjusting these
	int currentSearchPos = -1;

	Node* domRoot = nullptr;

};


















void SetTabTitle(std::string title);
int GUIRENDER();
int IMPORT(std::vector<Layout> layoutList, Node* node);
int LoadStarredPages();
int UpdateHTML();
#endif // ! GUI
#pragma once
