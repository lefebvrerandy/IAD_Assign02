/*
*  FILE          : client.h
*  PROJECT       : CNTR 2115 - A02
*  PROGRAMMER    : Randy Lefebvre 2256 & Bence Karner 5307
*  DESCRIPTION   : This file contains the function prototypes used by the client.
*/

#pragma once
#include "shared.h"

int start_client_protocol(const int stream_or_datagram, const int tcp_or_udp);
int connectToServer(SOCKET openSocketHandle, struct sockaddr_in socketAddress);
int CalculateFileSize(const unsigned char* file);