/*
*  FILE          : server.h
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the function prototypes used by the server application to execute it's functionality in full.
*/

#include "shared.h"

#pragma comment (lib, "ws2_32.lib")


//Struct containing the message properties as set by the clients CLA's
typedef struct {
	long blockSize;
	int blockCount;

}MessageProperties;


//Struct used to track the networks performance
typedef struct {
	int blocksReceivedCount;
	int missingBlockCount;
	int disorganizedBlocksCount;
	int blocksReceivedList[MESSAGE_BUFFER_SIZE_10000];

}NetworkResults;


//Function prototypes
int start_server();
int start_server_protocol(int* tcpOrUdp);
void printServerProperties(void);
long getBlockSize(char messageCopy[]);
int convertHexToDecimal(char* messageProperties);
int getNumberOfBlocks(char messageCopy[]);
int getBlockID(char messageCopy[]);
void saveBlockID(int blockIDList[], const int blockID, const int occupiedIndex);
int checkForMissedBlocks(int receivedBlockList[], int blocksReceivedCount);
void packageResults(char messagBuffer[], int packagedValue);
struct sockaddr_in intitializeSocket(void);
int getBlockCount(int blockIDList[]);
void sendResults(SOCKET acceptedSocketConnection, const int missingBlockCount, const int disorganizedBlockCount);
int checkForDisorganizedBlocks(int blockIDList[], int blocksReceivedCount);
int cmpfunc(const void * a, const void * b);