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
	inet_pton(AF_INET, "34.230.192.137", &server_address.sin_addr);

	//now we define our 
	//pull for data, say its http
	std::string request = "GET / HTTP/1.1\r\nHost: httpbin.org\r\n\r\n";

	//now we need to connect

	//we connect with our client, stating our adress, and the size of the adress
	if(connect(client, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) { std::cout << "Error connecting to server" << std::endl; return 1; }


	//send some bytes and see if we get an error

	//we send our client, a bit of data about the computer and our connection, the data we want, the length of our request, and the flags
	int bytes_sent = send(client, request.c_str(), request.length(), 0);
	if(bytes_sent == SOCKET_ERROR) { std::cout << "Error with sending bytes" << std::endl; return 1; }//check if the bytes are good

	//now we want to recive a response
	//first we build a buffer
	char buffer[4096];

	//we call recv to pull data from the socket connected, into our buffer
	//this takes our socket (client) the buffer, the size of the buffer, and flags
	int bytesR = recv(client, buffer, sizeof(buffer) - 1, 0);


	std::cout <<"bytes recived: " << bytesR << std::endl;
	return 0;
}