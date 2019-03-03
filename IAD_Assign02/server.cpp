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



/*
*  FUNCTION      : start_server
*  DESCRIPTION   : The function is used to create two threads for the server, in order to create a UDP and TCP socket 
*				   for each instance of the server
*  PARAMETERS    : void : This function takes no arguments
*  RETURNS       : int : Returns constant zero indicating the conditional server threading was completed without serious error
*/
int start_server()
{
	int tcpArray[2] = { {SOCK_STREAM},{IPPROTO_TCP} };
	int udpArray[2] = { {SOCK_DGRAM}, {IPPROTO_UDP} };
	//Spawn two threads. One for TCP, one for UDP
#if defined _WIN32
	HANDLE thread_windows_server[2];

	thread_windows_server[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_server_protocol, (LPVOID)tcpArray, 0, NULL);
	thread_windows_server[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_server_protocol, (LPVOID)udpArray, 0, NULL);

	printServerProperties();

	WaitForMultipleObjects(2, thread_windows_server, TRUE, INFINITE);
	Sleep(1000000);
	for (int i = 0; i < 2; i++)
	{
		CloseHandle(thread_windows_server[i]);
	}

#elif defined __linux__
	pthread_t thread_linux_server[2];
	if (pthread_create(&thread_linux_server[0], NULL, start_server_protocol, (void*)tcpArray) != 0)
	{
		// An error has occurred
		printf("Could not create Thread.");
	}
	else if (pthread_create(&thread_linux_server[1], NULL, start_server_protocol, (void*)udpArray) != 0)
	{
		// An error has occurred
		printf("Could not create Thread.");
	}

	for (int i = 0; i < 2; i++)
	{
		if (pthread_join(thread_linux_server[i], NULL) != 0)
		{
			printf("Cannot join Threads.");
		}
	}

#endif

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
	
	struct timeval timeout = {				
		.tv_sec = 5,						//5 second timeout
		.tv_usec = 0
	};//Tracks socket timeout

	NetworkResults messageData = 
	{			
		.blocksReceivedCount = 0,
		.missingBlockCount = 0,
		.disorganizedBlocksCount = 0,
		.blocksReceivedList = {0}
	};//Tracks the networks status based on the 

	MessageProperties protocol = {			//Tracks message properties
		.blockSize = 0,
		.blockCount = 0
	};

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
	int boundSocketHandle = bind(openSocketHandle, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
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
		if (TCPorUDP == IPPROTO_TCP)
		{
			//Stage 5A: Set the socket options (setsockopt) to have a timeout(seconds) set by struct timeval timeout
			//NOTE: SO_RCVTIMEO is for setting the socket receive timeout; SOL_SOCKET is for setting options at the socket level
			FD_ZERO(&readFDs);							//Clear the file descriptor
			FD_SET(acceptedSocketConnection, &readFDs);	//Set the accepted socket as part of the file descriptor array
			int socketSet = setsockopt(acceptedSocketConnection, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
			if (!(socketSet >= 0))
			{
				printError(SOCKET_SETTINGS_ERROR);
				return SOCKET_SETTINGS_ERROR;		//Set return to -1, and print an error for the stage of connection
			}
		}
		else
		{
			FD_ZERO(&readFDs);							//Clear the file descriptor
			FD_SET(openSocketHandle, &readFDs);	//Set the accepted socket as part of the file descriptor array
			int socketSet = setsockopt(openSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
			if (!(socketSet >= 0))
			{
				printError(SOCKET_SETTINGS_ERROR);
				return SOCKET_SETTINGS_ERROR;		//Set return to -1, and print an error for the stage of connection
			}
		}

		int len = sizeof(sender_addr);

		//Stage 6: Receive the clients reply
		char messageBuffer[MESSAGE_BUFFER_SIZE_10000] = { "" };
		int recvStatus = 0;
		while (recvStatus <= 0)
		{
			if (TCPorUDP == IPPROTO_TCP)
			{
				recvStatus = recv(acceptedSocketConnection, messageBuffer, sizeof(messageBuffer), 0);
			}
			else
			{
				recvStatus = recvfrom(openSocketHandle, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr *)&sender_addr, &len);
			}
		} 


		//Deconstruct the message and get its properties
		protocol.blockSize = getBlockSize(messageBuffer);
		protocol.blockCount = getNumberOfBlocks(messageBuffer);
		int freeIndex = 0;
		char* resizedBuffer = (char*)malloc(sizeof(char) * protocol.blockSize);
		while (true)
		{

			//Get the blocks ID and save it to the list
			saveBlockID(messageData.blocksReceivedList, getBlockID(messageBuffer), freeIndex);
			memset((void*)messageBuffer, 0, sizeof(messageBuffer));
			memset((void*)resizedBuffer, 0, ((sizeof(char)) * protocol.blockSize));

			//Clear the buffer and receive the next message if another one is still expected
			if (TCPorUDP == IPPROTO_TCP)
			{
				int selectResult = select(0, &readFDs, NULL, NULL, &timeout);
				if (!(selectResult > 0))
				{

					//Make one final recv() call to ensure the socket is indeed empty
					recvStatus = recv(acceptedSocketConnection, resizedBuffer, ((sizeof(char)) * protocol.blockSize), MSG_WAITALL);
					if (!(recvStatus > 0))
					{
						break;
					}
				}
				else
				{
					recvStatus = recv(acceptedSocketConnection, resizedBuffer, ((sizeof(char)) * protocol.blockSize), MSG_WAITALL);
				}
			}
			else
			{
				memset((void*)messageBuffer, 0, sizeof(messageBuffer));
				int selectResult = select(0, &readFDs, NULL, NULL, &timeout);
				if (!(selectResult > 0))
				{

					//Make one final recv() call to ensure the socket is indeed empty
					recvStatus = recvfrom(openSocketHandle, resizedBuffer, ((sizeof(char)) * protocol.blockSize), 0, (const struct sockaddr *)&sender_addr, &len);
					if (!(recvStatus > 0))
					{
						break;
					}
				}
				else
				{
					recvStatus = recvfrom(openSocketHandle, resizedBuffer, ((sizeof(char)) * protocol.blockSize), 0, (const struct sockaddr *)&sender_addr, &len);
				}
			}
			strcpy(messageBuffer, resizedBuffer);
			freeIndex++;
		}
		free(resizedBuffer);

		//Examine the data, and report the results to the client
		messageData.blocksReceivedCount = getBlockCount(messageData.blocksReceivedList);													//Get the number of blocks that were received
		messageData.disorganizedBlocksCount = checkForDisorganizedBlocks(messageData.blocksReceivedList, messageData.blocksReceivedCount);	//Get the number of blocks that were disorganized
		qsort(messageData.blocksReceivedList, (size_t)messageData.blocksReceivedCount, sizeof(int), cmpfunc);								//Sort the block ID's from lowest to highest
		messageData.missingBlockCount = checkForMissedBlocks(messageData.blocksReceivedList, messageData.blocksReceivedCount);				//Get the number of blocks that were missing

		if (TCPorUDP == IPPROTO_TCP)
		{
			sendResults(acceptedSocketConnection, messageData.missingBlockCount, messageData.disorganizedBlocksCount);
		}
		else
		{
			char messageBuffer[MESSAGE_BUFFER_SIZE_10000] = { '\0' };
			packageResults(messageBuffer, messageData.missingBlockCount);
			sendto(openSocketHandle, messageBuffer, strlen(messageBuffer), 0, (const struct sockaddr*)&sender_addr, len);
			packageResults(messageBuffer, messageData.disorganizedBlocksCount);
			sendto(openSocketHandle, messageBuffer, strlen(messageBuffer), 0, (const struct sockaddr*)&sender_addr, len);
		}
		#if defined _WIN32
		if (TCPorUDP == IPPROTO_TCP)
		{
			closesocket(acceptedSocketConnection);
		}
		#elif defined __linux__
		if (TCPorUDP == IPPROTO_TCP)
		{
			close(acceptedSocketConnection);
		}
		#endif
	} while (true);


	#if defined _WIN32
		closesocket(openSocketHandle);
	#elif defined __linux__
		close(openSocketHandle);
	#endif
	return SUCCESS;
}


/*
*  FUNCTION      : cmpfunc
*  DESCRIPTION   : This method is used to compare two int values when called as the comparison function from qsort
*  PARAMETERS    : Function parameters are as follows,
*	const void * a : The first integer
*	const void * b : The second integer
*  RETURNS       : int : Returns an integer of the two values subtracted from one another
*
*	NOTE: This function was taken entirely from an online tutorial hosted by TurotialsPoint. For more information, please see the reference below
*	Tutorials Point. (ND). C library function - qsort(). Retrieved on January 20, 2018, from
		https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
*/
int cmpfunc(const void * a, const void * b) 
{
	return (*(int*)a - *(int*)b);
}


/*
*  FUNCTION      : getBlockCount
*  DESCRIPTION   : This method is used to get a count of the number of blocks that were received during transmission
*  PARAMETERS    : Function parameters are as follows,
*	int blockIDList[]	: Int array containing the blocks ID's received during the connections lifetime
*  RETURNS       : int : Returns an integer of how many block ID's were received
*/
int getBlockCount(int blockIDList[])
{
	int numberOfChars = 0;
	for (unsigned int index = 0; index < MESSAGE_BUFFER_SIZE_10000; index++)
	{
		if (blockIDList[index] != 0)
		{
			numberOfChars++;
		}
	}
	return numberOfChars;
}


/*
*  FUNCTION      : checkForDisorganizedBlocks
*  DESCRIPTION   : This method is used to compare the ID's of two blocks, and check if they sequentially increase, or if an ID
*					is incorrect according to the numbering scheme
*  PARAMETERS    : Function parameters are as follows,
*	int blockIDList[]		: Int array containing the blocks ID's received during the connections lifetime
*	int blocksReceivedCount : The count of how many blocks were received (used to limit the loop count when checking the ID's)
*  RETURNS       : int : Returns an integer of how many block ID's were disorganized
*/
int checkForDisorganizedBlocks(int blockIDList[], int blocksReceivedCount)
{

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// The ID of each element will be compared to see if they increment sequentially by one; if
	//	the difference between the ID's is not one, then the disorganizedCounter will be incremented
	// 
	//  NOTE: In this case, we're not counting the number of missed packets, only if the order in which 
	//  they arrived was sequential
	//
	//	For example: First Comparison
	//	char blockIDList[] = {1, 3, 7, 4, 5, 6, 10}
	//	diffBetweenElems = elemTwo - elemOne
	//	diffBetweenElems = 3 - 1
	//	diffBetweenElems = 2 Therefore a packet was missed
	//	
	//	Increment the disorganizedCount and check the next to elements 7 & 3
	//
	//	Second Comparison:
	//	diffBetweenElems = elemTwo - elemOne
	//	diffBetweenElems = 7 - 3
	//	diffBetweenElems = 4 Therefore a packet was missed
	//
	//	Increment the disorganizedCount and check the next to elements 4 & 7
	//	
	/////////////////////////////////////////////////////////////////////////////////////
	
	unsigned int index;
	int numDisorganizedBlocks = 0;
	int elemOne = 0;
	int elemTwo = 0;

	//Only enter the array if there is at least two elements
	//Default disorganizedCount to 0 when only 1 block received
	for(index = 0; index < (blocksReceivedCount - 1); index++)
	{
		if (blockIDList[index] != 0)
		{
			elemOne = blockIDList[index];
		}
		if (blockIDList[++index] != 0)
		{
			elemTwo = blockIDList[index];
		}
		if ((elemOne != 0) && (elemTwo != 0))
		{
			if (elemTwo != (elemOne + 1))
			{
				numDisorganizedBlocks++;
			}
			index--;
		}
	}

	return numDisorganizedBlocks;
}


/*
*  FUNCTION      : countDigits
*  DESCRIPTION   : This method is used to get a count of the number of digits in number passed in as an argument
*  PARAMETERS    : const int arg : An integer that will be counted to determine how many digits it contains
*  RETURNS       : int : Returns an integer of how many digits are in the number
* 
*	NOTE: This function was taken entirely from an online post on stackoverflow.com. For more information, please see the reference.
*		   AShelly. (2012). Count number of digits - which method is most efficient?. Retrieved on January 15, 2018, from
				https://stackoverflow.com/questions/9721042/count-number-of-digits-which-method-is-most-efficient
*/
int countDigits(const int arg)
{
	return snprintf(NULL, 0, "%d", arg) - (arg < 0);
}


/*
*  FUNCTION      : void saveBlockID(int blockIDList[], const int blockID, const int freeIndex)
*  DESCRIPTION   : This method is used to copy the block's ID into the int array for use later when the server examines the results of the transmission
*  PARAMETERS    : Parameters are as follows,
*	int blockIDList[]	: Array containing a list of all block ID's received during the transmission
*	const int blockID	: The blocks ID that will be added to the array
*	const int freeIndex : The position in the array to write the ID at
*  RETURNS       : void : Function has no return
*/
void saveBlockID(int blockIDList[], const int blockID, const int freeIndex)
{
	int digitLength = countDigits(blockID);
	blockIDList[freeIndex] = blockID;
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
	send(acceptedSocketConnection, messageBuffer, strlen(messageBuffer), 0);
	packageResults(messageBuffer, disorganizedBlockCount);
	send(acceptedSocketConnection, messageBuffer, strlen(messageBuffer), 0);
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
	strcpy(hostPort, storedData[CLA_PORT_NUMBER]);
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

		default:
			printf("[ERROR]: Unidentified error has occurred");
			break;
	}
}


