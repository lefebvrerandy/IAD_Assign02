/*
*  FILE          : server.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains all the functions required to operate the sever component of the application.
*				   Functions are included for adding multi thread support, opening TCP & UDP sockets, and sending/receiving 
*				   messages from the clients
*/

#if defined _WIN32
#include "server.h"
#elif defined __linux__
#include "../inc/server.h"
#endif



/*
*  FUNCTION      : start_server
*  DESCRIPTION   : The function is used to create two threads for the server, in order to create a UDP and TCP socket 
*				   for each instance of the server
*  PARAMETERS    : void : This function takes no arguments
*  RETURNS       : int : Returns constant zero indicating the conditional server threading was completed without serious error
*/
int start_server()
{
	int udpArray[2] = { {SOCK_DGRAM}, {IPPROTO_UDP} };

	//Spawn two threads. One for TCP, one for UDP
	HANDLE thread_windows_server[2];

	thread_windows_server[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_server_protocol, (LPVOID)udpArray, 0, NULL);

	printServerProperties();

	WaitForMultipleObjects(2, thread_windows_server, TRUE, INFINITE);
	Sleep(1000000);
	for (int i = 0; i < 2; i++)
	{
		CloseHandle(thread_windows_server[i]);
	}

	return 0;
}


/*
*  FUNCTION      : start_server_protocol
*  DESCRIPTION   : This method is used to create, bind, listen, connect, and receive across standard socket for the server. 
*				   The function acts as a high level controller by calling each of the required functions and setting the properties 
*				   of the socket based on the parameters provided by the user from the command line
*  PARAMETERS    : Function parameters are as follows
*	int* tcpOrUdp		   : Gives detail on the socket type to be created
*  RETURNS       : int : Returns an integer indicating the functions success (ie. return > 0) or failure (ie. return < 0)
*/
int start_server_protocol(int* tcpOrUdp)
{
	
	struct timeval timeout = { 5, 0 };
	int TCPorUDP = tcpOrUdp[1];


	//Stage 1: Create local socket
	SOCKET openSocketHandle = createSocket(AF_INET, tcpOrUdp[0], tcpOrUdp[1]);
	if (!(openSocketHandle > ERROR_RETURN))
	{
		printError(SOCKET_CREATION_ERROR);
		return SOCKET_CREATION_ERROR;	
	}


	//Stage 2: Initialize the socket struct, and bind to the open socket
	struct sockaddr_in socketAddress = intitializeSocket();
	struct sockaddr_in sender_addr;
	auto boundSocketHandle = bind(openSocketHandle, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
	if (!(boundSocketHandle > SOCKET_ERROR))
	{
		printError(SOCKET_BIND_ERROR);				
		return SOCKET_BIND_ERROR;			
	}


	// Only trigger listen if we are using TCP port
	if (TCPorUDP == IPPROTO_TCP)
	{
		//Stage 3: Listen for an incoming connection to the open socket
		boundSocketHandle = listen(openSocketHandle, SOMAXCONN);
		if (!(boundSocketHandle > SOCKET_ERROR))
		{
			printError(SOCKET_LISTEN_ERROR);
			return SOCKET_LISTEN_ERROR;		
		}
	}

	do
	{

		//Stage 4: Accept the incoming client connection
		struct sockaddr_in remoteAddress;
		socklen_t addressSize = sizeof(remoteAddress);
		SOCKET acceptedSocketConnection;
		if (TCPorUDP == IPPROTO_TCP)
		{
			acceptedSocketConnection = accept(openSocketHandle, (struct sockaddr*)&remoteAddress, &addressSize);
			if (!(acceptedSocketConnection > ERROR_RETURN))
			{
				printError(SOCKET_CONNECTION_ERROR);
				return SOCKET_CONNECTION_ERROR;
			}
		}

		// Only set the timer if we are using TCP port
		fd_set readFDs;
		FD_ZERO(&readFDs);							//Clear the file descriptor
		FD_SET(openSocketHandle, &readFDs);	//Set the accepted socket as part of the file descriptor array
		int socketSet = setsockopt(openSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		if (!(socketSet >= 0))
		{
			printError(SOCKET_SETTINGS_ERROR);
			return SOCKET_SETTINGS_ERROR;		//Set return to -1, and print an error for the stage of connection
		}

		int len = sizeof(sender_addr);

		//Stage 6: Receive the clients reply
		char messageBuffer[MESSAGE_BUFFER_SIZE_10000] = { "" };
		int recvStatus = 0;
		while (recvStatus <= 0)
		{
			recvStatus = recvfrom(openSocketHandle, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr *)&sender_addr, &len);
		} 
		int freeIndex = 0;
		char* resizedBuffer = (char*)malloc(sizeof(char));	//DEBUG WILL NEED TO ADJUST THE MALLOC SIZE
		while (true)
		{

			//Get the blocks ID and save it to the list
			memset((void*)messageBuffer, 0, sizeof(messageBuffer));
			memset((void*)resizedBuffer, 0, (sizeof(char)));
			memset((void*)messageBuffer, 0, sizeof(messageBuffer));
			int selectResult = select(0, &readFDs, NULL, NULL, &timeout);	
			if (!(selectResult > 0))
			{

				//Make one final recv() call to ensure the socket is indeed empty
				//recvStatus = recvfrom(openSocketHandle, resizedBuffer, ((sizeof(char)) * protocol.blockSize), 0, (struct sockaddr *)&sender_addr, &len);
				if (!(recvStatus > 0))
				{
					break;
				}
			}
			else
			{
					//recvStatus = recvfrom(openSocketHandle, resizedBuffer, ((sizeof(char)) * protocol.blockSize), 0, (struct sockaddr *)&sender_addr, &len);
			}
			strcpy(messageBuffer, resizedBuffer);
			freeIndex++;
		}
		free(resizedBuffer);

		//Examine the data, and report the results to the client
		//char messageBuffer[MESSAGE_BUFFER_SIZE_10000] = { '\0' };
		//packageResults(messageBuffer, messageData.disorganizedBlocksCount);
		sendto(openSocketHandle, messageBuffer, (int)strlen(messageBuffer), 0, (const struct sockaddr*)&sender_addr, len);

	} while (true);



	closesocket(openSocketHandle);

	return SUCCESS;
}


/*
*  FUNCTION      : sendResults
*  DESCRIPTION   : This method is used to package the results of the transmission, and send them to the client
*  PARAMETERS    : Parameters are as follows,
*	SOCKET acceptedSocketConnection  : Socket used to connect to the client
*	const int missingBlockCount		 : count of the number of blocks missing
*	const int disorganizedBlockCount : Count of the number of blocks that were received in the incorrect order
*  RETURNS       : void : Has no return
*/
void sendResults(SOCKET acceptedSocketConnection, const int missingBlockCount, const int disorganizedBlockCount)
{
	char messageBuffer[MESSAGE_BUFFER_SIZE_10000] = {'\0'};
	packageResults(messageBuffer, missingBlockCount);
	send(acceptedSocketConnection, messageBuffer, (int)strlen(messageBuffer), 0);
	packageResults(messageBuffer, disorganizedBlockCount);
	send(acceptedSocketConnection, messageBuffer, (int)strlen(messageBuffer), 0);
}


/*
*  FUNCTION      : intitializeSocket
*  DESCRIPTION   : This method is used to initialize a struct used for setting up the servers socket settings
*  PARAMETERS    : void : Takes no arguments
*  RETURNS       : struct sockaddr_in  : Returns a fully initialized struct of type sockaddr_in 
*/
struct sockaddr_in intitializeSocket(void)
{
	struct sockaddr_in socketDetials; 
	memset((void*)&socketDetials, 0, sizeof(socketDetials));				//Clear the address struct for initialization
	socketDetials.sin_family = AF_INET;										//Address family internet protocol
	socketDetials.sin_addr.s_addr = htonl(INADDR_ANY);						//Convert from host byte order to network byte order
	socketDetials.sin_port = htons((u_short)storedData[CLA_PORT_NUMBER]);
	return socketDetials;
}


/*
*  FUNCTION      : printServerProperties
*  DESCRIPTION   : This method is used to print the servers properties to the screen
*  PARAMETERS    : void : Takes no arguments
*  RETURNS       : void : Has no return
*/
void printServerProperties(void)
{
	char hostbuffer[HOST_BUFFER_SIZE];
	char *IPbuffer;
	struct hostent *host_entry;
	int hostname;
	char hostPort[PORT_LENGTH];
	strcpy(hostPort, port);
	hostname = gethostname(hostbuffer, sizeof(hostbuffer));
	host_entry = gethostbyname(hostbuffer);
	IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));


	//Print the servers details
	printf("Hostname: %s\n", hostbuffer);
	printf("Host IP: %s\n", IPbuffer);
	printf("Port: %s\n", hostPort);
}


