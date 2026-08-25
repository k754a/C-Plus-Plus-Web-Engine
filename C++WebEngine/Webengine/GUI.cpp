//GUI FIXED FOR LINUX. (just some debug notes)

//switched the open in new tab to work for both, and work a bit better overall using SDL3

//Fixed issues with the printing button, as that used win libs only, so now it works for linux, even tho its a bit worse...

//That's it.


//THIS IS THE WINDOW RENDER
//(THIS WILL TAKE IN MULTIPLE INPUTS, FROM Layout.cpp, and Webengine.cpp.

#pragma region //Imports from the project file headers.

//=======IMPORTS-WEBENGINE-FILE'S=======\\

#include "ThreadPool.h" //for handling all our ThreadPool functs.
#include "GUI.h" //import from GUI.h file, holds the TAB struct, and a few other global classes.
#include "Parser.h" //used for sending some parts stright to the parser (like for the tab::new)
#include "Profiler.h" //used for handling the performace analization of some functs.
#include "ConnectSocket.h" //allows to send a request to connect socket.cpp -> parser.cpp -> Domtree -> Layout -> GUI.cpp tab render!
#include "Layout.h"

#pragma endregion

#pragma region //Imports from the projects external libs.

//=======IMPORTS-GLOBAL-LIBS'S=======\\

#include <SDL3/SDL.h> //include the SDL3 lib - For all the box and screen rendering.
#include <SDL3_ttf/SDL_ttf.h> //fonts lib - For all the fonts, and loading and handling the font.
#include <SDL3_image/SDL_image.h> //images lib - For all the Image rendering, and handling it.
#include <SDL3/SDL_dialog.h> //for the save menu
#include <filesystem> //for just the print window button, just that.
#include <iomanip> //for rounding the performace text

#pragma endregion

SDL_Texture* loadingTex = nullptr; //holds our GLOBAL loadingTexture "texture" -> so that multiple classes can use it! (displays just the loading...)

std::string SearchProvider = "lite.duckduckgo.com/lite/?q="; //you can change this to whatever you want. (note, most do not work)

#pragma region //Threads and Mutex global vars.

//=======CREATE THREAD HANDLERS=======\\

ThreadPool gNavPool(4); //A ThreadPool, that handles networking at the same time the main loop runs.
std::mutex gTabsMutex; //Prevent other threads from accessing the same mem and resources at the same time. this prevents crashes, and is used an a few parts. used for the tabs.


ThreadPool gTextRenderPool(4);  //A ThreadPool, that handles text rendering at the same time the main loop runs.
std::recursive_mutex gTTFMutex; //same as the other mutex but can be locked more than once, and used for the text.


ThreadPool gImageDownloadPool(6);  //A ThreadPool, that handles Image downloads at the same time the main loop runs.

#pragma endregion

#pragma region //Global vars.


SDL_Color backgroundColor = { 245, 245, 245, 255 }; //handles the bg color of the page, does change (is white on start)

float adjusted = 0; //this holds a value so that i can adjust the pos of the cursor with the arrow keys TODO

static bool DraggingScrollBar = false; //bool to hold if we are Dragging the Scroll Bar
static bool ContextMenu = false; //bool to hold if the context menu is open
static bool inputbarfocused;
bool SaveXYContextMenuPos = false; // checks if it should lock the X + Y of the thing
std::vector<std::string> starredPages; //create a string vector list to hold our starred, saved pages!

//CONTEXT MENU STUFF
float ContextMenuXPos, ContextMenuYPos;
std::vector<std::string> ContextMenuButtons; //Holds the names of the buttons, and the engine will fit them.
std::string clickedURL; //create a temp string to hold the url
int clickedTab;

//HISTORY STUFF
int TabBefore;
float zoomAmount = 1.0f; //global float to handle the zoom amount
float mouseX; //save the pos of the mouseX
float mouseY; //save the pos of the mouseY

std::string urlInput = "";    // holds the url we type in the input box
std::string currentURL = ""; //hold the url, but does not save till we press search.

std::vector<Tab> tabs;        // a vector that holds our custom tab struct, allowing us to create tabs, and save data to them.
int activeTab = 0;            // holds the amount of tabs open
int lastsearchedtabID; //holds the id of the last searched tab.

//SETTINGS
bool ShowPerformace = false;
bool ShowSettingsMenu = false;


//GLOBAL HISTORY
std::vector<std::pair<std::string, std::string>> GlobalHistory; //create a vector that holds an 2 strings, (one string will hold time), string will hold the link.

#pragma endregion

#pragma region //Create First Tab (On Start)

//=======BUILD THE NEW TAB - ON FIRST START=======\\

struct TabInit { //as soon as GUI is started, we run this before everything, this allows us to create a new tab.
	TabInit() {
		Tab t; //create a temp tab struct named "t"
		t.title = "New Tab"; //assign a title
		t.url = "new::tab"; //set the url to new::tab

		t.history.push_back("new::tab"); //set the history to new::tab
		t.historypos = 0; //set the history pos to 0 (starting)

		tabs.push_back(t); //push back to our main tabs vector list
		activeTab = 0; //set the activeTab to 0 (as in a vector, 0 is the first element)
		lastsearchedtabID = tabs[0].tabID; //update the last searched i
		
	}
} _tabInit; //insure it runs on start

#pragma endregion

#pragma region //GLOBAL History STUFF
void DestoryHistory()
{
	std::ofstream historyFile("History.history", std::ios::trunc); //clear the file

	if (!historyFile.is_open())
	{
		std::cerr << "COULD NOT OPEN FILE. ERR" << std::endl;
		return;
	}

	historyFile.close(); //we are done

	return;
}
void AddToHistory(std::string link)
{
	//OK, now that we have the link the user searched, lets first pull the current time
	auto now = std::chrono::system_clock::now();
	std::string time = std::format("{:%Y-%m-%d %H:%M}", now);
	

	//now that we have both info, lets add to our list...
	GlobalHistory.push_back({ time, link });

	//now that we have added it to the list, lets now load it into a file...
	std::ofstream historyFile("History.history", std::ios::trunc);  //open the file "History.history" but overwrite everything in it

	if (!historyFile.is_open())
	{
		std::cerr << "COULD NOT OPEN FILE. ERR" << std::endl;
		return;
	}
	for  (auto const history : GlobalHistory) { //go through each line in starPages
		historyFile << history.first + " | " + history.second + "\n";
	}

	historyFile.close(); //we are done

	return;
}
void LoadFromHistory() //load from the list... this will be used on things like new tab creation.
{
	//lets load onto the list...
	std::ifstream historyFile("History.history");  //open the file "History.history" but leave it as a readme


	if (!historyFile.is_open())
	{
		std::cerr << "COULD NOT OPEN FILE. ERR" << std::endl;
		return;
	}

	//now we are loaded, lets clear the GlobHist to prevent errors
	GlobalHistory.clear();
	//now lets read it line by line
	std::string line;

	while (std::getline(historyFile, line))
	{
		//ok, now we grab the line, but we need to populate the GlobalHistory
		size_t breakPos = line.find("|");

		if (breakPos == std::string::npos) break;

		std::string dateTemp = line.substr(0, breakPos);
		std::string urlTemp = line.substr(breakPos + 1);

		while (!urlTemp.empty() && urlTemp.front() == ' ')
		{
			urlTemp.erase(urlTemp.begin());
		}
		while (!urlTemp.empty() && urlTemp.back() == ' ')
		{
			urlTemp.pop_back();
		}
		std::cout << "GRABBED " << dateTemp << " & " << urlTemp << std::endl;

		GlobalHistory.push_back({ dateTemp, urlTemp }); //push back the date and url Temp's
	}

	historyFile.close(); //we are done

	return;
}

#pragma endregion

#pragma region //Handle Star Functions

//=======LOAD STARRED PAGES=======\\

int LoadStarredPages() //load star pages returns an int, and takes nothing in
{
	starredPages.clear(); //takes our global vector<string> class, and removes everything from it on start

	std::ifstream bookmarks("starred_pages.STAR"); //attempt to open our starred_pages.STAR file
	if (!bookmarks.is_open()) return false; //if we cannot find it, we return false
	
	std::string line; //create a temp string var.

	while (std::getline(bookmarks, line)) //while getting each line of the file returns true, (it exists), and set the 'line' var to it
	{
		
		if (!line.empty()) //ignore blank lines (skip
			starredPages.push_back(line); //if its not blank, we add it to the starred pages
	}

	return 0; //end

} //END OF LoadStarredPages

//=======UPDATE HTML=======\\

int UpdateHTML() //UPDATE HTML returns an int, and takes nothing in
{
	std::ofstream htmlFile("main.html", std::ios::trunc);  //open the file "main.html" but overwrite everything in it

	if (htmlFile.is_open()) //make sure we have it open
	{
				//add to the html file, the html base
				htmlFile << R"(<!DOCTYPE html><html lang="en"><head>
			<link rel="icon" href="data:,">
			<meta name="viewport" content="width=device-width, initial-scale=1">
			<title>New Tab</title>

			<style>
				h1 {
					color: #000000;
					font-size: 36px;
				}
				p {
					color: #444444;
					font-size: 24px;
				}
				a {
					color: #4A90E2;
					font-size: 22px;
				}
				
				.browser-image {
					max-width: 200px; 
					height: auto;
					display: block;
					margin-top: 20px;
					margin-bottom: 20px;
					border-radius: 8px;
				}
			</style></head><body>
			<div>
				<h1 style="text-align:center; font-weight:900;">C++Browse</h1>
				<p style="text-align:center; vertical-align:300;">This is my C++ web browser project.</p>
       

				<h1 style="text-align:center; vertical-align:400;">To start, search anything. ы </h1>

				<h1>&nbsp;</h1>
		)";

		if (!starredPages.empty()) //make sure that the starred pages list contain something
		{
			htmlFile << "        <br>Starred Pages:\n"; //small starred page header
			for (const std::string& site : starredPages) { //go through each line in starPages
				
				htmlFile << "        <p>ж -<a href=\"" << site << "\">" << site << "</a></p>\n"; //add the site to the html file, as a new line
			}
		}
		else { //does not contain anything
			htmlFile << "<br>No Starred Pages\n"; //display that
		}

		htmlFile << R"( </div></body></html>)"; //end peices, to insure its valid html

		htmlFile.close(); //close the file
	}
	return 0; //end, return 0
} //END OF UpdateHTML


#pragma endregion

#pragma region //Handle Import (holds IMPORT)

//=======IMPORT=======\\

//this is called in layout, and replaces whatever was on the screen with the new layout

int IMPORT(std::vector<Layout> layoutGOTTEN, Node* newRoot) //IMPORT
{
	PROFILE("IMPORT"); //Send the recording to the PROFILE funct, to measure its speed.
	std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock for the tab, this prevents crashes by preventing other functions with this lock, from changing the data preventing 2 things writing 1 data
	
	//if we have nothing in the tabs, or no active tabs, or activeTab is greater than the amount of tabs currently, we debug, and break to prevent an error
	if (tabs.empty() || activeTab < 0 || activeTab >= static_cast<int>(tabs.size())) { std::cout << "IMPORT called before tabs ready." << std::endl; return -1; }

	
	int idx = -1; //temp int to hold the current tab index
	for (int i = 0 ; i < (int)tabs.size(); i++) { //loop through each tab in our list

		//to make sure we update only the active tab, we loop through and check the active tab, 
		//if we find it, we set the idx to the 'i' and break.
		if (tabs[i].tabID == lastsearchedtabID) { idx = i; break; } 
	}
	
	//if our idx did not change (set to -1 at the start), we know we could not find it, so do some cleanup, and break to prevent any errors.
	if (idx == -1) { DeleteTree(newRoot); return -1; } 


	auto& tab = tabs[idx]; //small reference so i don't need to repeat ' tabs[idx] ' everywhere

	for (auto& item : tab.layout) { //loops through each 'item' in the tab.layout
		if (item.textTex) SDL_DestroyTexture(item.textTex); //if each item's textTex contains old stuff, (if it contains anything it returns true) destroy it.
	}
	if (tab.domRoot != nullptr) { DeleteTree(tabs[idx].domRoot); } //again, if our tab contains any old stuff in the domroot, destory. (using DeleteTree in Layout.cpp)

	for (auto& item : layoutGOTTEN) item.loadGen = tab.loadGen; //for each item in layoutGOTTEN, we assign the loadgen value from our tab.loadgen

	tab.domRoot = newRoot; //update the tab with our new domRoot
	tab.scrollpos = 40; //reset the tabs scroll pos to '40'
	tab.layout = layoutGOTTEN; //update the old layout with our newer layoutGOTTEN vector

	if (cssUpdated)
	{
		tab.css = globalCSS;
		cssUpdated = false;
	}
	
	if (idx == activeTab) { tabs[idx].url = urlInput; } //if the id of the tab = to the current tab, update the url with url input.
	currentURL = urlInput; //update the current url setting it to the urlInput.
	
	return 0; //return 0
} //END OF IMPORT

#pragma endregion

#pragma region //Handle URL PARSING

//=======Percent Decode=======\\

std::string PercentDecode(const std::string& src) //Percent Decode returns a std::string, and takes in a const string
{
	std::string out; //create a temp string to hold our final output.
	for (size_t i = 0; i < src.size(); i++) //for each char in src.size
	{
		if (src[i] == '%' && i + 2 < src.size()) //if src contains a % and i + 2 is not greater than the size.
		{
			std::string hex = src.substr(i + 1, 2); //create a temp hex, where we grab the 2 hex chars after the %
			try { //if it goes right
				char decoded = (char)std::stoul(hex, nullptr, 16); //convert the hex string from base 16 to a normal char
				out += decoded; //add it to our output string
				i += 2; //now we skip, as we just finished handling the 2 before.
			}
			catch (...)
			{
				//not valid, so just treat it like normal
			}
		}
		else if (src[i] == '+') //if src contains a +
		{
			out += ' '; //replace the plus for a blank char
		}
		else
		{
			out += src[i]; //if its a normal char, just add it on normally
		}
	}
	return out; //return the final string

} //END OF PercentDecode


//=======Resolve URL=======\\

std::string ResolveURL(const std::string& targetUrl, const std::string& currentUrl) //ResolveURL takes in 2 const strings, a target url, and the current url, and returns a string
{
	if (targetUrl.empty()) return targetUrl; //if the url is empty, just return, as we don't want to deal with that

	if (targetUrl.find("http://") == 0 || targetUrl.find("https://") == 0) return targetUrl; //if it starts as a full url, just return, we are good.
	if (targetUrl.find("//", 0) == 0) { return "https:" + targetUrl; } //if it starts like '//' add the https: + // + example.com

	
	//std::string domain = "";
	size_t protoend = currentUrl.find("://");
	if (protoend == std::string::npos) return targetUrl; //The URL does'nt have the :// style, so return and end.


	size_t pathStart = currentURL.find('/', protoend + 3); //create a size_t var, that takes the value of where the / is.


	//handle absolute paths like /assets/example.png -> https://Example.com/assets/example.com
	if (targetUrl[0] == '/') //if the start of the targetURL, is a / (like /image.png)
	{
		std::string domain; //make a temp string 'domain'
		if (pathStart != std::string::npos) //if the pathStart contains something
		{
			domain = currentUrl.substr(0, pathStart); // we set the domain to the currenturl from the start, to pathstart (https://Example.com)
		}
		else {
			return domain = currentUrl; //the path start does'nt have a '/' so we can guess there is no substring.
		}
		return domain + targetUrl; //return the domain + the targeturl https://example.com + /example.png
	}


	//handle something like images/pic.png
	size_t lastSlash = currentUrl.rfind('/'); //create a temp size t, turns out rfind is faster.
	if (lastSlash != std::string::npos && lastSlash > protoend + 2) //if the lastSlash contains something, and lastSlash > protoend + 2
	{
		return currentURL.substr(0, lastSlash + 1) + targetUrl; //if so, we return, grabbing our full url so https://Example.com/assets/example.com
	}



	//handle if we don't have any / after the domain
	std::string Fallback;
	if (pathStart != std::string::npos)
	{
		Fallback = currentUrl.substr(0, pathStart); //because pathstart contains a / we can go from 0 to pathstart
	}
	else {
		Fallback = currentUrl; //we could not find a / so we assume its basic
	}

	return Fallback + "/" + targetUrl; //return the final stuff


} //END OF ResolveURL