/*
*  FUNCTION      : getBlockSize
*  DESCRIPTION   : This method is used to get the block size from the message header,
*					and convert it to a single decimal value
*  PARAMETERS    : Function parameters are as follows
*	char messageBuffer[] : Message sent from the client
*  RETURNS       : long : Returns an long indicating the block size 
*/
long getBlockSize(char messageBuffer[])
{
	//Get the block size sub string from the message
	char* messageProperties = (char*)malloc(sizeof(char) * MESSAGE_PROPERTY_SIZE);
	int index = 0;
	for (index = 0; index < BLOCK_SIZE_LENGTH; index++)								
	{
		//The block size will never be larger than 4 chars
		messageProperties[index] = messageBuffer[index];	
	}
	messageProperties[index] = '\0';


	//Get the decimal value from the string 
	long blockSize = convertHexToDecimal(messageProperties);
	free(messageProperties);
	return blockSize;
}


/*
*  FUNCTION      : convertHexToDecimal
*  DESCRIPTION   : This method is used to convert a hexadecimal string of characters into a single integer of equivalent value
*  PARAMETERS    : Function parameters are as follows
*	char messageProperties : Pointer to the string containing the message properties
*  RETURNS       : int : Returns an int cast as a long, representing the decimal value of the hex string
*/
int convertHexToDecimal(char* messageProperties)
{
	int convertedHex = 0;
	sscanf(messageProperties, "%x", &convertedHex);
	return (long)convertedHex;
}


