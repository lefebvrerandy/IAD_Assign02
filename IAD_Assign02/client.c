/*
*  FILE          : client.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the functions and logic required to execute the client's required functionality.
*				   Functions are included for creating, connecting, and closing sockets, and for sending/receiving messages
*				   to and from the server
*/

#if defined _WIN32
#include "client.h"
#include "FileIO.h"


#elif defined __linux__
#include "../inc/client.h"
#endif



/*
*  FUNCTION      : start_client_protocol
*  DESCRIPTION   : This function is used to create the client instance of the application. The client
*  PARAMETERS    : Parameters are as follows,
*	int stream_or_datagram : Denotes if the socket is of type SOCK_STREAM or SOCK_DGRAM
*	int tcp_or_udp		   : Denotes if the protocol is IPPROTO_TCP or IPPROTO_UDO
*  RETURNS       : int : Returns positive if the operation completed without error
*/
int start_client_protocol(string filePath, int stream_or_datagram, int tcp_or_udp)
{
	struct sockaddr_in socketAddress;								//Local address struct
	memset((void*)&socketAddress, 0, sizeof(socketAddress));		//Clear the socket struct before initialization


	//Stage 1: Setup the client's address struct
	socketAddress.sin_family = AF_INET;											
	socketAddress.sin_addr.s_addr = inet_addr(storedData[CLA_IP_ADDRESS]);
	socketAddress.sin_port = htons((u_short)(storedData[CLA_PORT_NUMBER]));


	//Stage 2: Host data has been retried and set, proceed to open the socket
	SOCKET openSocketHandle = createSocket(AF_INET, stream_or_datagram, tcp_or_udp);
	if (!(openSocketHandle > ERROR_RETURN))
	{
		printError(SOCKET_CREATION_ERROR);
		return  SOCKET_CREATION_ERROR; 
	}

	if (strcmp(storedData[0], "-TCP") == 0)
	{
		//Stage 3: Connect to the server
		int boundSocketHandle = connectToServer(openSocketHandle, socketAddress);
		if (!(boundSocketHandle > SOCKET_ERROR))
		{
			printError(SOCKET_CONNECTION_ERROR);
			return SOCKET_CONNECTION_ERROR;
		}
	}

	//Stage 4: Read in the file determined by the CLA
	char* messageBuffer;
	FileIO::ReadBinaryFile(filePath);
	FileIO::ReadAsciiFile(filePath);



	//Stage 5: Start the message loop
	Timer stopwatch;
	#if defined _WIN32
	stopwatch.startTime = GetTickCount();							//Start the Windows timer

	#elif defined __linux__
	stopwatch.startTime = stopWatch();								//Start the UNIX timer

	#endif
	int currentblockCount = 0;
	int len = sizeof(socketAddress);
	messageBuffer = CreateMessageBuffer(blockSize, numberOfBlocks, currentblockCount + 1);
	sendto(openSocketHandle, messageBuffer, strlen(messageBuffer), 0, (const struct sockaddr*)&socketAddress, len);
	//sendMessage(openSocketHandle, messageBuffer, NETWORK_TYPE_UDP, recipient_addr);	//Send the blocks across the network
	currentblockCount++;


	#if defined _WIN32
	stopwatch.endTime = GetTickCount();								//Stop the Windows timer

	#elif defined __linux__
	stopwatch.endTime = stopWatch();								//Stop the UNIX timer

	#endif
	stopwatch.elapsedTime = stopwatch.endTime - stopwatch.startTime;


	//Stage 6: Receive the missing blocks results from the server
	struct sockaddr_in sender_addr;
	len = sizeof(sender_addr);
	memset((void*)messageBuffer, 0, sizeof(messageBuffer));
	recvfrom(openSocketHandle, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr *)&sender_addr, &len);
	free(messageBuffer);


	//Close the sockets
	#if defined _WIN32
		closesocket(openSocketHandle);
	#elif defined __linux__
		close(openSocketHandle);
	#endif
	return SUCCESS;
}

/*
*  FUNCTION      : connectToServer
*  DESCRIPTION   : This function is used to connect to the socket as defined by the arguments
*  PARAMETERS    : Parameters are as follows,
*	SOCKET openSocketHandle : The socket identifier which will be used to connect the client and server
*	struct sockaddr_in socketAddress : The socket struct containing the client's/socket properties
*  RETURNS       : int : Returns an integer denoting if the operation was completed successfully
*/
int connectToServer(SOCKET openSocketHandle, struct sockaddr_in socketAddress)
{
	int newBoundSocket = connect(openSocketHandle, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr));
	return newBoundSocket;
}


/*
*  FUNCTION      : CreateMessageBuffer
*  DESCRIPTION   : This function is used to reserve a chunk of space in memory with the size 
*				   defined at run time by the CLA's
*  PARAMETERS    : Parameters are as follows,
*	int bufferSize		: Size of memory to reserve for the message buffer
*	int numberOfBlocks	: Number of blocks that will be converted to hex
*	int currentMsgNum	: Current message ID that will be converted to hex
*  RETURNS       : char* : The function has no return value
*/
char* CreateMessageBuffer(int bufferSize, int numberOfBlocks, int currentMsgNum)
{
	
	char* returnArray = malloc(sizeof(char) * (bufferSize + 1));
	char messageProperties[MESSAGE_PROPERTY_SIZE] = { "" };


	//Set the message buffer's properties
	setMessageProperties(messageProperties, bufferSize, numberOfBlocks, currentMsgNum);
	strcpy(returnArray, messageProperties);
	

	//Find the amount of space occupied by the message properties, and offset the index
	int propertyLength = strlen(returnArray);	


	//Fill each block with chars 0 - 9
	fillMessageBuffer(returnArray, bufferSize, propertyLength);					
	return returnArray;
}


#if defined __linux__
/*
*  FUNCTION      : stopWatch
*  DESCRIPTION   : This function is used to get the number of milliseconds since the Epoch  (Jan 1, 1970)
*  PARAMETERS    : void: The function takes no arguments
*  RETURNS       : long : Returns the current microsecond count
*
*	NOTE: This function  was initially found online. Since then, the function has been partial modified to suit the projects needs.
*		   As a result, credit belongs to the original author on the website. For more information, please see the reference,
*		   Lee. (2018). How to measure time in milliseconds using ANSI C?. Retrieved on January 8, 2019, from 
				https://stackoverflow.com/questions/361363/how-to-measure-time-in-milliseconds-using-ansi-c/36095407#36095407
*/
double stopWatch(void)
{
	//struct contains the following fields:
	/*
		struct timeval {
			time_t      tv_sec;     //seconds
			suseconds_t tv_usec;    //microseconds
		};
	*/

	struct timeval time;
	if (gettimeofday(&time, NULL) == 0)					//Return of 0 indicates success
	{
		return (time.tv_usec  / 1000);					//Milliseconds = (microseconds  / 1000)
	}
	return ERROR_RETURN;

}


/*
*  FUNCTION      : calculateElapsedTime
*  DESCRIPTION   : This function is used to calculate the elapsed time for message transmission between the networked clients and server
*  PARAMETERS    : long startTime : Start time for when the transmission began
*				   long endTime	  : End time for when the transmission had finished
*  RETURNS       : double : Returns the elapsedTime time between the two values
*/
double calculateElapsedTime(long startTime, long endTime)
{
	return (endTime - startTime);

}
#endif