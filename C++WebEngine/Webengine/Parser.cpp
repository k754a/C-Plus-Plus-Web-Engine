//THIS SCRIPT WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.


#include "Parser.h" 


#include "DOMTree.h" //included for the DOM in the dom tree
#include "GUI.h"
#include <string_view>


//=======CHECK FOR WHITESPACE=======\\
	
//CHECK FOR WHITESPACE.
bool isWhitespace(std::string& input) //this function returns true/false, taking in a single input
{
	//check each char, and return if it contains a space.
	return std::all_of(input.begin(), input.end(), [](unsigned char c) {
		return std::isspace(c); 
	});

}


//=======DECODE ENTITIES======\\

//Decode entities is a std::string method taking in a constant string& from outside
std::string DecodeEntities(const std::string& input)
{
	//our final output to return. //reserve bytes in memory, using our input.size(), this saves time.
	std::string out; out.reserve(input.size()); 

	

	//do something like how we do our main tag stripping
	size_t i = 0;




	while (i < input.size()) //while our i is smaller than our input.size
	{

		
		if (input[i] == '&') //check if it starts with & (as that is the start of the chars we want to remove)
		{
			size_t semi = input.find(';', i); //find the end pos, as then we know its what we are looking for.

			//check if we found the endpos, and its not bigger than 12 char
			if (semi != std::string::npos && semi - i < 20)
			{
				//this is gonna be so slow, so a string_view is faster, and does not over allocate!
				//the std::string_view, takes in the "start", and the "end".
				std::string_view entity(input.data() + i, semi - i + 1);

				//if it = to any of these, we convert them
				if (entity == "&nbsp;") { out += ' '; }
				else if (entity == "&amp;") { out += '&'; }
				else if (entity == "&lt;") { out += '<'; }
				else if (entity == "&gt;") { out += '>'; }
				else if (entity == "&quot;") { out += '"'; }
				else if (entity == "&apos;") { out += '\''; }
				else if (entity == "&mdash;") { out += '-'; out += '-'; }
				else if (entity == "&ndash;") { out += '-'; }
				else if (entity == "&laquo;") { out += '<'; out += '<'; }
				else if (entity == "&raquo;") { out += '>'; out += '>'; }
				else if (entity == "&#160;") { out += ' '; }
				
				else if (entity == "&#xFFFD;") { out += ' '; }
				else if (entity == "&#65533;") { out += ' '; }
				else if (entity.size() > 3 && entity[1] == '#' && entity[2] == 'x') { i = semi + 1; continue; }
				else if (entity.size() > 2 && entity[1] == '#') { i = semi + 1; continue; }
				else
				{
					//unknown, just let it through
					out += input[i];
					i++;
					continue;
				}
				//after we are done, increase the i + the len of our entity.
				i = semi + 1;
				continue;
			}
		}
		
		
		out += input[i++]; //we have made it through the word, go to the next.
	}


	






	return out; //return our final out.

} // END OF DECODE ENTITIES






//=======STRIP TAGS======\\

