#pragma once
#ifndef  GET_GUI
#define GET_GUI


#include <iostream>
#include <atomic>

extern std::atomic<bool> running; //this is for the thread, so we can kill it at the end.
int GUIRENDER();

#endif // ! GUI
#pragma once
