//THIS WILL PARSE THE DATA GOTTEN FROM THE NETWORK SOCKET.
#include "Parser.h"
#include "DOMTree.h" //included for the DOM in the dom tree
#include "Layout.h" 
#include <vector>
#include <list>
#include <unordered_set> //for the check to make it faster
#include <algorithm>
#include <cctype>
#include <iterator>


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

std::string DecodeEntities(const std::string& input)
{
        std::string out; //the thing we are gonna return out
        //save some size in meme for it, using our input size
        out.reserve(input.size());
        //do somthin like how we do our main tag stripping
        size_t i = 0;

        while (i < input.size())
        {
                if (input[i] == '&') //check if it starts for our unknown entitites.
                {
                        size_t semi = input.find(';', i);
                        //check if its not null, or its not bigger than 12 char
                        if (semi != std::string::npos && semi - i < 20)
                        {
                                //get the thing
                                std::string entity = input.substr(i, semi - i + 1);

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
                                else if (entity.size() > 3 && entity[1] == '#' && entity[2] == 'x') { i = semi + 1; continue; }
                                else if (entity.size() > 2 && entity[1] == '#') { i = semi + 1; continue; }
                                else
                                {
                                        //unkown, just let it through
                                        out += input[i];
                                        i++;
                                        continue;
                                }
                                i = semi + 1;
                                continue;
                        }
                }

                out += input[i++];
        }

        return out;

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
        "area", "base", "br", "br/", "col", "embed", "hr", "img", "input",
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


        size_t i = htmlStart;   //size_t is much better than int. This is our starting spot, from the stuff above


        int len = htmldata.length(); //get the length of the html data


        std::string Savevar; //we will use this in our loop to add to our stack.



        std::vector < std::string > TokenStack; //so we are going to use a list, we use a list because we can check everything, and if they match, we remove from the list. but ofc we add it to the Tokenlist above


        //first lets build the loop.

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
                                // CHANGED WITH AI: decode entities (ported from Windows)
                                textT.value = DecodeEntities(text);
                                textT.type = TokenType::TEXT;
                                tokenList.push_back(textT);
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
                Token currenttoken; //build a temp class


                //ok this checks that it contains the full tag, then also checks if there is a / at the end
                //these both need to be true, so if its somthing like <div> it will return false, running the else!
                if (Savevar.length() > 1 && Savevar[1] == '/') {

                        //ok now we know there is, we first check if the tokenstack is empty (this was for our starting stuff)
                        //then we check that our token we are trying to match is the same as our savevar, and that our savevar is a <\> tag
                        std::string closeName = Savevar.substr(2, Savevar.length() - 3);

                        size_t spacePos = closeName.find(' ');
                        if (spacePos != std::string::npos) //make sure thats not null lol
                        {
                                //we have a space! because we do lets find and remove it.
                                closeName = closeName.substr(0, spacePos);
                        }

                        bool matchFound = false;

                        //if we have stuff in our tokenstack
                        while (!TokenStack.empty())
                        {
                                //check if our top current TokenStack "Token" is the same as the one we are looking for
                                if (TokenStack.back() == closeName)
                                {
                                        //we found a match! we add it to the list at the end
                                        matchFound = true;
                                        //we delate the matched tag!
                                        TokenStack.pop_back();
                                        break;

                                }


                                //because we found a match we break, and that allows us to fix a bug!
                                //it catches if we have open tags
                                TokenStack.pop_back();
                        }

                        if (matchFound)
                        {
                                currenttoken.type = TokenType::END;
                                currenttoken.value = Savevar;
                                tokenList.push_back(currenttoken);
                        }
                }

                else { //ok this is NOT a closing tag


                        //first we need just the tag name
                        std::string tagName = Savevar.substr(1, Savevar.length() - 2);

                        
                        if (!tagName.empty() && tagName.back() == '/') {
                                tagName.pop_back();
                        }

                        //this checks to see if we have any other properties like <a href="google.com">
                        size_t spacePos = tagName.find(' ');
                        if (spacePos != std::string::npos) //make sure thats not null lol
                        {
                                //we have a space! because we do lets find and remove it.
                                tagName = tagName.substr(0, spacePos);
                        }

                        std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

                        //before we throw in the towel, lets check to see if its one of our void tags
                        //we check if its not = to voidtags.end() because if we get to the end of a vector and cant find nothing, we are ok with that!
                        if (voidTags.find(tagName) != voidTags.end())
                        {
                                //currently we do nothing with this, but thats ok, we have a VOID tag, so we will in the future!
                                currenttoken.type = TokenType::START;
                                currenttoken.value = Savevar;
                                tokenList.push_back(currenttoken);

                                // close the tag right after, i didnt do this before and it just wreaked everything.
                                Token CloseToken;
                                CloseToken.type = TokenType::END;
                                CloseToken.value = "</" + tagName + ">";
                                tokenList.push_back(CloseToken);



                        }
                        else {

                                currenttoken.type = TokenType::START;
                                currenttoken.value = Savevar;
                                tokenList.push_back(currenttoken);

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

        std::cout << std::endl;

        return tokenList;

}

std::string pullTITLE(std::string htmldata)
{
        std::string titlepos;
        //there should only be one title btw, so just grab that ig
        size_t startpos = htmldata.find("<title");
        if (startpos == std::string::npos)
        {
                return "unk tab name";
        }

        size_t endpos = htmldata.find("</title>");
        if (endpos == std::string::npos)
        {
                return "unk tab name";
        }

        titlepos = htmldata.substr(startpos + 7, endpos - (startpos + 7));
        //grab between

        return titlepos;

}

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


                i = endpos + std::string("</script>").length(); //same as the erase but we dont erase anything, and its dynamic now!
                //if we are good, return this.
        }


        return returnhtmldata;

}