//Full StripTags code, does not handle things like <a href="link">
std::vector<HTMLToken> StripTags(std::string HtmlData)
{
	std::vector<HTMLToken> tokenList; tokenList.reserve(HtmlData.size() / 20); //saves our tokens, we do a vector to save multiple instances!

	//DEFINE SOME VOID TAGS
	//some tags are allowed to be single, so we will also check for that!
	//I got the tags from https://developer.mozilla.org/en-US/docs/Glossary/Void_element however it was missing quite a few.
	//we use a static constant, as these never change, and use an unordered_set, as its faster.
	static const std::unordered_set<std::string_view> voidTags = {
	"area", "base", "br", "br/", "col", "embed", "hr", "img", "input", 
	"link", "meta", "param", "source", "track", "wbr", "!doctype html"
	};


	
	std::cout << "Running StripTags" << std::endl; //just some debug


	size_t htmlStart = 0; //this var sets where i starts (changes if its HTTPS or just HTTP)

	
	size_t i = htmlStart; 	//size_t is much better than int. This is our starting point.
	int len = HtmlData.length(); //get the length of the html data






	std::string SaveVar; //we will use this in our loop to add to our stack.


	
	std::vector < std::string> TokenStack; TokenStack.reserve(64); //create a list with a string view, this is faster, as we dont need alocation, and we make it a vector, to store strings.



	while (i < len) //repeat till i > than the len.
	{
		
		
		size_t nextTag = HtmlData.find('<', i); //we find the next '<'tag>


		//we check if we found the tag, if we did'nt, we have finished, and can quit.
		if (nextTag == std::string::npos)
		{
			break; //we have finished.	
		}

		//we check if the next tag is > then the i, that means we have text between the "<"next tag> and our <current tag>
		if (nextTag > i)
		{
			std::string text;
			text.assign(HtmlData.substr(i, nextTag - i));//we grab the text in between, by getting next tags "<".

			if (!isWhitespace(text)) //check if we have whitespace.
			{
					
				HTMLToken textT; //we passed the test, we now create the temp token value
					
				//check if we have a &, for decoding
				if (text.find('&') == std::string::npos)
				{
					textT.value = text; //we have'nt found one.
				}
				else { //we have found one
					textT.value = DecodeEntities(text); //now we pass the text, filtering it first.
				}
					
				textT.type = TokenType::TEXT; //then we set the type for the DOM
				tokenList.push_back(textT); //finally we send it to our list.
			}
		}


		
		i = nextTag;// now we set our i to the "<" so we can save it for the next part

		//ok, check if we can find the close tag, (as we are finding the <>)
		size_t closePos = HtmlData.find('>', i);
		if (closePos == std::string::npos) {
			break; //exit the loop, fail condition
		}

		

		//a mistake to watch out for, is that substr, looks for starting char, and length of char, not start point and endpoint (i keep messing that up lol)
		SaveVar.assign(HtmlData, i, (closePos - i + 1));
		
		HTMLToken CurrentToken; //temp class


		//check if this an end tag, the savevar needs to be < 1 (cannot be <>), and the second char ([1]), is a '/'
		if (SaveVar.length() > 1 && SaveVar[1] == '/') {

			//get the close name, we remove the </, and the, and the var - 3 (to remove the >)
			std::string closeName = SaveVar.substr(2, SaveVar.length() - 3);

			size_t spacePos = closeName.find(' '); //check for spaces.

			if (spacePos != std::string::npos) { closeName = closeName.substr(0, spacePos); } //we have a space! because we do lets find and remove it.

			bool matchFound = false; //a temp var, for checking if we have found a match.


			while (!TokenStack.empty()) //only loop, IF we have stuff in our TokenStack.
			{

				if (TokenStack.back() == closeName) //check if our current token matches 
				{
					//we found a match! we add it to the list at the end
					matchFound = true;

					//we remove the matched tag!
					TokenStack.pop_back();
					break;

				}
				else {
					TokenStack.pop_back(); //we pop the token back, and check the next one.
				}

			}

	
			if (matchFound) //now that we have looped through each token in the stack, we now check if we found a match.
			{
				CurrentToken.type = TokenType::END; //set the token type, to the end token
				CurrentToken.value = SaveVar; //set the value to our <'example'>
				tokenList.push_back(CurrentToken); //send it to our list.
			}
		}

		else { //If this is not a closing tag
			std::string tagName;
			tagName.assign(SaveVar, 1, SaveVar.length() - 2); //get the tag name, without the <>
				 
			
			if (!tagName.empty() && tagName.back() == '/') { //handle self closing tags, like <br/>, we remove the '/'
				tagName.pop_back();
			}



			//this checks to see if we have any other properties like <a href="google.com">
			size_t spacePos = tagName.find(' ');
			if (spacePos != std::string::npos) //we fulfill the condition
			{
				tagName = tagName.substr(0, spacePos); //we just remove it rn, TODO - SAVE THE VALUE.
			}

			std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower); //force everything to be lowercase

			
			if ((voidTags.contains(tagName))) //before we throw in the towel, lets check to see if its one of our void tags
			{
				//we will break it if we just have a single non pair, so we convert it to a pair.
				
				CurrentToken.type = TokenType::START; //set the type to start
				CurrentToken.value = SaveVar; //set the value <example>
				tokenList.push_back(CurrentToken); //save the token

				
				HTMLToken CloseToken; //temp class token, for our closing condition.

				CloseToken.type = TokenType::END; //set the type to end
				CloseToken.value = "</" + tagName + ">"; //set the value to </example>
				tokenList.push_back(CloseToken); //send it back.

			}
			else { //not a void tag, but a start tag
				 
				CurrentToken.type = TokenType::START; //set the type to start
				CurrentToken.value = SaveVar; //set the value <example>
				tokenList.push_back(CurrentToken); //save the token

				TokenStack.push_back(tagName); //save the name, like <'example'>
			}


				
		}
				

			
		i = i + SaveVar.length(); //to not index the same thing again, we set the next i point, past the <, so that we find the next point
	}
		
	//we have gone through everything.

	//cleanness
	std::cout << "-------------" << std::endl;


	std::cout << "\r"; //return \r, as its faster.

	return tokenList; //return our processed list

} //END OF STRIPTAGS






