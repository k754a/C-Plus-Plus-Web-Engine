//THIS IS THE WINDOW RENDER
//(THIS WILL TAKE IN MULTIPLE INPUTS, FROM Layout.cpp, and Webengine.cpp.


#include "GUI.h"
#include <SDL3/SDL.h> //include the SDL3 lib
#include <SDL3_ttf/SDL_ttf.h> //fonts lib
#include <thread> //lets us do our multiple searches in one run
#include <regex> //checking for https or http format
#include <iostream>
#include "ConnectSocket.h" //pulls our validate string global libary
#include <fstream>  //for loading local files
#include <sstream> // for the buffer
#include "Parser.h"
#include <SDL3_image/SDL_image.h>
#include <windows.h> //for the print
#include <filesystem> //for the folder handling on the bmp
#include "Profiler.h" //DEBUG
#include "ThreadPool.h"
//MOVED TO TAB MODE.

//this is the layout list, and gets filled in by IMPORT
//the render loop reads from this 
std::vector<Layout> mainlayout;

SDL_Texture* loadingTex = nullptr;

ThreadPool gWorkerPool(8); //create a pool.

ThreadPool gImageThreadPool(4);

//wanted to devlop a way to have a back and forward arrow.
//the way we can do this is have a list with the websites, and 2 buttons besides the input box, if they are pressed, we change the index, going back or forth
//this is my thoughs

//SEARCH -> ADD TO LIST -> SAVED

//WHEN BUTTON PRESSED (either or) -> LOOK FORWARD OR BACKWARD IN THE LIST -> COMPARE TILL WE FIND THE INDEX -> MOVE BACK ONE.




// create the first tab immediately at startup before anything else runs
// this has to be here, not in GUIRENDER, because Parser/IMPORT run before GUIRENDER sets up tabs

//make the first tab (it crashes if you dont btw)

//C++ global structs always run first, so we can make it instantly


//MOVED CURRENT SEARCH HISTORY, AS WE NEED SEPRATE ONES FOR TABS!

//
////SUDO CODE
////if(SearchHistory[i] == urlInput && searchHistory.length() > 1){
////
////	urlInput = SearchHistory[i - 1];
//// 
//// 
//// 
//// 
////}
//
////we we search, we add one to the top, if we search, and our current pos is back one from our search history, we will overwrite that history.
//
//
//
//
//
//
//
//
//
//
////the pos and how far weve scrolled down for the page
////starts at the top of the page 0, and increases as the user scrolls down.
////its subtracted from each elements y, so it gives the illiusion of scrolling.
int scrollpos = 40;

std::string urlInput = "";    // holds the url we type
std::string currentURL = ""; //hold the url, but does not change till someone presses search!

std::vector<Tab> tabs;        // all open tabs
int activeTab = 0;            // active tab open

struct TabInit {
	TabInit() {
		Tab t;
		t.title = "New Tab";
		tabs.push_back(t);
		activeTab = 0;

		
	}
} _tabInit;



//======DELETE TREE======\\

void DeleteTree(Node* node) //this takes in our custom node tree, and returns nothing.
{
	//insure that our node is not null
	if (node == nullptr) return; //error case, just return

	for (Node* child : node->children) //loop through every child in this node
	{
		DeleteTree(child); //for each child we call this, to destroy its child's.
	}
	node->children.clear();

	delete node; //we have made it to the bottom, destroy ourselves.
}



std::vector<std::string> starredPages; //hold our starred, saved pages!
int LoadStarredPages()
{
	//fist clean our vector, just in case
	starredPages.clear();

	//then open the file
	std::ifstream bookmarks("starred_pages.STAR");

	//before we do anything, make sure that its open, (cause we will 100% crash lol)
	if (bookmarks.is_open())
	{
		std::string line;

		//read the file line by line, and then add it to the starredPages
		//when we have no more lines, we can return false
		while (std::getline(bookmarks, line))
		{
			//ignore blank lines
			if (!line.empty())
			{
				//add it
				starredPages.push_back(line);
			}
		}
		bookmarks.close();
	}

	return 0;
}


//now we update the html!

int UpdateHTML()
{
	std::ofstream htmlFile("main.html", std::ios::trunc); //make sure to overwrite it on open

	//checks to make sure that its good!
	if (htmlFile.is_open())
	{
				//first add the stuff to it: 
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
				<h1>‎ </h1>
		)";

			//now we loop through each starred page, and print them!
		if (!starredPages.empty()) //we display somthin else if they are empty!
		{
			htmlFile << "        <br>Starred Pages:\n"; //display the star pages
			for (const std::string& site : starredPages) { //go through each one, and add it to the file
				
				htmlFile << "        <p>ж -<a href=\"" << site << "\">" << site << "</a></p>\n";
			}
		}
		else {
			htmlFile << "<br>No Starred Pages\n";
		}

		htmlFile << R"(    </div></body></html>)"; 


		//close the file
		htmlFile.close();


	}
	return 0;
}






























bool urlBarFocused = true;    // currently we have no other inputs, so we can always have this focused!
SDL_Color backgroundColor = { 245, 245, 245, 255 }; //white bg, gets changed btw


