/*
*	Reliability and Flow Control Example
*	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
*	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Net.h"
#include "SendAndReceive.h"
#pragma warning(disable: 4996)



using namespace std;
using namespace net;



//Globals
const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;				//Set to 1/30 of some unknown unit; doesn't appear to ever change, and looks like it's used as an counter for the elapsed time/units between Update calls
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;



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
		printf( "flow control initialized\n" );
		Reset();
	}


	
	//In main() -> Used to reset the state of the FlowControl module once the application is set to server mode, and is showing all connections are in the green
	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}


	//deltaTime is defined above as a global (it's the elapsed time (in milliseconds?) since the last Update was performed)
	//Once ReliableConnection.IsConnected = true in main(), call this method to check the connection status again, and adjust it as required 
	void Update(float deltaTime, float roundTripTime)
	{

		
		const float RTT_Threshold = 250.0f;		//Threshold for signaling a passable connection (i.e. < 250) vs. a bad connection (i.e. > 250)
		if (mode == Good)
		{
			
			//If the round trip time is too long, then downgrade our network status to "bad" mode indicating the poor connection
			if (roundTripTime > RTT_Threshold)
			{

				//Set mode from Good -> Bad
				printf( "*** dropping to bad mode ***\n" );
				mode = Bad;


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
		if ( mode == Bad )
		{
				
			if ( roundTripTime <= RTT_Threshold )		//Threshold set to 250.0f
			{
				good_conditions_time += deltaTime;		//Update the time we've had a good connection since the last check
			}
			else
			{
				good_conditions_time = 0.0f;			//Reset the good connection time as our connection is now rated as bad
			}


			//
			if ( good_conditions_time > penalty_time )
			{
				printf( "*** upgrading to good mode ***\n" );
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}


	//Modulate our sending rate based on the current network status (good vs. bad)
	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;	//If mode == Good return a send rate of 30, else return 10
	}


private:
	enum Mode { Good, Bad };				//Define the Enum used to describe the operating mode of the FlowControl module
	Mode mode;								//Declaration of the Enum defined above
	float penalty_time;						//
	float good_conditions_time;				//
	float penalty_reduction_accumulator;	//
};
#pragma endregion



#pragma region Main
int main( int argc, char * argv[] )
{

	enum Mode{ Client, Server};			//Local enumerable for defining the state of the client and server
	Mode operatingMode = Server;		//Default to server operating mode, unless specified by the CLA
	Address address;					//We will use this for holding an address, and for controlling the sockets


	/*
	 *	If there's two or more Command Line Arguments (CLA), check if they supplied all the variables required by 
	 *	the "Address" class for setting the objects "address" field
	*/
	if (argc >= 2)
	{

		int a,b,c,d;	//Trash argument names, but they are used to set the Address objects attributes in its second constructor (See line 112 Constructor (2) in Net.h for an example)
		if ( sscanf( argv[1], "%d.%d.%d.%d", &a, &b, &c, &d ) )
		{
			operatingMode = Client;						//Change the enum mode from Server -> Client
			address = Address(a,b,c,d,ServerPort);		//Set the address field using the bitshift constructor, ServerPort is a global defaulted to 30000
		}
	}

	
	//Run the WSAStartup function to initiate the use of the Winsock DLL by a process
	if (!InitializeSockets())
	{
		printf( "failed to initialize sockets\n" );
		return ERROR_STATE;
	}


	//Instantiate a new connection object of type ReliableConnection (Inherited from Connection superclass)
	ReliableConnection connection(ProtocolId, TimeOut);



	/*
	* Port will be set based on the operating mode chosen from above
	* Syntax is as follows: (condition) ? (if_true) : (if_false)
	*/
	const int port = operatingMode == Server ? ServerPort : ClientPort;



	//Attempt a connection based on the port specified above (i.e. client or server)
	if (!connection.Start(port))
	{
		printf( "could not start connection on port %d\n", port );
		return ERROR_STATE;
	}
	if (operatingMode == Client)
	{
		connection.Connect(address);	//If the app is in client mode, make a connection using the address supplied as the CLA
	}
	else
	{
		connection.Listen();			//If the app is not in client mode, then start listening on the socket created above
	}
		


	bool connectionStatus = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	FlowControl flowControl;
	SendAndReceive sendAndReceive;
	//sendAndReceive.SetReceivingbytes(1, 0);
	//int byte = sendAndReceive.GetReceivingbytes(0);
	//printf("%d",byte);

	//Only way to stop loop is with breaks
	while ( true )
	{

		
		//Update flow control using a Reliable Connection object
		if (connection.IsConnected())
		{
			
			//Delta time is a global defaulted to 1/30
			flowControl.Update(DeltaTime, (connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f));
		}
			
		
		const float sendRate = flowControl.GetSendRate();


		//If the app is in server mode, and both connectionStatus, and  ReliableConnection are reading true, then reset, and flip the connection to false before rentering the loop
		if ( operatingMode == Server && (connectionStatus == true) && !connection.IsConnected() )
		{

			/*
			 *	flowControl.Reset() changes the following properties:
			 * 	mode = Bad;
			 *	penalty_time = 4.0f;
			 *	good_conditions_time = 0.0f;
			 *	penalty_reduction_accumulator = 0.0f;
			 */
			flowControl.Reset();
			printf( "reset flow control\n" );
			connectionStatus = false;
		}



		//If we're not connectionStatus, but the ReliableConnection is engaged, then change our state to be connectionStatus = true
		if ( !connectionStatus && connection.IsConnected() )
		{
			printf( "client connected to server\n" );
			connectionStatus = true;
		}


		//If we're not connectionStatus, and the ReliableConnection is false, then keep our state at connectionStatus = false, and exit the program
		if ( !connectionStatus && connection.ConnectFailed() )
		{
			printf( "connection failed\n" );
			break;
		}


		// send and receive packets
		
		sendAccumulator += DeltaTime;
		
		while ( sendAccumulator > 1.0f / sendRate )
		{
			unsigned char packet[PacketSize] = "test";
			memset( packet, 0, sizeof( packet ) );
			connection.SendPacket( packet, sizeof( packet ) );
			sendAccumulator -= 1.0f / sendRate;
		}
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket( packet, sizeof(packet) );
			if (bytes_read == 0)
				break;
			else
				printf("%d", bytes_read);
		}
		
		// show packets that were acked this frame
		
		#ifdef SHOW_ACKS
		unsigned int * acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks( &acks, ack_count );
		if ( ack_count > 0 )
		{
			printf( "acks: %d", acks[0] );
			for ( int i = 1; i < ack_count; ++i )
				printf( ",%d", acks[i] );
			printf( "\n" );
		}
		#endif


		//Update the connection status, and pass in the elapsed time since the last update was called
		connection.Update( DeltaTime );



		//Show connection stats
		statsAccumulator += DeltaTime;


		
		//General status display that is called as long as we are connected
		while ( statsAccumulator >= 0.25f && connection.IsConnected() )
		{
			
			//Get the round trip time between client and server
			float roundTripTime = connection.GetReliabilitySystem().GetRoundTripTime();


			//Get the packet metrics
			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();


			//Get the bandwidth metrics
			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();


			
			//General print statement for displaying the status of the connection
			printf( "roundTripTime %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n", 
				roundTripTime * 1000.0f, sent_packets, acked_packets, lost_packets, 
				sent_packets > 0.0f ? (float) lost_packets / (float) sent_packets * 100.0f : 0.0f, 
				sent_bandwidth, acked_bandwidth );
			
			statsAccumulator -= 0.25f;
		}

		wait( DeltaTime );
	}


	//Close the socket and exit the program
	ShutdownSockets();
	return 0;
}
#pragma endregion