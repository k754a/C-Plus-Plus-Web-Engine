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


//this is the layout list, and gets filled in by IMPORT
//the render loop reads from this 
std::vector<Layout> mainlayout;



//wanted to devlop a way to have a back and forward arrow.
//the way we can do this is have a list with the websites, and 2 buttons besides the input box, if they are pressed, we change the index, going back or forth
//this is my thoughs

//SEARCH -> ADD TO LIST -> SAVED

//WHEN BUTTON PRESSED (either or) -> LOOK FORWARD OR BACKWARD IN THE LIST -> COMPARE TILL WE FIND THE INDEX -> MOVE BACK ONE.




std::vector <std::string> SearchHistory;


//SUDO CODE
//if(SearchHistory[i] == urlInput && searchHistory.length() > 1){
//
//	urlInput = SearchHistory[i - 1];
// 
// 
// 
// 
//}

//we we search, we add one to the top, if we search, and our current pos is back one from our search history, we will overwrite that history.










//the pos and how far weve scrolled down for the page
//starts at the top of the page 0, and increases as the user scrolls down.
//its subtracted from each elements y, so it gives the illiusion of scrolling.
int scrollpos = 0;

std::string urlInput = "";    // holds the url we type
bool urlBarFocused = true;    // currently we have no other inputs, so we can always have this focused!
SDL_Color backgroundColor = { 245, 245, 245, 255 }; //white bg, gets changed btw


