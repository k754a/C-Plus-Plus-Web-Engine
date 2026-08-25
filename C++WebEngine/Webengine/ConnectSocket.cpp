#include "ConnectSocket.h"
#include "Parser.h"
//THIS IS A GLOBAL SCRIPT.





//=======CLEANUP=======\\

//this function cleans up the url, and splits it, it takes the path and sanitizes it.

std::string SplitURL(std::string OGlink, std::string& outPath) //this returns a string, and takes in the original url and a string that returns and gives the var provided the outPath
{
    outPath = "/"; //resets our var that holds the url path (the "https:\\example.com") part


    std::string host = OGlink; //create a separate hos var (contains the "\" and anything past that) we set it to our original url


    size_t SlashPos = host.find("://"); //attempt to find the position of our ://


    //first check that the url provided contains the ://
    if (SlashPos != std::string::npos) {
        //if it does

        host.erase(0, SlashPos + 3); //remove from h -> / (so 𝐡𝐭𝐭𝐩𝐬:// );
    }


    //we have made it through.

    //now we find the area we need the single /, because we have erased the 𝐡𝐭𝐭𝐩𝐬:// the only place closest is ("example.com\")

    size_t PathPos = host.find("/"); //we attempt to find this "\"


    //first check that the "PathPos" "/" we try to find, does exist!
    if (PathPos != std::string::npos)
    {
        //if it does

        outPath = host.substr(PathPos); //set the url path to before the "\" (example.com)

        host = host.substr(0, PathPos); //set the host to the "\" and after
    }


    //now, we have saved our outPath, so we need to save the other part, so because this is a string

    return host; //we return host.

}//END OF CLEANUP


#ifdef _WIN32 //for windows
    
    //this gives compiler specific instructions to c++, telling it to link the winsock libary to our main libary
    #pragma comment(lib, "ws2_32.lib")

    //this is for HTTPS, we also link the wininet lib to our main lib.
    #pragma comment(lib, "wininet.lib")


    //=======GetWinINetERRORS=======\\

    //grabbed from https://learn.microsoft.com/en-us/windows/win32/wininet/wininet-errors
    std::string GetWinINetERRORS(DWORD errorCode) //this returns a string, and takes in a DWORD errorcode, this is a lookup table for error codes.
    {
    #ifdef _WIN32
        //first grab the error code, the we compare it, using switch
        switch (errorCode) {
        case ERROR_INTERNET_OUT_OF_HANDLES: return "Out of handles";
        case ERROR_INTERNET_OPERATION_CANCELLED: return "Handle was closed before connection could be complete";
        case ERROR_INTERNET_TIMEOUT: return "Request timed out";
        case ERROR_INTERNET_INVALID_URL: return "Invalid URL format";
        case ERROR_INTERNET_NAME_NOT_RESOLVED: return "Server name could not be resolved";
        case ERROR_INTERNET_PROTOCOL_NOT_FOUND: return "The requested protocal could not be found";
        case ERROR_INTERNET_CANNOT_CONNECT: return "Cannot connect to server.";
        case ERROR_INTERNET_CONNECTION_ABORTED: return "Server aborted connection";
        case ERROR_INTERNET_CONNECTION_RESET: return "Connection reset by the server";
        case ERROR_INTERNET_FORCE_RETRY: return "The function needs to do the request.";
        case ERROR_INTERNET_INVALID_CA: return "SSL/TLS Security -> Invalid Certificate Authority";
        case ERROR_INTERNET_SEC_CERT_CN_INVALID: return "SSL/TLS Security -> Hostname mismatch on SSL certificate";
        case ERROR_INTERNET_SEC_CERT_DATE_INVALID: return "SSL/TLS Security -> SSL Certificate has expired";
        case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR: return "Redirected from HTTP to HTTPS";
        default:  return std::to_string(errorCode);

        }
    #endif //WIN32 


    } //END OF GetWinINetERRORS





//=======DOWNLOAD IMAGES=======\\



