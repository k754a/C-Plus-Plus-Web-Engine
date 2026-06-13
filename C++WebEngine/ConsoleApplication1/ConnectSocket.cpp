#include "ConnectSocket.h"
//THIS IS A GLOBAL SCRIPT.

//this gives compiler specific instructions to c++, telling it to link the winsock libary to our main libary
#pragma comment(lib, "ws2_32.lib")

WSAData wsaData; //structure to hold our network data, across our diffrent voids and ints
//this starts up our winsock at the begining
int StartWinSock()
{
	//start up our WSA for version 2.2, and set the datastructure to our wsaDATA
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 1)
	{
		//failed
		std::cout << "Failed to init Winsock" << std::endl;
		return 1;

	}
	else {
		//worked
		std::cout << "Winsock has been init" << std::endl;
	
	}
	return 0;
}

//kill the winSOCK
int EndWinSock()
{
	std::cout << "Winsock has been stopped" << std::endl;
	//cleanup and stop the winsock
	WSACleanup();
	return 0;
}

//save the URLPath to the page we want to load, we have to have a var out of this as we want to accsess this in our main scirpt.
std::string URLPath;
std::string Cleanup(std::string linkinput)
{

	//this function santizes the input, its super janky but it works lol
	std::string santizedlinkinput = linkinput;

	int s_count = 0; //count how many / we find, if we find 3, figure out how long weve been running, and destory everything past the / 
	for (int i = 0; i < santizedlinkinput.length(); i++)
	{
		
		//if we detect a repeat add it to the score
		if (santizedlinkinput[i] == '/')
		{
			s_count++;
		}
		if (s_count == 1)
		{
			//we do i + 1, because it will miss one of the /
			santizedlinkinput.erase(0, i + 2);
			std::cout << "Removed the start: " + santizedlinkinput << std::endl;
			//we do this to  avoid making this run again
			s_count++;
			
		}
		if (s_count >= 3)
		{
			//if we have seen 3 / (or two, as we add an extra one, just to keep one of the / alive)
			//we save this part, as we will need it
			URLPath = santizedlinkinput.substr(i, santizedlinkinput.length());

			//we then remove the extra part
			santizedlinkinput.erase(i, santizedlinkinput.length());
			//print its completed, and what we got.
			std::cout << "Removed the end: " + santizedlinkinput << std::endl;
			
		}
		
		
	}
	//bit of debug
	std::cout << "sanitized path: " << URLPath << std::endl;
	std::cout << "sanitized link: " << santizedlinkinput << std::endl;

	return santizedlinkinput;
}


