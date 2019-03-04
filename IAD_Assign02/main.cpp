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

	if (argumentCount >= 2)
	{
		// Client
		if (validateAddress((char*)args))
		{
			//programParameters.port = 3001;
			string newString((char*)args);
			programParameters.ipAddress = newString;
			return 2;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		// Server
		//programParameters.port = 3000;
		return 1;
	}
}



#pragma region FlowControl
/*
 * Best way I can describe the class, is as an indicator of network performance. The class never directly interacts with the sockets, Address, or the underlying networking module.
 * - It sits in main, and signals the applications network performance by setting it's mode between "Good" or "Bad" every time it's Update method is called. 
 * - A "Good" network has a "roundTripTime" value of < 250 (unknown units). The apps "SendRate" will likewise toggle between  30 (when mode = Good) and 10 (when mode = bad)
 * - It's private "good_conditions_time" attribute is used as a counter of how long has the network been set as "Good". Every time Update is called, if the mode = Good, 
 *		the good_conditions_time is incremented, if mode = Bad, good_conditions_time is reset to 0 
 */
class FlowControl
{

public:

	//Constructor 
	FlowControl() 
	{
		Reset();
	}

	//In main() -> Used to reset the state of the FlowControl module once the application is set to server mode, and is showing all connections are in the green
	void Reset()
	{
		flowControlMode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}


	//deltaTime is defined above as a global (it's the elapsed time (in milliseconds?) since the last Update was performed)
	//Once ReliableConnection.IsConnected = true in main(), call this method to check the connection status again, and adjust it as required 
	void Update(float deltaTime, float roundTripTime)
	{
		const float RTT_Threshold = 250.0f;		//Threshold for signaling a passable connection (i.e. < 250) vs. a bad connection (i.e. > 250)
		if (flowControlMode == Good)
		{
			
			//If the round trip time is too long, then downgrade our network status to "bad" mode indicating the poor connection
			if (roundTripTime > RTT_Threshold)
			{

				//Set mode from Good -> Bad
				printf( "*** dropping to bad mode ***\n" );
				flowControlMode = Bad;


				//good_conditions_time is field of this class 
				if (good_conditions_time < 10.0f && penalty_time < 60.0f )
				{
					penalty_time *= 2.0f;
					if ( penalty_time > 60.0f )
						penalty_time = 60.0f;
					printf( "penalty time increased to %.1f\n", penalty_time );
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}
			
			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;
			
			if ( penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f )
			{
				penalty_time /= 2.0f;
				if ( penalty_time < 1.0f )
					penalty_time = 1.0f;
				printf( "penalty time reduced to %.1f\n", penalty_time );
				penalty_reduction_accumulator = 0.0f;
			}
		}

		//During the update sequence, check if the connection was downgraded to "bad" quality"
		if ( flowControlMode == Bad )
		{
				
			if ( roundTripTime <= RTT_Threshold )		//Threshold set to 250.0f
			{
				good_conditions_time += deltaTime;		//Update the time we've had a good connection since the last check
			}
			else
			{
				good_conditions_time = 0.0f;			//Reset the good connection time as our connection is now rated as bad
			}
			if ( good_conditions_time > penalty_time )
			{
				printf( "*** upgrading to good mode ***\n" );
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				flowControlMode = Good;
				return;
			}
		}
	}

	//Modulate our sending rate based on the current network status (good vs. bad)
	float GetSendRate()
	{
		return flowControlMode == Good ? 30.0f : 10.0f;	//If mode == Good return a send rate of 30, else return 10
	}


private:
	Mode flowControlMode;								//Declaration of the Enum defined above
	float penalty_time;						//
	float good_conditions_time;				//
	float penalty_reduction_accumulator;	//
};
#pragma endregion


	///	OLD CODE
	///
	//char expectedSwitch[SWITCH_OPTIONS][MAX_ARGUMENT_LENGTH] = { {"-a"}, {"-p"}, {"-s"}, {"-n"} };


	//// Only two argument besides the actual exe call is allowed to start a server
	//if (argumentCount == CMDLINE_MAX_PARAMETERS_SERVER)
	//{
	//	if (strcmp(args[1], "-p") == 0)
	//	{
	//		strcpy(storedData[CLA_PORT_NUMBER], args[CLA_PORT_NUMBER]);		//CLA_PORT_NUMBER is set as 2 in accordance with storedData's declaration in shared.h
	//	}
	//	return 1;
	//}


	//// If 10 arguments, must mean to start client.
	//else if (argumentCount == CMDLINE_MAX_PARAMETERS_CLIENT)
	//{
	//	/*
	//		This section checks and stores the proper arguments into place for later use
	//		REFERENCE: INDEX LOCATION
	//			0			1	 2		3	 4	 5		 6		   7	 8			9
	//	./ispeed	-TCP/-UDP	-a ADDRESS	-p PORT		-s BLOCK_SIZE	-n NUM_BLOCKS
	//	*/

	//	// Check the Type of Connection
	//	if (strcmp(args[1], "-TCP") == 0)
	//		strcpy(storedData[0], args[1]);
	//	else if (strcmp(args[1], "-UDP") == 0)
	//		strcpy(storedData[0], args[1]);


	//	// Iterate through the arguments starting at 2 and iterating by 2 each time through the loop
	//	int j = 0;						 // Index for the 2D storedData array
	//	for (int i = CMDLINE_START_OFFSET; i < CMDLINE_MAX_PARAMETERS_CLIENT; i++)	 // Index for argument
	//	{
	//		// If the expected is found, store the data into the 2d array called "StoredData" 
	//		if (strcmp(args[i], expectedSwitch[j]) == 0)
	//		{
	//			strcpy(storedData[j + 1], args[i + 1]);

	//			int res = 0;
	//			switch (j)
	//			{
	//			case 0:
	//				res = validateAddress(storedData[j]);
	//				break;
	//			case 1:
	//				res = validatePort(storedData[j]);
	//				break;
	//			case 2:
	//				res = validateBlockSize(storedData[j]);
	//				break;
	//			case 3:
	//				res = validateNumOfBlocks(storedData[j]);
	//				break;
	//			}


	//			if (res == 1)
	//			{
	//				return -3;
	//			}
	//		}
	//		else
	//		{
	//			return -2;	// Return an error that a switch was misplaced or not found
	//		}

	//		i++;
	//		j++;
	//	}
	//}


	//if (strcmp(storedData[CLA_SOCKET_TYPE], "-TCP") == 0)			//CLA_SOCKET_TYPE is set to 0, in accordance with storedData's declaration in shared.h
	//{
	//	return 2;
	//}
	//else if (strcmp(storedData[CLA_SOCKET_TYPE], "-UDP") == 0)
	//{
	//	return 3;
	//}
	//return 0;