std::string ManageCOMMENTS(const std::string& htmldata)
{

        std::string returnhtmldata;
        returnhtmldata.reserve(htmldata.size());

        size_t i = 0;
        const size_t len = htmldata.size();

        while (i < len)
        {
                size_t startpos = htmldata.find("<!--", i);
                if (startpos == std::string::npos)
                {
                        returnhtmldata.append(htmldata, i, len - i);
                        break;
                }

                returnhtmldata.append(htmldata, i, startpos - i);

                size_t endpos = htmldata.find("-->", startpos);
                if (endpos == std::string::npos)
                {
                        break;

                }


                i = endpos + std::string("-->").length();
        }


        return returnhtmldata;

}

std::string ManageNoScript(const std::string& htmldata)
{

        std::string returnhtmldata;
        returnhtmldata.reserve(htmldata.size());

        size_t i = 0;
        const size_t len = htmldata.size();

        while (i < len)
        {
                size_t startpos = htmldata.find("<noscript", i);
                if (startpos == std::string::npos)
                {
                        returnhtmldata.append(htmldata, i, len - i);
                        break;
                }

                returnhtmldata.append(htmldata, i, startpos - i);

                size_t endpos = htmldata.find("</noscript>", startpos);
                if (endpos == std::string::npos)
                {
                        break;

                }


                i = endpos + std::string("</noscript>").length();
        }


        return returnhtmldata;

}

std::string RemoveEverythingButCSS(std::string input)
{
        std::string returnhtmldata;
        returnhtmldata.reserve(input.size());

        size_t i = 0;
        const size_t len = input.size();

        while (i < len)
        {
                size_t startpos = input.find("<style", i);

                if (startpos == std::string::npos)
                {
                        break;
                }

                size_t startpos_endtag = input.find('>', startpos);
                if (startpos_endtag == std::string::npos)
                {
                        break;
                }

                size_t endpos = input.find("</style>", startpos_endtag);
                if (endpos == std::string::npos)
                {
                        break;
                }


                returnhtmldata.append(input, (startpos_endtag + 1), (endpos - startpos - 7));


                i = endpos + std::string("</style>").length();
        }

        return returnhtmldata;


}

int StripCSS(std::string input) {

        std::vector<CSSRule> CSS;

        size_t cursor = 0;

        while (cursor < input.size())
        {
                CSSRule temp;


                size_t start_pos = input.find("{", cursor);
                if (start_pos == std::string::npos){break;}

                std::string get = input.substr(cursor, start_pos - cursor);

                get.erase(std::remove_if(get.begin(), get.end(), ::isspace), get.end());
                temp.id = get;


                size_t end_pos = input.find("}", start_pos);
                if (end_pos == std::string::npos){break;}


                size_t i = start_pos + 1;
                while (i < end_pos)
                {
                        size_t semi = input.find(";", i);
                        std::string getsemi;
                        size_t prop_start = i;

                        size_t prop_end;
                        if (semi == std::string::npos || semi > end_pos)
                        {
                                prop_end = end_pos;
                                i = end_pos;
                        }
                        else {
                                prop_end = semi;
                                i = semi + 1;
                        }

                        getsemi = input.substr(prop_start, prop_end - prop_start);


                        if (getsemi.find_first_not_of(" \t\n\r") != std::string::npos && getsemi.find_last_not_of(" \t\n\r") != std::string::npos)
                        {
                                getsemi = getsemi.substr(getsemi.find_first_not_of(" \t\n\r"), (getsemi.find_last_not_of(" \t\n\r") - getsemi.find_first_not_of(" \t\n\r") + 1));

                                temp.properties.push_back(getsemi);
                        }

                        i = prop_end + 1;


                }


                CSS.push_back(temp);

                cursor = end_pos + 1;
        }


        CSSDOM(CSS);


        return 0;
}




int Parser(std::string input)
{
        extern std::string g_pageTitle;
        std::string title = pullTITLE(input);
        if (!title.empty() && title != "unk tab name")
        {
                g_pageTitle = title;
        }

        StripCSS(
                RemoveEverythingButCSS(input));

        DOM(StripTags
                (ManageNoScript(
                        ManageCOMMENTS(
                                ManageCSS(
                                        ManageJAVASCIRPT(input))))));//not great to nest, but its ok



        return 0;
}
