//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"
#include <vector>
#include <list>





//ok, lets first define our strucutre
//currently we want to have a start tag, a text, and a end tag
//we say enum just to tell the complier that we wont be changing this
//this is saying class tokentype( start (like <p>), text (like "this is a test"), and end (like <p>))
enum class TokenType { START, TEXT, END};

//lets build the token structure
struct Token
{
	//we do the type and the value, as then we can identify the start middle or end to a real value.
	//all tokens follow the format "type"
	TokenType type;
	std::string value; //hold the actual text
};


//we currently have a bug where blank text is getting through.
//the way to fix this, is to check if the data is blank, and then decide if we want to parse or not.
bool iswhitespace(std::string& input)
{
	for (unsigned char c : input)
	{
		//checks to see that there are not any blank ones.
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' && c != 0xA0) {
			return false; 
		}
	}

	//we found whitespace
	return true;
}








std::string StripTags(std::string htmldata)
{
	std::cout << "Running Striptags" << std::endl;
	//first lets remove the htmldata \r\n\r\n, this is a index, so we will just start from this point in our loop.
	size_t htmlStart = htmldata.find("\r\n\r\n") + 4;
	int len = htmldata.length(); //get the length of the html data
	std::vector<Token> tokenList; //create the list for the token.

	//size_t is much better than int.
	size_t i = htmlStart;

	//Ok, this is the plan, we find the first html data token, the <html> and we store it in a stack. then when we find another tag like <css>
	//We check the stack, because we dont have it in the current stack, we understand that we should add it to the stack, so we do.
	// Ok, then we run into a </css> tag, we understand that it needs to link to a css tag, so we loop through our stack, and because we have a css tag, we grab the 2 values, and link them together, and remove them from the stack.
	// But lets say we have a <json> but we dont have the ending </json> what we need to do is that if we have leftover things in our stack that dont fit, we just remove and forget about them.
	// because we do this, this allows us to avoid things like compile errors because html is a very loose structure format.
	
	std::string Savevar; //we will use this in our loop to add to our stack.

	//I orignaly was going to use std::stack, however, there is no native lookup, and i cant edit things in the stack, then i was going to use a list, but i cant search a std::list, so we are using a vector
	//so we are going to use a list, we use a list because we can check everything, and if they match, we remove from the list.
	std::vector < std::string > TokenStack;




	//first lets build the loop.
	while (i < len) //while our i is greater then our len
	{
		
		//we first find the first spot.
		i = htmldata.find('<', i);
		//we need this because i lowky forgot it, and it ended up crashing lol
		if (i == std::string::npos)
		{
			break; //we have finished.	
		}



		//we have to add this, because what ended up happin is that we could find the < but not the > so it would freeze
		size_t closePos = htmldata.find('>', i);
		if (closePos == std::string::npos) {
			break; 
		}



		


		//if we are good: 
		//a mistake to watch out for, is that substr, looks for starting char, and length of char, not startpoint and endpoint (i keep messing that up lol)
		Savevar = htmldata.substr(i, (closePos - i + 1)); //this basicly splits and grabs the data between i and our end '>'
		//std::cout << Savevar << std::endl; //little telem
		
		

		//Ok, now lets add it to our stack list (lets compare to see if it exists first, before adding it, because it will be more effecent that way).
		
		if (!TokenStack.empty()) //ok, now we need to check if this is empty
		{
			bool matchfound = false;
			//because <list> has a search method like <stack> we can loop through and check everything
			//if we cant find a match, we add it to the list, and if we find it (the top stack = our current token) we print it out and remove it. (for now)
			for (int t = 0; t < TokenStack.size(); t++)
			{
				//check if it exists,or its the end version
				//something i messed up is i directly compared, and you should NOT do that as we want to find pairs lol
				if ("<" + ("/" + TokenStack[t]) + ">" == Savevar)
				{
					//we now assume the we found it!
					
					//first we print it
					std::cout << "Match found! " << Savevar << " + " << TokenStack[t] << std::endl;
					//because we dont have a erase[i] what we need to do, is to find our first one, and add our index onto it
					TokenStack.erase(TokenStack.begin() + t);
				}
				
			}
			//if we went through everything and didnt find it
			//this is more effecent, because then we dont gotta check for 2 conditions every cycle.
			if(!matchfound)
			{
				//if it does not exist
				//we clean off the </> stuff
				std::string tagName = Savevar.substr(1, Savevar.length() - 2);
				TokenStack.push_back(tagName);
			}


		}
		else { //this will only be used once, because of the starting one.

			// the token stack was empty, so we will push our token on
			//we clean off the </> stuff
			std::string tagName = Savevar.substr(1, Savevar.length() - 2);
			TokenStack.push_back(tagName);
		}
		


		i = i + Savevar.length(); //to not index the same thing again, we set the next i point, past the <, so that we find the next point


	}

	std::cout << "Done!" << std::endl;

	
	return "NULL";

}



//old tokenizer, this really doesnt work well lol, so this next one i build should.
//std::string StripTags(std::string htmldata)
//{
//	//system("cls"); //debug for clean
//	//UPDATED. We do the same thing first
//	//first we want to locate the start of a tag. for example <html></html>
//	
//	int len = htmldata.length();
//
//	//we will check 
//	bool flag = false;
//	int current = 0;
//	int startPeice = 0; // this will tell us the value of the first part.
//	//we first create the list to store the token
//	std::vector<Token> tokenList;
//	//we build a var just to pass it back when ready.
//	Token currenttoken;
//	
//	for (int i = 0; i < len; i++) {
//		//however instead of removing, when the conditions are met, we start to set the tokens
//
//		if (htmldata[i] == '<') {
//			current = i;
//		}
//		if (htmldata[i] == '>')
//		{
//
//
//
//			flag = !flag; //this checks to see if this is our second <> <>
//			std::string currenttag = htmldata.substr(current + 1, i - current - 1);
//			if (currenttag[0] == '/') {
//				std::cout << "END "; //what this is going to do, is assign what the END is, we dont do std::endl, because we want this to be injected for the token.
//				currenttoken.type = TokenType::END; //set that we are changing the end one
//				currenttag.erase(0, 1); //remove the /
//			}
//			else
//			{
//				std::cout << "START "; //what this is going to do, is assign what the START is, we dont do std::endl, because we want this to be injected for the token.
//				currenttoken.type = TokenType::START; //set that we are changing the start one
//			}
//			//lets now pull the data from the middle.
//			if (flag)
//			{
//				currenttoken.type = TokenType::TEXT; //set that we are changing the middle one
//				std::cout << "TEXT "; //what this is going to do, is assign what the text is, we dont do std::endl, because we want this to be injected for the token.
//				currenttag = htmldata.substr(startPeice + 1, current - startPeice - 1);
//			}
//			else{ startPeice = i; }
//
//			if (!iswhitespace(currenttag))
//			{
//				currenttoken.value = currenttag; //this gets our token val
//				std::cout << "Token: " << currenttoken.value << std::endl;
//			}
//			
//			
//			
//		
//			
//
//
//		}
//
//
//
//
//	}
//	return "test";
//	
//		
//}





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