//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"
#include <vector>
#include <list>
#include <unordered_set> //for the check to make it faster




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




//Full striptags code, does not handle things like <a href="link">
std::vector<Token> StripTags(std::string htmldata)
{
	std::vector<Token> tokenList; //saves our tokens, we do a vector to save multiple instances!

	//DEFINE SOME VOID TAGS
	//some tags are allowed to be single, so we will also check for that!
	//i got the tags from https://developer.mozilla.org/en-US/docs/Glossary/Void_element however it was missing br/ so that was added
	//this is faster because we can check as if there is no order at all!
	std::unordered_set<std::string> voidTags = {
	"area", "base", "br", "col", "embed", "hr", "img", "input",
	"link", "meta", "param", "source", "track", "wbr"
	};




	std::cout << "Running Striptags" << std::endl;
	size_t htmlStart = 0;
	//first lets remove the htmldata \r\n\r\n, this is a index, so we will just start from this point in our loop.
	//we check, because if we have https this wont work, and we will pull some random tags lol
	if (htmldata.find("\r\n\r\n") == std::string::npos)
	{
		htmlStart = 0; //we do this because using winInet, it alr removes that stuff, so we dont have to do it!
	}
	else {
		 htmlStart = htmldata.find("\r\n\r\n") + 4; //http does not, so we still do.
	}
	
	int len = htmldata.length(); //get the length of the html data


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
		size_t nextTag = htmldata.find('<', i);
		//we need this because i lowky forgot it, and it ended up crashing lol
		if (nextTag == std::string::npos)
		{
			break; //we have finished.	
		}

		//We do this because text is stored between chars like for example <Hello> the index of < is 0 and the index of > is 6.
		//if nexttag is bigger, it means we have text inbetween!
		//this works for nested tags because <><> has no space! even if it does <> <> we ignore whitespaces
		if (nextTag > i)
		{
			std::string text = htmldata.substr(i, nextTag - i);//we grab the data between our "i" and a certain amount of chars after

				if (!iswhitespace(text)) //our funct to check if we have whitespace
				{
					//now that we know there is valid text
					Token textT;
					textT.type = TokenType::TEXT;
					textT.value = text;

					tokenList.push_back(textT);

					std::cout << "Found Text: " << text << std::endl;
				}
		}

		//now we move our i to our next token, because we just found one!

		i = nextTag;

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
			Token currenttoken; //build a temp class

			bool matchfound = false;
			//because <list> has a search method like <stack> we can loop through and check everything
			//if we cant find a match, we add it to the list, and if we find it (the top stack = our current token) we print it out and remove it. (for now)
			
				//check if it exists,or its the end version
				//something i messed up is i directly compared, and you should NOT do that as we want to find pairs lol
			//first we check if the tokenstack is empty, and our last token in the loop is a pair to the save var
			if (!TokenStack.empty() && ("</" + TokenStack.back() + ">") == Savevar)
			{
					
				//we now assume the we found it!
					
				//first we print it
				std::cout << "Match found! " << std::endl;
					

					
				//now lets save it to our tokenlist
				//because we know the start+end, we can save both

				//because we know the start+end, we can save both
				currenttoken.type = TokenType::START;
				currenttoken.value = "<" + TokenStack.back() + ">";
				tokenList.push_back(currenttoken);

				currenttoken.type = TokenType::END;
				currenttoken.value = Savevar;
				tokenList.push_back(currenttoken);

				//because we dont have a erase[i] what we need to do, is to find our first one, and add our index onto it
				//NOW we can do     TokenStack.pop_back(); as this is better than the erase and does the same thing
				TokenStack.pop_back();
				//because we found a match we break, and that allows us to fix a bug!
				matchfound = true;
				
			}

				
			
			//if we went through everything and didnt find it
			//this is more effecent, because then we dont gotta check for 2 conditions every cycle.
			if(!matchfound)
			{
				//if it does not exist
				//we clean off the </> stuff
				std::string tagName = Savevar.substr(1, Savevar.length() - 2);
				size_t spacePos = tagName.find(' ');
				if (spacePos != std::string::npos)
				{
					//we have a space! because we do lets find and remove it.
					tagName = tagName.substr(0, spacePos);
				}

				//before we throw in the towel, lets check to see if its one of our void tags
				//we check if its not = to voidtags.end() because if we get to the end of a vector and cant find nothing, we are ok with that!
				if ((voidTags.contains(tagName)))
				{
					//currently we do nothing with this, but thats ok
					std::cout << "Void Tag Found! " << Savevar << std::endl;
				}



				TokenStack.push_back(tagName);

			}
			


		}
		else { //this will only be used once, because of the starting one.

			// the token stack was empty, so we will push our token on
			//we clean off the </> stuff
			std::string tagName = Savevar.substr(1, Savevar.length() - 2);
			//HOWEVER, WE NEED TO REMOVE THE a href="link.html" if it exists
			size_t spacePos = tagName.find(' ');
			if (spacePos != std::string::npos)
			{
				//we have a space! because we do lets find and remove it.
				tagName = tagName.substr(0, spacePos);
			}

			//before we throw in the towel, lets check to see if its one of our void tags
			//we check if its not = to voidtags.end() because if we get to the end of a vector and cant find nothing, we are ok with that!
			if (std::find(voidTags.begin(), voidTags.end(), tagName) != voidTags.end())
			{
				//currently we do nothing with this, but thats ok.
				std::cout << "Void Tag Found! " << Savevar << std::endl;
			}

			TokenStack.push_back(tagName);
		}
		


		i = i + Savevar.length(); //to not index the same thing again, we set the next i point, past the <, so that we find the next point

		

	}

	//cleanness
	std::cout << "-------------" << std::endl;
	//For debug (and fun) lets print the tags without a partner :(

	if (TokenStack.size() <= 0)
	{
		std::cout << "No extra tags found. :)" << std::endl;
	}
	else
	{
		//turns out we end up saving a few tags just with a bug, it doesn't effect the main thing, so if it aint broke, dont fix it!

		//UPDATE the bug has been fixed!
			std::copy(TokenStack.begin(), TokenStack.end(), std::ostream_iterator<std::string>(std::cout, " "));

			std::cout << std::endl;
			std::cout << "Extra tags found. This wont be a problem." << std::endl;
	}

	std::cout << "Done!" << std::endl;
	//cleanness
	std::cout << "-------------" << std::endl;


	std::cout << "Final Tokens:" << std::endl;
	//we want to print the token combos, just as a debug!
	for (int i = 0; i < tokenList.size(); i++)
	{
		//ok now we loop through and print
		//as our final thing
		std::cout << tokenList[i].value << std::endl;
	}

	std::cout << std::endl;

	return tokenList;

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


