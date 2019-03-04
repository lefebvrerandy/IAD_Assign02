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
void packageResults(char messagBuffer[], int packagedValue);
struct sockaddr_in intitializeSocket(void);
void sendResults(SOCKET acceptedSocketConnection, const int missingBlockCount, const int disorganizedBlockCount);
