/*
*  FILE          : shared.h
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the definitions, prototypes, and global constants used throughout the entirety of the application.
*/


//Disable warnings of things that may be considered unsafe if not watched properly 
#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable: 4996)
using namespace std;


#pragma region Libraries

	#include <stdio.h>
	#include <iostream>
	#include <stdbool.h>
	#include <time.h>
	#include <stdlib.h>
	#include <math.h>
	#include <fstream>
	#include <string>
	#include <list>
	#include <assert.h>
	#include <vector>
	#include <list>
	//#include <string.h>
	//#include <functional>
	#include <winsock2.h>
	#include <WS2tcpip.h>
	#include <windows.h>

#pragma endregion
#pragma region Constants


	//Defined application constants
	#define MESSAGE_BUFFER_SIZE_10000	30000
	#define MESSAGE_BUFFER_SIZE_5000	5000
	#define MESSAGE_BUFFER_SIZE_2000	2000
	#define MESSAGE_BUFFER_SIZE_1000	1000
	#define MAX_ARGUMENT_LENGTH			100
	#define SWITCH_OPTIONS				5
	#define SUCCESS						1
	#define ERROR_RETURN				0


	//Network properties
	#define HOST_BUFFER_SIZE		255
	#define PORT_LENGTH				10
	#define NETWORK_TYPE_TCP		1
	#define NETWORK_TYPE_UDP		2


	//Network error states
	#define SOCKET_CLOSED			0
	#define SOCKET_CREATION_ERROR	-1
	#define SOCKET_BIND_ERROR		-2
	#define SOCKET_LISTEN_ERROR		-3
	#define SOCKET_CONNECTION_ERROR -4
	#define SOCKET_SEND_ERROR		-5
	#define SOCKET_RECEIVE_ERROR	-6
	#define SOCKET_HOST_ERROR		-7
	#define SOCKET_TIMEOUT			-8
	#define SOCKET_SETTINGS_ERROR	-9
	#define FILE_READ_ERROR 		-10


	//Location ID's for saved CLA
	#define CLA_SOCKET_TYPE			0
	#define CLA_IP_ADDRESS			1
	#define CLA_PORT_NUMBER			2
	#define CLA_BUFFER_SIZE			3
	#define CLA_NUMBER_OF_BLOCKS	4


	//Message array index locations
	#define MESSAGE_PROPERTY_SIZE	255
	#define BLOCK_ID_OFFSET			8
	#define BLOCK_ID_LENGTH			5
	#define BLOCK_SIZE_LENGTH		4
	#define BLOCK_SIZE_OFFSET		4
	#define BLOCK_NUM_INDEX			4
	#define BLOCK_SIZE_INDEX		0
	#define ASCII_VALUE_0			48
	#define ASCII_VALUE_9			58
	#define MAX_FORMAT_SIZE			4
	#define CMDLINE_START_OFFSET	2
	#define CMDLINE_MAX_PARAMETERS_CLIENT	10
	#define CMDLINE_MAX_PARAMETERS_SERVER	3


	//ReliableConnection returns
	#define RELIABLE_CONN_HEADER_SIZE 	12
	#define BASE_HEADER_SIZE 			4
	#define RECV_BUFFER_EMPTY 			0
	#define RECV_INCOMPLETE_MSG 		0
	#define RECV_MSG_ERROR 				0

	

	//Reliable Connection
	#define PROTOCOL_ID					0x11223344
	#define DELTA_TIME					1.0f / 30.0f
	#define SEND_RATE					1.0f / 30.0f
	#define TIME_OUT					5.0f
	#define PACKET_SIZE					256


#pragma endregion
#pragma region Structs


	//Global struct for all server/client connection info supplied from the CLA
	typedef struct
	{
		string ipAddress;
		int port;
		string filepath;
		const int ProtocolId = PROTOCOL_ID;
		const float DeltaTime = DELTA_TIME;
		const float SendRate = SEND_RATE;
		const float TimeOut = TIME_OUT;
		const int PacketSize = PACKET_SIZE;
	}ConnectionData;
	static ConnectionData programParameters;



	//Structure defining the attributes of a packet 
	struct PacketData
	{
		unsigned int sequence;			// packet sequence number
		float time;					    // time offset since packet was sent or received (depending on context)
		int size;						// packet size in bytes
	};


	//Structure for keeping time between operations
	typedef struct {

		double startTime;
		double endTime;	
		double elapsedTime;
	}Timer;

#pragma endregion
#pragma region Enums
	
	//Define the Enum used to describe the operating mode of the FlowControl module
	enum Mode 
	{ 
		Good,
		Bad 
	};				

	//Enumerable for setting the connection state of the client/server
	enum State
	{
		Disconnected,
		Listening,
		Connecting,
		ConnectFail,
		Connected
	};


	//Enumerable for defining the state of the client and server
	enum OperatingMode
	{
		Client, 
		Server
	};	
	OperatingMode operatingMode;
	char ipAddress[10];
	char port[10];


#pragma endregion
#pragma region Prototypes


	SOCKET createSocket(int protocolDomain, int socketType, int protocolType);
	void sendMessage(SOCKET connectedSocket, char messageBuffer[], const struct sockaddr_in socketAddress);
	int convertCharToInt(char* stringToConvert);
	int proc_arguments(int argumentCount, char* args[]);
	int validateAddress(char address[]);
	void printError(int errorCode);
	int validateAddress(char address[]);
	int validatePort(char* portString);

#pragma endregion