//now all functions can parse much faster than before.


//these classes will be populate eventualy, to actully tokenize, and render css, but not today.
//this was updated to a loop, because https sites can have more than one stylesheet.
std::string ManageCSS(const std::string& htmldata)
{
	
	std::string returnhtmldata;
	returnhtmldata.reserve(htmldata.size()); //what this does, is reserve a chunk that doesnt need to be recalculated over and over.

	//we do the same for strip tags, instead of scanning everything, we just find and remove!
	size_t i = 0;
	const size_t len = htmldata.size();

	while (i < len) //same as strip tags
	{
		size_t startpos = htmldata.find("<style", i); //find the first half, from the i (so we dont check back from the start)
		if (startpos == std::string::npos)
		{
			//we need to make sure we save it
			returnhtmldata.append(htmldata, i, len - i); //we return our data, taking from htmldata, and going from i to out
			break;
		}

		returnhtmldata.append(htmldata, i, startpos - i);  //we return our data, taking from htmldata, and going from i to out

		size_t endpos = htmldata.find("</style>", startpos);
		if (endpos == std::string::npos)
		{
			break;

		}
		//Currently all we want to do in this is to just remove all the CSS, so lets find the css tags, and remove them.
	//super simple, look for the style parts, (as they are constant across all websites) and remove everything inbetween.
	//however some sites may not have a stylesheet, so we should check.


		i = endpos + std::string("</style>").length(); //same as the erase but we dont erase anything, and its dynamic now!
		//if we are good, return this.
	}

	
	return returnhtmldata;

}

//we wont work with JAVASCIRPT right now (we might if i run out of things on this project)
//this was updated as https can have more than one <script> area

