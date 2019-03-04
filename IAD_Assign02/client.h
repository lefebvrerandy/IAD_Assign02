/*
*  FILE          : client.h
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains the function prototypes used by the clients to execute their functionality in full
*/

#pragma once
#include "shared.h"

int start_client_protocol(const int stream_or_datagram, const int tcp_or_udp);
int connectToServer(SOCKET openSocketHandle, struct sockaddr_in socketAddress);