//this is called in layout, and repaces whatever was on the screen with the new layout
int IMPORT(std::vector<Layout> layoutGOTTEN, Node* newRoot)
{
	//handle if we have dark mode or not: 
	if (darkmode)
	{
		SDL_Color backgroundColor = { 1, 1, 1, 255 }; //black bg, gets changed, but will stay, if there isnt a defined color
	}
	else {
		SDL_Color backgroundColor = { 245, 245, 245, 255 }; //white bg, gets changed, but will stay, if there isnt a defined color
	}

	PROFILE("IMPORT"); //PROFILE THE IMPORT

	if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size())
	{
		std::cout << "IMPORT called before tabs ready." << std::endl;
		return -1;
	}

	scrollpos = 40; //reset the scroll pos, for new web sites
	tabs[activeTab].scrollpos = 40;
	//we need to destroy all the old textures, as these are loaded in gpu mem
	for (int i = 0; i < tabs[activeTab].layout.size(); i++)
	{
		if (tabs[activeTab].layout[i].textTex != nullptr) //check to make sure we dont double free
		{
			SDL_DestroyTexture(tabs[activeTab].layout[i].textTex); //tells the gpu to free this
			tabs[activeTab].layout[i].textTex = nullptr; //set it to null so we dont do it twice, and crash
		}
	}

	//rm the old tree
	if (tabs[activeTab].domRoot != nullptr)
	{
		DeleteTree(tabs[activeTab].domRoot);
	}

	tabs[activeTab].domRoot = newRoot;

	tabs[activeTab].layout = layoutGOTTEN;
	tabs[activeTab].url = urlInput;
	currentURL = urlInput;
	std::cout << "Updated" << std::endl;
	return 0;
}
//moved to this, for speed!
//this uploads the textures to the gpu
//the idea is that we only do the GPU stuff once a layout, then assign this to a texture, that moves

std::string PercentDecode(const std::string& src)
{
	std::string out;
	for (size_t i = 0; i < src.size(); i++)
	{
		if (src[i] == '%' && i + 2 < src.size())
		{
			std::string hex = src.substr(i + 1, 2);
			char decoded = (char)std::stoul(hex, nullptr, 16);
			out += decoded;
			i += 2;
		}
		else if (src[i] == '+') //handle things like +'s in urls
		{
			out += ' ';
		}
		else
		{
			out += src[i];
		}
	}
	return out;
}


std::string ResolveURL(const std::string& targetUrl, const std::string& currentUrl)
{
	//make sure its not blank
	if (targetUrl.empty()) return targetUrl;
	//if its good use as is
	if (targetUrl.find("http://") == 0 || targetUrl.find("https://") == 0) return targetUrl;


	//handle urls like //example.com
	if (targetUrl.find("//", 0) == 0)
	{
		return "https:" + targetUrl; //fix it to be https://example.com
	}

	//get the domain part, not the full link
	std::string domain = "";
	size_t protoend = currentUrl.find("://");
	//if we find the protoend
	if (protoend != std::string::npos)
	{
		size_t domainEnd = currentUrl.find("/", protoend + 3); //get the end of it
		if (domainEnd != std::string::npos)
		{
			domain = currentUrl.substr(0, domainEnd); //get it like https://Example.com
		}
		else {
			domain = currentUrl; //no path, this is the domain
		}
	}
	
	//handle absolute paths like /assets/example.png -> https://Example.coom/assets/example.com

	if (targetUrl[0] == '/')
	{
		return domain + targetUrl;
	}

	//get the direc of the current page for the paths
	std::string directory = domain;
	size_t lastSlash = currentUrl.rfind("/");
	size_t protoSlash = currentUrl.find("://");

	//make sure the last slash isnt appart of the ://
	if (lastSlash != std::string::npos && lastSlash > protoSlash + 3)
	{
		directory = currentUrl.substr(0, lastSlash);
	}



	//reletive path

	return directory + "/" + targetUrl;


}


//This function will check, and block some chars we dont want.
std::string FilterNOTDEF(TTF_Font* font, const std::string& text) //FilterNOTDEF retuns a 'std::string', and takes in a TTF_Font, and a const string.
{
	std::string out; //create a temp string
	out.reserve(text.size()); //reserve memory so we can save on performace.


	size_t i = 0;//create a size_t, to hold our loop

	while (i < text.size()) //repeate untill our i is larger than our size of the text.
	{
		unsigned char c = text[i]; //set a char to hold our single letter

		int len; //hold the len of our char
		//figure out how many bytes the char takes
		if (c < 0x80) {
			len = 1; //1 byte char
		}
		else if (c < 0xE0) {
			len = 2; //2 byte char
		}
		else if (c < 0xF0) {
			len = 3; //3 byte char
		}
		else {
			len = 4; //4 byte char
		}

		if (i + len > text.size()) break; //if we have some weird, super long text, break.

		//now lets grab the bits from the lead byte's

		Uint32 bytes;
		if (len == 1) {  //if the len is just one, easy set to just the char
			bytes = c;
		}
		else { //if its any other len, we need to remove the flag bits
			bytes = c & (0xFF >> (len + 1));
		} //keep only the data, remove the flag.



		//now add in the rest of the bytes
		for (size_t k = 1; k < len; k++) //cant do i, as we alr using that
		{
			Uint32 nextByte = text[i + k] & 0x3F; //grab the 6 data bits from the byte
			bytes = bytes << 6; //move our bytes left 6
			bytes = bytes | nextByte; //inject our nextBytes in that open space
		}


		//make sure the font can draw this, if not, we dont keep
		if (TTF_FontHasGlyph(font, bytes))
		{
			out.append(text, i, len); //keep the bytes!
		}






		i += len; //skip the string



	}

	return out; //end
}

































