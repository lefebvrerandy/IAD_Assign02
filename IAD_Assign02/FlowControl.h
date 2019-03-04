/*
*  FILE          : FlowControl.h
*  PROJECT       : CNTR 2115 - A02
*  PROGRAMMER    : Randy Lefebvre 2256 & Bence Karner 5307
*  DESCRIPTION   : This file contains the functions and logic required to execute the client's required functionality.
*				   Functions are included for creating, connecting, and closing sockets, and for sending/receiving messages
*				   to and from the server.
*/

#pragma once
#include "shared.h"


#pragma region FlowControl
/*
* CLASS			: FlowControl
* DESCRIPTION	: The class functions as indicator of network performance. The class never directly interacts with the sockets, or the underlying networking module.
 * - It sits in the client and server, and signals the applications network performance by setting it's mode between "Good" or "Bad" every time it's Update method is called.
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

	//Used to reset the state of the FlowControl module once the application is set to server mode, and is showing all connections are in the green
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
		const float RTT_Threshold = ROUND_TRIP_THRESHOLD;		//Threshold for signaling a passable connection (i.e. < 250) vs. a bad connection (i.e. > 250)
		if (flowControlMode == Good)
		{

			//If the round trip time is too long, then downgrade our network status to "bad" mode indicating the poor connection
			if (roundTripTime > RTT_Threshold)
			{

				//Set mode from Good -> Bad
				printf("*** dropping to bad mode ***\n");
				flowControlMode = Bad;


				//good_conditions_time is field of this class 
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f) penalty_time = 60.0f;
					
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f) penalty_time = 1.0f;

				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		//During the update sequence, check if the connection was downgraded to "bad" quality"
		if (flowControlMode == Bad)
		{

			if (roundTripTime <= RTT_Threshold)			//Threshold set to 250.0f
			{
				good_conditions_time += deltaTime;		//Update the time we've had a good connection since the last check
			}
			else
			{
				good_conditions_time = 0.0f;			//Reset the good connection time as our connection is now rated as bad
			}
			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
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
		return flowControlMode == Good ? 30.0f : 10.0f;		//If mode == Good return a send rate of 30, else return 10
	}


private:
	Mode flowControlMode;					//Declaration of the Enum defined above
	float penalty_time;						//DEBUG
	float good_conditions_time;				//DEBUG
	float penalty_reduction_accumulator;	//DEBUG
};
#pragma endregion