//std::string stripTagBlocks
//this will strip the tags, as we do that every time, but we can use one funct, rather than 3

//=======STRIP TAG BLOCKS======\\

std::string StripTagBlocks(const std::string& HtmlData, const std::string& start, const std::string& end)
{
	std::string ReturnHtmlData; ReturnHtmlData.reserve(HtmlData.size()); //holds our html data, we reserve the data, so its faster.

	size_t pos = 0;

	while (true)
	{
		size_t StartPos = HtmlData.find(start, pos); //check if we can find our start tag
		if(StartPos == std::string::npos) { ReturnHtmlData.append(HtmlData, pos, std::string::npos); break;} //this checks if it exists, if it dont, we assume we are done, and finish appending.

		ReturnHtmlData.append(HtmlData, pos, StartPos - pos); //add our data rn, taking from HtmlData and going from i to out.

		size_t EndPos = HtmlData.find(end, StartPos+ start.size()); //check if we can find our end tag
		if (EndPos == std::string::npos) { ReturnHtmlData.append(HtmlData, pos, std::string::npos); break; } //this checks if it exists, if it dont, we assume we are done, and finish appending.



		//now we want to move it forward, so we don't detect again, and find the next one.
		pos = EndPos + end.size();
	}

	//we are done, return.

	return ReturnHtmlData;

} //END OF StripTagBlocks










//=======MANAGE CSS======\\


std::string ManageCSS(const std::string& HtmlData) { return StripTagBlocks(HtmlData, "<style", "</style>"); } //remove everything between <style> and </style>

//=======MANAGE JAVASCIRPT======\\

std::string ManageJAVASCIRPT(const std::string& HtmlData) { return StripTagBlocks(HtmlData, "<script", "</script>"); } //remove everything between <script and </script>


//=======MANAGE COMMENTS======\\

std::string ManageCOMMENTS(const std::string& HtmlData) { return StripTagBlocks(HtmlData, "<!--", "-->"); } //remove everything between <!-- and -->

//=======MANAGE NO SCRIPT======\\

std::string ManageNoScript(const std::string& HtmlData) { return StripTagBlocks(HtmlData, "<noscript", "</noscript>"); } //remove everything between <noscript> and </noscript>

//=======MANAGE TITLE======\\

std::string ManageTITLE(const std::string& HtmlData) { return StripTagBlocks(HtmlData, "<title", "</title>"); } //remove everything between <title> and </title>










//=======REMOVE EVERYTHING BUT CSS======\\

std::string RemoveEverythingButCSS(std::string input) //this returns a string, and takes in a string.
{
	std::string ReturnHtmlData; 
	ReturnHtmlData.reserve(input.size()); //what this does, is reserve a chunk that does'nt need to be recalculated over and over.

	
	size_t pos = 0; //the current pos of our cursor

	while (true) 
	{
		size_t StartPos = input.find("<style", pos); //first find the start pos
		if (StartPos == std::string::npos){ break; } //if we can't find it, break



		size_t tag_end = input.find('>', StartPos); //then find the '>' of the '<style' 
		if (tag_end == std::string::npos) { break; } //if we can't find it, break

		size_t endpos = input.find("</style>", tag_end); //then find the end pos tag, '</style>'
		if (endpos == std::string::npos) { break; } //if we can't find it, break


		//then we want add to our final, adding the inside part to our css list.
		ReturnHtmlData.append(input, (tag_end + 1), (endpos - StartPos - 7));
		

		//then we check for the next one, and repeat!
		pos = endpos + std::string("</style>").length(); 
		
	}
	return ReturnHtmlData; //return the data


} //END OF RemoveEverythingButCSS




//=======STRIP CSS======\\


//this is different from the ManageCSS, this will parse CSS!