void PreRender(SDL_Renderer* render, TTF_Font* font)
{
	

	if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size())  return; //if we have no active tabs, ignore

	int xtrack = 20;
	int ytrack = 120; //issues with the y track, now fixed!
	int lasty = -1; //set to -1 for first run so we allways small first run
	int maxLineHeight = 0; //fix clipping

	for (int i = 0; i < tabs[activeTab].layout.size(); i++) {

		//if the item already has a texture
		//we skip so we dont rerender it

		//this old one worked, but didnt handle stuf flike line breaks or tabs cells or colums
		//if (lasty != -1 && mainlayout[i].y > lasty) {
		//	xtrack = 20;

		//	ytrack += (maxLineHeight + 15);
		//	maxLineHeight = 0;
		//}

		//check if we placed not the very first item, but an item, then we check that the y assigned is bigger than the curent, so that means we should be on a new line ish
		if (lasty != -1 && tabs[activeTab].layout[i].y > lasty) //check if this is a colum or line break
		{
			xtrack = 20;
			ytrack += (maxLineHeight + 15);
			maxLineHeight = 0;
		}

		lasty = tabs[activeTab].layout[i].y;
		//if we dedtect this as a cell with a x offset, do that


		//check if this is a table with a colum offset and stuff
		if (tabs[activeTab].layout[i].x > 20)
		{
			//pick whatever is furthest right 
			int colX = (xtrack > tabs[activeTab].layout[i].x) ? xtrack : tabs[activeTab].layout[i].x; //my max dont work, so i had to use google to fix ts
			tabs[activeTab].layout[i].x = colX;
			xtrack = colX;
		}
		else
		{
			//if its normal or whatever
			tabs[activeTab].layout[i].x = xtrack;
		}






		tabs[activeTab].layout[i].y = ytrack;






		//make sure its not an image (because then it draws img)
		if (tabs[activeTab].layout[i].textTex == nullptr && !tabs[activeTab].layout[i].isImage && !tabs[activeTab].layout[i].textAttempted)
		{

			


			//grab the text string from our current mainlayout node
			//the value holds the text.
			std::string text = tabs[activeTab].layout[i].node->tagValue;// Make sure this holds the text payload!

			//we pull the layout i (loop through everything)
			//Layout currentLayout = mainlayout[i];


			if (!text.empty())
			{
				tabs[activeTab].layout[i].textAttempted = true; //set the attempted to true.

				//grab everything the thread will need.
				int fontsize = tabs[activeTab].layout[i].fontSize;
				SDL_Color color = tabs[activeTab].layout[i].textColor;
				bool isLink = !tabs[activeTab].layout[i].href.empty(); //if its true, its false, and if its false, its true


				Layout* currentItem = &tabs[activeTab].layout[i];

			

				gImageThreadPool.enqueue([text, fontsize, color, isLink, currentItem]() {
					
					TTF_Font* threadFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", fontsize); //hold the font for the thread
					

					if (threadFont == nullptr) return; //if its null, end
					
					if(isLink) TTF_SetFontStyle(threadFont, TTF_STYLE_UNDERLINE); //hanlde underline text
					
					SDL_Surface* nodeSurf = TTF_RenderText_Solid(threadFont, text.c_str(), 0, color); //create a temp nodeSerf, to hold it

					
					TTF_CloseFont(threadFont); //done, close it

					if (nodeSurf != nullptr) {
						currentItem->pendingTextSurface = nodeSurf; //upload the surf
					
					}
					
					
				}); //end 


			}
		}

		//if the surf has something
		if (tabs[activeTab].layout[i].pendingTextSurface != nullptr)
		{
			//if it does
			SDL_Surface* nodeSurf = tabs[activeTab].layout[i].pendingTextSurface.exchange(nullptr); //grab the node serf, and make a temp var
			//make sure the node surf worked
			if (nodeSurf != nullptr)
			{
				//assign the properties
				tabs[activeTab].layout[i].width = nodeSurf->w; //assign the width
				tabs[activeTab].layout[i].height = nodeSurf->h; //assign the hight
				tabs[activeTab].layout[i].textTex = SDL_CreateTextureFromSurface(render, nodeSurf); //upload the rendering to the gpu
				SDL_DestroySurface(nodeSurf); //we are done, now that its on gpu
			}
		}

		//DO THE THREAD HERE

		//do somthing similar for images
		//first check if its an image, if it doesnt have a texture, and src has somthing
		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].imageTex == nullptr && tabs[activeTab].layout[i].node->src != "" && !tabs[activeTab].layout[i].imageAttempted)
		{

			tabs[activeTab].layout[i].imageAttempted = true;


			std::string src = tabs[activeTab].layout[i].node->src; //set it to the src value

			
			src = ResolveURL(src, urlInput);
			std::cout << "[IMG] Resolved URL: " << src << std::endl;

			std::cout << "Download IMG" << std::endl;


			Layout* currentItem = &tabs[activeTab].layout[i];

			//make the thread
			gWorkerPool.enqueue([src, currentItem]() {

				//download the bytes
				std::vector<unsigned char> bytes = DownloadImages(src);
				if (!bytes.empty())
				{

					//load from mem using the SDL3_IMG
												 //grab the bytes, and the size of it
					SDL_IOStream* io = SDL_IOFromMem(bytes.data(), (int)bytes.size());
					if (io != nullptr)
					{
						//make a serf for it
						SDL_Surface* imageSurf = IMG_Load_IO(io, 1); //one closes the io after
						if (imageSurf != nullptr) //make sure its made correctly
						{
							currentItem->pendingSurface = imageSurf;
							std::cout << "IMG Downloaded into RAM" << std::endl;
						}
						else {
							std::cout << "ERROR - Trying To Download IMG" << std::endl;

							std::cout << "FROM THIS - " << urlInput << std::endl;
						}
					}
				}
				else {
					std::cout << "ERROR - No image detected" << std::endl;
				}






			});
		}




		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].pendingSurface != nullptr)
		{
			SDL_Surface* surf = tabs[activeTab].layout[i].pendingSurface.exchange(nullptr); //load the surf

			if (surf != nullptr)
			{
				tabs[activeTab].layout[i].imageTex = SDL_CreateTextureFromSurface(render, surf);

				//save the sizes of it now
				tabs[activeTab].layout[i].width = surf->w;
				tabs[activeTab].layout[i].height = surf->h;

				//clear the surf for performace
				SDL_DestroySurface(surf);

				std::cout << "IMG Downloaded" << std::endl;

			}
		}
			


		TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

		if (tabs[activeTab].layout[i].height > maxLineHeight)
		{
			maxLineHeight = tabs[activeTab].layout[i].height;
		}



		xtrack += (tabs[activeTab].layout[i].width + 12);


	}



}












