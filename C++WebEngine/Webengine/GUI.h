#pragma once
#ifndef  GET_GUI
#define GET_GUI


#include <iostream>
#include <atomic>
#include "Layout.h" //need for the structs
#include <chrono> //load the c-time.
#include <fstream>  //for loading local files
#include <sstream> // for the buffer
#include "DOMTree.h"

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

	int loadGen = 0;

	//tab id is assigned off time, and this allows a unique id every time.
	//use long long to compress it, to prevent hitting the max on int
	long long tabID = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); //create a tab id, for the new tab. set it to the current time to prevent overlap.

	std::vector<CSSToken> css; //store the css per tab
};

extern float zoomAmount;

inline int WinW, WinH; //create 2 ints to hold the window's width, and the window's height - GLOBAL
void SetTabTitle(std::string title);
int GUIRENDER(std::string StartingTab);
int IMPORT(std::vector<Layout> layoutList, Node* node);
int LoadStarredPages();
int UpdateHTML();
#endif // ! GUI
#pragma once