void NavigateTo(std::string target, SDL_Renderer* render, TTF_Font* font, bool addToHistory = true) //Navigate To, takes in a string, a SDL_RENDER, and a FONT, and a single bool, and returns nothing.
{
	std::cout << "target-> " << target << std::endl;//DEBUG

	if (target.empty()) return; //if the target is empty, to save time, just return.

	if (addToHistory) //if we should save the history
	{
		std::lock_guard<std::mutex> lock(gTabsMutex); //lock to prevent errors

		//grab the current tab
		auto& tab = tabs[activeTab];

		if (tab.historypos < static_cast<int>(tab.history.size()) - 1) //if the historypos is less than the history (we behind) and then we made a new branch
		{
			tab.history.resize(tab.historypos + 1); //remove all the forward stuff 
		}
		//if the history is empty, or if the URL is different from the last thing
		if (tab.history.empty() || tab.history.back() != target)
		{
			tab.history.push_back(target); //add the new url to the end of the history
			tab.historypos = static_cast<int>(tab.history.size()) - 1;//update the current history to make it the new added page
		}
	}
	std::error_code ec; //temp var to hold the error code
	if (std::filesystem::exists(target, ec) && !ec) //check if the target contains a valid path, and no error is thrown
	{
		std::cout << "LOAD A FILE" << std::endl; //DEBUG

		std::string path = target; //temp var to hold the 'path'
		//now, we need to remove the file:: and check if its valid.
		std::ifstream file(path); //remove the file::, then attempt to search for it

		//now check that we could find it
		if (!file.is_open()) { std::cout << "Could not open local file." << std::endl; return; } //Debug and return if it failed to open

		//before we do anything, first check that its valid

		if (path.ends_with(".txt"))
		{
			std::cout << "Valid Local File Format" << std::endl; //DEBUG
			std::stringstream buffer; //create a temp buffer to hold the file.

			buffer << file.rdbuf(); //load the file into the buffer

			std::string fileinfo = buffer.str(); //now we convert the buffer, into a string
			{
				std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
				auto& active = tabs[activeTab]; //create a temp var to hold our active tab

				active.loadGen++; //increase the loadGen, stopping other threads

				for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
				{
					if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
					if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
				}
				active.layout.clear(); //clear the layout

				active.scrollpos = 40; //reset the scroll pos
				lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

				urlInput = path; 	//clear the url input, as it is a new tab
			}
			std::stringstream lines(fileinfo); //load each line
			std::string line; //to hold each line
			std::string html = "<html><title> " + path + "</title><body>"; //start the html, and set the title
			
			while (std::getline(lines, line)) //for each line in lines
			{
				if (line.empty()) //keep empty lines empty
				{
					html += "<div>&nbsp;</div>"; //make a blank line
				}
				else {
					html += "<div>" + line +"</div>"; //make a new line
				}
			}
				
			html += "</html>""</body>"; //end the html

			Parser(html);
			return; //return.
		}
		if (path.ends_with(".html") || path.ends_with(".htm")) //if it contains the valid .prefix at the end
		{
			std::cout << "Valid Local File Format" << std::endl; //DEBUG
			std::stringstream buffer; //create a temp buffer to hold the file.

			buffer << file.rdbuf(); //load the file into the buffer


			std::string fileinfo = buffer.str(); //now we convert the buffer, into a string

			{
				std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
				auto& active = tabs[activeTab]; //create a temp var to hold our active tab

				active.loadGen++; //increase the loadGen, stopping other threads

				for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
				{
					if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
					if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
				}
				active.layout.clear(); //clear the layout

				active.scrollpos = 40; //reset the scroll pos
				lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

				urlInput = path; 	//clear the url input, as it is a new tab
			}
			Parser(fileinfo); //now we inject it into the parser, to force it to load this html
			return; //return.
		}
		else if (path.ends_with(".png") || path.ends_with(".jpg")) //or if it contains something like a .jpg, or .png (an image)
		{
			std::cout << "Valid Local File Format" << std::endl; //DEBUG

			
			std::string html = "<html><title>" + path + "</title><body><img src=\"" + path + "\"></body></html>"; //fake html, that just forces an image
			Parser(html); //send it
			return; //return.
		}
		else {
			std::cout << "Not A Valid File Format To Load" << std::endl; //DEBUG

			std::string html = "<html><title>" + path + "</title><body><h1>ERROR, not a valid format.</h1><h3>You can disable this in settings</h3></body></html>"; //fake html, that just forces the error code
			{
				std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
				auto& active = tabs[activeTab]; //create a temp var to hold our active tab

				active.loadGen++; //increase the loadGen, stopping other threads

				for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
				{
					if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
					if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
				}
				active.layout.clear(); //clear the layout

				active.scrollpos = 40; //reset the scroll pos
				lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

				urlInput = path; 	//clear the url input, as it is a new tab
			}
			Parser(html); //send it

			return; //return.
		}
	}
	if (target.find("history::tab") == 0) //check if the target contains the value history::tab
	{
		LoadFromHistory(); //first load history
		
		//if so, load custom html file. (like new::tab)
		{
			std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
			auto& active = tabs[activeTab]; //create a temp var to hold our active tab

			active.loadGen++; //increase the loadGen, stopping other threads

			for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
			{
				if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
				if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
			}
			active.layout.clear(); //clear the layout

			active.scrollpos = 40; //reset the scroll pos
			//lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

			urlInput = "history::tab";
		}
		std::string temp = R"(
									<head>
										<title>History</title>
									</head>
									<body>
										<h1 style="text-align:center; vertical-align:top; color:#000000; font-weight:900;"> Search History </h1>
										<h3 style="text-align:center; vertical-align:185;">_______________________________________________</h3>
										)";													
		if (GlobalHistory.size() < 1)
		{
			temp += R"(<p style="text-align:center; vertical-align:400;">No history yet :( go search for some!</p>)";
		}
		else {
			for (auto line : GlobalHistory)
			{
				if (line.first.empty() || line.second.empty()) continue; 
				//create a link value, holding our link. however, make sure that we remove all the spaces, to prevent issues.
				std::string value = "<p style=\"text-align:center; font-size:fit;\" href=\"" + line.second + "\">" + line.first + "-- " + line.second + "</p> \r";
				temp += value;
			}
		}
		//grab the current time
		auto now = std::chrono::system_clock::now();
		temp += " <p style=\"text-align:left; font-size:25; vertical-align:top; color:#444444; font-weight:900;\"> Updated: " + std::format("{:%d %H:%M}", now) + "</p>"; //display the last time up to date
		temp += " <p style=\"text-align:left; font-size:25; vertical-align:150; color:#e6c670; font-weight:900;\"> Reset History </p>"; //Reset history Text, for button.
		temp += " <p style=\"text-align:70; font-size:20; vertical-align:180; color:#6b6969; font-weight:900;\"> Click it   τ </p>"; //Info for it

		temp += "<body><html>"; //add the end on.

		Parser(temp);
		return;
	}
	if (target.find("settings::tab") == 0) //check if the target contains the value settings::tab
	{	
		{
			std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
			auto& active = tabs[activeTab]; //create a temp var to hold our active tab

			active.loadGen++; //increase the loadGen, stopping other threads

			for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
			{
				if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
				if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
			}
			active.layout.clear(); //clear the layout

			active.scrollpos = 40; //reset the scroll pos
			lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

			urlInput = "settings::tab"; 
		}
		std::string temp = R"(
									<head>
										<title>Settings</title>
									</head>
									<body>
										<p style="text-align:100; vertical-align:150; font-weight:900;">Settings</p>
										<p style="text-align:center; vertical-align:240; color:#444444; ">Not done yet! (should be soon.)</p>
										)";


		temp += "<body><html>"; //add the end on.
		Parser(temp);
		return;
	}
	if (target.find("new::tab") == 0) //check if the target contains the value new::tab
	{
		std::cout << "FOUND NEW TAB" << std::endl; //DEBUG

		LoadStarredPages(); //make sure StarredPages are up to date
		UpdateHTML(); //Update the html with the new stars

		std::ifstream file("main.html"); //open the main.html file

		if (!file.is_open()) { std::cout << "Could not open main file." << std::endl; return; } //Debug and return if it failed to open

		std::stringstream buffer; //create a temp buffer to hold the file.

		buffer << file.rdbuf(); //load the file into the buffer

		
		std::string fileinfo = buffer.str(); //now we convert the buffer, into a string

		{
			std::lock_guard<std::mutex> lock(gTabsMutex); //lock to insure no corruption
			auto& active = tabs[activeTab]; //create a temp var to hold our active tab

			active.loadGen++; //increase the loadGen, stopping other threads

			for (auto& item : active.layout) //destroy the old item, and clear the layout for text and images, to avoid issues
			{
				if (item.textTex) SDL_DestroyTexture(item.textTex); //destroy the text
				if (item.imageTex) SDL_DestroyTexture(item.imageTex); //destroy the images
			}
			active.layout.clear(); //clear the layout

			active.scrollpos = 40; //reset the scroll pos
			lastsearchedtabID = tabs[activeTab].tabID; //reset the tab id

			urlInput = ""; 	//clear the url input, as it is a new tab
		}
	
		Parser(fileinfo); //now we inject it into the parser, to force it to load this html
		return; //return.
	}

	std::string resolvedTarget = target; //temp string that gets updated as we go along.

	//its a url if any of these conditions are met, as well as no spaces
	bool isUrl = (target.find("http://") == 0 || target.find("https://") == 0 || target[0] == '/' || target.find('.') != std::string::npos) && (target.find(' ') == std::string::npos);

	{

		std::lock_guard<std::mutex> lock(gTabsMutex); //create the lock to prevent other possesses from editing, and preventing a crash
		if (!isUrl) //if it not a url, we handle it like a search
		{
			std::string query = target; //set a temp var that holds the target (what we searched and pressed enter)
			std::replace(query.begin(), query.end(), ' ', '+'); //replace all ' ' with +, as we need that for a valid search
			resolvedTarget = SearchProvider + query; //combine it with a url to search, i use lite.duckduckgo.com/lite/?q=, as its the only one that does not flag me as a bot
		}
		else { //it is a url!
			resolvedTarget = ResolveURL(target, tabs[activeTab].url); //if it is a url, send the value to ResolveURL to clean it up, and insure its valid
		}

		if (addToHistory) { //if it was flagged being able to be added to the history
			auto& tab = tabs[activeTab]; //create a temp var, that calls the tabs[activeTab] once, to save performance

			//if we went back and searched something new, delete that multiverse
			if (tab.historypos < static_cast<int>(tab.history.size()) - 1) { //if the tab.historypos is less than the size, we resize the history, removing the old stuff
				tab.history.resize(tab.historypos + 1); //rm all the old history items that were ahead of the current one
			}

			if (tab.history.empty() || tab.history.back() != resolvedTarget) //check if the history is empty, or if the last URL isnt the same as this one
			{
				tab.history.push_back(resolvedTarget); //if so, we add the new URL to the end of the history vector
				tab.historypos = tab.history.size() - 1; //update the history index, so it maches the current url
			}

			AddToHistory(resolvedTarget); //hold the search pos.
		}
	} //Release the gTabMutex Lock

	//LOADING UI ---
	Layout loadingMSG{}; //create a new layout
	loadingMSG.x = 10; //set the x size to 10
	loadingMSG.y = 150; //set the y size to 150
	loadingMSG.fontSize = 72; //set the font size to 72
	loadingMSG.isImage = false; //set the isImage to false

	SDL_Color loadColor; //create the texture color
	if (darkmode) //if darkmode is on
	{
		loadColor = SDL_Color{ 255, 255, 255, 255 }; //make the text color white
	}
	else {
		loadColor = SDL_Color{ 0, 0, 0, 255 }; //make the text color black
	}
	{
		std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create the lock to prevent any crashes

		int ogFontSize = TTF_GetFontSize(font); //save the old font size
		//because we are injecting the values in, we don't run by the fontsize part normally, so we do it manually
		TTF_SetFontSize(font, loadingMSG.fontSize); //set the font size larger than normal
		
		SDL_Surface* loadSurf = TTF_RenderText_Solid(font, "Loading...", 0, loadColor); //create the surf holding our text loading...
		TTF_SetFontSize(font, ogFontSize); //set it back.

		if (loadSurf != nullptr) //if the loadSurf worked correctly
		{
			loadingMSG.textTex = SDL_CreateTextureFromSurface(render, loadSurf); //set the textTexture to our load serf.
			loadingMSG.width = loadSurf->w; //set the width to the width of loadSurf
			loadingMSG.height = loadSurf->h; //set the height to the height of loadSurf
			SDL_DestroySurface(loadSurf); //we are done with the temp surf, we can destroy it
		}

	}//Release the ttfLock 

	std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
	auto& active = tabs[activeTab]; //temp var for cleanness
	active.layout.clear(); //clear the last layout

	active.layout.push_back(loadingMSG); //push our loading message values into the active tab's layout
	
	active.url = resolvedTarget; //update the active tabs url, with the url
	urlInput = resolvedTarget; //update the urlInput with the url
	lastsearchedtabID = active.tabID; //set the tab id.
	
	std::wstring wUrl(resolvedTarget.begin(), resolvedTarget.end()); //create a temp wUrl string, for the connectSocketHTTPS.
	gNavPool.enqueue([wUrl]() { //create the thread
		ConnectSocketHTTPS(wUrl); //push it
	});

} //END OF NavigateTo


//=======Fix URL REDIRECT=======\\

//ok, this fixes duck duck go redirect strings!
std::string FixURLREDIRECT(const std::string& href)
{
	//first find our uddg=, we want to remove that
	size_t uddgPos = href.find("uddg=");

	if (uddgPos == std::string::npos) return ""; //if we cant find that, stop, and return blank

	size_t start = uddgPos + 5; // we found the uddg, but we want to skip it, so skip "uddg=" 

	std::string encoded = ""; //create a temp string to hold our fixed value

	//ok, starting at the start pos, we go till the end
	for (int i = start; i < href.size(); i++)
	{
		//ok we hit the &, so we stop, as its usually junk (example -> &other=1)
		if (href[i] == '&')
		{
			break;
		}

		//else, just keep going
		encoded += href[i];
	}

	return PercentDecode(encoded); //send it through our PercentDecode, to convert it back to chars, then return
} //END OF FixURLREDIRECT


#pragma endregion

#pragma region //Handle PRERENDER


