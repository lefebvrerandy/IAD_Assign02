/*
*  FILE          : main.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains main, and acts as the primary controller for the solution. Functions are included for 
*				   examining the command line arguments, and directing the flow of the application. The program is used to measure the speed of message
*				   transmission across TCP, and UDP sockets between the server and its clients. The program offers a hyper threaded ANSI C solution for 
*				   the creating networked processes between windows and UNIX systems.
*/

#if defined _WIN32
#include "shared.h"
#include "server.h"		//Needed for start_Server() prototype
#include "client.h"		//Needed for clientProtocol() prototype
#elif defined __linux__
#include "../inc/shared.h"
#include "../inc/server.h"
#include "../inc/client.h"	
#endif


int main(int argc, char* argv[])
{
    // startup WinSock in Windows
#if defined _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1,1), &wsa_data);
#endif

    // process the command line arguments
	// If 1 argument, must be start server. 
	// If 5 arguments, must be start client.
    switch(proc_arguments(argc, argv))
    {
    case 1:
        start_server();
        break;
    case 2:
        start_client_protocol(filePath, SOCK_STREAM, IPPROTO_TCP);		//TCP client
        break;
	case 3:
		start_client_protocol(filePath, SOCK_DGRAM, IPPROTO_UDP);		//UDP client
		break;
    }

    // cleanup WinSock in Windows
#if defined _WIN32
    WSACleanup();
#endif

    return 0;
}

/*
*  FUNCTION      : proc_arguments
*  DESCRIPTION   : This function is used to identify the number of arguments passed from the command line, and
*				   determine their values
*  PARAMETERS    : Parameters are as follows,
*	int argumentCount: Count of the number of arguments passed in
*	char* args[]	 : Switches passed from the command line
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int proc_arguments(int argumentCount, char* args[])
{
	char expectedSwitch[SWITCH_OPTIONS][MAX_ARGUMENT_LENGTH] = { {"-a"}, {"-p"}, {"-s"}, {"-n"} };


	// Only two argument besides the actual exe call is allowed to start a server
	if (argumentCount == CMDLINE_MAX_PARAMETERS_SERVER)
	{
		if (strcmp(args[1], "-p") == 0)
		{
			strcpy(storedData[CLA_PORT_NUMBER], args[CLA_PORT_NUMBER]);		//CLA_PORT_NUMBER is set as 2 in accordance with storedData's declaration in shared.h
		}
		return 1;
	}


	// If 10 arguments, must mean to start client.
	else if (argumentCount == CMDLINE_MAX_PARAMETERS_CLIENT)
	{
		/*
			This section checks and stores the proper arguments into place for later use
			REFERENCE: INDEX LOCATION
				0			1	 2		3	 4	 5		 6		   7	 8			9
		./ispeed	-TCP/-UDP	-a ADDRESS	-p PORT		-s BLOCK_SIZE	-n NUM_BLOCKS
		*/

		// Check the Type of Connection
		if (strcmp(args[1], "-TCP") == 0)
			strcpy(storedData[0], args[1]);
		else if (strcmp(args[1], "-UDP") == 0)
			strcpy(storedData[0], args[1]);


		// Iterate through the arguments starting at 2 and iterating by 2 each time through the loop
		int j = 0;						 // Index for the 2D storedData array
		for (int i = CMDLINE_START_OFFSET; i < CMDLINE_MAX_PARAMETERS_CLIENT; i++)	 // Index for argument
		{
			// If the expected is found, store the data into the 2d array called "StoredData" 
			if (strcmp(args[i], expectedSwitch[j]) == 0)
			{
				strcpy(storedData[j + 1], args[i + 1]);

				int res = 0;
				switch (j)
				{
				case 0:
					res = validateAddress(storedData[j]);
					break;
				case 1:
					res = validatePort(storedData[j]);
					break;
				case 2:
					res = validateBlockSize(storedData[j]);
					break;
				case 3:
					res = validateNumOfBlocks(storedData[j]);
					break;
				}


				if (res == 1)
				{
					return -3;
				}
			}
			else
			{
				return -2;	// Return an error that a switch was misplaced or not found
			}

			i++;
			j++;
		}
	}


	if (strcmp(storedData[CLA_SOCKET_TYPE], "-TCP") == 0)			//CLA_SOCKET_TYPE is set to 0, in accordance with storedData's declaration in shared.h
	{
		return 2;
	}
	else if (strcmp(storedData[CLA_SOCKET_TYPE], "-UDP") == 0)
	{
		return 3;
	}
	return 0;
}


/*
*  FUNCTION      : validateAddress
*  DESCRIPTION   : This function is used to check if the IP address supplied from the command line, is valid according to the
*					standards set by IPv4 (ie. its a 32-bit number of form DDD.DDD.DDD.DDD)
*  PARAMETERS    : char address[] : String containing the IP address
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validateAddress(char address[])
{
	int addressValid = -1;


	//Check if the address in the form of IPv4.
	int errorCount = 0;
	int IPaddressLength = strlen(address);
	if (IPaddressLength == 32)											//IPv4 is 32 bits in length DDD.DDD.DDD.DDD (ex. 192.168.2.100)
	{
		int index = 0;
		for (index = 0; index < IPaddressLength; index++)
		{
			if ((index == 3) || (index == 7) || (index == 11) )
			{
				if (address[index] != '.')
				{
					errorCount++;
				}
			}
			else
			{
				if (!((address[index] >= '0') && (address[index] <= '9')))	//Check if the character is a digit of 0 - 9
				{
					errorCount++;
				}
			}
		}

		if (errorCount > 0)
		{
			addressValid = -1;		//Errors detected, address was not valid
		}

		else
		{
			addressValid = 1;		//No errors, address is valid
		}
	}



	return addressValid;

}//Done 


/*
*  FUNCTION      : validatePort
*  DESCRIPTION   : This function is used to check if the port CLA is valid
*  PARAMETERS    : char* portString : String captured from the CLA indicating the target port number
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validatePort(char* portString)
{
	int portValid = 0;
	portValid = convertCharToInt(portString);			//return of -1 indicates an error has occurred
	if ((portValid > 0) && (portValid < 65535))
	{
		return portValid;
	}
	return ERROR_RETURN;

}//Done


/*
*  FUNCTION      : validateBlockSize
*  DESCRIPTION   : This function is used to check if the block size is valid
*  PARAMETERS    : char* blockSizeString : String containing the block size
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validateBlockSize(char* blockSizeString)
{
	int blockSizeValid = convertCharToInt(blockSizeString);
	switch (blockSizeValid)
	{
	case MESSAGE_BUFFER_SIZE_1000:
	case MESSAGE_BUFFER_SIZE_2000:
	case MESSAGE_BUFFER_SIZE_5000:
	case MESSAGE_BUFFER_SIZE_10000:
		blockSizeValid = 1;				//Valid block size
		break;

	default:
		blockSizeValid = ERROR_RETURN;	//Invalid block size
		break;
	}

	return blockSizeValid;

}//Done


/*
*  FUNCTION      : validateNumOfBlocks
*  DESCRIPTION   : This function is used to ensure the block count doesn't exceed a total size of 10,000
*  PARAMETERS    : char* blockCount : String representing the number of blocks to send over the network
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validateNumOfBlocks(char* blockCount)
{
	int blocksValid = convertCharToInt(blockCount);
	if (blocksValid <= 10000)
	{
		return blocksValid;							//Valid block count
	}
	return ERROR_RETURN;

}//Done