/*
*  FUNCTION      : getNumberOfBlocks
*  DESCRIPTION   : This method is used to get the number of expected blocks from the message header, and convert them from
*				   hex to decimal form
*  PARAMETERS    : Function parameters are as follows
*	char messageBuffer[] : Copy of the message array
*  RETURNS       : int : Returns an integer indicating the functions success (ie. return > 0) or failure (ie. return < 0)
*/
int getNumberOfBlocks(char messageBuffer[])
{
	char blockCountArray[MESSAGE_PROPERTY_SIZE] = { "" };


	//Scan the remainder of the string until the letter 'G' is encountered
	int i = BLOCK_SIZE_OFFSET;
	int j = 0;
	for (i = BLOCK_SIZE_OFFSET; i < (BLOCK_SIZE_OFFSET + BLOCK_SIZE_LENGTH); i++)
	{
		blockCountArray[i] = messageBuffer[i + BLOCK_SIZE_OFFSET];	//Block size will always be from index 4 to 7

		//Copy each element of the block count string
		blockCountArray[j] = messageBuffer[i];
		j++;
	}
	blockCountArray[j] = '\0';

	int blockCount = 0;
	blockCount = convertHexToDecimal(blockCountArray);
	return blockCount;

}


/*
*  FUNCTION      : getBlockID
*  DESCRIPTION   : This method is used to find the block ID in the message header, and convert the hex value to a decimal form
*  PARAMETERS    : Function parameters are as follows
*	char messageBuffer[] : Copy of the message array
*  RETURNS       : int : Returns an integer indicating the functions success (ie. return > 0) or failure (ie. return < 0)
*/
int getBlockID(char messageBuffer[])
{
	char blockIDSubSet[MESSAGE_PROPERTY_SIZE] = { "" };

	//Copy each element of the block count string
	for (int i = 0; i < BLOCK_SIZE_LENGTH; i++)
	{
		blockIDSubSet[i] = messageBuffer[i + BLOCK_ID_OFFSET];	//Block ID will always be from index 8 - 11

	}

	//Convert the hex string representing the block ID, and return the result
	int blockID = convertHexToDecimal(blockIDSubSet);
	return blockID;
}