int LoadAnimation(SDL_Renderer* render, TTF_Font* font)
{
	//reset everything, as we want to make sure we dont have old stuff
	if (!tabs.empty() && activeTab >= 0 && activeTab < (int)tabs.size())
	{

		//handle each item in the layout, we want to remove, and reset everything.
		for (auto& item : tabs[activeTab].layout)
		{
			//destroy text and images
			if (item.textTex) { SDL_DestroyTexture(item.textTex); item.textTex = nullptr; }
			if (item.imageTex) { SDL_DestroyTexture(item.imageTex); item.imageTex = nullptr; }

			item.textAttempted = false; //reset
			item.imageAttempted = false; //reset
			item.pendingTextSurface = nullptr; //reset
		}
		tabs[activeTab].layout.clear(); //clear

		if (tabs[activeTab].domRoot != nullptr)
		{
			DeleteTree(tabs[activeTab].domRoot); //destory the old tree
			tabs[activeTab].domRoot = nullptr;
		}

	}


	//now paint the blank frame
	if (darkmode) //darkmode (inverse)
	{
		SDL_SetRenderDrawColor(render, (255 - backgroundColor.r), (255 - backgroundColor.g), (255 -backgroundColor.b), 255);
	}
	else {
		SDL_SetRenderDrawColor(render, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);
	}
	
	SDL_RenderClear(render);


	//render the loading thing
	if (loadingTex != nullptr) //make sure the loadingTex works.
	{
		int WinW, WinH;
		SDL_GetCurrentRenderOutputSize(render, &WinW, &WinH); //get the size of the render

		float texW, texH;
		SDL_GetTextureSize(loadingTex, &texW, &texH);


		SDL_FRect loadingRect;
		loadingRect.x = 10;
		loadingRect.y = 48;
		loadingRect.w = texW;
		loadingRect.h = texH;

		SDL_RenderTexture(render, loadingTex, nullptr, &loadingRect);
	}












	
	SDL_RenderPresent(render);

	return 0;

}




//SET THE TAB TITLE

void SetTabTitle(std::string title)
{
	tabs[activeTab].title = title;
}