void PreRender(SDL_Renderer* render, TTF_Font* font) //PreRender Takes in a SDL render component, and a TTF Font component
{
	//PROFILE("PRERENDER"); //Send the recording to the PRERENDER funct, to measure its speed.

	std::unique_lock<std::mutex> lock(gTabsMutex); //set a lock mutex to prevent the Tabs being edited from other threads

	if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size())  return; //if we have no active tabs, skip

	if (lastsearchedtabID != tabs[activeTab].tabID) lastsearchedtabID = tabs[activeTab].tabID; //if the last searched tab ID is not == to the active tab, update it so it is

	int currentLoadGen = tabs[activeTab].loadGen; //create a temp int, and load the current loadGen
	
	int xtrack = 20; //start the X pos at 20, on start
	int ytrack = 120; //start the y pos at, 120 on start
	int lasty = -1; //saves the y pos of the last item, we set it to -1, to insure we record the first element.
	int maxLineHeight = 0; //saves the tallest element on the current line, to avoid clipping text.

	for (int i = 0; i < (int)tabs[activeTab].layout.size(); i++) { //loop for each element in the vector

		//first check if we have a custom Y element
		bool CustomY = tabs[activeTab].layout[i].node && (tabs[activeTab].layout[i].node->style.find("top") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("middle") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("center") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("left") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("right") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("bottom") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("vertical-align:") != std::string::npos ||
														  tabs[activeTab].layout[i].node->style.find("text-align") != std::string::npos); //will add more
		if (CustomY) //if its not custom
		{
			int bottomY = tabs[activeTab].layout[i].y + tabs[activeTab].layout[i].height;
			
			if (bottomY > ytrack)
			{
				xtrack = 20; //it is, so now lets set the x track to 20;
				ytrack = bottomY + 15;

				maxLineHeight = 0; //set the new maxLineHeight to 0, for this new line
			}
			
			if (bottomY > lasty)
			{
				lasty = bottomY;
			}
		}
		else {
			if (lasty != -1 && tabs[activeTab].layout[i].y > lasty) //if the last y not == 1 (prevents running on the first loop) and the layout[i].y > lasty
			{
				xtrack = 20; //it is, so now lets set the x track to 20;
				ytrack += (maxLineHeight + 15);

				maxLineHeight = 0; //set the new maxLineHeight to 0, for this new line
			}
			
			lasty = tabs[activeTab].layout[i].y; //update the last y with the current letters 'y'

			if (tabs[activeTab].layout[i].x > 20) //check if this is a table with a column offset larger than 20
			{
				//pick whatever x cords is further to the right
				int colX = (std::max)(xtrack, tabs[activeTab].layout[i].x);
				tabs[activeTab].layout[i].x = colX; //apply the updated x pos
			}
			else //if its not
			{
				//if its normal text, assign our current x track pos
				tabs[activeTab].layout[i].x = xtrack;
			}


			tabs[activeTab].layout[i].y = ytrack; // update the element's actual y screen pos too our current row
		}

		//check if the node is text, and make sure it hasn't generated a texture yet, and hasnt been attempted yet
		if (tabs[activeTab].layout[i].textTex == nullptr && !tabs[activeTab].layout[i].isImage && !tabs[activeTab].layout[i].textAttempted && tabs[activeTab].layout[i].node != nullptr)
		{
			//grab the text string from our current mainlayout node and make sure this holds the text payload!
			std::string text = tabs[activeTab].layout[i].node->tagValue; //create a temp string to hold the text 

			
			if (!text.empty()) // check to make sure the text contains something
			{
				tabs[activeTab].layout[i].textAttempted = true; //set the attempted to true, so we don't load it twice.

				//grab everything the thread will need.
				int fontsize = tabs[activeTab].layout[i].fontSize; //create an 'int' and set it to the fontsize.
				int fontweight = tabs[activeTab].layout[i].fontWeight; //create an 'int' and set it to the fontweight
				SDL_Color color = tabs[activeTab].layout[i].textColor; //set the color to the text color
				bool isLink = !tabs[activeTab].layout[i].href.empty(); //set the bool if its a link. if its true, its false, and if its false, its true


				int currentTabID = activeTab; //grab the active tab index.
				int myGen = tabs[activeTab].loadGen; //grab the current loadGen id.
				int currentIndex = i; //keep track of which index this is

				lock.unlock(); //unlock before the thread to prevent a crash in release mode
				gTextRenderPool.enqueue([text, fontsize, fontweight, color, isLink, currentTabID, myGen, currentIndex]() { //start the thread, and import the vars into the thread
					
					TTF_Font* threadFont = nullptr;
					SDL_Surface* nodeSurf = nullptr;
					{
						std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //lock the gTTFMutex, so that only this can load it.

						static std::unordered_map<int, TTF_Font*> fontCache; //create a map to hold the font.
						auto it = fontCache.find(fontsize);
						if (it != fontCache.end()) //make sure it only runs at the start, and does not run each loop
						{
							threadFont = it->second; //assign the thread font
						}
						else {
							threadFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", fontsize); //hold the font for the thread
							if (threadFont) //if it worked
								fontCache[fontsize] = threadFont; //update it
							else //if it did not
								std::cerr << "[TTF] Failed to open: " << SDL_GetError() << std::endl; //DEBUG

						}
						if (threadFont == nullptr) return; //if the font failed, exit.

						if (isLink) { TTF_SetFontStyle(threadFont, TTF_STYLE_UNDERLINE); } //if this text is a link, give it a underline

						TTF_FontStyleFlags style = TTF_STYLE_NORMAL;

						//BECAUSE FONTWEIGHTS NEED TO BE THE STUPID SDL3 FORMATS, im doing this..
						if (fontweight >= 700)
						{
							style |= TTF_STYLE_BOLD;
						}

						// underline for links
						if (isLink)
						{
							style |= TTF_STYLE_UNDERLINE;
						}

						TTF_SetFontStyle(threadFont, style);

						nodeSurf = TTF_RenderText_Solid(threadFont, text.c_str(), 0, color); //create a temp nodeSerf, to hold it
						
						//TTF_SetFontStyle(threadFont, fontweight); //RENDER THE FONT WEIGHT

						if (isLink) { TTF_SetFontStyle(threadFont, TTF_STYLE_NORMAL); } //reset the style after to prevent leaks

					}

					if (nodeSurf != nullptr) { //if the nodeSurf contains things

						std::lock_guard<std::mutex> lock(gTabsMutex); ///lock so we can update the tabs, without other threads doing so

						//check if the tab was closed, switched, or refreshed while the thread was working
						if (currentTabID < 0 || currentTabID >= (int)tabs.size() || tabs[currentTabID].loadGen != myGen || currentIndex < 0 || currentIndex >= (int)tabs[currentTabID].layout.size())
						{
							SDL_DestroySurface(nodeSurf); //destroy the surface if the tab state changed
						}
						else { //else
							//give the layout the nodesurf, so it can be indexed
							tabs[currentTabID].layout[currentIndex].pendingTextSurface = nodeSurf;
						}

					}		
				}); //end 
			lock.lock(); //relock to continue
			if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size() || tabs[activeTab].loadGen != currentLoadGen) return;//if tabs is empty, or the activetab is less than zero, or greater than size, or a loadGen mismatch, we return, and end;
			}
		}

		//make sure that the thread is done, and the tab holds data.
		if (tabs[activeTab].layout[i].pendingTextSurface != nullptr)
		{
			//create a temp SDL_Surface, where it swaps the surface pointer and clear's it
			SDL_Surface* nodeSurf = tabs[activeTab].layout[i].pendingTextSurface;
			tabs[activeTab].layout[i].pendingTextSurface = nullptr; //reset

			if (nodeSurf != nullptr) //make sure the node surf worked
			{
				int w = nodeSurf->w; //assign the width
				int h = nodeSurf->h; //assign the height
				
				lock.unlock(); //unlock for the gpu upload

				SDL_Texture* tex = SDL_CreateTextureFromSurface(render, nodeSurf); //upload the rendering to the gpu
				SDL_DestroySurface(nodeSurf); //we are done, now that its on gpu, to prevent meme leaks
				
				if (tex == nullptr) //if it failed
				{
					std::cerr << "[SDL] Failed to create the text texture: " << SDL_GetError() << std::endl;
				}
				lock.lock(); //relock
				if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size() || tabs[activeTab].loadGen != currentLoadGen) //if our state has changed
				{
					if (tex) SDL_DestroyTexture(tex); //if it has a texture, destroy it
					return; //end
				}
				if (i < 0 || i >= (int)tabs[activeTab].layout.size()) return; //Check to make sure we arnt outside the bounds, if we are, return

				if (tex != nullptr)
				{
					tabs[activeTab].layout[i].width = w;
					tabs[activeTab].layout[i].height = h;
					tabs[activeTab].layout[i].textTex = tex;

					//if its a movement style...
					if (tabs[activeTab].layout[i].node && tabs[activeTab].layout[i].node->style.find("center") != std::string::npos)
					{
						tabs[activeTab].layout[i].x = (WinW - w) / 2;
					}
					else if (tabs[activeTab].layout[i].node && tabs[activeTab].layout[i].node->style.find("right") != std::string::npos)
					{
						tabs[activeTab].layout[i].x = (WinW - 30) - w;
					}
				}
			}
		}

		//handle images
		//check if this item is an image, and that it has'nt been downloaded or queued yet.
		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].imageTex == nullptr && tabs[activeTab].layout[i].node != nullptr && !tabs[activeTab].layout[i].node->src.empty() && !tabs[activeTab].layout[i].imageAttempted)
		{
			tabs[activeTab].layout[i].imageAttempted = true; //make sure that we flag we are attempting it, so we avoid attempting it multiple times.

			//get the image path, and send it to ResolveURL to handle and fix any formatting like (\image.png)
			std::string src = ResolveURL(tabs[activeTab].layout[i].node->src, tabs[activeTab].url);
			std::cout << "[IMG] Resolved URL: " << src << std::endl; //DEBUG
			//Layout* currentItem = &tabs[activeTab].layout[i];

			int currentTabID = activeTab; //grab the currentTabID
			int myGen = tabs[activeTab].loadGen; //grab the loadGen
			int currentIndex = i; //get the current index (for the thread)
			std::string srcCopy = src; //create a temp to send to the thread, of the src
			std::string tabUrl = tabs[activeTab].url; //grab the tab url

			lock.unlock(); //unlock before the thread
			//make the thread
			gImageDownloadPool.enqueue([srcCopy, currentTabID, myGen, currentIndex, tabUrl]() { //send the image to our thread pool

				//download the bytes with our funct in ConnectSocket.cpp
				std::vector<unsigned char> bytes; //keep it out to prevent error
				auto startsWith = [](const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0;}; //check if it fits a url, rather than std::filesystem exists
				bool isLocalFile = startsWith(srcCopy, "file://") || startsWith(srcCopy, "./") || startsWith(srcCopy, "/") || (srcCopy.size() >= 2 && std::isalpha((unsigned char)srcCopy[0]) && srcCopy[1] == ':'); //if its a valid file (passes all the checks)
				if (isLocalFile) //if isLocalFile is true...
				{
					bytes = DownloadImages(srcCopy, true); //set the downloadImages to true, downloading to disk
				}
				else { //else
					bytes = DownloadImages(srcCopy, false); //set the downloadImages to false, downloading from the internet
				}
				if (!bytes.empty()) //if we got something (did NOT fail)
				{
					//create a IO mem stream from the raw bytes
												 //grab the bytes, and the size of it
					SDL_IOStream* io = SDL_IOFromMem(bytes.data(), (int)bytes.size());
					if (io != nullptr) //if the io contains something
					{
						//load the image surf from meme using SDL_image
						SDL_Surface* imageSurf = IMG_Load_IO(io, 1); //one closes the io after
						if (imageSurf != nullptr) //make sure its made correctly
						{
							std::lock_guard<std::mutex> lock(gTabsMutex); //make sure to lock, to prevent crashes with other threads editing gTabsMutex too.

							//make sure the tab did'nt change, (same as text)
							if (currentTabID < 0 || currentTabID >= (int)tabs.size() || tabs[currentTabID].loadGen != myGen || currentIndex < 0 || currentIndex >= (int)tabs[currentTabID].layout.size())
							{
								SDL_DestroySurface(imageSurf); //destroy the surf if the tab changes
							}
							
							else { //it did NOT change
								std::cout << "IMG Downloaded into RAM" << std::endl; //DEBUG
								tabs[currentTabID].layout[currentIndex].pendingSurface = imageSurf; //set the surf to the imageSurf.
							}
						}
						else { //ERROR, the bytes were screwed up.
							std::cout << "ERROR - Trying To Download IMG" << std::endl; //DEBUG

							std::cout << "FROM THIS - " << tabUrl << std::endl; //DEBUG
						}
					}
				}
				else { //could not find the image
					std::cout << "ERROR - No image detected" << std::endl; //DEBUG
				}
				});
			lock.lock(); //relock
			if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size() || tabs[activeTab].loadGen != currentLoadGen) return;
		}

		//--- GPU texture conversion for images ---\\

		//check if a background thread is done, and it is an image
		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].pendingSurface != nullptr)
		{
			//swap to the gpu
			SDL_Surface* surf = tabs[activeTab].layout[i].pendingSurface; //load the surf
			tabs[activeTab].layout[i].pendingSurface = nullptr; //clear it in the swap

			if (surf != nullptr) //if it worked
			{
				int w = surf->w; //assign the width
				int h = surf->h; //assign the height

				lock.unlock(); //unlock for the gpu upload

				SDL_Texture* tex = SDL_CreateTextureFromSurface(render, surf); //upload the image to the gpu
				SDL_DestroySurface(surf); //we are done, now that its on gpu, to prevent meme leaks

				std::cout << "IMG Downloaded" << std::endl; //DEBUG
				if (tex == nullptr) //if it failed
				{
					std::cerr << "[SDL] Failed to create the image texture: " << SDL_GetError() << std::endl;
				}
				lock.lock(); //relock

				if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size() || tabs[activeTab].loadGen != currentLoadGen) //if our state has changed
				{
					if (tex) SDL_DestroyTexture(tex); //if it has a texture, destroy it
					return; //end
				}
				if (i < 0 || i >= (int)tabs[activeTab].layout.size()) return; //Check to make sure we arnt outside the bounds, if we are, return

				if (tex != nullptr)
				{
					tabs[activeTab].layout[i].width = w;
					tabs[activeTab].layout[i].height = h;
					tabs[activeTab].layout[i].imageTex = tex;

					//if its a movement style...
					if (tabs[activeTab].layout[i].node && tabs[activeTab].layout[i].node->style.find("center") != std::string::npos)
					{
						tabs[activeTab].layout[i].x = (WinW - w) / 2;
					}
					else if (tabs[activeTab].layout[i].node && tabs[activeTab].layout[i].node->style.find("right") != std::string::npos)
					{
						tabs[activeTab].layout[i].x = (WinW - 30) - w;
					}
				}
			}
		}

		if (!CustomY)
		{
			//update the max line height, if its taller than its previous element
			if (tabs[activeTab].layout[i].height > maxLineHeight)
			{
				maxLineHeight = tabs[activeTab].layout[i].height;
			}

			//move the x by the width of the text + 12 for padding.
			xtrack += (tabs[activeTab].layout[i].width + 12);
		}
	}

} // END OF PreRender

#pragma endregion

#pragma region //Handle Tab Title

//======SetTabTitle======\\

//SET THE TAB TITLE
void SetTabTitle(std::string title) //SetTabTitle takes in a single string, and returns nothing.
{
	std::lock_guard<std::mutex> lock(gTabsMutex); //first lock, to make sure that other threads changing data like this won't cause a crash
	for (auto& t : tabs) //for each tab
	{
		if (t.tabID == lastsearchedtabID) //if the tabID matches the current tab
		{
			t.title = title; //set the title
			return; //end
		}
	}

} //END OF SetTabTitle

#pragma endregion

//Icon Guide! Home, Reload, Back+Forth+Arrows, search, star, printer, settings
//             љ	  њ				ђ		     ы		ж	    ξ        Ħ

bool isLinkClicked(std::string &clickedURL, bool RightClicked)
{
	{
		std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent other threads to change the tabs
		for (int i = 0; i < tabs[activeTab].layout.size(); i++) //loop through each tab in the tabs list
		{
			if (tabs[activeTab].layout[i].textTex == nullptr) continue; //check if its text, if it is, skip
			if (tabs[activeTab].layout[i].href.empty()) continue; //check if its a herf, if it does'nt contain anything, skip

			float screeny = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos; //ok we have a link, now lets adjust the activator block to be over it

			//test if the tabs activator button is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it

			if (RightClicked) //RightClicked
			{
				if (ContextMenuXPos >= tabs[activeTab].layout[i].x && ContextMenuXPos <= (tabs[activeTab].layout[i].x + tabs[activeTab].layout[i].width) &&
					ContextMenuYPos >= screeny && ContextMenuYPos <= (screeny + tabs[activeTab].layout[i].height))
				{
					std::string finalUrl = tabs[activeTab].layout[i].href; //grab the URL of the link, of what I just pressed

					std::string realDest = FixURLREDIRECT(finalUrl); //fixes issues with the link, and saves it to the string
					if (!realDest.empty()) { finalUrl = realDest; } //if it's not empty, update the finalURL to the realDest;

					clickedURL = finalUrl; //set the clicked url to the final url
					return true;
					break; //exit
				}
			}
			else {
				if (mouseX >= tabs[activeTab].layout[i].x && mouseX <= (tabs[activeTab].layout[i].x + tabs[activeTab].layout[i].width) &&
					mouseY >= screeny && mouseY <= (screeny + tabs[activeTab].layout[i].height))
				{
					std::cout << "link pressed" << std::endl; //DEBUG
					std::string finalUrl = tabs[activeTab].layout[i].href; //grab the URL of the link, of what I just pressed

					std::string realDest = FixURLREDIRECT(finalUrl); //fixes issues with the link, and saves it to the string
					if (!realDest.empty()) { finalUrl = realDest; } //if it's not empty, update the finalURL to the realDest;

					clickedURL = finalUrl; //set the clicked url to the final url
					return true;
					break; //exit
				}
			}	
		}
		return false;
	}
}
//FUNCTION TO RENDER THE File open
static void SaveCallback(void* str, const char* const* files, int filter) //SaveCallback loads a file, and takes in an int (though its unused)
{
	if (files && files[0]) //Check if the user closes the tab, if so, return NULL. The files[0] makes sure its valid.
	{
		SDL_SaveFile(files[0], (const char*)str, SDL_strlen((const char*)str)); //Save the file to the file path, using the string, and setting the size. 
	}
}

bool isHoverd(SDL_FRect rect) { return (mouseX >= rect.x && mouseX <= (rect.x + rect.w) && mouseY >= rect.y && mouseY <= (rect.y + rect.h)); }

void ReloadTabLayout()
{
	if (tabs.empty() || activeTab < 0 || activeTab >= tabs.size()) { return;  } //if the tabs are out of loading, or empty, dont do this

	Node* root = tabs[activeTab].domRoot; //build a new root
	if (root == nullptr)
	{
		return;
	}
	//now that we know we are for SURE loading the new stuff, first save the current scroll pos
	float oldScrollPos = tabs[activeTab].scrollpos;

	std::unordered_map<std::string, SDL_Texture*> oldImageCache; //holds the old images
	
	{ //LOCK
		
		std::lock_guard<std::mutex> lock(gTabsMutex);
		tabs[activeTab].loadGen++; //increase the loadGen, stopping other threads

		for (auto& item : tabs[activeTab].layout) {

			if (item.isImage && item.imageTex != nullptr && item.node != nullptr) //if the item has an image, is an image, and has a texture...
			{
				oldImageCache[item.node->src] = item.imageTex; //save the imageTex
				item.imageTex = nullptr;
			}
		}
	}
	
	tabs[activeTab].domRoot = nullptr; //force layouttree to regen the 

	activeCSS = &tabs[activeTab].css;

	LayoutTree(root); //regen

	tabs[activeTab].scrollpos = oldScrollPos; //make sure to set the scroll pos, to prevent teleportation to the top of the page

	//ok, load images back
	{ //LOCK
		std::lock_guard<std::mutex> lock(gTabsMutex);
		for (auto& newitem : tabs[activeTab].layout) {
			if (newitem.isImage && newitem.node != nullptr)
			{
				auto img = oldImageCache.find(newitem.node->src); //attempt to see if its an image node
				if (img != oldImageCache.end()) //if so...
				{
					newitem.imageTex = img->second; //set the newitem, to the imageText

					newitem.imageAttempted = true; //mark we started (like how we load above)
					
					float w, h; //hold w and h

					SDL_GetTextureSize(newitem.imageTex, &w, &h); //get the size of the text, and its w + h

					newitem.width = w;
					newitem.height = h;

					oldImageCache.erase(img); //we done, to prevent mem leak
				}
			}
		}
	}

	//Destory all the extra old images
	for (auto& pair : oldImageCache)
	{
		if (pair.second) { SDL_DestroyTexture(pair.second); } //if the second part contains an image, rm
	}


	

}

