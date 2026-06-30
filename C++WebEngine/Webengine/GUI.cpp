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



//MOVED TO TAB MODE.

//this is the layout list, and gets filled in by IMPORT
//the render loop reads from this 
std::vector<Layout> mainlayout;



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








bool urlBarFocused = true;    // currently we have no other inputs, so we can always have this focused!
SDL_Color backgroundColor = { 245, 245, 245, 255 }; //white bg, gets changed btw


//this is called in layout, and repaces whatever was on the screen with the new layout
int IMPORT(std::vector<Layout> layoutGOTTEN)
{
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





	tabs[activeTab].layout = layoutGOTTEN;
	tabs[activeTab].url = urlInput;
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

// added this above PreRender
std::string ResolveURL(std::string src, std::string currentUrl)
{
	std::string decoded = ""; //thing to hold decoded

	src = PercentDecode(src);
	//we gotta do a lot of things to get the image url right, as images got the weirdest urls oat

	size_t i = 0;
	//same loop we always do
	while (i < src.size())
	{
		//remove the &amps;
		if (src.substr(i, 5) == "&amp;")
		{
			decoded += "&"; //add a & to replace it
			i += 5;
		}
		else {
			decoded += src[i++]; //just add the normal val to the decoded
		}
	}
	src = decoded;

	//if its good use as is
	if (src.find("http://") == 0 || src.find("https://") == 0) return src;


	//make sure the url has a https:// on it
	std::string base = currentUrl;
	if (base.find("://") == std::string::npos)
	{
		//if we dont have it, we add it
		base = "https://" + base;
	}

	//get the domain part, not the full link
	std::string domain = "";
	size_t protoend = base.find("://");
	//if we find the protoend
	if (protoend != std::string::npos)
	{
		size_t domainEnd = base.find("/", protoend + 3); //get the end of it
		if (domainEnd != std::string::npos)
		{
			domain = base.substr(0, domainEnd); //get it like https://Example.com
		}
		else {
			domain = base; //no path, this is the domain
		}
	}


	//get the direc of the current page for the paths
	std::string direcotry = domain;
	size_t lastSlash = base.rfind("/");
	size_t protoSlash = base.find("://");

	//make sure the last slash isnt appart of the ://
	if (lastSlash != std::string::npos && lastSlash > protoSlash + 3)
	{
		direcotry = base.substr(0, lastSlash);
	}


	//handle absolute paths like /assets/example.png -> https://Example.coom/assets/example.com
	if (src[0] == '/')
	{
		return domain + src;
	}


	//reletive path

	return direcotry + "/" + src;






}

void PreRender(SDL_Renderer* render, TTF_Font* font)
{
	if (tabs.empty() || activeTab < 0 || activeTab >= (int)tabs.size()) return; //if we have no active tabs, ignore

	int xtrack = 20;
	int ytrack = 40;
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
		if (tabs[activeTab].layout[i].textTex == nullptr && !tabs[activeTab].layout[i].isImage)
		{
			//grab the text string from our current mainlayout node
			//the value holds the text.
			std::string text = tabs[activeTab].layout[i].node->tagValue;// Make sure this holds the text payload!

			//we pull the layout i (loop through everything)
			//Layout currentLayout = mainlayout[i];
			if (!text.empty())
			{

				//we set the fonts to diffrent sizes, that our layout dom tree already does!
				TTF_SetFontSize(font, tabs[activeTab].layout[i].fontSize);

				if (!tabs[activeTab].layout[i].href.empty()) {
					TTF_SetFontStyle(font, TTF_STYLE_UNDERLINE);
				}
				else {
					TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
				}


			
				//Create the surface (using black text)
				SDL_Surface* nodeSurf = TTF_RenderText_Solid(font, text.c_str(), 0, tabs[activeTab].layout[i].textColor);

				//Make sure the surface was created successfully
				if (nodeSurf != nullptr) {

					////make a texture and update it
					//SDL_Texture* nodeTex = SDL_CreateTextureFromSurface(render, nodeSurf);

					//SDL_FRect textRec;
					//textRec.x = currentLayout.x;
					//textRec.y = currentLayout.y - scrollpos;


					//save the dimetions of the block of text
					tabs[activeTab].layout[i].width = nodeSurf->w;
					tabs[activeTab].layout[i].hight = nodeSurf->h;

					//upload the surface from ram to gpu as a texture!
					// Draw and destroy to prevent mem leaks
					tabs[activeTab].layout[i].textTex = SDL_CreateTextureFromSurface(render, nodeSurf); //send it to the gpu

					//free the cpu to avoid ram leaks!
					SDL_DestroySurface(nodeSurf);
				}

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
			std::thread([src, currentItem]() {

				//download the bytes
				std::vector<unsigned char> bytes = DownloadBytes(src);
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






			}).detach();
		}




		if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].pendingSurface != nullptr)
		{
			SDL_Surface* surf = tabs[activeTab].layout[i].pendingSurface.exchange(nullptr); //load the surf

			if (surf != nullptr)
			{
				tabs[activeTab].layout[i].imageTex = SDL_CreateTextureFromSurface(render, surf);

				//save the sizes of it now
				tabs[activeTab].layout[i].width = surf->w;
				tabs[activeTab].layout[i].hight = surf->h;

				//clear the surf for performace
				SDL_DestroySurface(surf);

				std::cout << "IMG Downloaded" << std::endl;

			}
		}
			


		TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

		if (tabs[activeTab].layout[i].hight > maxLineHeight)
		{
			maxLineHeight = tabs[activeTab].layout[i].hight;
		}



		xtrack += (tabs[activeTab].layout[i].width + 12);


	}



}












