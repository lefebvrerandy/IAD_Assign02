/*
*  FILE          : client.h
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the function prototypes used by the clients to execute their functionality in full
*/

#if defined _WIN32
#include "shared.h"
#elif defined __linux__
#include "../inc/shared.h"
#endif



//Timer structure
typedef struct {
	double startTime;
	double endTime;
	double elapsedTime;
}Timer;


//Function prototypes
int start_client_protocol(int stream_or_datagram, int tcp_or_udp);
int connectToServer(SOCKET openSocketHandle, struct sockaddr_in socketAddress);
char* CreateMessageBuffer(int bufferSize, int numberOfBlocks, int currentMsgNum);
void setMessageProperties(char messageProperties[], int bufferSize, int numberOfBlocks, int currentMsgNum);
void fillMessageBuffer(char messageBuffer[], int bufferSize, int messageIndexOffset);
double stopWatch(void);
long unsigned int calculateSpeed(int bytes, int elapsedTimeMS);
void printResults(int size, int sent, int time, long unsigned int speed, int missing, int disordered);
void convertDecToHex(int decimal, char* hexaNum);