//DownloadImages handles our image downloads 
    std::vector<unsigned char> DownloadImages(std::string url, bool usingLocal) //we create a vector that can hold chars, and pass our URL input in.
    {
        if (usingLocal) //if it is a local file
        {
            std::ifstream file(url, std::ios::binary); //open the file, setting it to binary (as that's what we want it as)
            return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() }; //return the file's bytes, in the correct format
        }
        std::string localpath; //to hold our path
        std::string path = SplitURL(url, localpath); //grab the "path" of the url.

        //we do a char, as imageData[0] = 0x89; is something we would get, a string would have troubles.
        std::vector<unsigned char> result; //we want to store the result, so we can return it in the correct format


        std::wstring host_name(path.begin(), path.end()); //convert our "path" string, to a wstring

        std::wstring urlPath(localpath.begin(), localpath.end()); //convert our "url" string, to a wstring



        //arguments (What type of browser), (type of access), (info about your proxy), (proxy bypass), (flags). we will set the last 3 to NULL NULL 0.
        //we fake being a browser, to avoid issues like cloudflair.
        HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);



        //check if it properly worked, if not, return result
        if (hInternet == NULL) {
            std::cerr << "Failed to run hInternet" << "\r";
            return result; //error case
        }

        //create our connecters for Handle to Internet, it handles our requests network sessions, and data
        HINTERNET hConnect = InternetConnectW(hInternet, host_name.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);



        //check if our HConnect failed. if so, we return an error, and return the result cleanly.
        if (hConnect == NULL) { std::cerr << "Failed to run hInternet" << "\r";  InternetCloseHandle(hConnect); return result; }



        //got this through a google search too, apparently it will help with avoiding bot detections.
        //list of flags
        DWORD flags =
            INTERNET_FLAG_SECURE |  // HTTPS
            INTERNET_FLAG_RELOAD |  // Don't use cached version
            INTERNET_FLAG_NO_CACHE_WRITE |   // Don't cache our request
            INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS; //Ignore redirects

        //we take the connector value, specify a get, with the string, and put in our flags, this is telling the server to give us the image.

        HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", urlPath.c_str(), L"HTTP/1.1", NULL, NULL, flags, 0); //This was changed to the Wide version, as A was having an issue.

        //check if our HConnect failed. if so, we return an error, close the other handles, and return the result cleanly.
        if (hRequest == NULL)
        {
            //NOTE - This does not have a std::cerr, as if an image just fails to load, we will get issues

            //close the handles, our handle to internet, and handle to request
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hRequest);

            return result; //return the result so far.
        }

        //change it to grab the images
        std::wstring headers =
            L"Accept: image/webp,image/png,image/jpeg,image/*,*/*\r\n"
            L"Accept-Encoding: identity\r\n"
            L"Connection: keep-alive\r\n";


        //if this returns true (we send a request, with our request payload, headers, the size of them)
        if (HttpSendRequestW(hRequest, headers.c_str(), (DWORD)headers.size(), NULL, 0))
        {
            //we create a starting buffer holding 4096 bytes of data
            char buffer[4096];
            DWORD bytesRead = 0;//how many bytes read

            //while we are still getting info from the image we are requesting
            while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) //check if we are reading the file, and we are reading more than 0 bytes, if we are
            {
                //keep add more stuff ontop.
                result.insert(result.end(), buffer, buffer + bytesRead);
            }
        }

        //we are done now


        //now we close the handles for each part, to avoid memeory leaks.
        InternetCloseHandle(hConnect); InternetCloseHandle(hRequest);  InternetCloseHandle(hInternet);



        return result; //return our final result.

    } //END OF DownloadImages



    //=======DOWNLOAD HTTPS=======\\

    //this function grabs the HTTPS, and sends it to the parser
    int ConnectSocketHTTPS(std::wstring input)
    {
        //make a string that holds our final response
        std::string response = "";


        //first lets create a temp var, as its currently a w string
        //we use this to convert into a string where we split the url
        std::string temp(input.begin(), input.end());
        std::string localpath; //create a temp localvar to hold the localpath
        std::string Path = SplitURL(temp, localpath);


        //convert from the string of "path" to a Wstring.
        std::wstring host_name(Path.begin(), Path.end());

        //arguments (What type of browser), (type of access), (info about your proxy), (proxy bypass), (flags). we will set the last 3 to NULL NULL 0.
        //we fake being a browser, to avoid issues like cloudflair.
        HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);


        //check if it properly worked, if not, return result 
        if (hInternet == NULL)
        {
            std::cerr << "Failed to run hInternet" << "\r";
            return 1; //error case
        }

        DWORD optionValue = WININET_API_FLAG_ASYNC; //first we set, do not freeze the main thread when we wait for a connection

        //we do these 2 to let our engine handle redirects.
        InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, new DWORD(10000), sizeof(DWORD)); //ok, then we wait 10 seconds for data to come in 
        InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, new DWORD(10000), sizeof(DWORD)); //then at 10 seconds we have a timeout


        //we now open a TCP connection to the site, but we do not connect yet
        //it wants the url, we copy the parsing above for it
        //the INTERNET_DEFAULT_HTTPS_PORT is port 443
        //then it asks for username and password (we dont need)
        //the next one i dont understand, but the last 2 are flags and context
        HINTERNET hconnect = InternetConnectW(hInternet, host_name.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);


        if (hconnect != NULL)
        {
            //if what we just did was good
            //we build our http request
            //we send like our std::string request = std::string("GET ") + URLPath + std::string(" HTTP/1.1\r\n") + "Host: " + host_name + "\r\n" + "Connection: close\r\n\r\n"; 
            //connection handle, http method, url path (convert to wstring), version, referrer, and account types (we dont care), how we get https, is we use INTERNET FLAG SECURE

            std::wstring temp1(localpath.begin(), localpath.end());

            //got this through a google search too, apparently it will help with avoiding bot detections.
            //list of flags
            DWORD requestFlags =
                INTERNET_FLAG_SECURE |          // HTTPS
                INTERNET_FLAG_RELOAD |          // Don't use cached version
                INTERNET_FLAG_NO_CACHE_WRITE |  // Don't cache our request
                INTERNET_FLAG_KEEP_CONNECTION;  // Persist the connection (helps with cookies)


            //we take the connector value, specify a get, with the string, and put in our flags, this is telling the server to give us the https.
            HINTERNET hRequest = HttpOpenRequestW(hconnect, L"GET", temp1.c_str(), L"HTTP/1.1", NULL, NULL, requestFlags, 0);


            //we check to make sure that the h request sends!
            if (hRequest != NULL)
            {
                //common things browsers use for headers, we do this to convince that we are a real browser.
                std::wstring headers =
                    L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                    L"Accept-Language: en-US,en;q=0.9\r\n"
                    L"Accept-Encoding: identity\r\n"
                    L"Connection: keep-alive\r\n"
                    L"Upgrade-Insecure-Requests: 1\r\n"
                    L"Sec-Fetch-Dest: document\r\n"
                    L"Sec-Fetch-Mode: navigate\r\n"
                    L"Sec-Fetch-Site: none\r\n"
                    L"Sec-Fetch-User: ?1\r\n"
                    L"Cache-Control: max-age=0\r\n";


                //we send our packet of info, and just set everything else to 0 or NULL
                //we make an if and when it runs the if it sees if it returns true, and it works
                if (HttpSendRequestW(hRequest, headers.c_str(), (DWORD)headers.size(), NULL, 0))
                {

                    //check the http status code, as we want to make sure the website did not return a 404.
                    DWORD statusCode = 0; //none
                    DWORD statusCodeSize = sizeof(statusCode); //create another DWORD struct, holding the size of our status code.

                    //grab the header info for the http response, we take the request as input, we want to pull the query, then we want the text to go from "200" -> 200
                    // - &statuscode writes the staus code output to the DWORD, and same with the staus code size.
                    if (HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL))
                    {
                        if (statusCode >= 400) //if we get a status code that is bad (greater than 400)
                        {

                            std::string finalERROR = "<h1> [NETWORK ERROR]</h1><h3> Request failed with status " + std::to_string(statusCode) + "</h3>"; //return some html, that the engine can handle


                            if (statusCode == 404) //if we get 404
                            {
                                finalERROR = "<h1> [NETWORK ERROR]</h1><h3> Request failed with status 404 - Page not found. </h3>"; //return some html, that the engine can handle
                                Parser(finalERROR); //send the error!
                                return -1;
                                //return -1, but in style
                            }
                            else {
                                Parser(finalERROR); //send the error!
                                return -1; //just return -1
                            }
                        }
                    }


                    //create a buffer, to hold our packets
                    char buffer[4096];
                    DWORD bytesRead = 0; //how many bytes read


                    //now lets read!
                    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                    {
                        //keep adding onto our full response
                        response.append(buffer, bytesRead);
                    }


                    //ok now that we have everything, apparently these network nodes will leak resources untill we close them
                }
                else
                {
                    DWORD err = GetLastError(); //pull the last error we got, and store it in a DWORD datatype
                    std::string finalERROR = "<h1> AW SNAP - Something went wrong...</h1><h3> Reason: " + GetWinINetERRORS(err) + "</h3>"; //return some html, that the engine can handle
                    Parser(finalERROR); //send the error!
                    return -1;
                }

                //after we are done
                InternetCloseHandle(hRequest); //DISCONNECT
            }
        }
        else
        {
            InternetCloseHandle(hInternet);
            std::cout << "ERROR - hconnect is Null, thats all we know :(" << std::endl;
            return -1;
        }

        InternetCloseHandle(hconnect); //DISCONNECT
        InternetCloseHandle(hInternet); //DISCONNECT




        //IF it works, lets send it.
        Parser(response);
        return 0;
    } //END OF CONNECT SOCKET HTTPS

    //REMOVED HTTP, AS ITS DEPRECATED.

#else // Linux
    

    //LINUX PART - Uses Linux stuff instead of Windows stuff...
    //(still uses the same functs tho)



     std::vector<unsigned char> DownloadImages(std::string url, bool usingLocal) //we create a vector that can hold chars, and pass our URL input in.
     {

         std::vector<unsigned char> result; //return nothin






         return result; //return our final result.
     }


     //=======DOWNLOAD HTTPS=======\\

    //this function grabs the HTTPS, and sends it to the parser
    int ConnectSocketHTTPS(std::wstring input)
    {
        Parser("<p>Placeholder</p>");
        return 0;
    }

#endif