//this is called in layout, and repaces whatever was on the screen with the new layout
int IMPORT(std::vector<Layout> layoutGOTTEN)
{
	scrollpos = 0; //reset the scroll pos, for new web sites
	//we need to destroy all the old textures, as these are loaded in gpu mem
	for (int i = 0; i < mainlayout.size(); i++)
	{
		if (mainlayout[i].textTex != nullptr) //check to make sure we dont double free
		{
			SDL_DestroyTexture(mainlayout[i].textTex); //tells the gpu to free this
			mainlayout[i].textTex = nullptr; //set it to null so we dont do it twice, and crash
		}
	}











	mainlayout = layoutGOTTEN;
	std::cout << "Updated" << std::endl;
	return 0;
}
//moved to this, for speed!
//this uploads the textures to the gpu
//the idea is that we only do the GPU stuff once a layout, then assign this to a texture, that moves
void PreRender(SDL_Renderer* render, TTF_Font* font)
{
	int xtrack = 20;
	int ytrack = 40;
	int lasty = -1; //set to -1 for first run so we allways small first run
	int maxLineHeight = 0; //fix clipping

	for (int i = 0; i < mainlayout.size(); i++) {

		//if the item already has a texture
		//we skip so we dont rerender it

		if (lasty != -1 && mainlayout[i].y > lasty) {
			xtrack = 20;

			ytrack += (maxLineHeight + 15);
			maxLineHeight = 0;
		}

		lasty = mainlayout[i].y;

	
		mainlayout[i].x = xtrack;
		mainlayout[i].y = ytrack;

		if (mainlayout[i].textTex == nullptr)
		{
			//grab the text string from our current mainlayout node
			//the value holds the text.
			std::string text = mainlayout[i].node->tagValue;// Make sure this holds the text payload!

			//we pull the layout i (loop through everything)
			//Layout currentLayout = mainlayout[i];
			if (!text.empty())
			{

				//we set the fonts to diffrent sizes, that our layout dom tree already does!
				TTF_SetFontSize(font, mainlayout[i].fontSize);

				if (!mainlayout[i].href.empty()) {
					TTF_SetFontStyle(font, TTF_STYLE_UNDERLINE);
				}
				else {
					TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
				}


				//Create the surface (using black text)
				SDL_Surface* nodeSurf = TTF_RenderText_Solid(font, text.c_str(), 0, mainlayout[i].textColor);

				//Make sure the surface was created successfully
				if (nodeSurf != nullptr) {

					////make a texture and update it
					//SDL_Texture* nodeTex = SDL_CreateTextureFromSurface(render, nodeSurf);

					//SDL_FRect textRec;
					//textRec.x = currentLayout.x;
					//textRec.y = currentLayout.y - scrollpos;


					//save the dimetions of the block of text
					mainlayout[i].width = nodeSurf->w;
					mainlayout[i].hight = nodeSurf->h;

					//upload the surface from ram to gpu as a texture!
					// Draw and destroy to prevent mem leaks
					mainlayout[i].textTex = SDL_CreateTextureFromSurface(render, nodeSurf); //send it to the gpu

					//free the cpu to avoid ram leaks!
					SDL_DestroySurface(nodeSurf);
				}

			}
		
		
		
		
		}


		TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

		if (mainlayout[i].hight > maxLineHeight)
		{
			maxLineHeight = mainlayout[i].hight;
		}
	


		xtrack += (mainlayout[i].width + 12);








	}




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
	
	SDL_Surface* textserf = TTF_RenderText_Solid(font, "Browse++", 0, SDL_Color(255,255, 255,0));
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
				scrollpos -= event.wheel.y * 80;

				//prevent scrolling bast and leaving lol
				if (scrollpos < 0)
				{
					scrollpos = 0;
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
					std::cout << "going to: " << urlInput << std::endl; //DEBUG


					//ok lets make this work.


					std::string input = urlInput;
					//First, we need to prompt the user, what URL are they attempting to connect too?
					//std::cout << "Type the URL to open it: "; //CAN NOW BE HTTPS!!!!

					//pulls the input, then creates a new line for cleaness
					//std::cin >> input; std::cout << std::endl;


					//these 2 blocks autodetect if its a https or http.
					//std::thread([input]()
					//	{
							const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
							const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

							//check if this is a valid url
							//note, this will assume that this is a valid url, if it follows the design scheme, but it may not be, so we then do a network test on the server (attempt to check its status)
							if (std::regex_match(input, httpPattern)) {
								std::cout << "http url!" << std::endl;
								ConnectSocketHTTP(input);


								SearchHistory.push_back(input);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
							}


							if (std::regex_match(input, httpsPattern)) {
								std::cout << "https url!" << std::endl;
								std::wstring temp(input.begin(), input.end());
								ConnectSocketHTTPS(temp);


								SearchHistory.push_back(input);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
							}
						//}).detach();


					


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























					for (int i = 0; i < mainlayout.size(); i++)
					{
						//skip ones that arnt links
						if (mainlayout[i].textTex == nullptr) continue;
						if (mainlayout[i].href.empty()) continue;

						//ok we have a link, now lets adjust the activator block
						float screeny = mainlayout[i].y - scrollpos;

						//calculate if we are over it
						if (mouseX >= mainlayout[i].x && mouseX <= (mainlayout[i].x + mainlayout[i].width) &&
							mouseY >= screeny && mouseY <= (screeny + mainlayout[i].hight))
						{



							std::cout << "link pressed" << std::endl;

						


							std::string finalUrl = mainlayout[i].href;

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
							std::cout << "URLINPUT: " << urlInput << std::endl;
						









							//check if this is a valid url
							//note, this will assume that this is a valid url, if it follows the design scheme, but it may not be, so we then do a network test on the server (attempt to check its status)
							if (std::regex_match(urlInput, httpPattern)) {
								std::cout << "http url!" << std::endl;
								ConnectSocketHTTP(urlInput);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.q
								SearchHistory.push_back(urlInput);
							}


							if (std::regex_match(urlInput, httpsPattern)) {
								std::cout << "https url!" << std::endl;
								std::wstring temp(urlInput.begin(), urlInput.end());
								ConnectSocketHTTPS(temp);


								SearchHistory.push_back(urlInput);
								//now that we understand its a valid url, lets attempt a socket connect.
								//[FOR DEBUG, THE CONNECTSOCKET(input) IS NOT IN HERE, AS TO SAVE TIME.
							}

							break;
					
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
		for (int i = 0; i < mainlayout.size(); i++)
		{
			if (mainlayout[i].textTex == nullptr) continue; //skip if we have somthing thats nullptr, no texture

			//render our text, we make a rectange and assign our text to that!
			SDL_FRect textrec; 
			textrec.x = mainlayout[i].x;
			textrec.y = mainlayout[i].y - scrollpos;
			textrec.w = mainlayout[i].width;
			textrec.h = mainlayout[i].hight;

			//draw the bg color
			if (mainlayout[i].hasBg)
			{
				SDL_SetRenderDrawColor(render, mainlayout[i].bgColor.r, mainlayout[i].bgColor.g, mainlayout[i].bgColor.b, 255); //255 cause we dont want it transparent
				//fill the rec
				SDL_RenderFillRect(render, &textrec);
			}

		



			SDL_RenderTexture(render, mainlayout[i].textTex, nullptr, &textrec);

		}

		//get the current win dimentions
		int WinW, WinH;
		SDL_GetWindowSize(window, &WinW, &WinH);

		//little equation to place the url bar in the center
		float barWidth = 1200;
		float barX = (WinW - barWidth) / 2; //center it by taking half the window
		//draw the bar
		SDL_FRect bar = { barX, 6, barWidth, 30 };
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
		SDL_FRect backBtn = { barX - 40, 6, 30, 30 };
		SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
		SDL_RenderFillRect(render, &backBtn);

	
		SDL_FRect fwdBtn = { barX + barWidth + 10, 6, 30, 30 };
		SDL_SetRenderDrawColor(render, 200, 200, 200, 255);
		SDL_RenderFillRect(render, &fwdBtn);









		//this is like our pygame render thing
		SDL_RenderPresent(render);

	}
	
	//while (true)
	//{
	//	std::cout << "TEST" << std::endl;
	//}
	


	//we need to quit to clean up all the subsystems
	SDL_DestroyWindow(window); //kill the window, cleanly 
	SDL_Quit(); 

	//Kill TTF
	TTF_Quit();

	std::quick_exit(1);
	
	return 0;
}

