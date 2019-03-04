/*
*  FILE          : client.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the functions and logic required to execute the client's required functionality.
*				   Functions are included for creating, connecting, and closing sockets, and for sending/receiving messages
*				   to and from the server
*/

#pragma once
#include "shared.h"
#include "client.h"
#include "FileIO.h"
#include "ReliableConnection.h"


/*
*  FUNCTION      : start_client_protocol
*  DESCRIPTION   : This function is used to create the client instance of the application.
*  PARAMETERS    : Parameters are as follows,
*	int stream_or_datagram : Denotes if the socket is of type SOCK_STREAM or SOCK_DGRAM
*	int tcp_or_udp		   : Denotes if the protocol is IPPROTO_TCP or IPPROTO_UDO
*  RETURNS       : int : Returns positive if the operation completed without error
*/
int start_client_protocol(const int stream_or_datagram, const int tcp_or_udp)
{
	struct sockaddr_in socketAddress;								//Local address struct
	memset((void*)&socketAddress, 0, sizeof(socketAddress));		//Zero the socket struct before initialization


	//Stage 1: Setup the client's address struct
	socketAddress.sin_family = AF_INET;											
	socketAddress.sin_addr.s_addr = inet_addr(programParameters.ipAddress.c_str());
	socketAddress.sin_port = htons((u_short)(programParameters.port));


	//Stage 2: Host data has been retried and set, proceed to open the socket
	SOCKET openSocketHandle = createSocket(AF_INET, stream_or_datagram, tcp_or_udp);
	if (!(openSocketHandle > ERROR_RETURN))
	{
		printError(SOCKET_CREATION_ERROR);
		return  SOCKET_CREATION_ERROR; 
	}


	//Stage 3: Connect to the server
	int boundSocketHandle = connectToServer(openSocketHandle, socketAddress);
	if (!(boundSocketHandle > SOCKET_ERROR))
	{
		printError(SOCKET_CONNECTION_ERROR);
		return SOCKET_CONNECTION_ERROR;
	}


	//Stage 4: Setup the reliable connection
	ReliableConnection reliableConn(programParameters.ProtocolId, programParameters.TimeOut);
	reliableConn.SetConnectedSocket(boundSocketHandle);
	reliableConn.SetSocketAddress  (socketAddress);



	//Stage 5: Read in the file set by the CLA
	bool binaryFileReadError  = false; 
	string binaryFileContents = FileIO::ReadBinaryFile(storedData[3]);
	if (binaryFileContents.empty())
	{
		binaryFileReadError = true;
		binaryFileContents = "Error: Binary file read error";
	}

	bool asciiFileReadError = false;
	string asciiFileContents = FileIO::ReadAsciiFile(storedData[3]);
	if (asciiFileContents.empty())
	{
		asciiFileReadError = true;
		asciiFileContents = "Error: Ascii file read error";
	}
	if ((binaryFileReadError) && (asciiFileReadError))
	{
		printError(FILE_READ_ERROR);
		return FILE_READ_ERROR;
	}


	try
	{
		//Stage 6: Package the message
		unsigned char* messageBuffer = NULL;
		packageMessage(messageBuffer);
		reliableConn.SendPacket(messageBuffer, sizeof(messageBuffer), socketAddress, sizeof(socketAddress));


		//Stage 7: DEBUG
		memset((void*)messageBuffer, 0, sizeof(messageBuffer));
		reliableConn.ReceivePacket(messageBuffer, sizeof(messageBuffer), socketAddress);
		free(messageBuffer);
		throw new exception;
	}
	catch (...)
	{

		try
		{
			//Close the sockets when finished, or an error occurs
			closesocket(openSocketHandle);
		}
		catch (...) {}
	}
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
*  DESCRIPTION   : This function is used to 
*  PARAMETERS    : Parameters are as follows,
*  RETURNS       : char* : The function has no return value
*/
void packageMessage(unsigned char* message)
{
	
	//unsigned char* returnArray = (unsigned char*) malloc(sizeof(char) * (bufferSize + 1));
	//unsigned char messageProperties[MESSAGE_PROPERTY_SIZE] = { "" };


	//Set the message buffer's properties
	//setMessageProperties(messageProperties, bufferSize, numberOfBlocks, currentMsgNum);
	//strcpy(returnArray, messageProperties);
	//

	////Find the amount of space occupied by the message properties, and offset the index
	//int propertyLength = strlen(returnArray);	
}


#pragma region DEBUGtimer
//Stage 5: Start the timer
//Timer stopwatch;
//stopwatch.startTime = GetTickCount();
//stopwatch.endTime = GetTickCount();
//stopwatch.elapsedTime = stopwatch.endTime - stopwatch.startTime;
#pragma endregion

//sendto(openSocketHandle, messageBuffer, strlen(messageBuffer), 0, (const struct sockaddr*)&socketAddress, len);