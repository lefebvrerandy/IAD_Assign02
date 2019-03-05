


//Global struct for all server/client connection info supplied from the CLA
typedef struct
{
	string ipAddress;
	int port = 3000;
	string filepath;
	string fileExtension;
	FileReadMode readMode;
	const int ProtocolId = PROTOCOL_ID;
	const float DeltaTime = DELTA_TIME;
	const float SendRate = SEND_RATE;
	const float TimeOut = TIME_OUT;
	const int PacketSize = PACKET_SIZE;
}Properties;
__declspec(selectany) Properties programParameters;