//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"


std::string StripTags(std::string htmldata)
{
	//first we want to locate the start of a tag. for example <html></html>
	
	int len = htmldata.length();

	//we will check 
	bool flag = false;
	int current = 0;
	for (int i = 0; i < len; i++)
	{
		//we know we are on the first part of a tag
		if (htmldata[i] == '<')
		{
			flag = true;
			current = i;
		}
		//the reason we check for both, it to avoid errors, if we have a floating one.
		if (htmldata[i] == '>' && flag == true)
		{
			flag = false;

			//this gets our headers
			//std::string headersstring = htmldata.substr(current, i - current + 1);
			
			//debug stuff
			//std::cout << "Removed: " << htmldata.erase(current, i - current + 1) << std::endl;
			
			htmldata.erase(current, i - current + 1); //this will go through and erase the points we want
			
			
			i = current - 1; //because our loop will be off, it will cause an error, so we set our loop to our current - 1
			len = htmldata.length(); //also adjust our len to prevent another error.

			//now that we have appended the thing, we need to remove the old stuff, i + 1 and back.
			
		}
	}
	system("cls"); //this cleans our muddled terminal with all our debug stuff.
	std::cout << "Parsed output: " << htmldata << std::endl;


	//then we want to go through everything, and for now, print out each tag as we find it, and remove it so we dont index it over and over
	return htmldata;
}


//these classes will be populate eventualy, to actully tokenize, and render css, but not today.
std::string ManageCSS(std::string htmldata)
{
	//Currently all we want to do in this is to just remove all the CSS, so lets find the css tags, and remove them.
	//super simple, look for the style parts, (as they are constant across all websites) and remove everything inbetween.
	//however some sites may not have a stylesheet, so we should check.
	if (htmldata.find("<style>") == std::string::npos)
	{
		return htmldata; //dont change nothin, just prevent an error
	}
	//if we are good, return this.
	return htmldata.erase(htmldata.find("<style>"), (htmldata.find("</style>") + 8) - htmldata.find("<style>"));
}


std::string ManageJSON(std::string htmldata)
{
	//Currently all we want to do in this is to just remove all the JSON, so lets find the json tags, and remove them.
	//super simple, look for the scirpt tags, (as they are constant across all websites) and remove everything inbetween.
	//however some sites may not have json, so we should check.
	if (htmldata.find("<scirpt>") == std::string::npos)
	{
		return htmldata; //dont change nothin, just prevent an error
	}
	//if we are good, return this.
	return htmldata.erase(htmldata.find("<scirpt>"), (htmldata.find("</scirpt>") + 8) - htmldata.find("<scirpt>"));
}


int Parser(std::string input)
{
	//all this is doing, is removing, the json part, then the css part, then all of the tags, just to get the final text for this basic parser.
	StripTags(ManageCSS(ManageJSON(input))); //not great to nest, will be fixed
	return 0;
}