std::string ManageJAVASCIRPT(const std::string& htmldata)
{

	std::string returnhtmldata;
	returnhtmldata.reserve(htmldata.size()); //what this does, is reserve a chunk that doesnt need to be recalculated over and over.

	//we do the same for strip tags, instead of scanning everything, we just find and remove!
	size_t i = 0;
	const size_t len = htmldata.size();

	while (i < len) //same as strip tags
	{
		size_t startpos = htmldata.find("<script", i); //find the first half, from the i (so we dont check back from the start)
		if (startpos == std::string::npos)
		{
			//we need to make sure we save it
			returnhtmldata.append(htmldata, i, len - i); //we return our data, taking from htmldata, and going from i to out
			break;
		}

		returnhtmldata.append(htmldata, i, startpos - i);  //we return our data, taking from htmldata, and going from i to out

		size_t endpos = htmldata.find("</script>", startpos);
		if (endpos == std::string::npos)
		{
			break;

		}
		//Currently all we want to do in this is to just remove all the JAVA, so lets find the JAVA tags, and remove them.
	//super simple, look for the style parts, (as they are constant across all websites) and remove everything inbetween.
	//however some sites may not have a stylesheet, so we should check.


		i = endpos + std::string("</script>").length(); //same as the erase but we dont erase anything, and its dynamic now!
		//if we are good, return this.
	}


	return returnhtmldata;

}



std::string ManageCOMMENTS(const std::string& htmldata)
{

	std::string returnhtmldata;
	returnhtmldata.reserve(htmldata.size()); //what this does, is reserve a chunk that doesnt need to be recalculated over and over.

	//we do the same for strip tags, instead of scanning everything, we just find and remove!
	size_t i = 0;
	const size_t len = htmldata.size();

	while (i < len) //same as strip tags
	{
		size_t startpos = htmldata.find("<!--", i); //find the first half, from the i (so we dont check back from the start)
		if (startpos == std::string::npos)
		{
			//we need to make sure we save it
			returnhtmldata.append(htmldata, i, len - i); //we return our data, taking from htmldata, and going from i to out
			break;
		}

		returnhtmldata.append(htmldata, i, startpos - i);  //we return our data, taking from htmldata, and going from i to out

		size_t endpos = htmldata.find("-->", startpos);
		if (endpos == std::string::npos)
		{
			break;

		}
		//Currently all we want to do in this is to just remove all the comments, so lets find the comment tags, and remove them.
	//super simple, look for the scirpt tags, (as they are constant across all websites) and remove everything inbetween.
	//however some sites may not have comments , so we should check.


		i = endpos + std::string("-->").length(); //same as the erase but we dont erase anything, and its dynamic now!
		//if we are good, return this.
	}


	return returnhtmldata;

}

std::string ManageNoScript(const std::string& htmldata)
{

	std::string returnhtmldata;
	returnhtmldata.reserve(htmldata.size()); //what this does, is reserve a chunk that doesnt need to be recalculated over and over.

	//we do the same for strip tags, instead of scanning everything, we just find and remove!
	size_t i = 0;
	const size_t len = htmldata.size();

	while (i < len) //same as strip tags
	{
		size_t startpos = htmldata.find("<noscript", i); //find the first half, from the i (so we dont check back from the start)
		if (startpos == std::string::npos)
		{
			//we need to make sure we save it
			returnhtmldata.append(htmldata, i, len - i); //we return our data, taking from htmldata, and going from i to out
			break;
		}

		returnhtmldata.append(htmldata, i, startpos - i);  //we return our data, taking from htmldata, and going from i to out

		size_t endpos = htmldata.find("</noscript>", startpos);
		if (endpos == std::string::npos)
		{
			break;

		}
		//Currently all we want to do in this is to just remove all the No scirpt tags, so lets find the json tags, and remove them.
		//super simple, look for the scirpt tags, (as they are constant across all websites) and remove everything inbetween.
		//however some sites may not have json, so we should check.


		i = endpos + std::string("</noscript>").length(); //same as the erase but we dont erase anything, and its dynamic now!
		//if we are good, return this.
	}


	return returnhtmldata;

}


int Parser(std::string input)
{
	//all this is doing, is removing, the json part, then the css part, then all of the tags, just to get the final text for this basic parser.
	std::vector<Token> TokenList = StripTags(ManageNoScript(ManageCOMMENTS(ManageCSS(ManageJAVASCIRPT(input))))); //not great to nest, will be fixed, but its ok
	





	return 0;
}