/*
*  FUNCTION      : printError
*  DESCRIPTION   : This function is used to print an error to the console window
*  PARAMETERS    : int errorCode : Int representing the error to print
*  RETURNS       : void : Function has no return
*/
void printError(int errorCode)
{
	switch (errorCode)
	{
		case SOCKET_CREATION_ERROR:
			printf("[ERROR]: Could not create socket");
			break;

		case SOCKET_BIND_ERROR:
			printf("[ERROR]: Could not bind to socket");
			break;

		case SOCKET_LISTEN_ERROR:
			printf("[ERROR]: Could not listen to the socket");
			break;

		case SOCKET_CONNECTION_ERROR:
			printf("[ERROR]: Could not accept new connection");
			break;

		case SOCKET_SEND_ERROR:
			printf("[ERROR]: Could not send message");
			break;

		case SOCKET_RECEIVE_ERROR:
			printf("[ERROR]: Could not receive message");
			break;

		case SOCKET_HOST_ERROR:
			printf("[ERROR]: Could get host by address");
			break;

		case SOCKET_TIMEOUT:
			printf("[ERROR]: Socket connection timed out");
			break;

		case SOCKET_SETTINGS_ERROR:
			printf("[ERROR]: Socket could not be set to non-blocking with a time out");
			break;

		case FILE_READ_ERROR:
			printf("[ERROR]: Unable to read file");
			break;

		default:
			printf("[ERROR]: Unidentified error has occurred");
			break;
	}
}


/*
*  FUNCTION      : packageResults
*  DESCRIPTION   : This method is used to print an integer into a suitable array
*  PARAMETERS    : Function parameters are as follows,
*	char messagBuffer[] : Outbounnd message container
*	int packagedValue   : Int value to be added to the message
*  RETURNS       : void : Function has no return
*/
void packageResults(char messagBuffer[], int packagedValue)
{
	sprintf(messagBuffer, "%d", packagedValue);
}