#pragma once

#include <chrono> //For clock handling
#include <iostream> //For STD:: 
#include <string> // For std::string
//This cod will automatically measure how long a section of code takes to run.
//the way we do it, is by recording the start, then seeing the time at the end, comparing and we can figure out how many ms it took!

#ifdef _DEBUG //if we are running the debug version, we want to do this

class Timer {
public:
	//Make our constructor, this runs when we create a timer class.
	Timer(const std::string& name)  //we return nothing, but take in a name (the name of our timer)
	{
		Name = name; //update the name
		Start = std::chrono::high_resolution_clock::now(); //start recording
	}

	~Timer() //handle if the timer is destroyed, this also tells the program to end this once we leave the current file
	{
		//ok, we have finished, first lets grab our end time
		auto End = std::chrono::high_resolution_clock::now();

		//we can set the output type, set it to ns (microseconds) as milliseconds are even hard to see.
		auto Time = std::chrono::duration_cast<std::chrono::microseconds>(End - Start);

		//now we can return the results
		std::cout << Name << ": " << Time.count() << "ns" << "\n";
	}

private:
	//hold the vars here (we don't want them leaking into the main stuff)
	std::string Name; //hold the "name" of the timer

	std::chrono::high_resolution_clock::time_point Start; //create a clock to record the time.
};

#else //if we are running the relase version

class Timer //we still make the timer to avoid errors
{
	public: 
		Timer(const std::string&) {} //we still make this, however it dont do nothing, so theres no performace loss.
};




#endif
//now we create the id class
#ifdef _DEBUG //if we are running the debug version, we want to do this
#define PROFILE(name) Timer timer(name) //create another way to call Timer timer(name), thats much simpler. (we do this normaly in .h files)
#else
#define PROFILE(name) //dont run nothin
#endif