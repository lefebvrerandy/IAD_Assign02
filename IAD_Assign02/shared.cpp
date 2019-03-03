/*
*  FILE          : shared.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains a series of functions required by both client and server applications. 
*/



#include "shared.h"
#include "client.h"
#include "server.h"

/*
*  FUNCTION      : createSocket
*  DESCRIPTION   : This function is used to create and initialize a socket with the appropriate properties as set by the parameters
*  PARAMETERS    : parameters are as follows
*	int addressFamily : AF_INET or PF_INET
*	int socketType	: SOCK_STREAM or SOCK_DGRAM
*	int protocolType: IPPROTO_TCP or IPPROTO_UDO
*  RETURNS       : SOCKET : Returns an initialized socket
*/
SOCKET createSocket(int addressFamily, int socketType, int protocolType)
{
	SOCKET newSocket = socket(addressFamily, socketType, protocolType);
	return newSocket;

}//Done


/*
*  FUNCTION      : sendMessage
*  DESCRIPTION   : This function is used to send a message to the other networked applications, 
*				   across the supplied SOCKET
*  PARAMETERS    : The parameters are as follows,
*	SOCKET connectedSocket	: Socket through which the message will be sent
*	int message[]			: Contains the entire message
*  RETURNS       : void : has no return value
*/
void sendMessage(SOCKET connectedSocket, char messageBuffer[], const struct sockaddr_in socketAddress)
{
	int len = sizeof(socketAddress);
	sendto(connectedSocket, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr*)&socketAddress, len);
}


/*
*  FUNCTION      : convertCharToInt
*  DESCRIPTION   : This function is used to convert characters to an integer
*  PARAMETERS    : char* stringToConvert : The character string that will be converted to its integer counterpart
*  RETURNS       : int : Returns the converted number from the character array
*/
int convertCharToInt(char* stringToConvert)
{
	int returnNumber = 0;
	sscanf(stringToConvert, "%d", &returnNumber);
	return returnNumber;

}//Done