void NavigateTo(std::string target, SDL_Renderer* render, TTF_Font* font, bool addToHistory = true) //handle links, as they can sometimes have /path. simmilar to Resolve url
{
	if (target.empty()) return;

	//check if its a search or a website, because the old error was that somtimes we would search websites
	std::string resolvedTarget = target; //make a string to change and modify as we flush out our link.

	//its a url if any of these conditions are met, aswell as no spaces.
	bool isUrl = (target.find("http://") == 0 || target.find("https://") == 0 || target[0] == '/' || target.find('.') != std::string::npos) && (target.find(' ') == std::string::npos);

	if (!isUrl) //if it not a url
	{
		//its a search.
		std::string query = target;
		std::replace(query.begin(), query.end(), ' ', '+'); //replace all ' ' with +
		resolvedTarget = "lite.duckduckgo.com/lite/?q=" + query; //combine it
	}
	else { //it is a url!
		resolvedTarget = ResolveURL(target, tabs[activeTab].url);
	}

	

	if (addToHistory) {


		//if we went back and searched somthing new, deleate that multiverse
		if (tabs[activeTab].historypos < (int)tabs[activeTab].history.size() - 1) {
			tabs[activeTab].history.resize(tabs[activeTab].historypos + 1);
		}
		//prevent duplicates
		if (tabs[activeTab].history.empty() || tabs[activeTab].history.back() != resolvedTarget)
		{
			tabs[activeTab].history.push_back(resolvedTarget);
			tabs[activeTab].historypos = tabs[activeTab].history.size() - 1; // Keep index at the end
		}

	}

	tabs[activeTab].layout.clear();

	//make a temp layout that pretends to be a parsed thing
	Layout loadingMSG;
	loadingMSG.x = 10;
	loadingMSG.y = 150;
	loadingMSG.fontSize = 72;
	loadingMSG.isImage = false;

	//make the texture
	SDL_Color loadColor;
	if (darkmode)
	{
		loadColor = SDL_Color{ 255, 255, 255, 255 };
	}
	else {
		loadColor = SDL_Color{ 0, 0, 0, 255 };
	}
	//make a surf
	int ogFontSize = TTF_GetFontSize(font);
	TTF_SetFontSize(font, 72);
	SDL_Surface* loadSurf = TTF_RenderText_Solid(font, "Loading...", 0, loadColor);

	TTF_SetFontSize(font, ogFontSize);
	if (loadSurf != nullptr)
	{
		loadingMSG.textTex = SDL_CreateTextureFromSurface(render, loadSurf);
		loadingMSG.width = loadSurf->w;
		loadingMSG.height = loadSurf->h;
		SDL_DestroySurface(loadSurf);
	}
	tabs[activeTab].layout.push_back(loadingMSG);
	//update the active tab
	tabs[activeTab].url = resolvedTarget;
	urlInput = resolvedTarget;
	std::wstring wUrl(resolvedTarget.begin(), resolvedTarget.end());
	gWorkerPool.enqueue([wUrl]() {
		ConnectSocketHTTPS(wUrl);
	});
	
}












//ok, this fixes duck duck go redirect strings!
std::string FixURLREDIRECT(const std::string& href)
{
	//first find our uddg=, we want to remove that
	size_t uddgPos = href.find("uddg=");
	//if we cant find that, stop
	if (uddgPos == std::string::npos) return "";

	size_t start = uddgPos + 5; // skip "uddg="


	//hold our fixed value
	std::string encoded = "";

	//ok, starting at the start pos, we go till the end
	for (int i = start; i < href.size(); i++)
	{
		//ok, we hit a new parram, we dont want to put that into our main loop, so we stop!
		if (href[i] == '&')
		{
			break;
		}

		//else, just keep going
		encoded += href[i];
	}

	return PercentDecode(encoded);
}





