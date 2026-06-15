//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"
#include <vector>






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
	//first lets remove the htmldata \r\n\r\n, this is a index, so we will just start from this point in our loop.
	size_t htmlStart = htmldata.find("\r\n\r\n") + 4;
	int len = htmldata.length(); //get the length of the html data
	std::vector<Token> tokenList; //create the list for the token.

	//size_t is much better than int.
	size_t i = htmlStart;

	//Ok, this is the plan, we find the first html data token, the <html> and we store it in a 
	//while (i < len)
	//{
	//	//we start past this spot.
	//	size_t nextTag = htmldata.find('<', i);

	//	if (nextTag == std::string::npos) //if we cant find any (if we make it to the end, or an error)
	//	{
	//		//TODO, save any text after this point (if we have any)
	//		return "NULL";
	//	}

	//	if (nextTag > i)
	//	{
	//		
	//		std::string textContent = htmldata.substr(i, nextTag - i);
	//		std::cout << textContent << std::endl;
	//		i = nextTag;
	//	}



	//}
	
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