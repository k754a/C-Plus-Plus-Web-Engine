//THIS IS THE WINDOW RENDER
//(THIS WILL TAKE IN MULTIPLE INPUTS, FROM Layout.cpp, and Webengine.cpp.


#include "GUI.h"
#include <SDL3/SDL.h> //include the SDL3 lib
#include <SDL3_ttf/SDL_ttf.h>

std::vector<Layout> mainlayout;

int IMPORT(std::vector<Layout> layoutGOTTEN)
{
	mainlayout = layoutGOTTEN;
	std::cout << "Updated" << std::endl;
	return 0;
}
int GUIRENDER()
{
	//to render to a texture, we need to make a texture for the font
	//we make a font to hold it
	static TTF_Font* font = nullptr;
	//we make a texture to save our text too
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




	//INIT SDL TTF
	if (!TTF_Init())
	{
		//if we fail
		std::cout << "Failed to init SDL::TTF" << std::endl;
		return -1;
	}



	//Now that we got it init, lets render that font
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
	SDL_DestroySurface(textserf);
	

	int scrollpos = 0;

	SDL_Event event; //we make an SDL event handler

	while (running)
	{
		//like pygame python, we check to see if we have gotten our x pressed, and if we have we stop it
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

			if (event.type == SDL_EVENT_MOUSE_WHEEL)
			{
				scrollpos -= event.wheel.y * 40;

				//prevent scrolling bast and leaving lol
				if (scrollpos < 0)
				{
					scrollpos = 0;
				}
			}
		}


		//Clear the screen
		SDL_SetRenderDrawColor(render, 255, 255, 255, 255); // White background
		SDL_RenderClear(render);

		//Loop through the layout list
		
		for (int i = 0; i < mainlayout.size(); i++) {

			//we pull the layout i (loop through everything)
			Layout currentLayout = mainlayout[i];

			// make sure we dont draw blank stuff
			std::string textToDraw = currentLayout.node->tagValue; // Make sure this holds the text payload!
			if (textToDraw.empty()) continue;

			TTF_SetFontSize(font, currentLayout.fontSize);

			//Create the surface (using black text)
			SDL_Surface* nodeSurf = TTF_RenderText_Solid(font, textToDraw.c_str(), 0, SDL_Color{ 0, 0, 0, 255 });

			//Make sure the surface was created successfully
			if (nodeSurf != nullptr) {
				
				//make a texture and update it
				SDL_Texture* nodeTex = SDL_CreateTextureFromSurface(render, nodeSurf);

				SDL_FRect textRec;
				textRec.x = currentLayout.x;
				textRec.y = currentLayout.y - scrollpos;
				textRec.w = nodeSurf->w; 
				textRec.h = nodeSurf->h; 

				// Draw and destroy to prevent mem leaks
				SDL_RenderTexture(render, nodeTex, nullptr, &textRec);
				SDL_DestroyTexture(nodeTex);
				SDL_DestroySurface(nodeSurf);
			}
		}

		//// 3. Present the screen
		//SDL_RenderPresent(render);

		////render our text
		//SDL_FRect textrec;
		//textrec.x = 800;
		//textrec.y = 200;
		//textrec.w = 800;
		//textrec.h = 200;
		//SDL_RenderTexture(render, fontText, nullptr, &textrec);




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