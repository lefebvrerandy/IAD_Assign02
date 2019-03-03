/*
*  FILE          : server.h
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the function prototypes used by the server application to execute it's functionality in full.
*/


#pragma once
#pragma comment (lib, "ws2_32.lib")
#include "shared.h"



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