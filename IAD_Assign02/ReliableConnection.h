/*
*  FILE          : DEBUG
*  PROJECT       : DEBUG
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  DESCRIPTION   : DEBUG
*/


#pragma once
#include "shared.h"
#include "Network.h"


/*
* CLASS			: ReliableConnection
* DESCRIPTION	: DEBUG
*/
class ReliableConnection
{

private:

	unsigned int protocolId;				//Set in main as 0x11223344
	float timeoutLimit;						//Set in main as 5 seconds
	State state;							//Indicates the connection state as 0 (Disconnected) - 4 (Connected)
	float timeoutCounter;					//Tracks the elapsed time Update() and ReceivePacket() calls  
	ReliabilitySystem reliabilitySystem;	//Manages sequence numbers and acks, tracks network stats etc.
	SOCKET connectedSocket;
	struct sockaddr_in socketAddress;

public:

	//Constructor
	ReliableConnection(unsigned int newProtocolId, float newTimeoutLimit, unsigned int max_sequence = 0xFFFFFFFF)
	{
		protocolId = newProtocolId;			//Set in main as 0x11223344
		timeoutLimit = newTimeoutLimit;		//Set in main as 5.0
		state = Disconnected;				//Default to a disconnected state when first creating the object
		timeoutCounter = 0.0f;				//Updated every time Update(), and ReceivePacket() are called
		reliabilitySystem.Reset();			//Zeros the reliabilitySystem's counters (ready for a new connection)
	}

	//Destructor
	~ReliableConnection() {}


	/*
	*  METHOD        : SendPacket
	*  DESCRIPTION   : Sends a full message through the socket
	*  PARAMETERS    : Defined below,
	*	const unsigned char packetData[] : Contains the message that will be send
	*	int messageSize : Size of the message in bytes
	*  RETURNS			: bool : Returns true if the message as sent without error
	*/
	bool SendPacket(const unsigned char packetData[], int messageSize, struct sockaddr_in socketAddress, int len, FileReadMode fileReadMode, const string fileExtension)
	{
		//RELIABLE_CONN_HEADER_SIZE is a global set to 12					
		unsigned char* messageContainer = (unsigned char*)malloc(messageSize + RELIABLE_CONN_HEADER_SIZE);


		/*
		*  Values are set to 0 if this is the first message sent,
		*  otherwise, they are tracked, and incremented as messages are exchanged
		*  These don't need to be touched as far as I know
		*/
		int seq = reliabilitySystem.GetLocalSequence() + 5;		//Local sequence number for most recently sent packet
		int ack = reliabilitySystem.GetRemoteSequence() + 1;		//Remote sequence number for most recently received packet
		int ack_bits = reliabilitySystem.GenerateAckBits();	//Count of the acknowledged bits


		messageContainer[0] = reinterpret_cast<char>(&seq);
		messageContainer[2] = '0' + ack;
		messageContainer[4] = (unsigned char)ack_bits;

		//memcpy(messageContainer, (char*)sequence, 2);
		//memcpy(messageContainer + 4, (char*)ack, 2);
		//memcpy(messageContainer + 8, (char*)ack_bits, 2);
		messageContainer[0] = reinterpret_cast<unsigned char>(&seq);
		messageContainer[2] = (unsigned char)ack;
		messageContainer[4] = (unsigned char)ack_bits;

		memcpy(messageContainer + RELIABLE_CONN_HEADER_SIZE, packetData, messageSize);


		//Define the outbound packet, and add 5 bytes of extra space for the IP address 
		// data in the first 5 indexes
		unsigned char* packet = (unsigned char*)malloc(messageSize + PACKET_HEADER_SIZE);
		if (fileReadMode == Ascii)
		{
			packet[0] = (unsigned char)('A');
		}
		else
		{
			packet[0] = (unsigned char)('B');
		}


		/*
		* Copy in the file extension into the message header
		* The file extension will always be a max of three chars, and will ocupy index's one to three. 
		*	For example : 
		*	-> exe, txt, dat, bin etc. 
		*/
		packet[1] = (unsigned char)(fileExtension.c_str());


		//Copy the message contents to the outbound packet, and wave that bitch goodbye
		memcpy(&packet[4], messageContainer, (int)sizeof(messageContainer));
		bool sendComplete = sendto(connectedSocket, (const char*)packet, messageSize + 4, 0, (const struct sockaddr*)&socketAddress, len);
		if (!(sendComplete == true))
		{
			return false;
		}


		//DEBUG
		reliabilitySystem.PacketSent(messageSize);
		free(packet);
		free(messageContainer);
		return true;
	}


