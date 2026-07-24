//THIS IS THE WINDOW RENDER
//(THIS WILL TAKE IN MULTIPLE INPUTS, FROM Layout.cpp, and Webengine.cpp.

#pragma region //Imports from the project file headers.

//=======IMPORTS-WEBENGINE-FILE'S=======\\

#include "ThreadPool.h" //for handling all our ThreadPool functs.
#include "GUI.h" //import from GUI.h file, holds the TAB struct, and a few other global classes.
#include "Parser.h" //used for sending some parts stright to the parser (like for the tab::new)
#include "Profiler.h" //used for handling the performace analization of some functs.
#include "ConnectSocket.h" //allows to send a request to connect socket.cpp -> parser.cpp -> Domtree -> Layout -> GUI.cpp tab render!

#pragma endregion


#pragma region //Imports from the projects external libs.

//=======IMPORTS-GLOBAL-LIBS'S=======\\

#include <SDL3/SDL.h> //include the SDL3 lib - For all the box and screen rendering.
#include <SDL3_ttf/SDL_ttf.h> //fonts lib - For all the fonts, and loading and handling the font.
#include <SDL3_image/SDL_image.h> //images lib - For all the Image rendering, and handling it.

#include <filesystem> //for just the print window button, just that.


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

float adjusted = 0; //this holds a value so that i can ajust the pos of the cursor with the arrow keys TODO


std::vector<std::string> starredPages; //create a string vector list to hold our starred, saved pages!


float zoomAmount = 1.0f; //global float to handle the zoom amount


std::string urlInput = "";    // holds the url we type in the input box
std::string currentURL = ""; //hold the url, but does not save till we press search.

std::vector<Tab> tabs;        // a vector that holds our custom tab struct, allowing us to create tabs, and save data to them.
int activeTab = 0;            // holds the amount of tabs open
int lastsearchedtabID; //holds the id of the last searched tab.

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
					color: #111111;
					font-size: 24px;
				}
				a {
					color: #4A90E2; /* Visible blue link */
					font-size: 22px;
				}
				/* Style to make the logo look nice and neat */
				.browser-image {
					max-width: 200px; /* Limits the size of the logo */
					height: auto;
					display: block;
					margin-top: 20px;
					margin-bottom: 20px;
					border-radius: 8px;
				}
			</style></head><body>
			<div>
				<h1>C++Browse</h1>
				<p>This is my C++ web browser project.</p>
       

				<h1>To start, search anything. ы </h1>
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

		htmlFile << R"(    </div></body></html>)"; //end peices, to insure its valid html


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
			char decoded = (char)std::stoul(hex, nullptr, 16); //convert the hex string from base 16 to a normal char
			out += decoded; //add it to our output string
			i += 2; //now we skip, as we just finished handling the 2 before.
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
		if (path.ends_with(".txt") || path.ends_with(".html") || path.ends_with(".htm")) //if it contains the valid .prefix at the end
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

#pragma region //Handle PreRender


void PreRender(SDL_Renderer* render, TTF_Font* font) //PreRender Takes in a SDL render component, and a TTF Font component
{
	std::unique_lock<std::mutex> lock(gTabsMutex); //set a lock mutex to prevent the Tabs being edited from other threads

	if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size())  return; //if we have no active tabs, skip

	if (lastsearchedtabID != tabs[activeTab].tabID) lastsearchedtabID = tabs[activeTab].tabID; //if the last searched tab ID is not == to the active tab, update it so it is

	int currentLoadGen = tabs[activeTab].loadGen; //create a temp int, and load the current loadGen
	
	int xtrack = 20; //start the X pos at 20, on start
	int ytrack = 120; //start the y pos at, 120 on start
	int lasty = -1; //saves the y pos of the last item, we set it to -1, to insure we record the first element.
	int maxLineHeight = 0; //saves the tallest element on the current line, to avoid clipping text.

	for (int i = 0; i < (int)tabs[activeTab].layout.size(); i++) { //loop for each element in the vector

		if (lasty != -1 && tabs[activeTab].layout[i].y > lasty) //if the last y not == 1 (prevents running on the first loop) and the layout[i].y > lasty
		{
			xtrack = 20; //it is, so now lets set the x track to 20;
			ytrack += (maxLineHeight + 15); //update the y track, moving it down by the max line height + 15
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
				SDL_Color color = tabs[activeTab].layout[i].textColor; //set the color to the text color
				bool isLink = !tabs[activeTab].layout[i].href.empty(); //set the bool if its a link. if its true, its false, and if its false, its true


				int currentTabID = activeTab; //grab the active tab index.
				int myGen = tabs[activeTab].loadGen; //grab the current loadGen id.
				int currentIndex = i; //keep track of which index this is

				lock.unlock(); //unlock before the thread to prevent a crash in release mode
				gTextRenderPool.enqueue([text, fontsize, color, isLink, currentTabID, myGen, currentIndex]() { //start the thread, and import the vars into the thread
					
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

						nodeSurf = TTF_RenderText_Solid(threadFont, text.c_str(), 0, color); //create a temp nodeSerf, to hold it

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
							tabs[currentTabID].layout[currentIndex].pendingTextSurface.store(nodeSurf);
						}

					}		
				}); //end 
			lock.lock(); //relock to continue
			if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size() || tabs[activeTab].loadGen != currentLoadGen) return;//if tabs is empty, or the activetab is less than zero, or greater than size, or a loadGen mismatch, we return, and end;
			}
		}

		//make sure that the thread is done, and the tab holds data.
		if (tabs[activeTab].layout[i].pendingTextSurface.load() != nullptr)
		{
			//create a temp SDL_Surface, where it swaps the surface pointer and clear's it
			SDL_Surface* nodeSurf = tabs[activeTab].layout[i].pendingTextSurface.exchange(nullptr);

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
			gImageDownloadPool.enqueue([srcCopy, currentTabID, myGen, currentIndex, tabUrl]() { //send the image to our threadpool

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
								tabs[currentTabID].layout[currentIndex].pendingSurface.store(imageSurf); //set the surf to the imageSurf.
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
		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].pendingSurface.load() != nullptr)
		{
			//swap to the gpu
			SDL_Surface* surf = tabs[activeTab].layout[i].pendingSurface.exchange(nullptr); //load the surf

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
				}
			}
		}

		//update the max line height, if its taller than its previous element
		if (tabs[activeTab].layout[i].height > maxLineHeight)
		{
			maxLineHeight = tabs[activeTab].layout[i].height;
		}


		//move the x by the width of the text + 12 for padding.
		xtrack += (tabs[activeTab].layout[i].width + 12);


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
		if (t.tabID == lastsearchedtabID) //if the tabID maches the current tab
		{
			t.title = title; //set the title
			return; //end
		}
	}

} //END OF SetTabTitle

