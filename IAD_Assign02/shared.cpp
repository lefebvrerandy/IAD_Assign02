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

}


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

}



/*
*  FUNCTION      : validateAddress
*  DESCRIPTION   : This function is used to check if the IP address supplied from the command line, is valid according to the
*					standards set by IPv4 (ie. its a 32-bit number of form DDD.DDD.DDD.DDD)
*  PARAMETERS    : char address[] : String containing the IP address
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validateAddress(char address[])
{
	int addressValid = -1;

	//Check if the address in the form of IPv4.
	int errorCount = 0;
	int IPaddressLength = (int)strlen(address);
	if (IPaddressLength == 32)												//IPv4 is 32 bits in length DDD.DDD.DDD.DDD (ex. 192.168.2.100)
	{
		int index = 0;
		for (index = 0; index < IPaddressLength; index++)
		{
			if ((index == 3) || (index == 7) || (index == 11))
			{
				if (address[index] != '.')
				{
					errorCount++;
				}
			}
			else
			{
				if (!((address[index] >= '0') && (address[index] <= '9')))	//Check if the character is a digit of 0 - 9
				{
					errorCount++;
				}
			}
		}
		if (errorCount > 0) { addressValid = -1; }
		else { addressValid = 1; }
	}
	return addressValid;
}

/*  FUNCTION      : validatePort
*  DESCRIPTION   : This function is used to check if the port CLA is valid
*  PARAMETERS    : char* portString : String captured from the CLA indicating the target port number
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int validatePort(char* portString)
{
	int portValid = 0;
	portValid = convertCharToInt(portString);			//return of -1 indicates an error has occurred
	if ((portValid > 0) && (portValid < 65535))
	{
		return portValid;
	}
	return ERROR_RETURN;

}