int ConnectSocket(std::string input)
{
	//SOCKET id, for our client file descirptor
	//setting AF_INET, means we set it to ipv4, and that tells windows we want somthin like 192.168.43.1
	//then SOCK_STREAM tell windows we want a TCP protocol
	//we leave the last thing blank so windows choses the best protocal

	SOCKET client = socket(AF_INET, SOCK_STREAM, 0);
	if (client == INVALID_SOCKET) { std::cout << "Socket is invalid" << std::endl; return 1;}//check if the socket is good, we cant use null as thats a 0, and valid

	//[ADD CONVERTING FROM LINK TO IP]
	//this is going to be added later, and we are going to use ping (website name) and get the ip

	//first we need to get the host name we want to pull, however it cannot be like hackatime.hackclub.com/?interval=today, it needs to be hackatime.hackclub.com
	std::string host_name = Cleanup(input);

	//this is a network structure to hold infomratin for a dns lookup.
		//typedef struct hostent {
		//	char* h_name;
		//	char** h_aliases;
		//	short h_addrtype;
		//	short h_length;
		//	char** h_addr_list;
		//} HOSTENT, * PHOSTENT, * LPHOSTENT;
	struct hostent* remoteHost;

	//this get host by name pulls the name, addr type, the addr list, and a few other things
	//this is deprecated and i should swich to getaddrinfo, but idk how lol, i might do that one day
	remoteHost = gethostbyname(host_name.c_str());

	if (remoteHost == NULL)
	{
		std::cout << "error, requested host not found." << std::endl;
		return 1;
	}

	//print the data we got, this will look super weird, like 00000170E5143A10, as thats the structure stored in ram.
	std::cout << remoteHost << std::endl;

	//then we create a structure in_addr
		//typedef struct in_addr {
		//	union {
		//		struct {
		//			UCHAR s_b1;
		//			UCHAR s_b2;
		//			UCHAR s_b3;
		//			UCHAR s_b4;
		//		} S_un_b;
		//		struct {
		//			USHORT s_w1;
		//			USHORT s_w2;
		//		} S_un_w;
		//		ULONG S_addr;
		//	} S_un;
		//} IN_ADDR, * PIN_ADDR, * LPIN_ADDR;
	//it looks super complex, but all it is is stroring 4 bytes, of the ip addr
	struct in_addr addr;

	//the U_Long is a datatype, like a char, but we make sure to change it so it works for teh addr.s_addr
	//then we ask it to get the bytes of the internet adress, that we pulled a bit ago.
	//another way to write it is :
	
		//char* ipBytes = remoteHost->h_addr_list[0];

		//u_long* ipAsLong = (u_long*)ipBytes;

		//u_long ipValue = *ipAsLong;

		//addr.s_addr = ipValue;

	addr.s_addr = *(u_long*)remoteHost->h_addr_list[0];

	//then inet_ntoa, converts the binary into a IPV4 adress.
	std::cout << ("First IP Address: ", inet_ntoa(addr)) << std::endl;

	//Socket Adress Internet, its a datastructure to let us store IPV4 adresses and port numbers.
	sockaddr_in server_address;

	//We want to clear our server_address, as its gonna have a crazy amount of junk data, that we really dont need, from our ram.
	memset(&server_address, 0, sizeof(server_address)); //we clear the memory to the size of the server adress.

	//We now need to define what our protocal is, and tell windows that we are using IPV4
	server_address.sin_family = AF_INET;


	//now we need to set the port, and the ipadress
	//the HTONS stands for host to network short, and it takes our port (80) and converts it into the correct format the network needs.
	server_address.sin_port = htons(80);

	
	//ok now we use inet_pton to conver the ip adress into bytes, specifying its ipv4
	//so its (IP-V4, ip adress, the serveradress and port (.))
	inet_pton(AF_INET, inet_ntoa(addr), &server_address.sin_addr);

	//now we define our 
	//pull for data, say its http
	//we ask for the homepage, using http 1.1, then we tell the server what host we want (because some servers like aws have multiple), then we say send data and close
	if (URLPath.empty()) { URLPath = "/"; } 
	//replaced std::string(inet_ntoa(addr)) with host_name, just to work for servers with diffrent ip's

	std::string request = std::string("GET ") + URLPath + std::string(" HTTP/1.1\r\n") + "Host: " + host_name + "\r\n" + "Connection: close\r\n\r\n"; 

	//now we need to connect
	//we connect with our client, stating our adress, and the size of the adress
	if(connect(client, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) { std::cout << "Error connecting to server" << std::endl; return 1; }



	//we have now connected, anything below this is interfacing with the server.



	//send some bytes and see if we get an error

	//we send our client, a bit of data about the computer and our connection, the data we want, the length of our request, and the flags
	int bytes_sent = send(client, request.c_str(), request.length(), 0);
	if(bytes_sent == SOCKET_ERROR) { std::cout << "Error with sending bytes" << std::endl; return 1; }//check if the bytes are good

	

	//now we want to recive a response
	//first we build a buffer
	char buffer[4096];
	int bytesR;

	std::string fulldata = "";
	
	//we call recv to pull data from the socket connected, into our buffer
	//this takes our socket (client) the buffer, the size of the buffer, and flags
	//we check the number of bytes recived, when we get all of them, we will recive 0 bytes, so we can stop, we also do > 0, as if we dont do that, if we have an error, we will need to keep going.
	while ((bytesR = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0)
	{
		//we append the buffer with our data, and make sure we put in the length of it.
		//we dont do -1 like above, because we are trying to get all the bytes, and not a size.
		fulldata.append(buffer, bytesR);
		//print how many bytes
		std::cout << "pulled " << bytesR << " Bytes..." << std::endl;

	}
	
	//move down the terminal so its a bit cleaner
	std::cout << "\033[2J\033[1;1H";


	///--- OVERALL THIS STUFF IS CHECKING IF WE GOT EVERYTHING PULLED CORRECTLY.


	//ok now that we have pulled the data, its time to split the junk,however we want to keep some of it, especally the Content length.
	//lets find that content length first.
	int count = fulldata.find("Content-Length:");
	if (count == std::string::npos) {std::cout << "ERROR - COULD NOT FIND Content-Length" << std::endl;  return -1;}

	//after finding our content length lets extract some of the data past that
	//we create a one call content length, we create a branch of the string, off of this, pulling the value, we check between the context length, and the next part, to pull our data correctly.
	std::string contentlength = fulldata.substr((count + 16), fulldata.find("\r\n", count) - (count + 16));

	//debug.
	std::cout << "the content len is: " << contentlength << std::endl;



	//ok now we need an error check. we need to make sure that we have pulled the full thing before splitting, so we can compare the bytesR to the contentlength.
	//we check the content length, and if its not the same as the full data (minus the infomation part, (thats why we do fulldata.find(and that byte structure to tell it if its past that point)
	if (std::stoi(contentlength) != (fulldata.length() - (fulldata.find("\r\n\r\n") + 4)))
	{
		//if we detect a problem lets attempt to pull again
		std::cout << "Error!, bytes missmatch (" << contentlength << " to " << (fulldata.length() - (fulldata.find("\r\n\r\n") + 4)) << ") attempting to pull again."  << std::endl;
		fulldata = "";
		while ((bytesR = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0)
		{
			fulldata.append(buffer, bytesR); std::cout << "pulled " << bytesR << " Bytes..." << std::endl;
		}
	}

	//if we fail once more, just error out.
	if (std::stoi(contentlength) != (fulldata.length() - (fulldata.find("\r\n\r\n") + 4))){ std::cout << "Error!, bytes missmatch, cannot continue withour a risk of errors. Exiting." << std::endl; return -1; }




	//---








	//now lets use our count data to split it from everything else.
	count = fulldata.find("\r\n\r\n");
	if (count == std::string::npos){ std::cout << "ERROR - CORRUP DATA - COULD NOT FIND \r\n\r\n" << std::endl; return -1; }


	//HOWEVER, IF WE CANT FIND THIS (corrupt data) WE RETURN std::string::npos (no pos, basicly -1), lets do an error check.
	

	fulldata = fulldata.erase(0, count + 4); //we do count + 4 because we remove the \r\n\r\n (each \r or \n is one character)
	std::cout << count << std::endl;
	//print what we got from the buffer, after spliting junk
	std::cout << "\n\n" << fulldata << std::endl;














	return 0;
}