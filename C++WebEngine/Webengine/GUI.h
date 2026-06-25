#pragma once
#ifndef  GET_GUI
#define GET_GUI


#include <iostream>
#include <atomic>
#include "Layout.h" //need for the structs


extern SDL_Color backgroundColor; // default white, but this needs to be global.


extern std::atomic<bool> running; //this is for the thread, so we can kill it at the end.
int GUIRENDER();
int IMPORT(std::vector<Layout> layoutList);
#endif // ! GUI
#pragma once