int StripCSS(std::string input) { //this returns an int, and takes in a string.

	const size_t len = input.size(); //we only change this once, this saves a bit of time!



	//its stored as [NAME] { Properties }
	std::vector<CSSToken> CSS; //this is a constructer that holds our CSS data
	CSS.reserve(len / 10); //we get the entire html, and we /10, as CSS is much smaller than html.

	//Ok, now we have our stuff, the good news is it so much simpler to tokenize than something like our main html
	//we need to make a dynamic properties list thing, and deal with how we manage that
	//body{
	//background: #eee;
	//width: 60vw;
	//margin: 15vh auto;
	//font - family: system - ui, sans - serif
	//}
	//	h1{
	//		font - size: 1.5em
	//}
	//	div{
	//		opacity: 0.8
	//}
	//a:link, a : visited{
	//	color: #348
	//}



	//ok we first build the loop
	size_t cursor = 0; // we need this to track where we are in the string, rather than erasing it, this is for faster performance.
	

	while (cursor < len) //while our cursor is less than our Input Size
	{
		//find the first thing, we can do that by looking for { as that is the start
		CSSToken temp; //reset every loop. This holds our temp CSS


		//first search for our '{', starting from the pos of our cursor
		size_t start_pos = input.find("{", cursor);
		if (start_pos == std::string::npos){break;} //if we cannot find our '{', we break.

	
		//get the start id, "like h1", we get the raw len, and + cursor, then the startpos of the { - the cursor, this lets us find our 'h1'
		std::string_view id(input.data() + cursor, start_pos - cursor);


		//find the closing brace, but use StartPos so we don't grab a } from before
		size_t end_pos = input.find("}", start_pos);
		if (end_pos == std::string::npos){break;} //if we cannot find our '}', we break.

		




		//now lets make the parser, we want to find each ';', keep the text behind it, then add it to our temp properties
		size_t i = start_pos + 1;

		while (i < end_pos) //between i and end pos
		{
			size_t semi = input.find(";", i); //first we check for ';'
		
			//now we need to find the ;, and save and remove the ; and anything before it.
			size_t prop_start = i;

			size_t prop_end;
			if (semi == std::string::npos || semi > end_pos) //check if we are done
			{
				//get the last one that is missing the semicolon, this means we are at the last one 
				prop_end = end_pos; //we read the closing then
				i = end_pos; //make the loop end (if you don't do this, it freezes)
			}
			else {
				//we are not done, so we first pull it out
				prop_end = semi;
				//go to the next one
				i = semi + 1; //we check for the next property
			}


			//grab the one property using the start and end we just did
			//we need to use prop_start instead of i, because i is already moved forward
			std::string_view GetSemi(input.data() + prop_start, prop_end - prop_start);

			//remove all the whitespace, but not the spaces, as we need those
			
			//trim the whitespace at the start, without having to copy
			while (!GetSemi.empty() && isspace(GetSemi.front()))
			{
				GetSemi.remove_prefix(1);
			}
			//trim the whitespace at the end, without having to copy
			while (!GetSemi.empty() && isspace(GetSemi.back()))
			{
				GetSemi.remove_suffix(1);
			}


			//now just check, to make sure we have something
			if (!GetSemi.empty())
			{
				//ok, now that we found stuff, lets now add it to the temp
				temp.properties.push_back(std::string(GetSemi));
			}

			//move us up
			i = prop_end + 1;

			
		}


		
		CSS.push_back(temp);//send the temp, to our CSS.

		//go past this part, to start on our next part

		cursor = end_pos + 1;
	}






	std::cout << "DONE! " << std::endl; //DEBUG

	//send it to our DOMTREE
	CSSDOM(CSS);


	return 0; //done
} //END OF STRIP CSS





//=======PULL TITLE======\\

std::string PullTITLE(const std::string HtmlData) //PullTITLE returns an std::string, and takes a single string as input
{
	std::string TitlePos; //temp var, to hold our final input

	//there is only one title, so we find the first part
	size_t StartPos = HtmlData.find("<title");
	if (StartPos == std::string::npos) //however, there are times where we don't have a title, so lets check.
	{
		return "unk tab name"; //fail, we could not find one, so lets return a temp tab name.
	}


	//there is only one title, so we find the last part
	size_t endpos = HtmlData.find("</title>");
	if (endpos == std::string::npos)
	{
		return "unk tab name"; //fail, we could not find the end part, so lets return a temp tab name.
	}


	TitlePos = HtmlData.substr(StartPos + 7, endpos - (StartPos + 7)); //now that we have found the pos of the start tag, and the end tag, we can grab it.


	return TitlePos; //return our title

}// END OF PULLTITLE


//=======PARSER======\\

int Parser(std::string input) //this is the global class, and will manage the data we get from the network parser. It returns an int, and takes in a single string as input
{

	//strip the CSS.
	StripCSS(
		RemoveEverythingButCSS(input));

	//strip to get the html, then send it to the DOM.
	DOM(StripTags
		(ManageNoScript(
			ManageCOMMENTS(
					ManageCSS(
						ManageJAVASCIRPT(
								ManageTITLE(input
							)
						)
					)
				)
			) 
		)
	);
	

	
	//grab the title
	std::string title = PullTITLE(input);

	if (!title.empty()) //Insure we do not send a empty title.
	{
		SetTabTitle(title); //Sending to a DOM class
	}
	


	return 0; //Done

} //END OF PARSER