int LoadAnimation(SDL_Renderer* render, TTF_Font* font)
{
	std::ifstream file("reload.html");

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





	//now we force render it
	PreRender(render, font);


	//set the background
	SDL_SetRenderDrawColor(render, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);
	SDL_RenderClear(render); //clean the render

	//safty check so no crash
	if (!tabs.empty() && activeTab >= 0 && activeTab < (int)tabs.size())
	{
		//force draw the new thing
		for (int i = 0; i < tabs[activeTab].layout.size(); i++)
		{
			if (tabs[activeTab].layout[i].textTex == nullptr) continue; //skip if we have somthing thats nullptr, no texture

			//render our text, we make a rectange and assign our text to that!
			SDL_FRect textrec;
			textrec.x = tabs[activeTab].layout[i].x;
			textrec.y = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos;
			textrec.w = tabs[activeTab].layout[i].width;
			textrec.h = tabs[activeTab].layout[i].hight;

			//draw the bg color
			if (tabs[activeTab].layout[i].hasBg)
			{
				SDL_SetRenderDrawColor(render, tabs[activeTab].layout[i].bgColor.r, tabs[activeTab].layout[i].bgColor.g, tabs[activeTab].layout[i].bgColor.b, 255); //255 cause we dont want it transparent
				//fill the rec
				SDL_RenderFillRect(render, &textrec);
			}

			//check if we should render an image
			if (tabs[activeTab].layout[i].isImage && tabs[activeTab].layout[i].imageTex != nullptr)
			{
				SDL_RenderTexture(render, tabs[activeTab].layout[i].imageTex, nullptr, &textrec);
			}
			// 2. Otherwise, paint the text
			else if (tabs[activeTab].layout[i].textTex != nullptr)
			{
				SDL_RenderTexture(render, tabs[activeTab].layout[i].textTex, nullptr, &textrec);
			}

		}
	}
	SDL_RenderPresent(render);

	return 0;

}




//SET THE TAB TITLE

void SetTabTitle(std::string title)
{
	tabs[activeTab].title = title;
}







