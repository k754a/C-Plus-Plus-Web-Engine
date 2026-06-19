//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"
#include <vector>
#include <list>
#include <unordered_set> //for the check to make it faster




//ok, lets first define our strucutre
//currently we want to have a start tag, a text, and a end tag
//we say enum just to tell the complier that we wont be changing this
//this is saying class tokentype( start (like <p>), text (like "this is a test"), and end (like <p>))
//i added a VOID tag, and COMMENT, just for the dom tree
enum class TokenType { START, TEXT, END, VOID};

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
	"link", "meta", "param", "source", "track", "wbr", "!doctype html"
	};



	
	std::cout << "Running Striptags" << std::endl; //just some debug
	size_t htmlStart = 0; //this var sets where i starts (changes if its HTTPS or just HTTP)

	//first lets remove the htmldata \r\n\r\n, this is a index, so we will just start from this point in our loop.
	//we check, because if we have https this wont work, and we will pull some random tags lol
	if (htmldata.find("\r\n\r\n") == std::string::npos)
	{
		htmlStart = 0; //we do this because using winInet, it alr removes that stuff, so we dont have to do it!
	}
	else {
		 htmlStart = htmldata.find("\r\n\r\n") + 4; //http does not, so we still do.
	}
	

	size_t i = htmlStart; 	//size_t is much better than int. This is our starting spot, from the stuff above


	int len = htmldata.length(); //get the length of the html data













	std::string Savevar; //we will use this in our loop to add to our stack.


	
	std::vector < std::string > TokenStack; //so we are going to use a list, we use a list because we can check everything, and if they match, we remove from the list. but ofc we add it to the Tokenlist above


	//first lets build the loop.

	//MAIN CHANGE
	//ok, the main thing that i changed, is i went from fufuling multiple conditions with one if, to looping through each <> or <\> and checking if it meets the condtions for a TEXT block
	//a START or and END, this allows it to work so much better, and order the text, because before we printed <> TEXT </> together, now we dont.



	while (i < len) //while our i is greater then our len, so we keep going till our i cant find anymore
	{
		
		
		size_t nextTag = htmldata.find('<', i); //we first find the first spot.


		//we need this because i lowky forgot it, and it ended up crashing lol
		//it checks to make sure that we have found all our things, and if we have we exit.
		if (nextTag == std::string::npos)
		{
			break; //we have finished.	
		}

		//We do this because text is stored between chars like for example <Hello> the index of < is 0 and the index of > is 6.
		//if nexttag is bigger, it means we have text inbetween!
		//this works for nested tags because <><> has no space! even if it does <> <> we ignore whitespaces
		if (nextTag > i)
		{
			std::string text = htmldata.substr(i, nextTag - i);//we grab the data between our "i" and a certain amount of chars after, making sure we grab all our text

				if (!iswhitespace(text)) //our funct to check if we have whitespace
				{


					//we do this lower now
					Token textT;

					//we just made a temp token, now we need to do 
					textT.value = text;
					textT.type = TokenType::TEXT;
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
		
		

		
			Token currenttoken; //build a temp class


			//ok this checks that it contains the full tag, then also checks if there is a / at the end
			//these both need to be true, so if its somthing like <div> it will return false, running the else!
			if(Savevar.length() > 1 && Savevar[1] == '/'){

				//ok now we know there is, we first check if the tokenstack is empty (this was for our starting stuff)
				//then we check that our token we are trying to match is the same as our savevar, and that our savevar is a <\> tag
				if (!TokenStack.empty() && ("</" + TokenStack.back() + ">") == Savevar)
				{

					//we now assume the we found it!
					//first we print it
					std::cout << "Match found! " << std::endl;

					//because its a <\> we are good!

					currenttoken.type = TokenType::END;
					currenttoken.value = Savevar;
					tokenList.push_back(currenttoken);

					//because we dont have a erase[i] what we need to do, is to find our first one, and add our index onto it
					//NOW we can do     TokenStack.pop_back(); as this is better than the erase and does the same thing
					TokenStack.pop_back();
					//because we found a match we break, and that allows us to fix a bug!

				}
			}
			else { //ok this is NOT a closing tag
			
			
				//first we need just the tag name
				std::string tagName = Savevar.substr(1, Savevar.length() - 2);
				 
				//this checks to see if we have any other properties like <a href="google.com">
				size_t spacePos = tagName.find(' ');
				if (spacePos != std::string::npos) //make sure thats not null lol
				{
					//we have a space! because we do lets find and remove it.
					tagName = tagName.substr(0, spacePos);
				}


				//before we throw in the towel, lets check to see if its one of our void tags
				//we check if its not = to voidtags.end() because if we get to the end of a vector and cant find nothing, we are ok with that!
				if ((voidTags.contains(tagName)))
				{
					//currently we do nothing with this, but thats ok, we have a VOID tag, so we will in the future!
					std::cout << "Void Tag Found! " << Savevar << std::endl;
				}
				else {

					//now we save it as a start token! Now that we know its not a void tag
					currenttoken.type = TokenType::START;
					currenttoken.value = Savevar;
					tokenList.push_back(currenttoken);

					//we also push it into our other stack, to match with a closing tag later.
					TokenStack.push_back(tagName);
				}


				
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

//removed old stuff

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
	


	//Sending to a DOM class


	return 0;
}