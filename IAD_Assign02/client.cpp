/*
*  FILE          : client.cpp
*  PROJECT       : CNTR 2115 - A02
*  PROGRAMMER    : Randy Lefebvre 2256 & Bence Karner 5307
*  DESCRIPTION   : This file contains the functions and logic required to execute the client's required functionality.
*				   Functions are included for creating, connecting, and closing sockets, and for sending/receiving messages
*				   to and from the server.
*/


#include "shared.h"
#include "client.h"
#include "FileIO.h"
#include "ReliableConnection.h"
#include "FlowControl.h"


/*
*  FUNCTION      : start_client_protocol
*  DESCRIPTION   : This function is used to create the client instance of the application.
*  PARAMETERS    : Parameters are as follows,
*	int stream_or_datagram : Denotes if the socket is of type SOCK_STREAM or SOCK_DGRAM
*	int tcp_or_udp		   : Denotes if the protocol is IPPROTO_TCP or IPPROTO_UDP
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


	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	FlowControl flowControl;


	/*
	* Delta time is a global defaulted to 1/30
	* Get the round trip time value from the ReliabilitySystem, and set it to a large number. This defaults the clients connection to be in 
	*	bad mode before a message is sent
	*/
	flowControl.Update(programParameters.DeltaTime, (reliableConn.GetReliabilitySystem().GetRoundTripTime() * 1000.0f));
	float sendRate = flowControl.GetSendRate();		//Returns 30 if mode = Good, or 10 if mode = Bad


	//Stage 5: Read in the file in binary or ascii mode
	string fileContents = NULL;
	switch (programParameters.readMode)
	{
		case Binary:
			fileContents = FileIO::ReadBinaryFile(programParameters.filepath);
			
			size_t extension = programParameters.filepath.find_last_of(".");
			programParameters.fileExtension.substr(extension + 1);
			if (fileContents.empty())
			{ 
				//If the file can't be read, then the tests can't be completed; return with an error
				printError(FILE_READ_ERROR);
				return FILE_READ_ERROR;
			}
			break;

		case Ascii:
			fileContents = FileIO::ReadAsciiFile(programParameters.filepath);
			if (fileContents.empty())
			{
				//If the file can't be read, then the tests can't be completed; return with an error
				printError(FILE_READ_ERROR);
				return FILE_READ_ERROR;
			}
			break;
	}
	try
	{
		//Stage 6: Package the message, and send it to the server
		reliableConn.SendPacket((unsigned char *)fileContents.c_str(), sizeof((unsigned char *)fileContents.c_str()), socketAddress, sizeof(socketAddress), programParameters.readMode, programParameters.fileExtension);


		//Stage 7: Receive the servers response, and print the results
		unsigned char* recieveBuffer = (unsigned char*)malloc(sizeof(char) * (PACKET_SIZE));
		reliableConn.ReceivePacket(recieveBuffer, sizeof(recieveBuffer), socketAddress);
		free(recieveBuffer);


		printf("Transmissison Results: \n");
		//printf("File Size: %i\n", CalculateFileSize());
		//printf("Transfer Speed: %i \n", );
		//printf("Sent file MD5: %s \n", sentFileMD5);
		//printf("Recieved file MD5: %s \n", recvFileMD5);
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
*  FUNCTION      : DEBUG
*  DESCRIPTION   : DEBUG
*  PARAMETERS    : Parameters are as follows,
*  RETURNS       : DEBUG
*/
int CalculateFileSize(const unsigned char* file)
{
	int fileSize = 0;



	return fileSize;
}

//sendto(openSocketHandle, messageBuffer, strlen(messageBuffer), 0, (const struct sockaddr*)&socketAddress, len);		//DEBUG REMOVE BEFORE SUBMISSION