/*
*  FUNCTION      : getDifference
*  DESCRIPTION   : This method is used to get the difference between two integers passed in as arguments
*  PARAMETERS    : Function parameters are as follows
*	const int elemOne : Int used to get the difference
*	const int elemTwo : Int used in the subtraction
*  RETURNS       : int : Returns an integer of the absolute difference between the two ints
*/
int getDifference(const int elemOne, const int elemTwo)
{
	return abs(elemOne - elemTwo);
}


/*
*  FUNCTION      : checkForMissedBlocks
*  DESCRIPTION   : This method is used to get the number of missing ID's between each element.
*			       The array of ID's are sorted first, and then a comparison is made to count if
*				   the ID's increment by 1, or if some numbers are missing
*  PARAMETERS    : Function parameters are as follows,
*	int receivedBlockList[] : Array containing the ID's of all the blocks that have been received
*  RETURNS       : int : Retruns an int that represents the number of blocks missed during transmission
*/
int checkForMissedBlocks(int receivedBlockList[], int blocksReceivedCount)
{
	int missingBlocks = 0;
	int elemOne = 0;
	int elemTwo = 0;
	for (unsigned int index = 0; index < (blocksReceivedCount - 1); index++)
	{
		elemOne = receivedBlockList[index];
		elemTwo = receivedBlockList[++index];
		missingBlocks += getDifference(elemOne, elemTwo) - 1;	//Difference - 1 shows the number of missing ID's between each element (eg: 10 - 7 = 3 - 1 = 2 missing ID's (8 & 9))
		index--;												//Restore index to value before elemTwo was calculated
	}

	return missingBlocks;
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