int search(std::string input, bool addToHistory, SDL_Renderer* render, TTF_Font* font, int CurrentTab)
{
	const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
	const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

	LoadAnimation(render, font);










	//check if this is a valid url
	//note, this will assume that this is a valid url, if it follows the design scheme, but it may not be, so we then do a network test on the server (attempt to check its status)
	if (std::regex_match(input, httpPattern)) {
		std::cout << "http url!" << std::endl;
		ConnectSocketHTTP(input);


		//SearchHistory.push_back(input);
		//now that we understand its a valid url, lets attempt a socket connect.
		//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
	}


	if (std::regex_match(input, httpsPattern)) {
		std::cout << "https url!" << std::endl;
		std::wstring temp(input.begin(), input.end());
		ConnectSocketHTTPS(temp);


		//SearchHistory.push_back(input);
		//now that we understand its a valid url, lets attempt a socket connect.
		//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
	}

	// --- Inside search() in GUI.cpp ---
	if (!std::regex_match(input, httpPattern) && !std::regex_match(input, httpsPattern)) {
		//if we dont match https or http, its prob a search, so lets do that
		std::string query = "";


		//ok if its a space, we repace it with a +

		for (int i = 0; i < input.size(); i++)
		{
			//std::cout << "Attempting search " << input << std::endl;

			if (input[i] == ' ')
			{
				query += "+";
			}
			else {
				query += input[i];
			}
		}

		input = "lite.duckduckgo.com/lite/?q=" + query;
		urlInput = input;
		//send it to https
		std::wstring temp(input.begin(), input.end());
		ConnectSocketHTTPS(temp);
	}

	if (addToHistory) {

		
		//if we went back and searched somthing new, deleate that multiverse
		if (tabs[activeTab].currentSearchPos < (int)tabs[activeTab].SearchHistory.size() - 1) {
			tabs[activeTab].SearchHistory.resize(tabs[activeTab].currentSearchPos + 1);
		}

		tabs[activeTab].SearchHistory.push_back(input);
		tabs[activeTab].currentSearchPos = tabs[activeTab].SearchHistory.size() - 1; // Keep index at the end
	}



	return 0;
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
	//title, width, hight, and flags
	//make it 1080p sizewise
	//for flags, its in this format -> UINT64_C(0X0000000000000020), we also can add more tags through |
	window = SDL_CreateWindow("Browse++", 1920, 1080, SDL_WINDOW_RESIZABLE);

	SDL_Renderer* render = SDL_CreateRenderer(window, nullptr);



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
	font = TTF_OpenFont("./fonts/PixelifySans-Regular.ttf", 16);
	//make sure it worked
	if (font == nullptr)
	{
		std::cout << "Failed to open TTF" << std::endl;
		return -1;
	}


	//lets make the text, we set the font, the info we want to render, length, and finaly color

	SDL_Surface* textserf = TTF_RenderText_Solid(font, "Browse++", 0, SDL_Color(255, 255, 255, 0));
	fontText = SDL_CreateTextureFromSurface(render, textserf);

	//clear and remove it
	SDL_DestroySurface(textserf); //done wiht the text serface



	//shows events like window changes and stuff
	SDL_Event event; //we make an SDL event handler




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


					std::string input = urlInput;
					//First, we need to prompt the user, what URL are they attempting to connect too?
					//std::cout << "Type the URL to open it: "; //CAN NOW BE HTTPS!!!!

					//pulls the input, then creates a new line for cleaness
					//std::cin >> input; std::cout << std::endl;


					//these 2 blocks autodetect if its a https or http.
					//std::thread([input]()
					//	{

						//}).detach();

					search(urlInput, true, render, font, activeTab);



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

					float barwidth = 1200; //the width of the bar
					float barx = (WinW - barwidth) / 2; //we use this to align the triggers

					SDL_FRect backbuttontrigger = { barx - 40, 35 , 30, 30 }; //pos
					SDL_FRect forwardbuttontrigger = { barx + barwidth + 10, 35, 30, 30 }; //pos
					SDL_FRect reloadbuttontrigger = { barx - 80,35,30,30 }; //pos


					//test if the backbutton is pressed
					if (mouseX >= backbuttontrigger.x && mouseX <= (backbuttontrigger.x + backbuttontrigger.w) &&
						mouseY >= backbuttontrigger.y && mouseY <= (backbuttontrigger.y + backbuttontrigger.h)) {

						

						
						//first check, is our index var for the area >= 0? (we need this so we dont have an error
						if (tabs[activeTab].currentSearchPos > 0) //we do > than 0, as we dont want to have an error
						{
							tabs[activeTab].currentSearchPos = tabs[activeTab].currentSearchPos - 1; //remove 1 from the searchpos
							//std::cout << "BACK " << currentSearchPos << std::endl;
							//ok, now, get the thing

							urlInput = tabs[activeTab].SearchHistory[tabs[activeTab].currentSearchPos];


							search(urlInput, false, render, font, activeTab);


						}
					}

					if (mouseX >= forwardbuttontrigger.x && mouseX <= (forwardbuttontrigger.x + forwardbuttontrigger.w) &&
						mouseY >= forwardbuttontrigger.y && mouseY <= (forwardbuttontrigger.y + forwardbuttontrigger.h)) {


						//std::cout << "FORWARD " << currentSearchPos << std::endl;
						if (!tabs[activeTab].SearchHistory.empty() && tabs[activeTab].currentSearchPos < (int)tabs[activeTab].SearchHistory.size() - 1) //we do > than 0, as we dont want to have an error
						{
							tabs[activeTab].currentSearchPos = tabs[activeTab].currentSearchPos + 1; //add 1 from the searchpos
							//std::cout << "FORWARD " << currentSearchPos << std::endl;

							urlInput = tabs[activeTab].SearchHistory[tabs[activeTab].currentSearchPos];

							//load it now!
							search(urlInput, false, render, font, activeTab);
							//ok, now, get the thing
						}
					}




					if (mouseX >= reloadbuttontrigger.x && mouseX <= (reloadbuttontrigger.x + reloadbuttontrigger.w) &&
						mouseY >= reloadbuttontrigger.y && mouseY <= (reloadbuttontrigger.y + reloadbuttontrigger.h)) {
						//reload

						//we need to load a white screen, just so people know it has been reloaded
						if (urlInput != "")
						{

							//we load the anim, render it and stuff!
							//LoadAnimation(render, font);

							search(urlInput, false, render, font, activeTab); //now start the search for the other one.
						}

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
							mouseY >= screeny && mouseY <= (screeny + tabs[activeTab].layout[i].hight))
						{



							std::cout << "link pressed" << std::endl;




							std::string finalUrl = tabs[activeTab].layout[i].href;


							std::string realDest = FixURLREDIRECT(finalUrl);
							if (!realDest.empty())
							{
								finalUrl = realDest;
							}

							//we got a few issues with the url links, as they break lol, so to fix, we add a https for it!
							if (finalUrl.size() >= 2 && finalUrl[0] == '/' && finalUrl[1] == '/')
							{
								//fix it
								finalUrl = "https:" + finalUrl;
							}





							//connect to it
							const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
							const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");


							//if our links are kinda weird, and arnt the full thing (wikipedia does this a lot lol
							if (!std::regex_search(finalUrl, httpPattern) && !std::regex_search(finalUrl, httpsPattern))
							{
								size_t endstr = urlInput.find("/", urlInput.find("://") + 3);
								std::string basedomain = urlInput.substr(0, endstr);

								//add them together
								// We also add a "/" in the middle just in case the link doesnt have one
								finalUrl = basedomain + (finalUrl.empty() || finalUrl[0] != '/' ? "/" : "") + finalUrl;
							}

							urlInput = finalUrl;
							//std::cout << "URLINPUT: " << urlInput << std::endl;



							LoadAnimation(render, font);

							//check if this is a valid url
							//note, this will assume that this is a valid url, if it follows the design scheme, but it may not be, so we then do a network test on the server (attempt to check its status)
							if (std::regex_match(urlInput, httpPattern)) {
								std::cout << "http url!" << std::endl;
								ConnectSocketHTTP(urlInput);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.q

								tabs[activeTab].SearchHistory.push_back(urlInput);
							}


							if (std::regex_match(urlInput, httpsPattern)) {
								std::cout << "https url!" << std::endl;
								std::wstring temp(urlInput.begin(), urlInput.end());
								ConnectSocketHTTPS(temp);


								tabs[activeTab].SearchHistory.push_back(urlInput);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
							}



							tabs[activeTab].currentSearchPos = tabs[activeTab].SearchHistory.size() - 1;

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
		SDL_SetRenderDrawColor(render, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255); // auto choses based on the site!
		SDL_RenderClear(render);

		//Loop through the layout list

		//// 3. Present the screen
		//SDL_RenderPresent(render);


		//draw the page by looping thorugh the layout list
		for (int i = 0; i < tabs[activeTab].layout.size(); i++)
		{
			if (tabs[activeTab].layout[i].textTex == nullptr && tabs[activeTab].layout[i].imageTex == nullptr) continue; //skip if we have somthing thats nullptr, no texture

			//render our text, we make a rectange and assign our text to that!
			SDL_FRect textrec;
			textrec.x = tabs[activeTab].layout[i].x;
			textrec.y = tabs[activeTab].layout[i].y - tabs[activeTab].scrollpos;
			textrec.w = tabs[activeTab].layout[i].width;
			textrec.h = tabs[activeTab].layout[i].hight;

			//draw the bg color
			if (tabs[activeTab].layout[i].hasBg)
			{
				SDL_SetRenderDrawColor(render, tabs[activeTab].layout[i].bgColor.r, tabs[activeTab].layout[i].bgColor.g, tabs[activeTab].layout[i].bgColor.b, 255); //255 cause we dont want it transparent
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





		int WinW, WinH;
		SDL_GetWindowSize(window, &WinW, &WinH);


		SDL_SetRenderDrawColor(render, 245, 245, 245, 255);
		SDL_FRect searchBarBg = { 0, 30, (float)WinW, 39 };
		SDL_RenderFillRect(render, &searchBarBg);





		//get the current win dimentions
		

		//little equation to place the url bar in the center
		float barWidth = 1200;
		float barX = (WinW - barWidth) / 2; //center it by taking half the window
		//draw the bar
		SDL_FRect bar = { barX, 35, barWidth, 30 };
		SDL_SetRenderDrawColor(render, 240, 240, 240, 255); //draw a grayish color
		//fill the rec with this
		SDL_RenderFillRect(render, &bar);


		//draw the text typed
		TTF_SetFontSize(font, 17); //define the font size
		std::string displayText = urlInput;
		//set the txt color
		SDL_Color color = { 0,0,0,255 }; //black

		if (urlInput.empty()) //check if its empty
		{
			displayText = "Enter a url...";
			color = { 180, 180, 180, 255 }; //just a dark gray
		}

		//render it!
		SDL_Surface* urlSurf = TTF_RenderText_Solid(font, displayText.c_str(), 0, color);

		if (urlSurf != nullptr) //check to make sure we arnt trying to render somthing that is null
		{
			//upload it to the gpu for drawing
			SDL_Texture* urlTexture = SDL_CreateTextureFromSurface(render, urlSurf);
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
				SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
				SDL_RenderLine(render, cursorX, bar.y + 5, cursorX, bar.y + 23);

			}

			//cleanup
			SDL_DestroySurface(urlSurf);


		}


		//render the 2 buttons
		SDL_FRect backBtn;

		if (tabs[activeTab].currentSearchPos > 0) //we do > than 0, as we dont want to have an error
		{
			backBtn = { barX - 40, 35, 30, 30 };
			SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			SDL_RenderFillRect(render, &backBtn);
		}
		else {
			backBtn = { barX - 40, 35, 30, 30 };
			SDL_SetRenderDrawColor(render, 235, 235, 235, 255);
			SDL_RenderFillRect(render, &backBtn);
		}


		SDL_Color textColor = { 0, 0, 0, 255 };
		SDL_Surface* backSurf = TTF_RenderText_Solid(font, "<", 0, textColor);
		if (backSurf != nullptr)
		{
			SDL_Texture* backTex = SDL_CreateTextureFromSurface(render, backSurf);

			SDL_FRect backTexRect = { backBtn.x + 8, backBtn.y + 4, (float)backSurf->w, (float)backSurf->h };
			SDL_RenderTexture(render, backTex, nullptr, &backTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(backTex);
			SDL_DestroySurface(backSurf);
		}

		SDL_FRect fwdBtn;
		if (!tabs[activeTab].SearchHistory.empty() && tabs[activeTab].currentSearchPos < (int)tabs[activeTab].SearchHistory.size() - 1) //we do > than 0, as we dont want to have an error
		{
			fwdBtn = { barX + barWidth + 10, 35, 30, 30 };
			SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
			SDL_RenderFillRect(render, &fwdBtn);
		}
		else {
			//not enabled
			fwdBtn = { barX + barWidth + 10, 35, 30, 30 };
			SDL_SetRenderDrawColor(render, 235, 235, 235, 255);
			SDL_RenderFillRect(render, &fwdBtn);
		}



		SDL_Surface* forwardSurf = TTF_RenderText_Solid(font, ">", 0, textColor);
		if (forwardSurf != nullptr)
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, forwardSurf);

			SDL_FRect fwdTexRect = { fwdBtn.x + 8, fwdBtn.y + 4, (float)forwardSurf->w, (float)forwardSurf->h };
			SDL_RenderTexture(render, forwardTex, nullptr, &fwdTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex);
			SDL_DestroySurface(forwardSurf);
		}

		SDL_FRect reloadButton;

		//reload button
		reloadButton = { barX - 80, 35, 30, 30 };
		SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
		SDL_RenderFillRect(render, &reloadButton);

		SDL_Surface* reloadSurf = TTF_RenderText_Solid(font, "R", 0, textColor);
		if (reloadSurf != nullptr)
		{
			SDL_Texture* forwardTex = SDL_CreateTextureFromSurface(render, reloadSurf);

			SDL_FRect reloadTexRect = { reloadButton.x + 9, reloadButton.y + 2, (float)reloadSurf->w, (float)reloadSurf->h };
			SDL_RenderTexture(render, forwardTex, nullptr, &reloadTexRect);

			//prevent mem leaks
			SDL_DestroyTexture(forwardTex);
			SDL_DestroySurface(reloadSurf);
		}




		//draw the background of the taskbar.
		SDL_SetRenderDrawColor(render, 210, 210, 210, 255);
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
				SDL_SetRenderDrawColor(render, 245, 245, 245, 255);
			else
				SDL_SetRenderDrawColor(render, 190, 190, 190, 255);

			//make our tab rec, putting in our tab width and pos
			SDL_FRect tabrect = { (float)tabX, 0, (float)tabW, 30 }; //with a hight of 30
			//render it
			SDL_RenderFillRect(render, &tabrect);



			//ok now lets draw the tab text (gonna be the <title> text)
			TTF_SetFontSize(font, 13);
			//set the color to black, and the title to the title
			SDL_Color tabTextColor = { 0,0,0,255 };
			
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
			SDL_Surface* xSurf = TTF_RenderText_Solid(font, "x", 0, tabTextColor);

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
		SDL_SetRenderDrawColor(render, 180, 180, 180, 255);
		//make the button for it
		SDL_FRect newTabBtn = { (float)tabX, 2, 26, 26 };

		//fill it
		SDL_RenderFillRect(render, &newTabBtn);

		//just a copy and paste atp
		SDL_Surface* plusSurf = TTF_RenderText_Solid(font, "+", 0, { 0,0,0,255 });

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

