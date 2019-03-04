/*
*  FILE          : server.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains all the functions required to operate the sever component of the application.
*				   Functions are included for adding multi thread support, opening TCP & UDP sockets, and sending/receiving 
*				   messages from the clients
*/

#include "server.h"
#include "shared.h"
#include "FileIO.h"
#include "ReliableConnection.h"



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

	//Spawn ta thread for UDP
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
	ReliableConnection reliableConn(programParameters.ProtocolId, programParameters.TimeOut);
	reliableConn.SetConnectedSocket(boundSocketHandle);
	reliableConn.SetSocketAddress(socketAddress);
	do
	{
		programParameters.filepath.clear();	// Clear the string value
		//Stage 4: Accept the incoming client connection
		struct sockaddr_in remoteAddress;
		socklen_t addressSize = sizeof(remoteAddress);
		SOCKET acceptedSocketConnection;


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
		clock_t timeRequired = clock();
		int recvStatus = 0;
		char* recieveBuffer = (char*)malloc(sizeof(char) * (PACKET_SIZE));
		while (recvStatus <= 0)
		{
			recvStatus = reliableConn.ReceivePacket((unsigned char*)recieveBuffer, sizeof(recieveBuffer), socketAddress);
			//recvStatus = recvfrom(openSocketHandle, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr *)&sender_addr, &len);
		} 

		//recieveBuffer now contains the filename and extention. Lets store it
		programParameters.fileExtension = recieveBuffer;

		//char* resizedBuffer = (char*)malloc(sizeof(char));	//DEBUG WILL NEED TO ADJUST THE MALLOC SIZE
		while (true)
		{

			//Get the blocks ID and save it to the list
			memset((void*)recieveBuffer, 0, (sizeof(recieveBuffer)));
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
			programParameters.filepath += recieveBuffer;
		}
		timeRequired = clock() - timeRequired;
		//http://forums.devshed.com/programming-42/convert-timeval-double-568348.html
		float totalTime = ((float)timeRequired / CLOCKS_PER_SEC) - (timeout.tv_sec * 1000000.0 + timeout.tv_usec);
		// fileInString now contains full file.. Lets print that back into a file and get the hash value
		//Print to .//destination//
		if (true/*ASCII*/)
		{
			FileIO::WriteAsciiFile(programParameters.filepath, programParameters.fileExtension);
		}
		else //Binary
		{
			FileIO::WriteBinaryFile(programParameters.filepath, programParameters.fileExtension);
		}

		// Get hash value of file stored in .//destination//
		string tempString = ".//destination//" + programParameters.fileExtension;
		LPCSTR filename = tempString.c_str();
		char* hashValue = GetMd5Value(filename);

		memset((void*)recieveBuffer, 0, (sizeof(recieveBuffer)));
		// Fill recieveBuffer with hashvalue and send
		recieveBuffer = hashValue;
		reliableConn.SendPacket((unsigned char *)recieveBuffer, sizeof((unsigned char *)recieveBuffer), socketAddress, sizeof(socketAddress));

		memset((void*)recieveBuffer, 0, (sizeof(recieveBuffer)));
		// Fill recieveBuffer with time and send
		itoa(totalTime, recieveBuffer,10);	// Store total in recieveBuffer for sending
		reliableConn.SendPacket((unsigned char *)recieveBuffer, sizeof((unsigned char *)recieveBuffer), socketAddress, sizeof(socketAddress));

		// All done
		free(recieveBuffer);
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
	socketDetials.sin_port = htons((u_short)programParameters.port);
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
	int hostPort = programParameters.port;
	//strcpy(hostPort, programParameters.port);
	hostname = gethostname(hostbuffer, sizeof(hostbuffer));
	host_entry = gethostbyname(hostbuffer);
	IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));


	//Print the servers details
	printf("Hostname: %s\n", hostbuffer);
	printf("Host IP: %s\n", IPbuffer);
	printf("Port: %d\n", hostPort);
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



//Stage 5: Start the timer
//Timer stopwatch;
//stopwatch.startTime = GetTickCount();
//stopwatch.endTime = GetTickCount();
//stopwatch.elapsedTime = stopwatch.endTime - stopwatch.startTime;