int GUIRENDER()
{
	//to render to a texture, we need to make a texture for the font
	//we make a font to hold it
	static TTF_Font* font = nullptr;

	//for any standlone text ouside the layout system
	//we dont use it rn, but for future use i might
	SDL_Texture* fontText = nullptr;


	//returns a true or false val if it worked.
	//we want to use the screen so use video
	//we prob wont using audio, but we can through | SDL_INIT_AUDIO
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		//if we get false (it didnt work)
		std::cout << "Failed to init SDL" << std::endl;
		return -1;
	}

	//added a pointer to it, so that we can accses it later
	SDL_Window* window;
	//create window, some parrams are 
	//title, width, height, and flags
	//make it 1080p sizewise
	//for flags, its in this format -> UINT64_C(0X0000000000000020), we also can add more tags through |
	window = SDL_CreateWindow("Browse++", 1920, 1080, SDL_WINDOW_RESIZABLE);

	SDL_Renderer* render = SDL_CreateRenderer(window, nullptr);

	SDL_SetRenderVSync(render, 1); //enable VSync, to keep performace high!

	//tell SDL we want to record SDL_EVENT_TEXT_INPUTs when the user types
	SDL_StartTextInput(window); //detect inputs from text

	//We need this to render fonts,and needs to be called before the openfont
	//INIT SDL TTF
	if (!TTF_Init())
	{
		//if we fail
		std::cout << "Failed to init SDL::TTF" << std::endl;
		return -1;
	}


	//Now that we got it init, lets render that font
	//open the font file from disk.
	//16 is the defalt size (we change this in PreRender)
	font = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 16);
	TTF_Font* iconFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 28); // For icons
	TTF_Font* reloadFont = TTF_OpenFont("./fonts/PixelifySans-edited.ttf", 72); // For icons
	//make sure it worked
	if (font == nullptr)
	{
		std::cout << "Failed to open TTF" << std::endl;
		return -1;
	}
 

	//lets make the text, we set the font, the info we want to render, length, and finaly color
	if (darkmode) //if darkmode
	{
		SDL_Surface* textserf = TTF_RenderText_Solid(font, "Browse++", 0, SDL_Color(0, 0, 0, 0)); //draw black
		fontText = SDL_CreateTextureFromSurface(render, textserf);

		//clear and remove it
		SDL_DestroySurface(textserf); //done wiht the text serface


		//make the loading texture
		SDL_Color loadingColor = { 255,255,255,255 }; //WHITE
		SDL_Surface* loadingserf = TTF_RenderText_Solid(reloadFont, "Loading...", 0, loadingColor); //make the surface
		if (loadingserf != nullptr)
		{
			loadingTex = SDL_CreateTextureFromSurface(render, loadingserf);
			SDL_DestroySurface(loadingserf);
		}
	}
	else //if lightmode.
	{
		SDL_Surface* textserf = TTF_RenderText_Solid(font, "Browse++", 0, SDL_Color(255, 255, 255, 0));
		fontText = SDL_CreateTextureFromSurface(render, textserf);

		//clear and remove it
		SDL_DestroySurface(textserf); //done wiht the text serface


		//make the loading texture
		SDL_Color loadingColor = { 0,0,0,255 }; //Black
		SDL_Surface* loadingserf = TTF_RenderText_Solid(reloadFont, "Loading...", 0, loadingColor); //make the surface
		if (loadingserf != nullptr)
		{
			loadingTex = SDL_CreateTextureFromSurface(render, loadingserf);
			SDL_DestroySurface(loadingserf);
		}
	}
	






	//shows events like window changes and stuff
	SDL_Event event; //we make an SDL event handler

	//i added this, because my padding and stuff kept moving and it was super annoying!
	const float btnSize = 30.0f;   // Keep button size constant
	const float padding = 15.0f;   // alwasy 10px of space
	const float topMargin = 37.0f; // dist from the top of the window

	bool running = true;

	//main loop
	while (running)
	{





		//like pygame python, we check to see if we have gotten our x pressed, and if we have we stop it
		while (SDL_PollEvent(&event))
		{
			//if the window x is pressed
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

			//if we detect the wheel scrolled
			if (event.type == SDL_EVENT_MOUSE_WHEEL)
			{
				//scrolling up, but instead of 1px, its 80!
				tabs[activeTab].scrollpos -= event.wheel.y * 80;
				if (tabs[activeTab].scrollpos < 40)
				{
					tabs[activeTab].scrollpos = 40;
				}
				if (tabs[activeTab].scrollpos > tabs[activeTab].maxscroll)
				{
					tabs[activeTab].scrollpos = tabs[activeTab].maxscroll;
				}
			}

			//detect text inputs, and add them to our input str
			if (event.type == SDL_EVENT_TEXT_INPUT)
			{
				urlInput += event.text.text; //append to whatever they typed(aka allowing a textbox)
			}

			//check for keys down
			if (event.type == SDL_EVENT_KEY_DOWN)
			{
				//if we detect backspace, we remove the last character
				if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !urlInput.empty())
				{
					urlInput.pop_back();
				}

				//if we press enter to search
				if (event.key.scancode == SDL_SCANCODE_RETURN)
				{
					//std::cout << "going to: " << urlInput << std::endl; //DEBUG


					//ok lets make this work.


					NavigateTo(urlInput, render, font, true);



				}

				//moved it in to prevent mutliple input detection
				//get ctrl c + v
				if (event.key.scancode == SDL_SCANCODE_V && (event.key.mod & SDL_KMOD_CTRL))
				{
					//grab the clipboard, thankfully SDL got me!
					const char* clipboard = SDL_GetClipboardText();

					//check if its not null, as then we would break the loop
					if (clipboard != nullptr)
					{
						urlInput += clipboard;
					}

				}
			}

			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
			{
				//check if our left mouse pressed
				if (event.button.button == SDL_BUTTON_LEFT) {

					float mouseX = event.button.x;
					float mouseY = event.button.y;

					//check the main 2 buttons

					int WinW, WinH;

					SDL_GetWindowSize(window, &WinW, &WinH);

					

					SDL_FRect backBtnRect = { padding, topMargin, btnSize, btnSize };
					SDL_FRect fwdBtnRect = { backBtnRect.x + backBtnRect.w + padding, topMargin, btnSize, btnSize };
					SDL_FRect reloadBtnRect = { fwdBtnRect.x + fwdBtnRect.w + padding, topMargin, btnSize, btnSize };
					SDL_FRect homeBtnRect = { reloadBtnRect.x + reloadBtnRect.w + padding, topMargin, btnSize, btnSize };
					
					//we need to do this, or else our button dont work!
					float searchX = homeBtnRect.x + homeBtnRect.w + padding;
					float searchW = (float)WinW - btnSize - padding - padding - searchX - (btnSize * 2);
					SDL_FRect searchBarRect = { searchX, topMargin, searchW, btnSize };

				
					SDL_FRect starBtnRect = { searchBarRect.x + searchBarRect.w + padding, topMargin, btnSize, btnSize };
					SDL_FRect printerBtnRect = { starBtnRect.x + starBtnRect.w + padding, topMargin, btnSize, btnSize };

					//test if the backbutton is pressed
					if (mouseX >= backBtnRect.x && mouseX <= (backBtnRect.x + backBtnRect.w) &&
						mouseY >= backBtnRect.y && mouseY <= (backBtnRect.y + backBtnRect.h)) {

						

						
						//first check, is our index var for the area >= 0? (we need this so we dont have an error
						if (tabs[activeTab].historypos > 0) //we do > than 0, as we dont want to have an error
						{
							tabs[activeTab].historypos--;
							std::string prevURL = tabs[activeTab].history[tabs[activeTab].historypos];
							urlInput = prevURL;
							
							NavigateTo(prevURL, render, font, false);
						}
					}

					if (mouseX >= fwdBtnRect.x && mouseX <= (fwdBtnRect.x + fwdBtnRect.w) &&
						mouseY >= fwdBtnRect.y && mouseY <= (fwdBtnRect.y + fwdBtnRect.h)) {


						//std::cout << "FORWARD " << currentSearchPos << std::endl;
						if (tabs[activeTab].historypos < (int)tabs[activeTab].history.size() - 1) //we do > than 0, as we dont want to have an error
						{
							tabs[activeTab].historypos++;
							std::string nextURL = tabs[activeTab].history[tabs[activeTab].historypos];
							urlInput = nextURL;
							NavigateTo(nextURL, render, font, false);
							//ok, now, get the thing
						}
					}




					if (mouseX >= reloadBtnRect.x && mouseX <= (reloadBtnRect.x + reloadBtnRect.w) &&
						mouseY >= reloadBtnRect.y && mouseY <= (reloadBtnRect.y + reloadBtnRect.h)) {
						//reload

						//we need to load a white screen, just so people know it has been reloaded
						if (urlInput != "")
						{

							//we load the anim, render it and stuff!
							//LoadAnimation(render, font);

							NavigateTo(urlInput, render, font, false);
						}

					}


					//handle the new home buttn
					if (mouseX >= homeBtnRect.x && mouseX <= (homeBtnRect.x + homeBtnRect.w) &&
						mouseY >= homeBtnRect.y && mouseY <= (homeBtnRect.y + homeBtnRect.h)) {

						std::ifstream file("main.html");

						if (!file.is_open()) {
							std::cout << "Could not open local file." << std::endl;
							return -1;
						}
						//we dump the file into a buffer
						LoadAnimation(render, font);
						std::stringstream buffer;

						//load it in 
						buffer << file.rdbuf();

						//now we load the full file into a temp var
						//we use the buffer and convert it into a string
						std::string fileinfo = buffer.str();

						//now we do something diffrent, we just inject it right into the parser to have the same effect
						Parser(fileinfo);

						//lets clear the url input too
						urlInput = "";
					}


					//handle the print button
					if (mouseX >= printerBtnRect.x && mouseX <= (printerBtnRect.x + printerBtnRect.w) &&
						mouseY >= printerBtnRect.y && mouseY <= (printerBtnRect.y + printerBtnRect.h)) {

						//first we capture the entire render!
						//then we crop it, as we dont want everything yk

						int uiHeight = 80;

						SDL_Rect contentArea; //create the contetent area

						//x,w, h we leave, but, y we adjust
						contentArea.x = 0;
						contentArea.y = uiHeight;
						contentArea.w = WinW;
						contentArea.h = WinH - uiHeight;





						SDL_Surface* screenshot = SDL_RenderReadPixels(render, &contentArea);

					

						//we need to make sure that this hasnt failed
						if (screenshot != nullptr)
						{
							std::cout << "Printing..." << std::endl;

							std::string saveFolder = "Printed_Pages";
							std::filesystem::create_directories(saveFolder);

							//make a char
							//we need a string first
							//we use + std::to_string(SDL_GetTicks()), because it wont want to print more than once in a loop with the same name
							//i used to have the name of the site, but sites with / and stuff dont work, so i just make it Tab
							std::string filenameStr = saveFolder + "\\Tab_print_" + std::to_string(SDL_GetTicks()) + ".bmp";

							//then into the char
							const char* filename = filenameStr.c_str();

							SDL_SaveBMP(screenshot, filename);
							//now we run the print command
							std::string paintParams = "\"" + filenameStr + "\"";
							// Ask Windows to handle the printing, but SHOW the menu normally
							ShellExecuteA(NULL, "print", filename, NULL, NULL, SW_SHOWNORMAL);


							//we are done here, destory the screenshot
							SDL_DestroySurface(screenshot);
						}

					}

					//handle the star button!

					//ok this is my plan for the star button, we save the stars, to a txt file, its a toggle, so it saves between loads of the browser, through a custom file (idk what yet tho)
					//we update the saved links in the https main thing, so its easy to get back to them, and a uncheck removes them, udpates the main.html, and continues!
					if (mouseX >= starBtnRect.x && mouseX <= (starBtnRect.x + starBtnRect.w) &&
						mouseY >= starBtnRect.y && mouseY <= (starBtnRect.y + starBtnRect.h)) {

						//first we get the current url
						std::string currentSite = currentURL;

						//first, we should check our database to see if its alr starred

						auto it = std::find(starredPages.begin(), starredPages.end(), currentSite); //find it!


						if (it != starredPages.end())
						{

							//we found it, so we toggle it off
							starredPages.erase(it);
							std::cout << currentSite << " Removed from stars!" << std::endl;

						}
						else {
							//it was not found, so that means we are toggling in on
							starredPages.push_back(currentSite);
							std::cout << currentSite << " Added to stars!" << std::endl;
						}


						//then lets handle text file that holds it
						//we should just wipe it for cleanness
						std::ofstream bookmark("starred_pages.STAR", std::ios::trunc);
						if (bookmark.is_open())
						{
							//go through each site and add the star pages
							for (const std::string& site : starredPages)
							{
								bookmark << site << "\n";
							}
							//close clean
							bookmark.close();
						}

						UpdateHTML();

					}





					for (int i = 0; i < tabs[activeTab].layout.size(); i++)
					{
						//skip ones that arnt links
						if (tabs[activeTab].layout[i].textTex == nullptr) continue;
						if (tabs[activeTab].layout[i].href.empty()) continue;

						//ok we have a link, now lets adjust the activator block
						float screeny = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos;

						//calculate if we are over it
						if (mouseX >= tabs[activeTab].layout[i].x && mouseX <= (tabs[activeTab].layout[i].x + tabs[activeTab].layout[i].width) &&
							mouseY >= screeny && mouseY <= (screeny + tabs[activeTab].layout[i].height))
						{



							std::cout << "link pressed" << std::endl;




							std::string finalUrl = tabs[activeTab].layout[i].href;


							std::string realDest = FixURLREDIRECT(finalUrl);
							if (!realDest.empty())
							{
								finalUrl = realDest;
							}
							
							urlInput = finalUrl;
							NavigateTo(finalUrl, render, font, true);

							break;

						}
					} 


					//now we settup button detection for the tabs
					if (mouseY < 30) //instead of checking tabs every time (thats super slow lol) we just first check if its in the right spot
					{
						//we gotta loop through each part, so lets do the same thing
						int tabX = 0;
						int tabW = 180; //starting vals



						for (int t = 0; t < tabs.size(); t++)
						{
							//first check if the x button was clicked
							float closeX = tabX + tabW - 20;

							//check for intersects
							if (mouseX >= closeX && mouseX <= closeX + 14 && mouseY >= 2 && mouseY <= 28 && tabs.size() > 1)
							{
								if (tabs[t].domRoot != nullptr)
								{
									DeleteTree(tabs[t].domRoot);
								}
							
								//ok close the tab
								tabs.erase(tabs.begin() + t); //remove the tab

								//make sure we cant make it -
								//because if we attempt to assgin an active tab to -1, it will error (happend so much lol)
								if (activeTab >= (int)tabs.size())
								{
									activeTab = tabs.size() - 1;
								}

								//if we end up closing, and we got NO active tabs, we just make one (like how google does)
								if (tabs.empty())
								{
									tabs.push_back(Tab());
									activeTab = 0;
								}



								//update the urls to the tab, this saves time and s
								currentURL = tabs[activeTab].url;
								urlInput = tabs[activeTab].url;



								break; //we are done (saves performace)
							}


							//check if the tab was clicked
							if (mouseX >= tabX && mouseX <= tabX + tabW)
							{
								activeTab = t;

								urlInput = tabs[activeTab].url;
								break;
							}

							//move to the next tab
							tabX += tabW + 2;


						}

						//check for a new tab made
						int newTabX = tabs.size() * (tabW + 2);
						if (mouseX >= newTabX && mouseX <= newTabX + 26)
						{
							Tab newTab;
							newTab.title = "New Tab";
							//push it back
							tabs.push_back(newTab);
							//update the active tab
							activeTab = tabs.size() - 1;
							//reset the url input
							urlInput = "";

							std::ifstream file("main.html");

							if (!file.is_open()) {
								std::cout << "Could not open local file." << std::endl;
								return -1;
							}
							//we dump the file into a buffer

							std::stringstream buffer;

							//load it in 
							buffer << file.rdbuf();

							//now we load the full file into a temp var
							//we use the buffer and convert it into a string
							std::string fileinfo = buffer.str();

							//now we do something diffrent, we just inject it right into the parser to have the same effect
							Parser(fileinfo);

							//load it early!


						}















					}

































				}
			}



		}


		//this will auto load the textures, and only change if we get a new layout in
		PreRender(render, font);

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

		//// 3. Present the screen
		//SDL_RenderPresent(render);
		if (tabs[activeTab].layout.empty() && tabs[activeTab].url != "" && tabs[activeTab].url != "main.html")
		{
			
		}

		//draw the page by looping thorugh the layout list
		for (int i = 0; i < tabs[activeTab].layout.size(); i++)
		{
			//render our text, we make a rectange and assign our text to that!
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
					SDL_SetRenderDrawColor(render, (255 - tabs[activeTab].layout[i].bgColor.r), (255 - tabs[activeTab].layout[i].bgColor.g), (255 - tabs[activeTab].layout[i].bgColor.b),  255); //255 cause we dont want it transparent
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


		//icons! Home, Reload, Back+Forth arrows, search, star, printer
		//љ њ ђ ы ж ξ

		int WinW, WinH;
		SDL_GetWindowSize(window, &WinW, &WinH);

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
		SDL_Surface* backSurf = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor);
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
		SDL_Surface* forwardSurf = TTF_RenderText_Solid(iconFont, "ђ", 0, textColor);
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

		SDL_Surface* reloadSurf = TTF_RenderText_Solid(iconFont, "њ", 0, textColor);
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
		

		
		
		SDL_Surface* homeSurf = TTF_RenderText_Blended(iconFont, "љ", 0, textColor);
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
				float cursorX = bar.x + 8 + urlSurf->w + 1;
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
		SDL_Surface* starSurf = TTF_RenderText_Solid(iconFont, "ж", 0, textColor);
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
		SDL_Surface* printerSurf = TTF_RenderText_Solid(iconFont, "ξ", 0, textColor);
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
		if (darkmode) //darkmode
		{
			plusSurf = TTF_RenderText_Solid(font, "+", 0, { 255,255,255,255 });
		}
		else {
			plusSurf = TTF_RenderText_Solid(font, "+", 0, { 0,0,0,255 });
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