#pragma endregion

int GUIRENDER() //GUIRENDER returns an ' int ' and takes in nothing
{
	SDL_Texture* fontText = nullptr; //create a SDL_Texture holds the font texture overall.

	//we want to use the screen so use video
	//we prob wont using audio, but we can through | SDL_INIT_AUDIO
	if (!SDL_Init(SDL_INIT_VIDEO)) { std::cout << "Failed to init SDL" << std::endl;return -1; }//if we get false (it didnt work), we send a DEBUG, then return -1

	
	SDL_Window* window; //create a SDL_Window to handle and hold the window overall
	//create window, some prams are 
	//title, width, height, and flags
	//make it 1080p sizewise
	//for flags, its in this format -> UINT64_C(0X0000000000000020), we also can add more tags through |
	window = SDL_CreateWindow("Browse++", 1920, 1080, SDL_WINDOW_RESIZABLE);

	SDL_Renderer* render = SDL_CreateRenderer(window, nullptr); //Create a render, using the window values, and that we can assign more values to this render, to render more things

	SDL_SetRenderVSync(render, 1); //enable VSync, to keep performance high!

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

	while (running) //loop till running is false
	{
		#pragma region //SDL event's
		while (SDL_PollEvent(&event)) //poll the SDL event's
		{
			
			if (event.type == SDL_EVENT_QUIT) { running = false; } //if the window 'x' close button is pressed, we end the loop

	
			if (event.type == SDL_EVENT_MOUSE_WHEEL) {//if we detect the wheel scrolled
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

			//handle zoom - TODO (as it causes a crash)
			//if (event.key.mod & SDL_KMOD_CTRL) //detect if the ctrl is pressed
			//{
			//	if (event.key.scancode == SDL_SCANCODE_EQUALS)
			//	{
			//		std::lock_guard<std::mutex> lock(gTabsMutex); 
			//		std::cout << "ZOOM IN" << std::endl;
			//		zoomAmount = (std::min)(zoomAmount + 0.1f, 3.0f);
			//		LayoutTree(tabs[activeTab].domRoot);
			//	}
			//	if (event.key.scancode == SDL_SCANCODE_MINUS)
			//	{
			//		std::lock_guard<std::mutex> lock(gTabsMutex); 
			//		std::cout << "ZOOM OUT" << std::endl;
			//		zoomAmount = (std::max)(zoomAmount - 0.1f, 0.5f);
			//		LayoutTree(tabs[activeTab].domRoot);
			//	}
			//}

			if (event.type == SDL_EVENT_TEXT_INPUT) { //detect any text inputs, and add them to our input str
				urlInput += event.text.text; //append to whatever they typed, to our urlInput var
			}

			
			if (event.type == SDL_EVENT_KEY_DOWN) //check for any event key down
			{
		
				if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !urlInput.empty()) { //if we detect backspace, and the urlInput contains something
					urlInput.pop_back(); //we remove the last character
				}

				//if we press enter to search
				if (event.key.scancode == SDL_SCANCODE_RETURN) {
					lastsearchedtabID = tabs[activeTab].tabID; //set the TabID, beforehand, to prevent the loading bug
					NavigateTo(urlInput, render, font, true); //Send the url we want to go to, the render, font, and we want to save it to history
				}

			
				if (event.key.scancode == SDL_SCANCODE_V && (event.key.mod & SDL_KMOD_CTRL)) { //if control+V is pressed
			
					const char* clipboard = SDL_GetClipboardText(); //grab the clipboard from the device.
			
					if (clipboard != nullptr) { urlInput += clipboard; } //make sure the clipboard has something, if it does, append the clipboard info onto the end of the urlInput
				}
			}

			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) //handle if the mouse is clicked
			{
				if (event.button.button == SDL_BUTTON_LEFT) { //check if our left mouse pressed

					int WinW, WinH; //create a temp int to save the WinW, and WinH, that get's assigned in SDL_GetWindowSize
					float mouseX = event.button.x; //save the pos of the mouseX
					float mouseY = event.button.y; //save the pos of the mouseY

					SDL_GetWindowSize(window, &WinW, &WinH); //grab the window, and assign the WinW, and the WinH.


					SDL_FRect backBtnRect = { padding, topMargin, btnSize, btnSize }; //set a rect for the backBtnRect
					SDL_FRect fwdBtnRect = { backBtnRect.x + backBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the fwdBtnRect, using the backBtnRect for offset
					SDL_FRect reloadBtnRect = { fwdBtnRect.x + fwdBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the reloadBtnRect, using the fwdBtnRect for offset
					SDL_FRect homeBtnRect = { reloadBtnRect.x + reloadBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the homeBtnRect, using the reloadBtnRect for offset
					float searchX = homeBtnRect.x + homeBtnRect.w + padding; //get the x size of the search bar
					float searchW = (float)WinW - btnSize - padding - padding - searchX - (btnSize * 2); //get the width of the search bar
					SDL_FRect searchBarRect = { searchX, topMargin, searchW, btnSize }; //get the rect of the search bar, using the searchX and searchW
					SDL_FRect starBtnRect = { searchBarRect.x + searchBarRect.w + padding, topMargin, btnSize, btnSize };  //set a rect for the starBtnRect, using the searchBarRect for offset
					SDL_FRect printerBtnRect = { starBtnRect.x + starBtnRect.w + padding, topMargin, btnSize, btnSize }; //set a rect for the printerBtnRect, using the starBtnRect for offset
					


					//test if the backBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (mouseX >= backBtnRect.x && mouseX <= (backBtnRect.x + backBtnRect.w) &&
						mouseY >= backBtnRect.y && mouseY <= (backBtnRect.y + backBtnRect.h)) { 
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
					if (mouseX >= fwdBtnRect.x && mouseX <= (fwdBtnRect.x + fwdBtnRect.w) &&
						mouseY >= fwdBtnRect.y && mouseY <= (fwdBtnRect.y + fwdBtnRect.h)) {
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
					if (mouseX >= reloadBtnRect.x && mouseX <= (reloadBtnRect.x + reloadBtnRect.w) &&
						mouseY >= reloadBtnRect.y && mouseY <= (reloadBtnRect.y + reloadBtnRect.h)) {
						if (currentURL != "") { NavigateTo(urlInput, render, font, false); } //don't reload for the mainpage, but just index the same url

					}


					//test if the homeBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (mouseX >= homeBtnRect.x && mouseX <= (homeBtnRect.x + homeBtnRect.w) &&
						mouseY >= homeBtnRect.y && mouseY <= (homeBtnRect.y + homeBtnRect.h)) {
						if (currentURL != "") { NavigateTo("new::tab", render, font, false); } //don't go if we are already on the mainpage, but load new::tab
					}


					//test if the printerBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (mouseX >= printerBtnRect.x && mouseX <= (printerBtnRect.x + printerBtnRect.w) &&
						mouseY >= printerBtnRect.y && mouseY <= (printerBtnRect.y + printerBtnRect.h)) {
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
							ShellExecuteA(NULL, "print", filename, NULL, NULL, SW_SHOWNORMAL); 	// Ask Windows to handle the printing, but SHOW the menu normally

							SDL_DestroySurface(screenshot);//we are done here, destroy the old screenshot
						}
					}

					//test if the starBtnRect is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
					if (mouseX >= starBtnRect.x && mouseX <= (starBtnRect.x + starBtnRect.w) &&
						mouseY >= starBtnRect.y && mouseY <= (starBtnRect.y + starBtnRect.h) && currentURL != "") { //we also make sure we have something in the currentURL, as we don't want to star newtab

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

					bool linkwasClicked = false; //create a temp bool to hold if we clicked a link, start it at false
					std::string clickedURL; //create a temp string to hold the url
					{
						std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent other threads to change the tabs
						for (int i = 0; i < tabs[activeTab].layout.size(); i++) //loop through each tab in the tabs list
						{
							if (tabs[activeTab].layout[i].textTex == nullptr) continue; //check if its text, if it is, skip
							if (tabs[activeTab].layout[i].href.empty()) continue; //check if its a herf, if it does'nt contain anything, skip

							float screeny = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos; //ok we have a link, now lets adjust the activator block to be over it

							//test if the tabs activator button is pressed, checking the x of our mouse, and the x of the button, to see if the mouse click overlaps with it
							if (mouseX >= tabs[activeTab].layout[i].x && mouseX <= (tabs[activeTab].layout[i].x + tabs[activeTab].layout[i].width) &&
								mouseY >= screeny && mouseY <= (screeny + tabs[activeTab].layout[i].height))
							{
								std::cout << "link pressed" << std::endl; //DEBUG
								std::string finalUrl = tabs[activeTab].layout[i].href; //grab the URL of the link, of what I just pressed

								std::string realDest = FixURLREDIRECT(finalUrl); //fixes issues with the link, and saves it to the string
								if (!realDest.empty()) { finalUrl = realDest; } //if it's not empty, update the finalURL to the realDest;

								clickedURL = finalUrl; //set the clicked url to the final url
								linkwasClicked = true; //and say the link was clicked
								break; //exit
							}
						}
					}
					if (linkwasClicked) //now that we know we clicked the link
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
								std::lock_guard<std::mutex> lock(gTabsMutex); //create a lock to prevent a crash when other threads access the values
								activeTab = t; //set the active tab, to the current tab we just pressed
								urlInput = tabs[activeTab].url; //then update the url input to the tab we pressed
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
			}
		}

		#pragma endregion

		// --- RENDERING --- \\

		PreRender(render, font); //load the textures, images, and fonts, and the style for them.

		//Clear the screen
		//set it to the darkmode/lightmode
		if (darkmode) //inverse
		{
			SDL_SetRenderDrawColor(render, (255 - backgroundColor.r), (255 - backgroundColor.g), (255 - backgroundColor.b), 255); // auto choses based on the site!
		}
		else{
			SDL_SetRenderDrawColor(render, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255); // auto choses based on the site!
		}
		SDL_RenderClear(render);

		//Loop through the layout list

		//draw the page by looping through the layout list
		{
			std::lock_guard<std::mutex> lock(gTabsMutex);
			for (int i = 0; i < tabs[activeTab].layout.size(); i++)
			{
				//render our text, we make a rectangle and assign our text to that!
				SDL_FRect textrec;
				textrec.x = tabs[activeTab].layout[i].x;
				textrec.y = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos;
				textrec.w = tabs[activeTab].layout[i].width;
				textrec.h = tabs[activeTab].layout[i].height;

				//draw the bg color
				if (tabs[activeTab].layout[i].hasBg)
				{
					if (darkmode) //inverse
					{
						SDL_SetRenderDrawColor(render, (255 - tabs[activeTab].layout[i].bgColor.r), (255 - tabs[activeTab].layout[i].bgColor.g), (255 - tabs[activeTab].layout[i].bgColor.b), 255); //255 cause we dont want it transparent
					}
					else {
						SDL_SetRenderDrawColor(render, tabs[activeTab].layout[i].bgColor.r, tabs[activeTab].layout[i].bgColor.g, tabs[activeTab].layout[i].bgColor.b, 255); //255 cause we dont want it transparent
					}

					//fill the rec
					SDL_RenderFillRect(render, &textrec);
				}

				//draw image or text now
				//check if we got an image, and if the texture has somthing
				if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].imageTex != nullptr)
				{
					//for now, we are gonna clamp images
					//if (textrec.w > 300)
					//{
					//	float scale = 300 / textrec.w; //make a scale thing, so we adjust right
					//	textrec.w = 300; //se the width to 300
					//	textrec.h = (int)(textrec.h * scale); //so that the scale is accurate
					//}

					SDL_RenderTexture(render, tabs[activeTab].layout[i].imageTex, nullptr, &textrec);
				}
				//if our text contains something
				else if (tabs[activeTab].layout[i].textTex != nullptr)
				{
					SDL_RenderTexture(render, tabs[activeTab].layout[i].textTex, nullptr, &textrec);
				}









			}
		}

		//icons! Home, Reload, Back+Forth arrows, search, star, printer
		//љ њ ђ ы ж ξ

		int WinW, WinH;
		SDL_GetCurrentRenderOutputSize(render, &WinW, &WinH);
	

		float scaleX = 1.0f, scaleY = 1.0f;
		SDL_GetRenderScale(render, &scaleX, &scaleY);

		if (darkmode) //serach scroll bar color bg
		{
			SDL_SetRenderDrawColor(render, 10, 10, 10, 255);
		}
		else {
			SDL_SetRenderDrawColor(render, 245, 245, 245, 255);
		}
	
		SDL_FRect searchBarBg = { 0, 30, (float)WinW, 39 };
		SDL_RenderFillRect(render, &searchBarBg);



		//fist draw the scroll bar!
		//my plan is to draw the track, and then the bar, the bar shoudl adjust size based on len of the document
		//the track is easy, it never moves

		//fist lets make the max scroll, as we should limit it with a bar (however, we should prob get this working from the layout in the future) (its stored in the gui tab folder)
		float scrollBarWidth = 20.0f;
		float uiTopBarHeight = 70.0f; 
		float trackHeight = WinH - uiTopBarHeight;

		//draw the scroll bar
		if (darkmode)
		{
			SDL_SetRenderDrawColor(render, 31, 31, 31, 255);
		}
		else {
			SDL_SetRenderDrawColor(render, 224, 224, 224, 255);
		}
		
		SDL_FRect scrollTrack = { WinW - scrollBarWidth, uiTopBarHeight, scrollBarWidth, trackHeight };
		SDL_RenderFillRect(render, &scrollTrack);

		//now we handle the size of the bar
		float barHeight = (trackHeight / (float)(tabs[activeTab].maxscroll + WinH)) * trackHeight; //the height of the bar, we take teh winH, the trackHight, and the max scroll!
		if (barHeight < 20.0f) barHeight = 20.0f; //we also make sure it cant get smaller than this, or it might dissapear!

		//we calculate its pos out of 100, with the size and stuff, so that we get an accurate bar!
		float scrollPercentage = (float)tabs[activeTab].scrollpos / (float)tabs[activeTab].maxscroll;
		float barY = uiTopBarHeight + (scrollPercentage * (trackHeight - barHeight));

		//draw it
		if (darkmode) //darkmode - inverted
		{
			SDL_SetRenderDrawColor(render, 63, 63, 63, 255);
		}
		else {
			SDL_SetRenderDrawColor(render, 192, 192, 192, 255);
		}
		
		SDL_FRect scrollbar = { WinW - scrollBarWidth, barY, scrollBarWidth, barHeight };
		SDL_RenderFillRect(render, &scrollbar);

	
		SDL_SetRenderDrawColor(render, 255, 255, 255, 180); // White top/left highlight
		SDL_RenderLine(render, scrollbar.x, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y);
		SDL_RenderLine(render, scrollbar.x, scrollbar.y, scrollbar.x, scrollbar.y + scrollbar.h);

	
		SDL_SetRenderDrawColor(render, 100, 100, 100, 255);
		SDL_RenderLine(render, scrollbar.x, scrollbar.y + scrollbar.h, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h);
		SDL_RenderLine(render, scrollbar.x + scrollbar.w, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h);



		//find the lowest point
		int totalPageHeight = 0;

		//go through each node, and find its pos
		for (int i = 0; i < tabs[activeTab].layout.size(); i++) {
		
			int nodeBottom = tabs[activeTab].layout[i].y + tabs[activeTab].layout[i].height;

			//if this node is > than the last, update it!
			if (nodeBottom > totalPageHeight) {
				totalPageHeight = nodeBottom;
			}
		}

	
		tabs[activeTab].maxscroll = totalPageHeight - (WinH - uiTopBarHeight) + 100;

		
		if (tabs[activeTab].maxscroll < 0) {
			tabs[activeTab].maxscroll = 0;
		}






















		//render the 2 buttons
		SDL_FRect backBtn;

		if (tabs[activeTab].currentSearchPos > 0) //we do > than 0, as we dont want to have an error
		{
			backBtn.x = SDL_floorf(padding * scaleX) / scaleX;
			backBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
			backBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
			backBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;
			if (darkmode) //darkmode
			{
				SDL_SetRenderDrawColor(render, 55, 55, 55, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			}
			
			SDL_RenderFillRect(render, &backBtn);
		}
		else {
			backBtn.x = SDL_floorf(padding * scaleX) / scaleX;
			backBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
			backBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
			backBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;
			if (darkmode)
			{
				SDL_SetRenderDrawColor(render, 20, 20, 20, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 235, 235, 235, 255);
			}
			
			SDL_RenderFillRect(render, &backBtn);
		}


		SDL_Color textColor; //do not affect this, as we want the buttons to be this color no matter what.
		if (darkmode)
		{
			 textColor = { 255, 255, 255, 255 };
		}
		else {
			 textColor = { 0, 0, 0, 255 };
		}
		

		//handle my monitor
	

		//BACK ARROW
		SDL_Surface* backSurf; 
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			backSurf = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor);
		}
		if (backSurf != nullptr)
		{
			SDL_Texture* backTex = SDL_CreateTextureFromSurface(render, backSurf);


			SDL_SetTextureScaleMode(backTex, SDL_SCALEMODE_NEAREST);

			//before i had them hardcoded, not anymore!
			//we snap to the screen
			float backX = SDL_floorf((backBtn.x + (backBtn.w - (float)backSurf->w) / 2.0f) * scaleX) / scaleX;
			float backY = SDL_floorf((backBtn.y + (backBtn.h - (float)backSurf->h) / 2.0f) * scaleY) / scaleY;

			
			SDL_FRect backTexRect = { backX, backY, (float)backSurf->w, (float)backSurf->h };
			SDL_RenderTexture(render, backTex, nullptr, &backTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(backTex);
			SDL_DestroySurface(backSurf);
		}

		SDL_FRect fwdBtn;
		if (!tabs[activeTab].SearchHistory.empty() && tabs[activeTab].currentSearchPos < (int)tabs[activeTab].SearchHistory.size() - 1) //we do > than 0, as we dont want to have an error
		{
			//changed to snap the <> correctly!
			fwdBtn.x = SDL_floorf((backBtn.x + backBtn.w + padding) * scaleX) / scaleX;
			fwdBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
			fwdBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
			fwdBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;

			if (darkmode) //darkmode
			{
				SDL_SetRenderDrawColor(render, 55, 55, 55, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			}
			SDL_RenderFillRect(render, &fwdBtn);
		}
		else {
			//not enabled
			//changed to snap the <> correctly!
			fwdBtn.x = SDL_floorf((backBtn.x + backBtn.w + padding) * scaleX) / scaleX;
			fwdBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
			fwdBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
			fwdBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;

			if (darkmode)
			{
				SDL_SetRenderDrawColor(render, 20, 20, 20, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 235, 235, 235, 255);
			}
			SDL_RenderFillRect(render, &fwdBtn);
		}


		//FORWARD ARROW
		SDL_Surface* forwardSurf; 
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			forwardSurf = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor);
		}
		if (forwardSurf != nullptr)
		{
			//flip the sprite, as currently its ugly lol
			SDL_FlipSurface(forwardSurf, SDL_FLIP_HORIZONTAL);

			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, forwardSurf);

			//force it to fix the pixel art! (found this with a bit of searching!)
			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST);

			float fwdX = SDL_floorf((fwdBtn.x + (fwdBtn.w - (float)forwardSurf->w) / 2.0f) * scaleX) / scaleX;
			float fwdY = SDL_floorf((fwdBtn.y + (fwdBtn.h - (float)forwardSurf->h) / 2.0f) * scaleY) / scaleY;

			SDL_FRect fwdTexRect = { fwdX, fwdY, (float)forwardSurf->w, (float)forwardSurf->h };

			//flip the sprite, as currently its ugly lol
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex);
			SDL_DestroySurface(forwardSurf);
		}



		SDL_FRect reloadButton;

		//reload button
		reloadButton.x = fwdBtn.x + fwdBtn.w + padding;
		reloadButton.y = topMargin;
		reloadButton.w = btnSize;
		reloadButton.h = btnSize;
		if (darkmode) //darkmode
		{
			SDL_SetRenderDrawColor(render, 55, 55, 55, 255);
		}
		else {
			SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
		}
		SDL_RenderFillRect(render, &reloadButton);

		SDL_Surface* reloadSurf;
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			reloadSurf = TTF_RenderText_Solid(iconFont, "њ", 0, textColor);
		}

		if (reloadSurf != nullptr)
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, reloadSurf);

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST);

			SDL_FRect reloadTexRect = { reloadButton.x + 3, reloadButton.y - 4, (float)reloadSurf->w, (float)reloadSurf->h };
			SDL_RenderTexture(render, forwardTex, nullptr, &reloadTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex);
			SDL_DestroySurface(reloadSurf);
		}






		SDL_FRect homeBtn;
		
			homeBtn.x = reloadButton.x + reloadButton.w + padding;
			homeBtn.y = topMargin;
			homeBtn.w = btnSize;
			homeBtn.h = btnSize;
			if (darkmode) //darkmode
			{
				SDL_SetRenderDrawColor(render, 55, 55, 55, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			}
			SDL_RenderFillRect(render, &homeBtn);
		

		
		
		SDL_Surface* homeSurf; 
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			homeSurf = TTF_RenderText_Blended(iconFont, "љ", 0, textColor);
		}

		if (homeSurf != nullptr)
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, homeSurf);

			SDL_SetTextureScaleMode(forwardTex, SDL_SCALEMODE_NEAREST);

			SDL_FRect fwdTexRect = { homeBtn.x + 2, homeBtn.y - 2, (float)homeSurf->w, (float)homeSurf->h };
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex);
			SDL_DestroySurface(homeSurf);
		}




		
		//INPUT BOX
		float searchX = homeBtn.x + homeBtn.w + padding;
		float searchW = (float)WinW - btnSize - padding - padding - searchX - ((btnSize) * 2); //for 2 buttons

		

		SDL_FRect bar = {
			SDL_floorf(searchX * scaleX) / scaleX,
			SDL_floorf(topMargin * scaleY) / scaleY,
			SDL_floorf(searchW * scaleX) / scaleX,
			SDL_floorf(btnSize * scaleY) / scaleY
		};
		if (darkmode) //handle dark mode
		{
			SDL_SetRenderDrawColor(render, 15, 15, 15, 255); //draw a grayish color
		}
		else {
			SDL_SetRenderDrawColor(render, 240, 240, 240, 255); //draw a grayish color
		}
		
		//fill the rec with this
		SDL_RenderFillRect(render, &bar);

		std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
		//draw the text typed
		TTF_SetFontSize(font, 17); //define the font size
		std::string displayText = urlInput;
		//set the txt color

		SDL_Color color;
		if (darkmode) //handle dark mode
		{
			color = { 255,255,255,255 };
		}
		else {
			color = { 0,0,0,255 };
		}
		

		if (urlInput.empty()) //check if its empty
		{
			displayText = "ы Enter a url...";
			if (darkmode) //handle dark mode
			{
				color = { 75, 75, 75, 255 }; //just a dark gray
			}
			else{
				color = { 180, 180, 180, 255 }; //just a dark gray
			}
			
		}

		//render it!
		SDL_Surface* urlSurf = TTF_RenderText_Solid(font, displayText.c_str(), 0, color);

	
		if (urlSurf != nullptr) //check to make sure we arnt trying to render somthing that is null
		{
			//upload it to the gpu for drawing
			SDL_Texture* urlTexture = SDL_CreateTextureFromSurface(render, urlSurf);

			SDL_SetTextureScaleMode(urlTexture, SDL_SCALEMODE_NEAREST);
			//make a rectangle that is going to render our text
			//give the text a bit of padding and stuff
			SDL_FRect urlTextureRect = { bar.x + 8, bar.y + 6, (float)urlSurf->w, (float)urlSurf->h };
			SDL_RenderTexture(render, urlTexture, nullptr, &urlTextureRect);

			//we are done with it, destory!
			SDL_DestroyTexture(urlTexture);


			//draw the cursor
			//doing SDL_GETTICKS() returns miliseconds since hte app started
			//dividing this by 500 gives us incrmetns every half a tick, that flip between t or f
			//this gives a blinking effect without a timer
			bool showcursor = (SDL_GetTicks() / 500) % 2 == 0;
			if (!urlInput.empty() && showcursor)
			{
				//draw it now
				//urlSurf-w is the total pixel width of the string
				float cursorX = bar.x + 8 + urlSurf->w + 1 + adjusted;
				if (darkmode) //handle dark mode
				{
					SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
				}
				else {
					SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
				}

				
				SDL_RenderLine(render, cursorX, bar.y + 5, cursorX, bar.y + 23);

			}

			//cleanup
			SDL_DestroySurface(urlSurf);


		}



		
		//new button, star
		SDL_FRect starBtn;
		starBtn.x = SDL_floorf((bar.x + bar.w + padding) * scaleX) / scaleX;
		starBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
		starBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
		starBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;

		bool isStarred = (std::find(starredPages.begin(), starredPages.end(), currentURL) != starredPages.end());
		
		if (isStarred)
		{
			if (darkmode)//darkmode
			{
				SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
			}
			else {
				SDL_SetRenderDrawColor(render, 255, 255, 0, 255); 
			}
			
		}
		else {
			if (darkmode) //darkmode
			{
				SDL_SetRenderDrawColor(render, 55, 55, 55, 255); 
			}
			else {
				SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			}
			
		}




	
		SDL_RenderFillRect(render, &starBtn);

		//RENDER STAR
		SDL_Surface* starSurf;
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			starSurf = TTF_RenderText_Solid(iconFont, "ж", 0, textColor);
		}

		if (starSurf != nullptr)
		{
			SDL_Texture* starTex = SDL_CreateTextureFromSurface(render, starSurf);


			SDL_SetTextureScaleMode(starTex, SDL_SCALEMODE_NEAREST);

			//before i had them hardcoded, not anymore!
			//we snap to the screen
			float starX = SDL_floorf((starBtn.x + (starBtn.w - (float)starSurf->w) / 2.0f) * scaleX) / scaleX;
			float starY = SDL_floorf((starBtn.y + (starBtn.h - (float)starSurf->h) / 2.0f) * scaleY) / scaleY;


			SDL_FRect backTexRect = { starX, starY, (float)starSurf->w, (float)starSurf->h };
			SDL_RenderTexture(render, starTex, nullptr, &backTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(starTex);
			SDL_DestroySurface(starSurf);
		}


		//new button, printer
		SDL_FRect printerBtn;
		printerBtn.x = SDL_floorf((bar.x + bar.w + padding + btnSize + padding) * scaleX) / scaleX;
		printerBtn.y = SDL_floorf(topMargin * scaleY) / scaleY;
		printerBtn.w = SDL_floorf(btnSize * scaleX) / scaleX;
		printerBtn.h = SDL_floorf(btnSize * scaleY) / scaleY;

		if (darkmode) //darkmode
		{
			SDL_SetRenderDrawColor(render, 55, 55, 55, 255);
		}
		else {
			SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
		}
		SDL_RenderFillRect(render, &printerBtn);

		//RENDER PRINTER
		SDL_Surface* printerSurf;
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			printerSurf = TTF_RenderText_Solid(iconFont, "ξ", 0, textColor);
		}

		if (printerSurf != nullptr)
		{
			SDL_Texture* printerTex = SDL_CreateTextureFromSurface(render, printerSurf);


			SDL_SetTextureScaleMode(printerTex, SDL_SCALEMODE_NEAREST);

			//before i had them hardcoded, not anymore!
			//we snap to the screen
			float printerX = SDL_floorf((printerBtn.x + (printerBtn.w - (float)printerSurf->w) / 2.0f) * scaleX) / scaleX;
			float printerY = SDL_floorf((printerBtn.y + (printerBtn.h - (float)printerSurf->h) / 2.0f) * scaleY) / scaleY;


			SDL_FRect backTexRect = { printerX, printerY, (float)printerSurf->w, (float)printerSurf->h };
			SDL_RenderTexture(render, printerTex, nullptr, &backTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(printerTex);
			SDL_DestroySurface(printerSurf);
		}



















		//draw the background of the taskbar.
		if (darkmode) //darkmode
		{
			SDL_SetRenderDrawColor(render, 45, 45, 45, 255); //make the taskbar bg inverted.
		}
		else {
			SDL_SetRenderDrawColor(render, 210, 210, 210, 255);
		}
		
		SDL_FRect tabBarBg = { 0, 0, (float)WinW, 30 };
		SDL_RenderFillRect(render, &tabBarBg);


		//ok, lets draw each tab
		int tabX = 0; //stores the X of each tab
		for (int t = 0; t < tabs.size(); t++)
		{
			//ok lets do a tab width, and lets make it fixed for now

			int tabW = 180;

			//change the color based on the tab (if its active or not)
			if (t == activeTab)
			{
				if (darkmode) //darkmode
				{
					SDL_SetRenderDrawColor(render, 10, 10, 10, 255); //make the taskbar img inverted, active)
				}
				else {
					SDL_SetRenderDrawColor(render, 245, 245, 245, 255);
				}
			}
				
			else
			{
				if (darkmode)
				{
					SDL_SetRenderDrawColor(render, 65, 65, 65, 255);  //make the taskbar img inverted, unactive.
				}
				else {
					SDL_SetRenderDrawColor(render, 190, 190, 190, 255);
				}
				
				
			}
				

			//make our tab rec, putting in our tab width and pos
			SDL_FRect tabrect = { (float)tabX, 0, (float)tabW, 30 }; //with a height of 30
			//render it
			SDL_RenderFillRect(render, &tabrect);


			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			//ok now lets draw the tab text (gonna be the <title> text)
			TTF_SetFontSize(font, 13);
			//set the color to black, and the title to the title
			SDL_Color tabTextColor;
			if (darkmode) //darkmode
			{
				tabTextColor = { 255,255,255,255 };
			}
			else {
				tabTextColor = { 0,0,0,255 };
			}
			
			
			std::string tabTitle = tabs[t].title;


			//ok, now lets trim the long titles, as they are gonna overspill
			if (tabTitle.size() > 20) tabTitle = tabTitle.substr(0, 17) + "..."; //replace the overspill (greater than 17 chars) with ...

			//render it
			SDL_Surface* tabSurf = TTF_RenderText_Solid(font, tabTitle.c_str(), 0, tabTextColor);
			if (tabSurf != nullptr) //make sure its not null
			{
				//create a texture for the text
				SDL_Texture* tabTex = SDL_CreateTextureFromSurface(render, tabSurf);
				//move it to fit in the tab
				SDL_FRect tabTextRect = { (float)tabX + 8, 6, (float)tabSurf->w, (float)tabSurf->h };

				//render it
				SDL_RenderTexture(render, tabTex, nullptr, &tabTextRect);
				//clear to prevent mem leaks

				SDL_DestroyTexture(tabTex);
				SDL_DestroySurface(tabSurf);
			}



			//we also want the ability to close the tabs with an x, like a normal browser.

			//make the serf for it
			SDL_Surface* xSurf;
			if (tabs.size() > 1)
			{
				xSurf = TTF_RenderText_Solid(font, "x", 0, tabTextColor);
			}
			else {
				xSurf = TTF_RenderText_Solid(font, "", 0, tabTextColor);
			}
			

			//make sure we have made it
			if (xSurf != nullptr)
			{
				//create the Tex from the serface of our text
				SDL_Texture* xTex = SDL_CreateTextureFromSurface(render, xSurf);

				//make a rec, and position it right
				SDL_FRect xRect = { (float)(tabX + tabW - 20), 7, (float)xSurf->w, (float)xSurf->h };
				SDL_RenderTexture(render, xTex, nullptr, &xRect); //render it

				//destory both
				SDL_DestroyTexture(xTex);
				SDL_DestroySurface(xSurf);
			}

			//increase the gap between the next tab
			tabX += tabW + 2;  



		}







		//draw the + for the tabs
		if (darkmode) //darkmode
		{
			SDL_SetRenderDrawColor(render, 75, 75, 75, 255); //inverted
		}
		else {
			SDL_SetRenderDrawColor(render, 180, 180, 180, 255);
		}
		//make the button for it
		SDL_FRect newTabBtn = { (float)tabX, 2, 26, 26 };

		//fill it
		SDL_RenderFillRect(render, &newTabBtn);

		SDL_Surface* plusSurf;

		//just a copy and paste atp
		{
			std::lock_guard<std::recursive_mutex> ttfLock(gTTFMutex);
			if (darkmode) //darkmode
			{
				plusSurf = TTF_RenderText_Solid(font, "+", 0, { 255,255,255,255 });
			}
			else {
				plusSurf = TTF_RenderText_Solid(font, "+", 0, { 0,0,0,255 });
			}
		}

		//make sure we have made it
		if (plusSurf != nullptr)
		{
			//create the Tex from the serface of our text
			SDL_Texture* plusTex = SDL_CreateTextureFromSurface(render, plusSurf);

			//make a rec, and position it right
			SDL_FRect xRect = { (float)(tabX + 8),  7, (float)plusSurf->w, (float)plusSurf->h };
			SDL_RenderTexture(render, plusTex, nullptr, &xRect); //render it

			//destory both
			SDL_DestroyTexture(plusTex);
			SDL_DestroySurface(plusSurf);
		}






		//this is like our pygame render thing
		SDL_RenderPresent(render);

	}


	//we need to quit to clean up all the subsystems
	SDL_DestroyWindow(window); //kill the window, cleanly 
	SDL_Quit();

	//Kill TTF
	TTF_Quit();

	std::quick_exit(1);

	return 0;
}