int GUIRENDER(std::string StartingTab) //GUIRENDER returns an ' int ' and takes in nothing
{
	currentURL = tabs[activeTab].url;
	urlInput = tabs[activeTab].url;

	SDL_Texture* fontText = nullptr; //create a SDL_Texture holds the font texture overall.

	//we want to use the screen so use video
	//we prob wont using audio, but we can through | SDL_INIT_AUDIO
	if (!SDL_Init(SDL_INIT_VIDEO)) { std::cout << "Failed to init SDL" << std::endl;return -1; }//if we get false (it didnt work), we send a DEBUG, then return -1

	
	SDL_Window* window; //create a SDL_Window to handle and hold the window overall
	//create window, some prams are 
	//title, width, height, and flags
	//make it 1080p sizewise
	//for flags, its in this format -> UINT64_C(0X0000000000000020), we also can add more tags through |
	window = SDL_CreateWindow("Browse++", 1600, 900, SDL_WINDOW_RESIZABLE);

	SDL_Renderer* render = SDL_CreateRenderer(window, nullptr); //Create a render, using the window values, and that we can assign more values to this render, to render more things

	SDL_SetRenderVSync(render, 1); //enable VSync, to keep performance high!

	//BUILD THE ICON
	SDL_Surface* icon = IMG_Load("./logo/logo.png"); //grab the icon
	if (icon) //if its been created
	{
		SDL_SetWindowIcon(window, icon); //set the window icon to the png
		SDL_DestroySurface(icon); //destroy.
	}










	//tell SDL we want to record SDL_EVENT_TEXT_INPUTs when the user types
	SDL_StartTextInput(window); //detect inputs from text

	if (!TTF_Init()){ std::cout << "Failed to init SDL::TTF" << std::endl; return -1; } //We Init the Font lib, and if it fails, we print "Failed to init SDL::TTF


	//Now that we got it init, lets render that font
	//open the font file from disk.
	//16 is the default size (we change this in PreRender)
	static TTF_Font* font = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 16); // Create a custom main font, that holds a base size for normal text.
	static TTF_Font* iconFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 28); // Create a custom icon font, that is scaled for icons
	static TTF_Font* reloadFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 72); // Create a custom reload font, that is scaled for the reload text
	
	if (font == nullptr || iconFont == nullptr || reloadFont == nullptr) { std::cout << "Failed to open TTF" << std::endl; return -1; } //if any of the fonts fail to load, we return some DEBUG, then return -1
 
	//shows events like window changes and stuff
	SDL_Event event; //we make an SDL event handler

	//i added this, because my padding and stuff kept moving and it was super annoying!
	const float btnSize = 30.0f;   // Keep button size constant
	const float padding = 15.0f;   // always 10px of space
	const float topMargin = 37.0f; // dist from the top of the window

	bool running = true; //this var gets set to false when the window is closed, cleanly ending this.
	static bool RecalcLayout = false;
	static float LastTickSinceRecalcLayout = 0; 

	
	//LOAD
	if (!StartingTab.empty())
	{
		NavigateTo(StartingTab, render, font);
	}

	SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND); //For the shadow

	LoadFromHistory(); //turn it on

	while (running) //loop till running is false
	{
		Uint64 start = SDL_GetPerformanceCounter(); //start the Performance overview


		//SET THE SCALE X AND Y EARLY (for buttn detections)
		float scaleX, scaleY; //create 2 floats to hold the windows's scaleX and scaleY
		SDL_GetRenderScale(render, &scaleX, &scaleY); //assign the scaleX, scaleY using SDL_GetRenderScale

		#pragma region //SDL event's
		while (SDL_PollEvent(&event)) //poll the SDL event's
		{
			if (event.type == SDL_EVENT_WINDOW_RESIZED) //if the size is resized
			{
				WinW = event.window.data1; //update the Width
				WinH = event.window.data2; //update the height

				RecalcLayout = true;
				LastTickSinceRecalcLayout = SDL_GetTicks(); //get the ticks
			}

			if (event.type == SDL_EVENT_QUIT) { running = false; } //if the window 'x' close button is pressed, we end the loop

			if (event.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN)
			{
				ReloadTabLayout();
			}
			if (event.type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN)
			{
				ReloadTabLayout();
			}


			mouseX = event.button.x; //save the pos of the mouseX
			mouseY = event.button.y; //save the pos of the mouseY

			if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) { DraggingScrollBar = false; } //if we stop clicking, and this works anywhere, we set the dragging to false
			if (event.type == SDL_EVENT_MOUSE_MOTION && DraggingScrollBar && !tabs.empty()) // update the pos if the mouse is moving, mouse is down, and tabs contain something
			{
				
				int WinW, WinH; //create a int to hold the W and H
				SDL_GetCurrentRenderOutputSize(render, &WinW, &WinH); // grab the output size, and assign the WinW and WinH
				float uiTopBarHeight = 70.0f;  //float to hold the start of the bar
				float trackHeight = WinH - uiTopBarHeight; //trackheight holds how long the bar should be, subtracting so that it ends on that start pos
				if (trackHeight > 0) //if you can scroll on the page
				{
					float barHeight = (trackHeight / (float)(tabs[activeTab].maxscroll + WinH)) * trackHeight; //the height of the bar, we take teh winH, the trackHight, and the max scroll!
					if (barHeight < 20.0f) barHeight = 20.0f; //we also make sure it cant get smaller than this, or it might disappear!
					if (barHeight > 50.0f) barHeight = 50.0f;

					//figure out where the bar should be based on the mouse pos
					float mouseY = event.motion.y; //save the .y
					float newBarY = mouseY - (barHeight / 2.0f); //subtract the height so that the middle of the bar is the by my mouse
					float minBarY = uiTopBarHeight; //hold the current height
					float maxBarY = uiTopBarHeight + trackHeight - barHeight; //hold the max height of the bar

					if (maxBarY > minBarY) //if we still can scroll, and have'nt gon past the max bar height
					{
						newBarY = std::clamp(newBarY, minBarY, maxBarY); //clamp so it cannot be dragged outside

						float scrollPercentage = (newBarY - minBarY) / (maxBarY - minBarY); //convert the Y to the %

						std::lock_guard<std::mutex> lock(gTabsMutex); //make sure no other threads can change this while this changes it, to prevent a crash
						tabs[activeTab].scrollpos = scrollPercentage * tabs[activeTab].maxscroll; //update the pos
					}
				}
			}

			if (event.type == SDL_EVENT_MOUSE_WHEEL) {//if we detect the wheel scrolled

				ContextMenu = false, SaveXYContextMenuPos = false; //set that we want to hide the context menu, as thats how most browsers work

				float wheelY = event.wheel.y; //hold the wheel 'y'
				if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) //if the mouse is inverted
				{
					wheelY *= -1.0f; //flip the dir
				}
				if (wheelY != 0.0f) //if its greater than the start
				{
					std::lock_guard<std::mutex> lock(gTabsMutex); //make sure no other threads can change this while this changes it, to prevent a crash
					auto& active = tabs[activeTab]; //create a temp holder for the activetab

					float targetScroll = active.scrollpos - (wheelY * 80); //figure out the scrollpos, a bit simpler

					float minScroll = 40.0f; //hold the minimum max
					float maxScroll = (std::max)(minScroll, static_cast<float>(active.maxscroll)); //make sure maxScroll isn't less than minScroll if content is short

					active.scrollpos = std::clamp(targetScroll, minScroll, maxScroll); //Clamp scroll pos
				}
			}

			if (event.type == SDL_EVENT_TEXT_INPUT) { //detect any text inputs, and add them to our input str
				ContextMenu = false, SaveXYContextMenuPos = false; //set that we want to hide the context menu, as the context menu is all mouse clicks
				if (inputbarfocused)
				{
					urlInput += event.text.text; //append to whatever they typed, to our urlInput var
				}
			}

			if (event.type == SDL_EVENT_KEY_DOWN) //check for any event key down
			{

				if (event.key.mod & SDL_KMOD_CTRL)
				{
					std::cout << "CTRL - Pressed" << std::endl; //DEBUG

					if (event.key.key == SDLK_T) //ctr + T -> NEW TAB
					{
						Tab newTab; //create a temp Tab handler to hold the newTab.
						newTab.title = "New Tab"; //give it a title
						//push it back
						{
							std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
							tabs.push_back(newTab); //send a new tab to the list
							//update the active tab
							activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
							lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
						}
						NavigateTo("new::tab", render, font, true); //go to the new tab, using the id
					}
					if (event.key.key == SDLK_W && tabs.size() > 1) //ctr + W -> CLOSE TAB
					{
						std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
						if (!tabs.empty()) //if the tabs contain something
						{
							if (tabs[activeTab].domRoot != nullptr) //make sure that the current tab has a dom
							{
								DeleteTree(tabs[activeTab].domRoot); //destroy it
							}
							tabs.erase(tabs.begin() + activeTab); //erase the old tab

							activeTab = (int)tabs.size() - 1; //update the active tab, to another one

							currentURL = tabs[activeTab].url; //update the current url
							urlInput = tabs[activeTab].url; //update the urlInput
						}
					}
					if (event.key.key == SDLK_R) //ctr + R -> RELOAD TAB
					{
						if (currentURL != "") { NavigateTo(urlInput, render, font, false); } //don't reload for the mainpage, but just index the same url	
					}
				}

				if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !urlInput.empty()) { //if we detect backspace, and the urlInput contains something
					urlInput.pop_back(); //we remove the last character
				}

				//if we press enter to search
				if (event.key.scancode == SDL_SCANCODE_RETURN) {
					lastsearchedtabID = tabs[activeTab].tabID; //set the TabID, beforehand, to prevent the loading bug
					inputbarfocused = false;
					NavigateTo(urlInput, render, font, true); //Send the url we want to go to, the render, font, and we want to save it to history
				}

				if (event.key.scancode == SDL_SCANCODE_V && (event.key.mod & SDL_KMOD_CTRL)) { //if control+V is pressed
			
					const char* clipboard = SDL_GetClipboardText(); //grab the clipboard from the device.
			
					if (clipboard != nullptr) { urlInput += clipboard; } //make sure the clipboard has something, if it does, append the clipboard info onto the end of the urlInput
				}
			}

			int WinW, WinH; //create a int to hold the W and H
			SDL_GetCurrentRenderOutputSize(render, &WinW, &WinH); // grab the output size, and assign the WinW and WinH
		

			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) //handle if the mouse is clicked
			{
				
				if (event.button.button == SDL_BUTTON_RIGHT) {//check if our right mouse pressed
					ContextMenuXPos = mouseX, ContextMenuYPos = mouseY;
					ContextMenu = true, SaveXYContextMenuPos = true; //set that we want to render the context menu


					//CHECK IF A TAB IS PRESSED
					if (mouseY < 30)
					{
						int tabX = 0; //pos of the first tab (X) (as we always have at least 1 tab)
						int tabW = 180; //pos of the first tab (Y) (as we always have at least 1 tab)

						for (int t = 0; t < tabs.size(); t++) //loop through each tab
						{
							//check if the tab itself was clicked
							if (mouseX >= tabX && mouseX <= tabX + tabW)
							{
								std::cout << "Clicked top bar, TAB: " << t << std::endl; //DEBUG

								clickedTab = t; //store the clicked tab.
								break; //exit, we are done
							}
							else {
								clickedTab = -1; //we did not press a tab, so just return a blank
							}
							tabX += tabW + 2; //move the collider box to the next tab
						}
					}
					else {
						clickedTab = -1; //we did not even click the top bar, so return -1
					}
				}

				SDL_FRect ContextMenuRect;  //set a rect for the ContextMenuRect

				if (ContextMenuYPos < WinH / 2) //if you click on the top half
				{
					if ( ContextMenuYPos < WinW / 2 ) //that means we are on the left side
					{
						ContextMenuRect = { ContextMenuXPos, ContextMenuYPos, 200, 315 }; //down
					}
					else {
						ContextMenuRect = { ContextMenuXPos - 200, ContextMenuYPos, 200, 315 }; //down
					}

				}
				else { //if you click on the bottom half
					if (ContextMenuXPos < WinW / 2 ) //that means we are on the right side
					{
						ContextMenuRect = { ContextMenuXPos, ContextMenuYPos - 315, 200, 315 }; //up
					}
					else {
						ContextMenuRect = { ContextMenuXPos - 200, ContextMenuYPos - 315, 200, 315 }; //up
					}
				}

				bool clickingContextMenu = ContextMenu && isHoverd(ContextMenuRect);//handle to see if its being clicked, and the context menu is on (clean wrapper)


				if (!clickingContextMenu && !(event.button.button == SDL_BUTTON_RIGHT)) { //if we DONT click on the rect, and its not the sdlRightButton
					ContextMenu = false, SaveXYContextMenuPos = false; //set that we want to hide the context menu, as we clicked nothing else	
				}
				if (event.button.button == SDL_BUTTON_LEFT) //check to see if SDL button left is pressed
				{
					if (clickingContextMenu) //if we click the context menu, we want to see what button we have clicked on it, then we want to skip the stack
					{
						//now we need a way to figure out, how we are going to go from 'clicked' -> detecting the correct buttn -> action, im thinking like how we do tabs, a loop.

						float buttonHeight = 45.0;
					
						SDL_FRect CurrentButtnRect;

						//ContextMenuRect.w for our button width, maybe with some padding

						for (int i = 0; i < ContextMenuButtons.size(); i++)
						{

							//now we want to render each button, like how we render tabs.
							float buttnX = ContextMenuRect.x; //hold the x of it
							float buttnY = ContextMenuRect.y + (i * (buttonHeight)); //change the x, based on the size of the button * the i, with some sort of padding
							CurrentButtnRect = { buttnX, buttnY, ContextMenuRect.w, buttonHeight };

							if (mouseX >= CurrentButtnRect.x && mouseX <= (CurrentButtnRect.x + CurrentButtnRect.w) &&
								mouseY >= CurrentButtnRect.y && mouseY <= (CurrentButtnRect.y + CurrentButtnRect.h)) {
								if (ContextMenuButtons[i] == "-------") { break; } //if its not a real button
								std::cout << "buttnPressed: " << ContextMenuButtons[i] << std::endl; //DEBUG


								//ContextMenuButtons = { "New Tab", "Print", "Change Theme", "-------", "Back", "Forward", "-------", }; ///legend
								
								//NOW LOGIC FOR EACH BUTTON
								if (ContextMenuButtons[i] == "New Tab")
								{
									Tab newTab; //create a temp Tab handler to hold the newTab.
									newTab.title = "New Tab"; //give it a title
									//push it back
									{
										std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
										tabs.push_back(newTab); //send a new tab to the list
										//update the active tab
										activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
										lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
									}
									NavigateTo("new::tab", render, font, true); //go to the new tab, using the id
								}
								if (ContextMenuButtons[i] == "Print")
								{
									int uiHeight = 80; //crop the uiHeight
									SDL_Rect contentArea; //create a temp content area

									//x,w, h we leave, but, y we adjust
									contentArea.x = 0; 	contentArea.y = uiHeight; 	contentArea.w = WinW; 	contentArea.h = WinH - uiHeight; //we save the x, y, and width of the window, but we crop the h	
									SDL_Surface* screenshot = SDL_RenderReadPixels(render, &contentArea); //create a surface, holding the screenshot, taking the rect we want to capture

									if (screenshot != nullptr)
									{
										std::cout << "Printing..." << std::endl; //DEBUG
										std::string saveFolder = "Printed_Pages"; //create a temp string "Printed Pages"
										std::filesystem::create_directories(saveFolder); //create a new dir, called "Printed_Pages"
										#ifdef _WIN32 //handle windows
											std::string filenameStr = saveFolder + "\\Tab_print_" + std::to_string(SDL_GetTicks()) + ".bmp"; //create a file, in our folder, calling it Tab_print_ (then a time) .bmp
											const char* filename = filenameStr.c_str(); //create a char to prevent errors

										#else //Handle linux
											std::string filenameStr = saveFolder + "/Tab_print_" + std::to_string(SDL_GetTicks()) + ".bmp"; //create a file, in our folder, calling it Tab_print_ (then a time) .bmp
											const char* filename = filenameStr.c_str(); //create a char to prevent errors
										#endif // _WIN32
										
										SDL_SaveBMP(screenshot, filename); 	//now we run the print command

										#ifdef _WIN32 //handle windows
											ShellExecuteA(NULL, "print", filename, NULL, NULL, SW_SHOWNORMAL); 	// Ask Windows to handle the printing, but SHOW the menu normally

										#else //Handle linux

											std::string printcmd = "lp \"" + filenameStr + "\""; //build the command to send to CUPS (To print)
											system(printcmd.c_str());
										#endif // _WIN32

									
										SDL_DestroySurface(screenshot);//we are done here, destroy the old screenshot
									}
								}
								if (ContextMenuButtons[i] == "Change Theme") //CURRENT BUG, TEXT DONT CHANGE COLOR - TODO
								{
									darkmode = !darkmode; //flip it
									ReloadTabLayout();
									
								}
								if (ContextMenuButtons[i] == "Back") //CURRENT BUG, NEED TO SHOW IF YOU CAN PRESS OR NOT - TODO
								{
									if (tabs[activeTab].historypos > 0) //we do > 0, as we want to go back one, and we cannot go to -1
									{
										tabs[activeTab].historypos--; //move the history back one
										std::string prevURL = tabs[activeTab].history[tabs[activeTab].historypos]; //grab the prev url
										urlInput = prevURL; //set the urlInput to the prevURL
										std::cout << prevURL; //DEBUG
										NavigateTo(prevURL, render, font, false); //Navigate to the new url
									}
								}
								if (ContextMenuButtons[i] == "Forward") //CURRENT BUG, NEED TO SHOW IF YOU CAN PRESS OR NOT - TODO
								{
									if (tabs[activeTab].historypos < (int)tabs[activeTab].history.size() - 1)  //we do < (int)tabs[activeTab].history.size() - 1, as we want to go forward one, and we cannot go past the limit
									{
										tabs[activeTab].historypos++; //move the history forward one
										std::string nextURL = tabs[activeTab].history[tabs[activeTab].historypos]; //grab the next url
										urlInput = nextURL; //set the urlInput, to the nextURL
										currentURL = ""; //reset
										tabs[activeTab].url = "";  //reset
										NavigateTo(nextURL, render, font, false); //Navigate to the new url
									}
								}

								//------------------------------------------------------------------------------------------------------------------------------------------------
								//HANDLE TAB CONTEXT MENU STUFF


								// ContextMenuButtons = { "Reload", "Close", "-------", "History", "Close All Tabs", "Performance", "-------", }; ///legend

								if (ContextMenuButtons[i] == "Reload")
								{
									if (currentURL != "") { NavigateTo(urlInput, render, font, false); } //don't reload for the mainpage, but just index the same url
								}
								if (ContextMenuButtons[i] == "Close" && tabs.size() > 1) //CURRENT BUG, NEED TO SHOW IF YOU CAN PRESS OR NOT - TODO
								{
									std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values

									if (clickedTab == -1)
									{
										if (tabs[activeTab].domRoot != nullptr) { DeleteTree(tabs[activeTab].domRoot); } // if the domRoot contains something, we destory the tree, for cleanup
										tabs.erase(tabs.begin() + activeTab); //remove the tab

										//make sure that that we cannot let the tabs go negative, if that were possible.
										if (activeTab >= (int)tabs.size()) { activeTab = tabs.size() - 1; }//if we don't meet the condition, we can close it.

										//if we end up closing, and have no active tabs, create a new one, to prevent a softlock
										if (tabs.empty()) { tabs.push_back(Tab()); activeTab = 0; }

										//update the url's to the tab, and fills in the info
										currentURL = tabs[activeTab].url;
										urlInput = tabs[activeTab].url;
									}
									else {
										if (tabs[clickedTab].domRoot != nullptr) { DeleteTree(tabs[clickedTab].domRoot); } // if the domRoot contains something, we destory the tree, for cleanup
										tabs.erase(tabs.begin() + clickedTab); //remove the tab

										if (clickedTab < activeTab) { activeTab--; }
										//make sure that that we cannot let the tabs go negative, if that were possible.
										if (clickedTab >= (int)tabs.size()) { clickedTab = tabs.size() - 1; }//if we don't meet the condition, we can close it.

										//if we end up closing, and have no active tabs, create a new one, to prevent a softlock
										if (tabs.empty()) { tabs.push_back(Tab()); activeTab = 0; }

										//update the url's to the tab, and fills in the info
										currentURL = tabs[activeTab].url;
										urlInput = tabs[activeTab].url;
									}	
								}
								if (ContextMenuButtons[i] == "History" && tabs[activeTab].url != "history::tab") //Dont allow it to work for the history tab itself.
								{
									//grab the last tab id (before we switch)
									TabBefore = activeTab;

									Tab settingsTab; //create a temp Tab handler to hold the settingsTab.
									settingsTab.title = currentURL; //give it a transition title...
									settingsTab.url = "history::tab"; //set the url
									currentURL = "history::tab";
									//push it back
									{
										std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
										tabs.push_back(settingsTab); //send a new tab to the list
										//update the active tab
										activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
										lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
									}
									LoadFromHistory(); //load the history

									std::cout << TabBefore << std::endl;
									NavigateTo("history::tab", render, font, false); //go to the link, as opening a new tab, using the id, do not save history when opening


									//History
								}
								if (ContextMenuButtons[i] == "Close All Tabs")
								{
									for(int t = tabs.size() - 2; t >= 0; t--)
									{	
										std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values

										if (tabs[activeTab].domRoot != nullptr) { DeleteTree(tabs[activeTab].domRoot); } // if the domRoot contains something, we destory the tree, for cleanup
										tabs.erase(tabs.begin() + activeTab); //remove the tab
										 
										//make sure that that we cannot let the tabs go negative, if that were possible.
										if (activeTab >= (int)tabs.size()) { activeTab = tabs.size() - 1; }//if we don't meet the condition, we can close it.

										//if we end up closing, and have no active tabs, create a new one, to prevent a softlock
										if (tabs.empty()) { tabs.push_back(Tab()); activeTab = 0; }

										//update the url's to the tab, and fills in the info
										currentURL = tabs[activeTab].url;
										urlInput = tabs[activeTab].url;
									}	
									NavigateTo("new::tab", render, font, false); //Open the new tab at the end.
								}

								if (ContextMenuButtons[i] == "Performance") //TODO
								{
									ShowPerformace = !ShowPerformace;
								}
								//------------------------------------------------------------------------------------------------------------------------------------------------
								//HANDLE LINK CONTEXT MENU STUFF
								
								// ContextMenuButtons = { "Open", "Open New Tab", "In New Window", "-------", "Copy Link", "Save Link As", "-------", }; ///legend

								if (ContextMenuButtons[i] == "Open") //OPEN the link
								{
									urlInput = clickedURL; //set the urlInput to the clicked url
									NavigateTo(clickedURL, render, font, true); //Now we navigate to it
								}
								if (ContextMenuButtons[i] == "Open New Tab") //OPEN the link (in a new tab.)
								{
									Tab newTab; //create a temp Tab handler to hold the newTab.
									newTab.title = "Loading..."; //give it a transition title...
									//push it back
									{
										std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
										tabs.push_back(newTab); //send a new tab to the list
										//update the active tab
										activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
										lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
									}
									NavigateTo(clickedURL, render, font, true); //go to the link, as opening a new tab, using the id
								}
								if (ContextMenuButtons[i] == "Copy Link") //COPY the link
								{
									
									if (SDL_SetClipboardText(clickedURL.c_str())) { std::cout << "Copied." << std::endl; } //DEBUG
									else { std::cout << "Error, failed to copy...  Unknown.." << std::endl; }
								}
								if (ContextMenuButtons[i] == "Save Link As") //TODO
								{
									static std::string saveFileHTMLTemp;

									if (clickedURL.find("http://") != 0 && clickedURL.find("https://") != 0) //if the link does not have a https:// (for other browsers)
									{
										clickedURL = "https://" + clickedURL;
									}

									saveFileHTMLTemp = 
										"<!DOCTYPE html>\n"
										"<html>\n"
										"<head>\n"
										"    <meta http-equiv=\"refresh\" content=\"0; url=" + clickedURL + "\" />\n"
										"</head>\n"
										"<body>\n"
										"    <p>If you are not redirected, <a href=\"" + clickedURL + "\">click here</a>.</p>\n"
										"</body>\n"
										"</html>";


									const SDL_DialogFileFilter filters[] = {
										{ "HTML Files", "html;htm" }
									};
									//save settings
									SDL_ShowSaveFileDialog(
										SaveCallback, 
										(void*)(saveFileHTMLTemp.c_str()), //load the file
										window,             // main SDL window
										filters, 1,            // save only html
										NULL                // No default folder path
									);
								}
								if (ContextMenuButtons[i] == "In New Window") //open a new window
								{
					
									const char* mainpath = SDL_GetBasePath(); //found this in the docs, this grabs the dir where the app is running, but cross platform!

									if (!mainpath)
									{
										std::cerr << "ERROR, could not find exe: " << SDL_GetError() << std::endl;
									}
									else 
									{
										//handle the path off the os, for the exe.

										#ifdef _WIN32 //windows
											std::string exepath = std::string(mainpath) + "WebEngine.exe";

										#else //linux
											std::string exepath = std::string(mainpath) + "WebEngine";
										#endif // DEBUG

										const char* args[] = { exepath.c_str(), clickedURL.c_str(), nullptr }; //provide the better args, to work for both, and works a bit better for sdl3

										SDL_Process* main = SDL_CreateProcess(args, false); //create the process

										if (!main)
										{
											std::cerr << "ERROR, could not open new window: " << SDL_GetError() << std::endl;
										}
										else {
											SDL_DestroyProcess(main);
										}
									}
								}
								//after we click something, we should close the context menu
								ContextMenu = false, SaveXYContextMenuPos = false;
							}
						}
						continue; //skip the stack
					}
					float uiTopBarHeight = 70.0f;  //float to hold the start of the bar
					float trackHeight = WinH - uiTopBarHeight; //trackheight holds how long the bar should be, subtracting so that it ends on that start pos
					if (trackHeight > 0) //if you can scroll on the page
					{
						float scrollBarWidth = 20.0f;
						float barHeight = (trackHeight / (float)(tabs[activeTab].maxscroll + WinH)) * trackHeight; //the height of the bar, we take teh winH, the trackHight, and the max scroll!
						if (barHeight < 20.0f) barHeight = 20.0f; //we also make sure it cant get smaller than this, or it might disappear!
						if (barHeight > 50.0f) barHeight = 50.0f;

						float scrollPercentage = (float)tabs[activeTab].scrollpos / (float)tabs[activeTab].maxscroll; //calculate its pos out of the len of the bar, based on how far we are down the page
						float barY = uiTopBarHeight + (scrollPercentage * (trackHeight - barHeight)); //then using the scrollPercentage float, we use it to draw the bar accurately

						SDL_FRect scrollbarRect = { WinW - scrollBarWidth, barY, scrollBarWidth, barHeight }; //create a rect to hold the scroll bar, like backBtnRect

						//check if the bar is pressed
						if (mouseX >= scrollbarRect.x && mouseX <= (scrollbarRect.x + scrollbarRect.w) &&
							mouseY >= scrollbarRect.y && mouseY <= (scrollbarRect.y + scrollbarRect.h)) {
							DraggingScrollBar = true; //set the drag to true
						}
					}

					SDL_GetWindowSize(window, &WinW, &WinH); //grab the window, and assign the WinW, and the WinH.
					SDL_FRect ResetHistoryBtn; //create a rect to hold the settings button

					ResetHistoryBtn.h = btnSize; ResetHistoryBtn.w = 200; ResetHistoryBtn.x = 0 + ResetHistoryBtn.w - 178; ResetHistoryBtn.y = 0 + ResetHistoryBtn.h + 82;
					
					SDL_FRect ResetHistoryBtnRect = {(float)ResetHistoryBtn.x, (float)ResetHistoryBtn.y, (float)ResetHistoryBtn.w, (float)ResetHistoryBtn.h};
					SDL_FRect backBtnRect = { padding, topMargin, btnSize, btnSize }; //set a rect for the backBtnRect
					SDL_FRect fwdBtnRect = { backBtnRect.x + backBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the fwdBtnRect, using the backBtnRect for offset
					SDL_FRect reloadBtnRect = { fwdBtnRect.x + fwdBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the reloadBtnRect, using the fwdBtnRect for offset
					SDL_FRect homeBtnRect = { reloadBtnRect.x + reloadBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the homeBtnRect, using the reloadBtnRect for offset
					float searchX = homeBtnRect.x + homeBtnRect.w + padding; //get the x size of the search bar
					float searchW = (float)WinW - btnSize - padding - padding - padding - searchX - (btnSize * 3); //get the width of the search bar
					SDL_FRect searchBarRect = { searchX, topMargin, searchW, btnSize }; //get the rect of the search bar, using the searchX and searchW
					SDL_FRect starBtnRect = { searchBarRect.x + searchBarRect.w + padding, topMargin, btnSize, btnSize };  //set a rect for the starBtnRect, using the searchBarRect for offset
					SDL_FRect printerBtnRect = { starBtnRect.x + starBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the printerBtnRect, using the starBtnRect for offset
					SDL_FRect settingsBtnRect = { printerBtnRect.x + printerBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the printerBtnRect, using the printerBtnRect for offset
					SDL_FRect bar = { //hold the dimensions of the input bar
						SDL_floorf(searchX * scaleX) / scaleX, //set the x of the bar
						SDL_floorf(topMargin * scaleY) / scaleY, //set the y of the bar
						SDL_floorf(searchW * scaleX) / scaleX, //set the w of the bar
						SDL_floorf(btnSize * scaleY) / scaleY //set the h of the bar
					};

					bool ClickedSuggest = false;

					if (inputbarfocused && !urlInput.empty())
					{
						std::vector<std::string> clickedSuggestions; //hold the suggestions

						std::string typed = urlInput;

						std::transform(typed.begin(), typed.end(), typed.begin(), [](unsigned char c) {
							return std::tolower(c);
							}); //convert to lower

						
						for (const auto& history : GlobalHistory) //for each val in GlobalHistory
						{

							std::string url = history.second; std::string lowerUrl = url; //grab the url, then make another var to get it lower.
							std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), [](unsigned char c) {
								return std::tolower(c); }); //convert to lower

							if (lowerUrl.starts_with(typed)) //if the url starts with the typed
							{
								clickedSuggestions.push_back(history.second); //push the url back if it matches with the typed stuff

								if (clickedSuggestions.size() >= 5) //make sure we dont go over.
									break;
							}
						}

						//now that we found things that START with it, lets handle if they contain it

						if (clickedSuggestions.size() < 5)
						{
							for (const auto& history : GlobalHistory) //for each val in GlobalHistory
							{
								std::string url = history.second; std::string lowerUrl = url; //grab the url, then make another var to get it lower.

								std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), [](unsigned char c) {
									return std::tolower(c); }); //convert to lower

								if (lowerUrl.find(typed) != std::string::npos && !lowerUrl.starts_with(typed)) //if the url starts with the typed
								{
									clickedSuggestions.push_back(history.second); //push the url back if it matches with the typed stuff

									if (clickedSuggestions.size() >= 5) //make sure we dont go over.
										break;
								}
							}
						}


						for (int i = 0; i < static_cast<int>(clickedSuggestions.size()); i++)
						{
							SDL_FRect suggestionRect = { bar.x, bar.y + 30 + (bar.h * i), bar.w, bar.h }; //hold the rect of the buttn

							if (isHoverd(suggestionRect))
							{
								std::string selectedURL = clickedSuggestions[i];

								std::cout << "clicked: " << selectedURL << " Pushing..." << std::endl;

								urlInput = selectedURL;
								inputbarfocused = false;
								ClickedSuggest = true;


								lastsearchedtabID = tabs[activeTab].tabID;

								//load

								NavigateTo(selectedURL, render, font, true);

								break;
							}
						}
					}

					if (!ClickedSuggest)
					{
						if (isHoverd(bar)) //handle reset button for the history menu
						{
							inputbarfocused = true;
						}
						else {
							inputbarfocused = false;
						}
					}
					

					if (isHoverd(ResetHistoryBtnRect)) //handle reset button for the history menu
					{
						DestoryHistory(); //rm the history
						if (currentURL != "") { NavigateTo(urlInput, render, font, false); } //reload
					}

					//test if the backBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(backBtnRect)) {
						//first check, is our index var for the area >= 0? as we reference it
						if (tabs[activeTab].historypos > 0) //we do > 0, as we want to go back one, and we cannot go to -1
						{
							tabs[activeTab].historypos--; //move the history back one
							std::string prevURL = tabs[activeTab].history[tabs[activeTab].historypos]; //grab the prev url
							urlInput = prevURL; //set the urlInput to the prevURL
							std::cout << prevURL; //DEBUG
							NavigateTo(prevURL, render, font, false); //Navigate to the new url
						}
					}
					//test if the fwdBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(fwdBtnRect)) {
						//first check, is our index var for the area <= 0? as we reference it
						if (tabs[activeTab].historypos < (int)tabs[activeTab].history.size() - 1)  //we do < (int)tabs[activeTab].history.size() - 1, as we want to go forward one, and we cannot go past the limit
						{
							tabs[activeTab].historypos++; //move the history forward one
							std::string nextURL = tabs[activeTab].history[tabs[activeTab].historypos]; //grab the next url
							urlInput = nextURL; //set the urlInput, to the nextURL
							currentURL = ""; //reset
							tabs[activeTab].url = "";  //reset
							NavigateTo(nextURL, render, font, false); //Navigate to the new url
						}
					}

					//test if the reloadBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(reloadBtnRect)) {
						if (currentURL != "") { NavigateTo(urlInput, render, font, false); } //don't reload for the mainpage, but just index the same url
					}

					//test if the homeBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(homeBtnRect)) {
						if (currentURL != "") { NavigateTo("new::tab", render, font, false); } //don't go if we are already on the mainpage, but load new::tab
					}

					//test if the printerBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(printerBtnRect)) {
						int uiHeight = 80; //crop the uiHeight
						SDL_Rect contentArea; //create a temp content area

						//x,w, h we leave, but, y we adjust
						contentArea.x = 0; 	contentArea.y = uiHeight; 	contentArea.w = WinW; 	contentArea.h = WinH - uiHeight; //we save the x, y, and width of the window, but we crop the h	
						SDL_Surface* screenshot = SDL_RenderReadPixels(render, &contentArea); //create a surface, holding the screenshot, taking the rect we want to capture

						if (screenshot != nullptr)
						{
							std::cout << "Printing..." << std::endl; //DEBUG
							std::string saveFolder = "Printed_Pages"; //create a temp string "Printed Pages"
							std::filesystem::create_directories(saveFolder); //create a new dir, called "Printed_Pages"

							std::string filenameStr = saveFolder + "\\Tab_print_" + std::to_string(SDL_GetTicks()) + ".bmp"; //create a file, in our folder, calling it Tab_print_ (then a time) .bmp
							const char* filename = filenameStr.c_str(); //create a char to prevent errors

							SDL_SaveBMP(screenshot, filename); 	//now we run the print command

							#ifdef _WIN32 //windows
								ShellExecuteA(NULL, "print", filename, NULL, NULL, SW_SHOWNORMAL); 	// Ask Windows to handle the printing, but SHOW the menu normally
							#else //linux
								std::string cmd = "lp \"" + std::string(filename) + "\""; //use the lp command, for CUPS (again)
								system(cmd.c_str()); //send it
							#endif
							SDL_DestroySurface(screenshot);//we are done here, destroy the old screenshot
						}
					}

					//test if the starBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (isHoverd(starBtnRect) && currentURL != "") { //we also make sure we have something in the currentURL, as we don't want to star newtab

						std::string currentSite = currentURL;	//first we get the current url
						auto it = std::find(starredPages.begin(), starredPages.end(), currentSite); //first, we should check our database to see if its alr starred

						if (it != starredPages.end()) //we found it, so we want to toggle it off
						{
							starredPages.erase(it); //remove the saved starredPage list
							std::cout << currentSite << " Removed from stars!" << std::endl; //DEBUG
						}
						else { //it was not found, so we want to toggle it on
							starredPages.push_back(currentSite); //save the saved starredPage list
							std::cout << currentSite << " Added to stars!" << std::endl; //DEBUG
						}

						std::ofstream bookmark("starred_pages.STAR", std::ios::trunc); //Remove the info inside starred_pages.STAR
						if (bookmark.is_open()) //if it opened correctly
						{
							for (const std::string& site : starredPages) { bookmark << site << "\n"; } //loop through each part in the list, and add it to the bookmark file
							//close clean
							bookmark.close(); //close the file
						}
						UpdateHTML(); //update the HTML with the new stuff, on the homepage
					}

					if (isHoverd(settingsBtnRect))
					{
						Tab settingsTab; //create a temp Tab handler to hold the settingsTab.
						settingsTab.title = "Loading..."; //give it a transition title...
						settingsTab.url = "settings::tab"; //set the url
						currentURL = "settings::tab";
						//push it back
						{
							std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
							tabs.push_back(settingsTab); //send a new tab to the list
							//update the active tab
							activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
							lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
						}
						NavigateTo("settings::tab", render, font, true); //go to the link, as opening a new tab, using the id
					}
					
					if (isLinkClicked(clickedURL, false)) //now that we know we clicked the link
					{
						urlInput = clickedURL; //set the urlInput to the clicked url
						NavigateTo(clickedURL, render, font, true); //Now we navigate to it
					}

					//check if the Y is in the general area of the tabs
					if (mouseY < 30) //instead of checking tabs every time (that's super slow lol) we just first check if its in the right spot
					{
						int tabX = 0; //pos of the first tab (X) (as we always have at least 1 tab)
						int tabW = 180; //pos of the first tab (Y) (as we always have at least 1 tab)

						for (int t = 0; t < tabs.size(); t++) //loop through each tab
						{
							//first check if the x button was clicked
							float closeX = tabX + tabW - 20;

						
							if (mouseX >= closeX && mouseX <= closeX + 14 && mouseY >= 2 && mouseY <= 28 && tabs.size() > 1) //check if the mouseX + mouseY overlap with the current tab's 'x'
							{
								std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values

								if (tabs[t].domRoot != nullptr) { DeleteTree(tabs[t].domRoot); } // if the domRoot contains something, we destory the tree, for cleanup
								tabs.erase(tabs.begin() + t); //remove the tab

								//make sure that that we cannot let the tabs go negative, if that were possible.
								if (activeTab >= (int)tabs.size()) { activeTab = tabs.size() - 1; }//if we don't meet the condition, we can close it.
								
								//if we end up closing, and have no active tabs, create a new one, to prevent a softlock
								if (tabs.empty()) { tabs.push_back(Tab()); activeTab = 0; }

								//update the url's to the tab, and fills in the info
								currentURL = tabs[activeTab].url;
								urlInput = tabs[activeTab].url;
								break; //we are done.
							}


							//check if the tab itself was clicked
							if (mouseX >= tabX && mouseX <= tabX + tabW)
							{

								{
									std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
									activeTab = t; //set the active tab, to the current tab we just pressed

									lastsearchedtabID = tabs[activeTab].tabID; //save the last search tab, to avoid overwriting it

									urlInput = tabs[activeTab].url; //then update the url input to the tab we pressed
								}

								ReloadTabLayout(); //update the thing
								break; //exit, we are done
							}
							tabX += tabW + 2; //move the collider box to the next tab
						}

					
						int newTabX = tabs.size() * (tabW + 2); //adjust the pos of the + depending on the amount of tabs.
						if (mouseX >= newTabX && mouseX <= newTabX + 26) //if the + button is pressed.
						{
							
							Tab newTab; //create a temp Tab handler to hold the newTab.
							newTab.title = "New Tab"; //give it a title
							//push it back
							{
								std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
								tabs.push_back(newTab); //send a new tab to the list
								//update the active tab
								activeTab = tabs.size() - 1; //set the activetab to the size of the tabs, -1 to fit the 0 = element 1
								lastsearchedtabID = tabs[activeTab].tabID;//set the tab when i click.
							}
							NavigateTo("new::tab", render, font, true); //go to the new tab, using the id
						}
					}
				}

				if (event.button.button == SDL_BUTTON_MIDDLE && tabs.size() > 1) //if we detect the middle button down, and we have more than 1 tab
				{
					std::cout << "Middle Button Pressed" << std::endl;
					int tabX = 0; //pos of the first tab (X) (as we always have at least 1 tab)
					int tabW = 180; //pos of the first tab (Y) (as we always have at least 1 tab)
					for (int t = 0; t < tabs.size(); t++) //loop through each tab
					{

						if (mouseY < 30) //instead of checking tabs every time (that's super slow lol) we just first check if its in the right spot
						{
							if (mouseX >= tabX && mouseX <= tabX + tabW)
							{
								std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values

								if (tabs[t].domRoot != nullptr) { DeleteTree(tabs[t].domRoot); } // if the domRoot contains something, we destory the tree, for cleanup
								tabs.erase(tabs.begin() + t); //remove the tab

								//make sure that that we cannot let the tabs go negative, if that were possible.
								if (activeTab >= (int)tabs.size()) { activeTab = tabs.size() - 1; }//if we don't meet the condition, we can close it.

								//if we end up closing, and have no active tabs, create a new one, to prevent a softlock
								if (tabs.empty()) { tabs.push_back(Tab()); activeTab = 0; }

								//update the url's to the tab, and fills in the info
								currentURL = tabs[activeTab].url;
								urlInput = tabs[activeTab].url;
								break; //we are done.
							}
						}
						tabX += tabW + 2;
					}
				}
			}
		}

		#pragma endregion

		//recalc layout
		if (RecalcLayout && SDL_GetTicks() - LastTickSinceRecalcLayout > 100)
		{
			float OldScrollPos = tabs[activeTab].scrollpos; //save the old scrollPos
			ReloadTabLayout(); //reload
			tabs[activeTab].scrollpos = OldScrollPos; //New scroll pos

			RecalcLayout = false;
		}


		// --- RENDERING --- \\

		PreRender(render, font); //load the textures, images, and fonts, and the style for them.

		#pragma region //Render Text and Images from websites

		//if (darkmode) //if the darkmode tag is flagged, and inverted colors
		//{
		//	SDL_SetRenderDrawColor(render, (255 - backgroundColor.r), (255 - backgroundColor.g), (255 - backgroundColor.b), 255); // reverse the colors of the site, as darkmode is a flipped colored version, or on starting, black
		//}
		
		SDL_SetRenderDrawColor(render, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255); // keep the colors normal, and set them to the colors of the site, or on starting, white
		
		SDL_RenderClear(render); //send it to the render, as our base color

		
		{ //create the lock area
			std::lock_guard<std::mutex> lock(gTabsMutex); //Lock the gTabsMutex to prevent any other threads from changing the values
			for (int i = 0; i < tabs[activeTab].layout.size(); i++) //loop through each tab''s layout and values in the list.
			{
				
				SDL_FRect textrec; //create a temp rect, to hold our text
				textrec.x = tabs[activeTab].layout[i].x; //give our textrec the layout.x part
				textrec.y = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos;//give our textrec the layout.y part - the scroll offset
				textrec.w = tabs[activeTab].layout[i].width; //give our textrec the layout.width part
				textrec.h = tabs[activeTab].layout[i].height; //give our textrec the layout.height part

				//draw the bg color
				if (tabs[activeTab].layout[i].hasBg)
				{
					
					if (darkmode) { SDL_SetRenderDrawColor(render, tabs[activeTab].layout[i].bgColor.r, tabs[activeTab].layout[i].bgColor.g, tabs[activeTab].layout[i].bgColor.b, 255); }
					else { SDL_SetRenderDrawColor(render, tabs[activeTab].layout[i].bgColor.r, tabs[activeTab].layout[i].bgColor.g, tabs[activeTab].layout[i].bgColor.b, 255); } //Not dark mode, rendering the text bg color normaly

					SDL_RenderFillRect(render, &textrec); //draw the BackGround of the text, and draw it over the rect of the text, but under the text, creating a bg
				}
			
				if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].imageTex != nullptr) //check if the element is a image, and the activeTab.imageTex contains something
				{
					//for now, we are gonna clamp images
					//if (textrec.w > 300)
					//{
					//	float scale = 300 / textrec.w; //make a scale thing, so we adjust right
					//	textrec.w = 300; //se the width to 300
					//	textrec.h = (int)(textrec.h * scale); //so that the scale is accurate
					//}

					SDL_RenderTexture(render, tabs[activeTab].layout[i].imageTex, nullptr, &textrec); //send the image texture to be rendered
				}
				
				else if (tabs[activeTab].layout[i].textTex != nullptr) //check if the element has text
				{
					SDL_RenderTexture(render, tabs[activeTab].layout[i].textTex, nullptr, &textrec); //send the text to be rendered
				}
			}
		}
		#pragma endregion

		SDL_GetCurrentRenderOutputSize(render, &WinW, &WinH); // grab the output size, and assign the WinW and WinH
	

		
		
		// --- SCROLL BAR BG --- \\

		float scrollBarWidth = 20.0f; //float to hold the width of the scroll bar
		float uiTopBarHeight = 69.0f;  //float to hold the start of the bar
		float trackHeight = WinH - uiTopBarHeight; //trackheight holds how long the bar should be, subtracting so that it ends on that start pos

		
		if (darkmode) { SDL_SetRenderDrawColor(render, 31, 31, 31, 255); }//if C is on, make the scroll bar BG a dark gray
		else { SDL_SetRenderDrawColor(render, 224, 224, 224, 255); } //if darkmode is off, make the scroll bar BG a light gray
			
		SDL_FRect scrollTrack = { WinW - scrollBarWidth, uiTopBarHeight, scrollBarWidth, trackHeight }; //create a rect, holding the pos and size of the bar, on screen
		SDL_RenderFillRect(render, &scrollTrack); //send it to be uploaded to the render.


		// --- SCROLL BAR --- \\
		
		//now we handle the size of the bar
		float barHeight = (trackHeight / (float)(tabs[activeTab].maxscroll + WinH)) * trackHeight; //the height of the bar, we take teh winH, the trackHight, and the max scroll!
		if (barHeight < 20.0f) barHeight = 20.0f; //we also make sure it cant get smaller than this, or it might disappear!
		if (barHeight > 50.0f) barHeight = 50.0f; //we also make sure it cant get bigger than this, or it looks dumb!

		//we calculate its pos out of 100, with the size and stuff, so that we get an accurate bar!
		float scrollPercentage = (float)tabs[activeTab].scrollpos / (float)tabs[activeTab].maxscroll; //calculate its pos out of the len of the bar, based on how far we are down the page
		float barY = uiTopBarHeight + (scrollPercentage * (trackHeight - barHeight)); //then using the scrollPercentage float, we use it to draw the bar accurately

		//color it
		if (darkmode) { SDL_SetRenderDrawColor(render, 63, 63, 63, 255); } //if darkmode is on, set the render color for the scrollbar to a dark gray
		else { SDL_SetRenderDrawColor(render, 192, 192, 192, 255); } //if darkmode is off,  set the render color for the scrollbar to a light gray
		
		SDL_FRect scrollbar = { WinW - scrollBarWidth, barY, scrollBarWidth, barHeight }; //create the rect, with our calculated pos.
		SDL_RenderFillRect(render, &scrollbar); //fill in the rect with our color

		//SCROLL BAR HIGHLIGHTS
		SDL_SetRenderDrawColor(render, 255, 255, 255, 180); //set to draw a white highlight
		SDL_RenderLine(render, scrollbar.x, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y); //draw a horizontal line across the top edge of the bar, using the color
		SDL_RenderLine(render, scrollbar.x, scrollbar.y, scrollbar.x, scrollbar.y + scrollbar.h);//draw a vertical line down the left edge of the bar, using the color

		//SCROLL BAR HIGHLIGHTS
		SDL_SetRenderDrawColor(render, 100, 100, 100, 255); //set to draw a dark gray highlight
		SDL_RenderLine(render, scrollbar.x, scrollbar.y + scrollbar.h, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h);  //draw a horizontal line across the bottom edge of the bar, using the color
		SDL_RenderLine(render, scrollbar.x + scrollbar.w, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h);  //draw a horizontal line across the right edge of the bar, using the color


		// --- OVERSCROLL PREVENTION --- \\
		
		int totalPageHeight = 0; //temp int to hold the total height of the page

		for (const auto& item : tabs[activeTab].layout) {  //loop through each layout item in the current active tab
		
			totalPageHeight = (std::max)(totalPageHeight, item.y + item.height); //we set our totalPageHeight to calculate the bottom edge of the current item, and store the larger value, to track the tallest overall content height

		}
		int viewableHeight = WinH - uiTopBarHeight; //now calculate the visable part of the page
		tabs[activeTab].maxscroll = (std::max)(0, totalPageHeight - viewableHeight + 100); //lock so we cant go over 0, and below the totalPageHeight - viewableHeight + 100px of padding

		// --- NAV BAR BG --- \\

		if (darkmode) { SDL_SetRenderDrawColor(render, 10, 10, 10, 255); } //if darkmode, we make it a gray almost black
		else { SDL_SetRenderDrawColor(render, 245, 245, 245, 255); } //if not darkmode we make it a very light gray, almost white
		SDL_FRect navBarBg = { 0, 30, (float)WinW, 39 }; //create a rect bg.
		SDL_RenderFillRect(render, &navBarBg); //render it with our bg


		// --- BUTTON TEXT COLOR --- \\

		SDL_Color textColor; //create a sdl color to hold the color of the text
		if (darkmode) { textColor = { 255, 255, 255, 255 }; } //if dark mode, we set the color to white
		else { textColor = { 0, 0, 0, 255 }; } //if not dark mode, we set the color to black


		// --- BACK+FORWARD BUTTON'S --- \\
		
				//BACK-BUTTON\\


		SDL_FRect backBtn; //create a rect to hold the backBtn

		if (tabs[activeTab].historypos > 0) //if we have the ability to go back...
		{
			backBtn.x = SDL_floorf(padding * scaleX) / scaleX; //set the x of the button
			backBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the button
			backBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the button
			backBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the button
			if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255); } //if darkmode is on, draw it as a dark gray
			else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); }//if darkmode is off, draw it as a light gray

			if (isHoverd(backBtn))
			{
				if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
				else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
			}
		}
		else { //if we don't have the ability to go back...
			backBtn.x = SDL_floorf(padding * scaleX) / scaleX; //set the x of the button
			backBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the button
			backBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the button
			backBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the button
			if (darkmode) { SDL_SetRenderDrawColor(render, 20, 20, 20, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 235, 235, 235, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &backBtn); //draw the backBtn, using the colors, and the rect, 

		

		SDL_Surface* backSurficon; //create a surf to hold the icon
		{ //guard
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
			backSurficon = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor); //set the icon to the 'ђ' that has been manipulated to be a arrow
		}
		if (backSurficon != nullptr) //if the backSurficon worked
		{
			SDL_Texture* backTex = SDL_CreateTextureFromSurface(render, backSurficon); //create a texture to hold the BackSurfIcon

			SDL_SetTextureScaleMode(backTex, SDL_SCALEMODE_NEAREST); //change the scale of the SDL_Texture, to prevent blur

			float backX = SDL_floorf((backBtn.x + (backBtn.w - (float)backSurficon->w) / 2.0f) * scaleX) / scaleX; //adjust them to the size of the screen, x
			float backY = SDL_floorf((backBtn.y + (backBtn.h - (float)backSurficon->h) / 2.0f) * scaleY) / scaleY; //adjust them to the size of the screen, y

			SDL_FRect backTexRect = { backX, backY, (float)backSurficon->w, (float)backSurficon->h }; //create a rectangle that sets the size of the ' ђ '
			SDL_RenderTexture(render, backTex, nullptr, &backTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(backTex); //we are done, destroy the texture
			SDL_DestroySurface(backSurficon); //we are done, destroy the surface
		}

				//FORWARD-BUTTON\\

		SDL_FRect fwdBtn; //create a rect to hold the fwdBtn

		if (tabs[activeTab].historypos < (int)tabs[activeTab].history.size() - 1) //insure the active pos, is less than the entire history - 1
		{
			fwdBtn.x = SDL_floorf((backBtn.x + backBtn.w + padding) * scaleX) / scaleX; //make the x of the button, using the backbuttn for offset
			fwdBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the button
			fwdBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the button
			fwdBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the button

			if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); } //if darkmode is off, draw it as a grayish white


			if (isHoverd(fwdBtn))
			{
				if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
				else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
			}

			SDL_RenderFillRect(render, &fwdBtn); //send the colors and bounds to the renderer to be queued for rendering
		}
		else { //if the forward button does NOT meet the conditions to be pressed
			fwdBtn.x = SDL_floorf((backBtn.x + backBtn.w + padding) * scaleX) / scaleX; //make the x of the button, using the backbuttn for offset
			fwdBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the button
			fwdBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the button
			fwdBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the button

			if (darkmode) { SDL_SetRenderDrawColor(render, 20, 20, 20, 255); } //if darkmode is on, draw it as a almost black
			else { SDL_SetRenderDrawColor(render, 235, 235, 235, 255); } //if darkmode is off, draw it as a almost white
			SDL_RenderFillRect(render, &fwdBtn); //send the colors and bounds to the renderer to be queued for rendering
		}

		//FORWARD ARROW
		SDL_Surface* forwardSurf;   //create a surf to hold the icon
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);  //create a guard to prevent other threads editing the TTF list, and causing a crash.
			forwardSurf = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor);  //set the icon to the 'ђ' that has been manipulated to be a arrow
		}
		if (forwardSurf != nullptr) //if the backSurficon worked
		{
			SDL_FlipSurface(forwardSurf, SDL_FLIP_HORIZONTAL); //to prevent pixel issues, we flip the arrow, and this allows us to have the same pixel look, but only use one sprite on the sheet
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, forwardSurf); //create a temp texture, to hold the arrow, passing in our font, and the flip

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur

			float fwdX = SDL_floorf((fwdBtn.x + (fwdBtn.w - (float)forwardSurf->w) / 2.0f) * scaleX) / scaleX; //adjust them to the size of the screen, x
			float fwdY = SDL_floorf((fwdBtn.y + (fwdBtn.h - (float)forwardSurf->h) / 2.0f) * scaleY) / scaleY; //adjust them to the size of the screen, y

			SDL_FRect fwdTexRect = { fwdX, fwdY, (float)forwardSurf->w, (float)forwardSurf->h }; //create a rectangle that sets the size of the ' ђ '
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect); //send the texture to the render to be rendered

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex); //we are done, destroy the texture
			SDL_DestroySurface(forwardSurf); //we are done, destroy the surface
		}

		// --- RELOAD BUTTON --- \\

		SDL_FRect reloadButton; //create a rect to hold the reload button

		reloadButton.x = fwdBtn.x + fwdBtn.w + padding; //set the X pos of the reload button offsetting off the fwdBtn.x
		reloadButton.y = topMargin; //set the pos of the y
		reloadButton.w = btnSize; //set the pos of the w
		reloadButton.h = btnSize; //set the pos of the h
		if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255); } //if darkmode is on, draw it as a darker gray
		else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); } //if darkmode is off, draw it as a grayish white

		if (isHoverd(reloadButton))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &reloadButton); //send the rect to the render to be queued for rendering

		SDL_Surface* reloadSurf; //create a serf to hold the text icon
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);  //create a guard to prevent other threads editing the TTF list, and causing a crash.
			reloadSurf = TTF_RenderText_Solid(iconFont, "њ", 0, textColor);  //set the icon to the 'њ' that has been manipulated to be a reload arrow
		}

		if (reloadSurf != nullptr) //if the reloadSurf worked
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, reloadSurf); //create a temp texture, to hold the reload arrow, passing in our font

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur

			SDL_FRect reloadTexRect = { reloadButton.x + 3, reloadButton.y - 4, (float)reloadSurf->w, (float)reloadSurf->h }; //create a rectangle that sets the size of the ' њ '
			SDL_RenderTexture(render, forwardTex, nullptr, &reloadTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(forwardTex); //we are done, destroy the texture
			SDL_DestroySurface(reloadSurf); //we are done, destroy the surface
		}

		// --- HOME BUTTON --- \\

		SDL_FRect homeBtn; //create a rect to hold the home button
		
		homeBtn.x = reloadButton.x + reloadButton.w + padding; //set the X pos of the reload button offsetting off the reloadButton.x
		homeBtn.y = topMargin; //set the pos of the y
		homeBtn.w = btnSize; //set the pos of the w
		homeBtn.h = btnSize; //set the pos of the h


		if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255); } //if darkmode is on, draw it as a darker gray
		else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); } //if darkmode is off, draw it as a grayish white

		if (isHoverd(homeBtn))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &homeBtn); //send the rect to the render to be queued for rendering
		
		
		SDL_Surface* homeSurf; //create a serf to hold the home icon
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
			homeSurf = TTF_RenderText_Blended(iconFont, "љ", 0, textColor); //set the icon to the 'љ' that has been manipulated to be a home icon
		}

		if (homeSurf != nullptr) //if the homeSurf worked
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, homeSurf); //create a temp texture, to hold the home icon , passing in our font

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur

			SDL_FRect fwdTexRect = { homeBtn.x + 2, homeBtn.y - 2, (float)homeSurf->w, (float)homeSurf->h }; //create a rectangle that sets the size of the ' љ '
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(forwardTex); //we are done, destroy the texture
			SDL_DestroySurface(homeSurf); //we are done, destroy the surface
		}

		// --- INPUT BOX --- \\
		
		float searchX = homeBtn.x + homeBtn.w + padding; //create the start x, taking in the homeBtn for offset
		float searchW = (float)WinW - btnSize -  searchX - ((btnSize) * 3) - ((padding)*3); //create the width, the window - padding*2 for 2 buttons - the starting x, and - the 2 buttons

		SDL_FRect bar = { //hold the dimensions of the input bar
			SDL_floorf(searchX * scaleX) / scaleX, //set the x of the bar
			SDL_floorf(topMargin * scaleY) / scaleY, //set the y of the bar
			SDL_floorf(searchW * scaleX) / scaleX, //set the w of the bar
			SDL_floorf(btnSize * scaleY) / scaleY //set the h of the bar
		};
		if (darkmode) { SDL_SetRenderDrawColor(render, 15, 15, 15, 255); } //if darkmode is on, draw it as a almost black
		else { SDL_SetRenderDrawColor(render, 240, 240, 240, 255);} //if darkmode is off, draw it as a almost white
		
		if (isHoverd(bar))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 20, 20, 20, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 235, 235, 235, 255); }//if darkmode is off, draw it as a lighter gray
		}
		
		//fill the rec with this
		SDL_RenderFillRect(render, &bar); //send the rect to the render to be queued for rendering

		TTF_SetFontSize(font, 17); //set text size
		std::string displayText = urlInput; //hold the display text, need a second string, to display the ы Enter a url...
		SDL_Color color; //holds the color of the input text
		SDL_Surface* urlSurf; //holds the surf of the text
		
		if (urlInput.empty() && inputbarfocused)
		{
			displayText = " "; //create a blank space
		}
		//display ghost text, with a different color, if nothing typed
		if (urlInput.empty() && !inputbarfocused) //do a check if i've typed anything, if not..
		{
			displayText = "ы Click and type a url..."; //display some ghost text
			if (darkmode) { color = { 75, 75, 75, 255 }; } //if darkmode is off, set it to a dark gray	
			else{ color = { 180, 180, 180, 255 }; } //if darkmode is off, set it to a light-ish gray	
		}
		else { //if i've typed anything...
			if (darkmode) { color = { 255,255,255,255 }; } //if darkmode is on, set the color to white
			else { color = { 0,0,0,255 }; } //if darkmode is off, set the color to black
		}

		{ //LOCK
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
			urlSurf = TTF_RenderText_Solid(font, displayText.c_str(), 0, color); //create a surf, to hold the text, and that renders it.
		}

		if (inputbarfocused)
		{
			std::vector<std::string> suggestionsList; //hold the suggestions
			std::string typed = urlInput;

			std::transform(typed.begin(), typed.end(), typed.begin(), [](unsigned char c) {
				return std::tolower(c);
				}); //convert to lower

			if (!typed.empty()) //make sure somethin is typed.
			{
				for (const auto& history : GlobalHistory) //for each val in GlobalHistory
				{
					std::string url = history.second; std::string lowerUrl = url; //grab the url, then make another var to get it lower.
					std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), [](unsigned char c) {
						return std::tolower(c); }); //convert to lower

					if (lowerUrl.starts_with(typed)) //if the url starts with the typed
					{
						suggestionsList.push_back(history.second); //push the url back if it matches with the typed stuff

						if (suggestionsList.size() >= 5) //make sure we dont go over.
							break;
					}
				}

				//now that we found things that START with it, lets handle if they contain it

				if (suggestionsList.size() < 5)
				{
					for (const auto& history : GlobalHistory) //for each val in GlobalHistory
					{
						std::string url = history.second; std::string lowerUrl = url; //grab the url, then make another var to get it lower.

						std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), [](unsigned char c) {
							return std::tolower(c); }); //convert to lower

						if (lowerUrl.find(typed) != std::string::npos && !lowerUrl.starts_with(typed)) //if the url starts with the typed
						{
							suggestionsList.push_back(history.second); //push the url back if it matches with the typed stuff

							if (suggestionsList.size() >= 5) //make sure we dont go over.
								break;
						}
					}
				}
			}

			//first, grab the size of the history
			int suggestions = static_cast<int>(suggestionsList.size()); //min 5 results
	
			SDL_FRect menuboarder = { bar.x, bar.y + 30, bar.w, bar.h * suggestions }; //calculate the menu boarder
			
			if (darkmode) { SDL_SetRenderDrawColor(render, 0, 0, 0, 90); } //set the theme, if darkmode, black clear
			else { SDL_SetRenderDrawColor(render, 100, 100, 100, 90); } //set theme, if not darmode, light clear

			SDL_FRect shadow = {
					menuboarder.x + 5,
					menuboarder.y + 5,
					menuboarder.w,
					menuboarder.h,
			};

			if (suggestions > 0)
			{
				SDL_RenderFillRect(render, &shadow);
			}
			
			
			SDL_FRect ContextMenuRect;
			

			//render the bar, off the size of the suggestions
			for (int i = 0; i < suggestions; i++)
			{
				
				ContextMenuRect = { bar.x, (bar.y + 30) + (bar.h * i), bar.w, bar.h }; //render the context menu

				if (isHoverd(ContextMenuRect)) //if i hover over it
				{
					if (darkmode) { SDL_SetRenderDrawColor(render, 52, 52, 52, 255); } //if darkmode is on, draw it as a lightish gray
					else { SDL_SetRenderDrawColor(render, 230, 230, 230, 255); } //if darkmode is off, draw it as a almost white
				}
				else {
					if (darkmode) { SDL_SetRenderDrawColor(render, 42, 42, 42, 255); } //if darkmode is on, draw it as a lightish gray
					else { SDL_SetRenderDrawColor(render, 240, 240, 240, 255); } //if darkmode is off, draw it as a almost white
				}
					

				SDL_RenderFillRect(render, &ContextMenuRect); //send it to be uploaded to the render.

				if (i < suggestions - 1)
				{
					if (darkmode) { SDL_SetRenderDrawColor(render, 58, 58, 58, 255); }
					else { SDL_SetRenderDrawColor(render, 100, 100, 100, 255); }

					SDL_RenderLine(
						render,
						ContextMenuRect.x + 8,
						ContextMenuRect.y + ContextMenuRect.h - 1,
						ContextMenuRect.x + ContextMenuRect.w - 8,
						ContextMenuRect.y + ContextMenuRect.h - 1
					);
				}	


				//render the text now.

				if (darkmode) { SDL_Color color = { 255, 255, 255, 255 }; }
				else { SDL_Color color = { 0, 0, 0, 255 }; }
				SDL_Surface* buttnSerf = TTF_RenderText_Solid(font, suggestionsList[i].c_str(), 0, color);


				if (buttnSerf != nullptr) //if our xSurf has been created 
				{
					float textX = SDL_roundf(ContextMenuRect.x + 10);
					float textY = SDL_roundf(ContextMenuRect.y + (ContextMenuRect.h - (float)buttnSerf->h) / 2.0);

					SDL_Texture* xTex = SDL_CreateTextureFromSurface(render, buttnSerf); //create a temp texture to hold the 'x' text
					SDL_FRect xRect = { textX, textY, (float)buttnSerf->w, (float)buttnSerf->h }; //move the rect to fit in the tab

					SDL_RenderTexture(render, xTex, nullptr, &xRect); //send the texture to the render to be rendered

					SDL_DestroyTexture(xTex); //we are done, destroy the texture
					
				}
				SDL_DestroySurface(buttnSerf); //we are done, destroy the surface
				

			}	

			if (darkmode) { SDL_SetRenderDrawColor(render, 58, 58, 58, 255); }
			else { SDL_SetRenderDrawColor(render, 100, 100, 100, 255); }

			int thickness = 3;

			if (suggestions > 0)
			{
				for (int i = 0; i < thickness; i++)
				{
					SDL_FRect border = {
						menuboarder.x + i,
						menuboarder.y + i,
						menuboarder.w - (i * 2),
						menuboarder.h - (i * 2),
					};

					SDL_RenderRect(render, &border);
				}
			}
			

		}

		//std::cout << urlSurf->w << ":" << bar.w << std::endl; //DEBUG
		if (urlSurf != nullptr && urlSurf->w > bar.w - 16) //if its greater than its max width
		{
			urlInput.pop_back();
			displayText = urlInput;

			color = { 255,0,0,255 }; //red warn debug color thing

			//rm the old texture
			SDL_DestroySurface(urlSurf);

			{ //LOCK
				std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
				urlSurf = TTF_RenderText_Solid(font, displayText.c_str(), 0, color); //rerender
			}
		}

		// --- INPUT CURSOR --- \\

		if (urlSurf != nullptr) //make sure the urlSurf is created correctly
		{
			SDL_Texture* urlTexture = SDL_CreateTextureFromSurface(render, urlSurf); //create a temp texture, to hold the url link.

			SDL_SetTextureScaleMode(urlTexture, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur
			SDL_FRect urlTextureRect = { bar.x + 8, bar.y + 6, (float)urlSurf->w, (float)urlSurf->h }; //create a temp rect to hold our bar
			SDL_RenderTexture(render, urlTexture, nullptr, &urlTextureRect); //send the bar to the render to be rendered

			SDL_DestroyTexture(urlTexture); //we are done with it, destroy!

			//draw the cursor
			//doing SDL_GETTICKS() returns milliseconds since the app started
			//dividing this by 500 gives us increments every half a tick, that flip between t or f
			//this gives a blinking effect without a timer
			bool ShowCursor = (SDL_GetTicks() / 500) % 2 == 0;
			if (ShowCursor && inputbarfocused) //if the url contains something, and the cursor should be shown
			{
				float cursorX = bar.x + 8 + urlSurf->w + 1 + adjusted; //set the x of the cursor to the last line in the text, with the adjusted
				if (darkmode) { SDL_SetRenderDrawColor(render, 255, 255, 255, 255); } //if darkmode, set it to white
				else { SDL_SetRenderDrawColor(render, 0, 0, 0, 255); } //if not darkmode, set it to black

				SDL_RenderLine(render, cursorX, bar.y + 5, cursorX, bar.y + 23); //render the line with the x, and colors
			}
			SDL_DestroySurface(urlSurf); //now destroy the surf
		}

		// --- STAR BUTTON --- \\

		SDL_FRect starBtn; //create a rect to hold the star button
		starBtn.x = SDL_floorf((bar.x + bar.w + padding) * scaleX) / scaleX; //set the end X pos of the bar offsetting off the starBtn.x
		starBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the bar
		starBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the bar
		starBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the bar

		bool isStarred = (std::find(starredPages.begin(), starredPages.end(), currentURL) != starredPages.end()); //loop through the starredPages list, and check if it = the currentURL
		
		if (isStarred) //if the webpage has been starred
		{
			if (darkmode){ SDL_SetRenderDrawColor(render, 255, 255, 0, 255); } //If darkmode is on, set the color to blue, as its an invert of what it is for lightmode
			else { SDL_SetRenderDrawColor(render, 255, 255, 0, 255); } //If darkmode is off, set the color to yellow, as its an invert of what it is for darkmode
		}
		else { //if it has not
			if (tabs[activeTab].title != "New Tab") { //if its NOT a new tab, we want to show the user that its clickable
				if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255); } //set it to a lighter shade of black, if darkmode
				else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); }  //set it to a darker shade of white, if lightmode

				if (isHoverd(starBtn))
				{
					if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
					else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
				}
			}
			else { //if its IS a new tab, we want to show the user its NOT clickable
				if (darkmode) { SDL_SetRenderDrawColor(render, 20, 20, 20, 255); } //set it to a darker shade of black, if darkmode
				else { SDL_SetRenderDrawColor(render, 235, 235, 235, 255); } //set it to a lighter shade of white, if lightmode
			}
		}
		SDL_RenderFillRect(render, &starBtn); //send the rect to the render to be queued for rendering

		SDL_Surface* starSurf; //create a serf to hold the home icon
		{ //LOCK
			SDL_Color custom;
			if (darkmode && isStarred) {
				custom = { 20, 20, 20, 255 };
				starSurf = TTF_RenderText_Solid(iconFont, "ж", 0, custom); //set the icon to the 'ж' that has been manipulated to be a star icon
			}
			else {
				starSurf = TTF_RenderText_Solid(iconFont, "ж", 0, textColor); //set the icon to the 'ж' that has been manipulated to be a star icon
			}
			
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
		}

		if (starSurf != nullptr) //if the icon was created successfully
		{
			SDL_Texture* starTex = SDL_CreateTextureFromSurface(render, starSurf); //create a temp texture, to hold the star icon.
			SDL_SetTextureScaleMode(starTex, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur
			float starX = SDL_floorf((starBtn.x + (starBtn.w - (float)starSurf->w) / 2.0f) * scaleX) / scaleX; //adjust them to the size of the screen, x
			float starY = SDL_floorf((starBtn.y + (starBtn.h - (float)starSurf->h) / 2.0f) * scaleY) / scaleY; //adjust them to the size of the screen, y

			SDL_FRect backTexRect = { starX, starY, (float)starSurf->w, (float)starSurf->h }; //create a rect to make the button
			SDL_RenderTexture(render, starTex, nullptr, &backTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(starTex); //we are done, destroy the texture
			SDL_DestroySurface(starSurf); //we are done, destroy the surface
		}

		// --- PRINTER BUTTON --- \\

		SDL_FRect printerBtn; //create a rect to hold the printer button
		printerBtn.x = SDL_floorf((bar.x + bar.w + padding + btnSize + padding) * scaleX) / scaleX; //set the end X pos of the bar offsetting off the printerBtn.x
		printerBtn.y = SDL_floorf(topMargin * scaleY) / scaleY; //set the y of the bar
		printerBtn.w = SDL_floorf(btnSize * scaleX) / scaleX; //set the w of the bar
		printerBtn.h = SDL_floorf(btnSize * scaleY) / scaleY; //set the h of the bar

		if (darkmode) { SDL_SetRenderDrawColor(render, 55, 55, 55, 255); } //set it to a lighter shade of black, if darkmode
		else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); }  //set it to a darker shade of white, if lightmode

		if (isHoverd(printerBtn))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &printerBtn); //send the rect to the render to be queued for rendering

		SDL_Surface* printerSurf; //create a serf to hold the printer icon
		{ //LOCK
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
			printerSurf = TTF_RenderText_Solid(iconFont, "ξ", 0, textColor); //set the icon to the 'ξ' that has been manipulated to be a printer icon
		}
		
		if (printerSurf != nullptr) //if the icon was created successfully
		{
			SDL_Texture* printerTex = SDL_CreateTextureFromSurface(render, printerSurf);
			SDL_SetTextureScaleMode(printerTex, SDL_SCALEMODE_NEAREST);
			float printerX = SDL_floorf((printerBtn.x + (printerBtn.w - (float)printerSurf->w) / 2.0f) * scaleX) / scaleX; //adjust them to the size of the screen, x
			float printerY = SDL_floorf((printerBtn.y + (printerBtn.h - (float)printerSurf->h) / 2.0f) * scaleY) / scaleY; //adjust them to the size of the screen, y

			SDL_FRect backTexRect = { printerX, printerY, (float)printerSurf->w, (float)printerSurf->h }; //create a rect to make the button
			SDL_RenderTexture(render, printerTex, nullptr, &backTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(printerTex); //we are done, destroy the texture
			SDL_DestroySurface(printerSurf); //we are done, destroy the surface
		}

		// --- SETTINGS BUTTON --- \\

		SDL_FRect settingsBtn; //create a rect to hold the settings button

		settingsBtn.x = printerBtn.x + printerBtn.w + padding; //set the X pos of the reload button offsetting off the printerBtn.x
		settingsBtn.y = topMargin; //set the pos of the y
		settingsBtn.w = btnSize; //set the pos of the w
		settingsBtn.h = btnSize; //set the pos of the h
		if (darkmode) { SDL_SetRenderDrawColor(render, 55, 55, 55, 255); } //if darkmode is on, draw it as a darker gray
		else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); } //if darkmode is off, draw it as a grayish white

		if (isHoverd(settingsBtn))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 69, 69, 69, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 185, 185, 185, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &settingsBtn); //send the rect to the render to be queued for rendering

		static float oldSizeTemp = TTF_GetFontSize(iconFont); TTF_SetFontSize(iconFont, 24); //change the font size to adjust the settings icon

		TTF_SetFontHinting(iconFont, TTF_HINTING_LIGHT_SUBPIXEL); //force an update to prevent blur, that resizes the font
		SDL_Surface* settingsSurf; //create a serf to hold the settings icon
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
			settingsSurf = TTF_RenderText_Blended(iconFont, "Ħ", 0, textColor); //set the icon to the 'Ħ' that has been manipulated to be a settings icon
		}

		TTF_SetFontSize(iconFont, oldSizeTemp); //change it back to before
		TTF_SetFontHinting(iconFont, TTF_HINTING_LIGHT_SUBPIXEL); //force an update to prevent blur, that resizes the font

		if (settingsSurf != nullptr) //if the settingsSurf worked
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, settingsSurf); //create a temp texture, to hold the settings icon , passing in our font

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST); //force it to pixel art to prevent blur
			SDL_FRect fwdTexRect = { settingsBtn.x + 3, settingsBtn.y - 1, (float)settingsSurf->w, (float)settingsSurf->h }; //create a rectangle that sets the size of the ' љ '
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(forwardTex); //we are done, destroy the texture
			SDL_DestroySurface(homeSurf); //we are done, destroy the surface
		}

		// --- TABS --- \\

		if (darkmode) { SDL_SetRenderDrawColor(render, 45, 45, 45, 255); } //if darkmode is enabled, set the tab bg color to a dark gray
		else { SDL_SetRenderDrawColor(render, 210, 210, 210, 255); } //if darkmode is disabled, set the tab bg color to a light gray
		
		SDL_FRect tabBarBg = { 0, 0, (float)WinW, 30 }; //create a rect to hold the color, of the bg of the tabs
		SDL_RenderFillRect(render, &tabBarBg);  //send the rect to the render to be queued for rendering

		//DRAW EACH TAB
		int tabX = 0; //temp var to hold the X of each tab
		for (int t = 0; t < tabs.size(); t++)
		{
			int tabW = 180; //set the width of each tab

			SDL_FRect TabRect = { (float)tabX, 0, (float)tabW, 30 }; //make one tab rect, with a height of 30

			//change the color based on the tab (if its active or not)
			if (t == activeTab)
			{
				if (darkmode) { SDL_SetRenderDrawColor(render, 10, 10, 10, 255); } //set it to a almost black, if darkmode
				else { SDL_SetRenderDrawColor(render, 245, 245, 245, 255); } //set it to a almost white, if lightmode
			}
				
			else //if its not active
			{
				if (darkmode) { SDL_SetRenderDrawColor(render, 65, 65, 65, 255); } //set the taskbar color to a grayish black, if darkmode
				else { SDL_SetRenderDrawColor(render, 190, 190, 190, 255); }  //set the taskbar color to a light gray, if lightmode
			}
		
			SDL_RenderFillRect(render, &TabRect); //send the rect to the render to be queued for rendering

			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a lock for the tab, this prevents crashes by preventing other functions with this lock, from changing the data preventing 2 things writing 1 data
		
			TTF_SetFontSize(font, 13); 	//ok now lets draw the tab text (gonna be the <title> text)
			
			SDL_Color tabTextColor; //create a var to hold the RGB color
			if (darkmode) { tabTextColor = { 255,255,255,255 }; } //if darkmode is enabled, set the text color to white
			else { tabTextColor = { 0,0,0,255 }; } //if darkmode is disabled, set the text color to black 
				
			
			std::string tabTitle = tabs[t].title; //temp string to hold the title

			//ok, now lets trim the long titles, as they are gonna overspill
			if (tabTitle.size() > 20) tabTitle = tabTitle.substr(0, 17) + "..."; //replace the overspill (greater than 17 chars) with '...'

			
			SDL_Surface* tabSurf = TTF_RenderText_Solid(font, tabTitle.c_str(), 0, tabTextColor); //create a serf to hold the text, and send it to be rendered
			if (tabSurf != nullptr) //if our tabSurf has been created 
			{
				SDL_Texture* tabTex = SDL_CreateTextureFromSurface(render, tabSurf); //create a temp texture to hold the tab text
				SDL_SetTextureScaleMode(tabTex, SDL_SCALEMODE_NEAREST);
				SDL_FRect tabTextRect = { (float)tabX + 8, 6, (float)tabSurf->w, (float)tabSurf->h }; //move the rect to fit in the tab
				SDL_RenderTexture(render, tabTex, nullptr, &tabTextRect); //send the texture to the render to be rendered
				
				SDL_DestroyTexture(tabTex); //we are done, destroy the texture
				SDL_DestroySurface(tabSurf); //we are done, destroy the surface
			}

			SDL_Surface* xSurf; //build a surf to hold the 'x' for closing a tab
			if (tabs.size() > 1) { xSurf = TTF_RenderText_Solid(font, "x", 0, tabTextColor); } //if we have more than one tab, we show the x
			else {xSurf = TTF_RenderText_Solid(font, "", 0, tabTextColor); } //if we do not have more than one tab (just have one), we hide the x
			
			if (xSurf != nullptr) //if our xSurf has been created 
			{
				SDL_Texture* xTex = SDL_CreateTextureFromSurface(render, xSurf); //create a temp texture to hold the 'x' text
				SDL_SetTextureScaleMode(xTex, SDL_SCALEMODE_NEAREST);
				SDL_FRect xRect = { (float)(tabX + tabW - 20), 7, (float)xSurf->w, (float)xSurf->h }; //move the rect to fit in the tab
				SDL_RenderTexture(render, xTex, nullptr, &xRect); //send the texture to the render to be rendered

				SDL_DestroyTexture(xTex); //we are done, destroy the texture
				SDL_DestroySurface(xSurf); //we are done, destroy the surface
			}	
			tabX += tabW + 2; //increase the gap between the next tab, and start the next tab
		}

		//draw the + button for creating new tabs
		if (darkmode) { SDL_SetRenderDrawColor(render, 75, 75, 75, 255); } //if darkmode is on, draw the color as a dark gray
		else { SDL_SetRenderDrawColor(render, 180, 180, 180, 255); } //if darkmode is off, draw the color as a light gray
		
		SDL_FRect newTabBtn = { (float)tabX, 2, 26, 26 }; //make rect to hold the image of the button

		if (isHoverd(newTabBtn))
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 80, 80, 80, 255); } //if darkmode is on, draw it as a darker gray
			else { SDL_SetRenderDrawColor(render, 175, 175, 175, 255); }//if darkmode is off, draw it as a lighter gray
		}

		SDL_RenderFillRect(render, &newTabBtn); //send it to be rendered

		SDL_Surface* plusSurf; //create a surf to hold the '+' of the + button

		{ //LOCK
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a lock for the tab, this prevents crashes by preventing other functions with this lock, from changing the data preventing 2 things writing 1 data
			if (darkmode) { plusSurf = TTF_RenderText_Solid(font, "+", 0, { 255,255,255,255 }); } //if darkmode, render the '+' white
			else { plusSurf = TTF_RenderText_Solid(font, "+", 0, { 0,0,0,255 }); } //if NOT darkmode, render the '+' black
		}

		if (plusSurf != nullptr) //if our plusSurf has been created 
		{
			SDL_Texture* plusTex = SDL_CreateTextureFromSurface(render, plusSurf); //create a temp texture to hold the '+' text
			SDL_FRect xRect = { (float)(tabX + 8),  7, (float)plusSurf->w, (float)plusSurf->h }; //move the rect to fit in the tab
			SDL_RenderTexture(render, plusTex, nullptr, &xRect); //send the texture to the render to be rendered

			SDL_DestroyTexture(plusTex); //we are done, destroy the texture
			SDL_DestroySurface(plusSurf); //we are done, destroy the surface
		}

		//render the Context menu
		if (ContextMenu)
		{
			if (darkmode) { SDL_SetRenderDrawColor(render, 31, 31, 31, 255); }//if darkmode is on, make the ContextMenu a dark gray
			else { SDL_SetRenderDrawColor(render, 224, 224, 224, 255); } //if darkmode is off, make the ContextMenu a light gray
			SDL_FRect ContextMenuRect;
			if (SaveXYContextMenuPos) {

				if (ContextMenuYPos >= 30) //Top bar being pressed 
				{
					ContextMenuButtons = { "New Tab", "Print", "Change Theme", "-------", "Back", "Forward", "-------", }; //Holds the names of the buttons, and the engine will fit them.
				}
				else {
					ContextMenuButtons = { "Reload", "Close", "-------", "History", "Close All Tabs", "Performance", "-------", }; //Holds the names of the buttons, and the engine will fit them.
				}
				if (isLinkClicked(clickedURL, true))
				{
					ContextMenuButtons = { "Open", "Open New Tab", "In New Window", "-------", "Copy Link", "Save Link As", "-------", }; //Holds the names of the buttons, and the engine will fit them.
				}

				if (WinH / 2 > mouseY) //if you click on the lower half
				{
					if (WinW / 2 > mouseX) //that means we are on the right side
					{
						ContextMenuXPos = mouseX; ContextMenuYPos = mouseY; //set the temp x and y
					}
					else {
						ContextMenuXPos = mouseX - 200; ContextMenuYPos = mouseY; //set the temp x and y with an offset to the X
					}
				}
				else { //if you click on the upper half
					if (WinW / 2 > mouseX) //that means we are on the right side
					{
						ContextMenuXPos = mouseX; ContextMenuYPos = mouseY - 315; //set the temp x and y with an offset to the Y
					}
					else { //check if we are on the left side
						ContextMenuXPos = mouseX - 200; ContextMenuYPos = mouseY - 315; //set the temp x and y with an offset to the Y and X
					}
				}
				SaveXYContextMenuPos = false; //set it to false, to not update again
			}
			else { //RENDER IT
				ContextMenuRect = { ContextMenuXPos, ContextMenuYPos, 200, 315 }; //render the context menu

				SDL_RenderFillRect(render, &ContextMenuRect); //send it to be uploaded to the render.

				//-- RENDER EACH BUTTON --\\

				float buttonHeight = 45.0;
				
				TTF_SetFontHinting(iconFont, TTF_HINTING_NORMAL); // Options: TTF_HINTING_NORMAL, TTF_HINTING_LIGHT, or TTF_HINTING_MONO
				SDL_FRect CurrentButtnRect;
				
				//ContextMenuRect.w for our button width, maybe with some padding
				
				if (darkmode) { SDL_SetRenderDrawColor(render, 0, 0, 0, 90); }
				else { SDL_SetRenderDrawColor(render, 100, 100, 100, 90); }
				SDL_FRect shadow = {
						ContextMenuRect.x + 5,
						ContextMenuRect.y + 5,
						ContextMenuRect.w,
						ContextMenuRect.h,
				};

				SDL_RenderFillRect(render, &shadow);

				for (int i = 0; i < ContextMenuButtons.size(); i++)
				{
					//now we want to render each button, like how we render tabs.
					float buttnX = ContextMenuRect.x; //hold the x of it
					float buttnY = ContextMenuRect.y + (i * (buttonHeight + 0)); //change the x, based on the size of the button * the i, with some sort of padding
					CurrentButtnRect = { buttnX, buttnY, ContextMenuRect.w, buttonHeight };
					
					if (mouseX >= CurrentButtnRect.x && mouseX <= (CurrentButtnRect.x + CurrentButtnRect.w) &&
						mouseY >= CurrentButtnRect.y && mouseY <= (CurrentButtnRect.y + CurrentButtnRect.h) && ContextMenuButtons[i] != "-------") {
						if (darkmode) { SDL_SetRenderDrawColor(render, 54, 54, 54, 255);  } //if darkmode is on, make the ContextMenu a dark gray
						else { SDL_SetRenderDrawColor(render, 221, 221, 221, 255); } //if darkmode is off, make the ContextMenu a light gray
					}
					else {
						if (darkmode) { SDL_SetRenderDrawColor(render, 31, 31, 31, 255); } 
						else { SDL_SetRenderDrawColor(render, 200, 200, 200, 255); }
					}

					SDL_RenderFillRect(render, &CurrentButtnRect); //send it to be uploaded to the render.

					SDL_Color buttnTextColor; //create a var to hold the RGB color
					if (darkmode) { buttnTextColor = { 255,255,255,255 }; } //if darkmode is enabled, set the text color to white
					else { buttnTextColor = { 0,0,0,255 }; } //if darkmode is disabled, set the text color to black 


					//now lets render the text
					SDL_Surface* buttnSerf; //build a surf to hold the 'x' for closing a tab
					if (ContextMenuButtons[i] == "-------") //if we find the ---- placeholder, draw a line
					{
						if (darkmode) { SDL_SetRenderDrawColor(render, 58, 58, 58, 255); }
						else { SDL_SetRenderDrawColor(render, 100, 100, 100, 255); }

						SDL_RenderLine(
							render,
							CurrentButtnRect.x + 8,
							CurrentButtnRect.y + CurrentButtnRect.h / 2,
							CurrentButtnRect.x + CurrentButtnRect.w - 8,
							CurrentButtnRect.y + CurrentButtnRect.h / 2
						);
					}
					else {
						buttnSerf = TTF_RenderText_Solid(iconFont, ContextMenuButtons[i].c_str(), 0, buttnTextColor);


						if (buttnSerf != nullptr) //if our xSurf has been created 
						{
							float textX = SDL_roundf(CurrentButtnRect.x + (CurrentButtnRect.w - (float)buttnSerf->w) / 2.0);
							float textY = SDL_roundf(CurrentButtnRect.y + (CurrentButtnRect.h - (float)buttnSerf->h) / 2.0);

							SDL_Texture* xTex = SDL_CreateTextureFromSurface(render, buttnSerf); //create a temp texture to hold the 'x' text
							SDL_FRect xRect = { textX, textY, (float)buttnSerf->w, (float)buttnSerf->h }; //move the rect to fit in the tab
							SDL_RenderTexture(render, xTex, nullptr, &xRect); //send the texture to the render to be rendered

							SDL_DestroyTexture(xTex); //we are done, destroy the texture
							SDL_DestroySurface(buttnSerf); //we are done, destroy the surface
						}
					}
				}
				
				if (darkmode) { SDL_SetRenderDrawColor(render, 58, 58, 58, 255); }
				else { SDL_SetRenderDrawColor(render, 100, 100, 100, 255); }
				int thickness = 3;
				for (int i = 0; i < thickness; i++)
				{
					SDL_FRect border = {
						ContextMenuRect.x + i,
						ContextMenuRect.y + i,
						ContextMenuRect.w - (i * 2),
						ContextMenuRect.h - (i * 2),
					};
					SDL_RenderRect(render, &border);
				}	
			}
		}
		//HANDLE THE SETTINGS.
		if (tabs[activeTab].url == "settings::tab")
		{
			//HANDLE LATER
		}

		//RENDER THE PERFORMANCE MENU
		if (ShowPerformace)
		{
			static std::string performanceText = "";
			static Uint64 lastUpdate = 0;

			if (SDL_GetTicks() - lastUpdate >= 500) //if the ticks - last update are correct, half a second has passed
			{
				lastUpdate = SDL_GetTicks();
				Uint64 end = SDL_GetPerformanceCounter();//grab the performace val
				double ms = (double)(end - start) / SDL_GetPerformanceFrequency() * 1000.0; //convert to ms
				double fps = (1000 / ms);
				std::stringstream ss;
				ss << std::fixed << std::setprecision(1) << fps << "FPS | " << ms << "ms";
				performanceText = "Render Time: " + ss.str();
			}
			SDL_Surface* PERFORMANCESurf = nullptr; //create a serf to hold the printer icon
			{ //LOCK
				std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex); //create a guard to prevent other threads editing the TTF list, and causing a crash.
				PERFORMANCESurf = TTF_RenderText_Solid(iconFont, performanceText.c_str(), 0, textColor);		
			}

			if (PERFORMANCESurf != nullptr) //if the icon was created successfully
			{
				SDL_Texture* PERFORMANCETex = SDL_CreateTextureFromSurface(render, PERFORMANCESurf);
				SDL_SetTextureScaleMode(PERFORMANCETex, SDL_SCALEMODE_NEAREST);
				float printerX = WinW - PERFORMANCESurf->w; //adjust them to the size of the screen, x
				float printerY = WinH - PERFORMANCESurf->h; //adjust them to the size of the screen, y

				SDL_FRect backTexRect = { printerX, printerY, (float)PERFORMANCESurf->w, (float)PERFORMANCESurf->h }; //create a rect to make the button
				SDL_RenderTexture(render, PERFORMANCETex, nullptr, &backTexRect); //send the texture to the render to be rendered

				SDL_DestroyTexture(PERFORMANCETex); //we are done, destroy the texture
				SDL_DestroySurface(PERFORMANCESurf); //we are done, destroy the surface
			}
		}
		static bool lastDarkMode = !darkmode; //hold the opp darkmode
		//render the icon
		if (darkmode != lastDarkMode) //if its been swiched
		{
			std::cout << "FLIP" << std::endl;
			SDL_Surface* icon; //base
			if (darkmode)
			{
				icon = IMG_Load("./logo/darkM_icon.png");
			}
			else {
				icon = IMG_Load("./logo/lightM_icon.png");
			}
			
			if (icon) //if it worked
			{
				SDL_SetWindowIcon(window, icon);
				SDL_DestroySurface(icon); //rm the icon
			}

			lastDarkMode = darkmode; //flip
		}

		SDL_RenderPresent(render); //Send our final render, with all the data, to the screen
	}
	//we need to quit to clean up all the subsystems
	SDL_DestroyWindow(window); //kill the window, cleanly 
	SDL_Quit(); //Destroy SDL

	TTF_Quit(); //Kill TTF
	//std::quick_exit(1); //clean up other threads, and stop

	return 0; //return, stop
} //END OF GUI.cpp