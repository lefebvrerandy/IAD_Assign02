/*
*  FILE          : main.c
*  PROJECT       : CNTR 2115 - Assignment #1
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : 2019-01-08
*  DESCRIPTION   : This file contains main, and acts as the primary controller for the solution. Functions are included for 
*				   examining the command line arguments, and directing the flow of the application. The program is used to measure the speed of message
*				   transmission across TCP, and UDP sockets between the server and its clients. The program offers a hyper threaded ANSI C solution for 
*				   the creating networked processes between windows and UNIX systems.
*/

#pragma once
#include "shared.h"
#include "server.h"
#include "client.h"

int main(int argc, char* argv[])
{
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1,1), &wsa_data);
	OperatingMode operatingMode;

    // process the command line arguments
	// If 1 argument, must be start server. 
	// If 5 arguments, must be start client.
    switch(proc_arguments(argc, argv))
    {
		case 1:
			start_server();
			break;
		case 2:
			start_client_protocol(SOCK_DGRAM, IPPROTO_UDP);		//UDP client
			break;

		default:
			break;
    }
    WSACleanup();
    return 0;
}

/*
*  FUNCTION      : proc_arguments
*  DESCRIPTION   : This function is used to identify the number of arguments passed from the command line, and
*				   determine their values
*  PARAMETERS    : Parameters are as follows,
*	int argumentCount: Count of the number of arguments passed in
*	char* args[]	 : Switches passed from the command line
*  RETURNS       : int : Denotes if the operation completed successfully (ie. return > -1)
*/
int proc_arguments(int argumentCount, char* args[])
{

	try
	{
		if (argumentCount >= 2)
		{
			//Client
			if (!validateAddress((char*)args))
			{
				throw new exception();
			}
			if (!validatePort((char*)args[CLA_PORT_NUMBER]))
			{
				throw new exception();

			}
			programParameters.ipAddress = string((char*)args[CLA_IP_ADDRESS]);
			programParameters.port = convertCharToInt((char*)args[CLA_PORT_NUMBER]);
			programParameters.filepath = string((char*)args[CLA_FILEPATH]);
			programParameters.readMode = IdentifyReadMode(args[CLA_FILE_READ_MODE]);
			return START_CLIENT;
		}
		else
		{
			//Server
			programParameters.port = convertCharToInt((char*)args[CLA_PORT_NUMBER]);
			return START_SERVER;
		}
	}
	catch (...) { return ERROR_RETURN; }
}



//DEBUG
FileReadMode IdentifyReadMode(const char* args)
{
	if (strcmp(args, "B") == 0)
	{
		return Binary;
	}
	else if (strcmp(args, "A") == 0)
	{
		return Ascii;
	}
	else
	{
		throw new exception();
	}
}