	/*
	*  METHOD        :
	*  DESCRIPTION   :
	*	1)
	*	2)
	*	3)
	*	4)
	*  PARAMETERS    : Defined below,
	*  RETURNS       :
	*/
	int ReceivePacket(unsigned char data[], int bufferSize, struct sockaddr_in socketAddress)
	{
		if ((int)bufferSize <= RELIABLE_CONN_HEADER_SIZE) return false;													//Header set to 12
		unsigned char* packet = (unsigned char*)malloc(bufferSize + RELIABLE_CONN_HEADER_SIZE + BASE_HEADER_SIZE);	//Base header size set to 4
		char receivePacket[256];
		memset((void*)receivePacket, 0, (sizeof(receivePacket)));
		SOCKET sender_addr;
		socklen_t fromLength = sizeof(sender_addr);
		int len = sizeof(socketAddress);

		//Receive a message from the socket
		int bytes_read =	recvfrom(connectedSocket,	 (char*)receivePacket,		bufferSize,				0, (sockaddr*)&sender_addr,	&fromLength);
		memcpy(packet, &receivePacket, sizeof(receivePacket));
		int bytes_read = recvfrom(connectedSocket, (char*)packet, bufferSize + RELIABLE_CONN_HEADER_SIZE + BASE_HEADER_SIZE, 0, (struct sockaddr*)&socketAddress, &len);


		//
		if (bytes_read == 0)
		{
			return RECV_BUFFER_EMPTY;		//Set return to 0
		}
		else if (bytes_read <= RELIABLE_CONN_HEADER_SIZE)
		{
			return RECV_INCOMPLETE_MSG;		//Set return to 0
		}
		else if (packet[0] != (unsigned char)(protocolId >> 24) ||
			packet[1] != (unsigned char)((protocolId >> 16) & 0xFF) ||
			packet[2] != (unsigned char)((protocolId >> 8) & 0xFF) ||
			packet[3] != (unsigned char)(protocolId & 0xFF))
		{
			return RECV_MSG_ERROR;			//Set return to 0
		}
		int received_bytes = 0;


		/*
		* If the IP addresses match, then we know the packet was from the client
		* Reset the timeout counter as the message came in correctly
		*/
		timeoutCounter = 0.0f;


		//Copy the received message to the fourth index of the packet
		memcpy(data, &packet[4], (bytes_read - 4));
		received_bytes = bytes_read - 4;

		//
		unsigned int packet_sequence = 0;
		unsigned int packet_ack = 0;
		unsigned int packet_ack_bits = 0;


		//
		ReadHeader(packet, packet_sequence, packet_ack, packet_ack_bits);
		reliabilitySystem.PacketReceived(packet_sequence, received_bytes - RELIABLE_CONN_HEADER_SIZE);
		reliabilitySystem.ProcessAck(packet_ack, packet_ack_bits);
		memcpy(data, packet + RELIABLE_CONN_HEADER_SIZE, received_bytes - RELIABLE_CONN_HEADER_SIZE);


		free(packet);
		return received_bytes - RELIABLE_CONN_HEADER_SIZE;
	}


	/*
	*  METHOD        : Update
	*  DESCRIPTION   : Adds the arguments value to the timeoutCounter; resets the connection and
	*				   reliabilitySystem counters if the timeoutCounter is greater than the timeoutLimit
	*  PARAMETERS    : float deltaTime : global defined in main as 1.0/30.0f
	*  RETURNS       : void : Returns no values
	*
	* NOTE: The method is always called from main, directly after ReceivePacket() has been called enough times that the sockets
	*	incoming buffer is emptied, and returns zero bytes received. It then takes the global "DeltaTime" which is defined
	*	in main as 1.0/30.0f
	*/
	void Update(float deltaTime)
	{

		//Increment the timeoutCounter based on the elapsed time since the last ReceivePacket() call was made
		timeoutCounter += deltaTime;
		if (timeoutCounter > timeoutLimit)
		{

			//If the socket was open and accepting connections, then reset all (connection and reliabilitySystem) counters to zero, 
			//	and print an error message indicating the timeout limit has been reached
			if ((state == Connecting) || (state == Connected))
			{
				printf("ERROR: connection timed out \n");
				state = ConnectFail;
				ClearData();
			}
		}

		//If the connection was ON, reset the counting process for the reliabilitySystem
		//Else, just tack on the additional time passed in by the argument
		reliabilitySystem.Update(deltaTime);
	}

	/*
	*  METHOD        :
	*  DESCRIPTION   :
	*  PARAMETERS    : Defined below,
	*  RETURNS       :
	*/
	void ReadInteger(const unsigned char * data, unsigned int & value)
	{
		value = (((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) |
			((unsigned int)data[2] << 8) | ((unsigned int)data[3]));
	}

	/*
	*  METHOD        : ReadHeader
	*  DESCRIPTION   :
	*  PARAMETERS    : Defined below,
	*	const unsigned char * header :
	*	unsigned int & sequence :
	*	unsigned int & ack 		:
	*	unsigned int & ack_bits :
	*  RETURNS       : void : Returns no values
	*/
	void ReadHeader(const unsigned char* header, unsigned int& sequence, unsigned int& ack, unsigned int& ack_bits)
	{
		ReadInteger(header, sequence);
		ReadInteger(header + 4, ack);
		ReadInteger(header + 8, ack_bits);
	}

	/*
	*  METHOD        : ClearData
	*  DESCRIPTION   : Resets the connections properties, and reliabilitySystem's counters to zero, also clears the saved IP address
	*  PARAMETERS    : void : Takes no arguments
	*  RETURNS       : void : Returns no values
	*/
	void ClearData()
	{
		state = Disconnected;
		timeoutCounter = 0.0f;
		reliabilitySystem.Reset();
	};



	void OnStop(void)								{ ClearData(); }									//When the connection is stopped, or closed, reset the counter data used to ensure the connection is reliable
	void OnDisconnect(void)							{ ClearData(); }									//When the connection is stopped, or closed, reset the counter data used to ensure the connection is reliable
	ReliabilitySystem& GetReliabilitySystem(void)	{ return reliabilitySystem; }						//Return a reference to the reliabilitySystem object
	struct sockaddr_in GetSocketAddressStruct(void) { return socketAddress; }							//
	SOCKET GetConnectedSocket(void)					{ return connectedSocket; }							//
	void SetConnectedSocket(SOCKET newSocket)		{ connectedSocket = newSocket; }					//
	void SetSocketAddress(struct sockaddr_in newAddressStruct) { socketAddress = newAddressStruct; }	//
	int GetHeaderSize() const								   { return BASE_HEADER_SIZE + reliabilitySystem.GetHeaderSize(); }


#pragma endregion
};
