/*
*  FILE          : DEBUG
*  PROJECT       : DEBUG
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  DESCRIPTION   : DEBUG
*/


#pragma once
#include "shared.h"
#include "server.h"


// packet queue to store information about sent and received packets sorted in sequence order
//  + we define ordering using the "sequence_more_recent" function, this works provided there is a large gap when sequence wrap occurs
inline bool sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence)
{
	auto half_max = max_sequence / 2;
	return (
		((s1 > s2) && (s1 - s2 <= half_max))
		||
		((s2 > s1) && (s2 - s1 > half_max))
		);
}


#pragma region PacketQueueClass 


	/*
	 * CLASS		: PacketQueue
	 * DESCRIPTION	: DEBUG
	 */
	class PacketQueue : public list<PacketData>
	{
	public:


		bool exists(unsigned int sequence)
		{
			for (iterator itor = begin(); itor != end(); ++itor)
			{
				if (itor->sequence == sequence)
				{
					return true;
				}
			}

			return false;
		}

		void insert_sorted(const PacketData & p, unsigned int max_sequence)
		{
			if (empty())
			{
				push_back(p);
			}
			else
			{
				if (!sequence_more_recent(p.sequence, front().sequence, max_sequence))
				{
					push_front(p);
				}
				else if (sequence_more_recent(p.sequence, back().sequence, max_sequence))
				{
					push_back(p);
				}
				else
				{
					for (PacketQueue::iterator itor = begin(); itor != end(); itor++)
					{
						assert(itor->sequence != p.sequence);
						if (sequence_more_recent(itor->sequence, p.sequence, max_sequence))
						{
							insert(itor, p);
							break;
						}
					}
				}
			}
		}

		void verify_sorted(unsigned int max_sequence)
		{
			PacketQueue::iterator prev = end();
			for (PacketQueue::iterator itor = begin(); itor != end(); itor++)
			{
				assert(itor->sequence <= max_sequence);
				if (prev != end())
				{
					assert(sequence_more_recent(itor->sequence, prev->sequence, max_sequence));
					prev = itor;
				}
			}
		}
	};

#pragma endregion
#pragma region ReliabilitySystem


		/*
		*	CLASS		: ReliabilitySystem
		*	DESCRIPTION : The reliability system is used to support the reliable connection. It manages sent, received,
		*	pending acks (acknowledgments) and ack (acknowledged) packet queues.
		*/
		class ReliabilitySystem
		{

		public:

			//Constructor
			ReliabilitySystem(unsigned int max_sequence = 0xFFFFFFFF)
			{
				this->rtt_maximum = rtt_maximum;
				this->max_sequence = max_sequence;
				Reset();
			}


			/*
			*  METHOD        : Reset
			*  DESCRIPTION   : Resets the objects properties/counters to zero
			*  PARAMETERS    : void : Takes no arguments
			*  RETURNS       : void : Returns no values
			*/
			void Reset()
			{
				local_sequence = 0;
				remote_sequence = 0;
				sentQueue.clear();
				receivedQueue.clear();
				pendingAckQueue.clear();
				ackedQueue.clear();
				sent_packets = 0;
				recv_packets = 0;
				lost_packets = 0;
				acked_packets = 0;
				sent_bandwidth = 0.0f;
				acked_bandwidth = 0.0f;
				rtt = 0.0f;
				rtt_maximum = 1.0f;
			}


			/*
			*  METHOD        : PacketSent
			*  DESCRIPTION   : Checks if theres a record of the "sequence" value, from the previous SendPacket() operation;
			*	if the sequence exists, then it iterates through the queue and prints the sequence values of all sent packets.
			*	The method also adds the sequence, time, and size of the packet to the sentQueue & pendingAckQueue.
			*  PARAMETERS    : int size : Size in bytes of the most recent packet that was sent through the socket
			*  RETURNS       : void : Returns no values
			*/
			void PacketSent(int size)
			{

				//Check if there's a record of the previously sent packets
				if (sentQueue.exists(local_sequence))
				{
					printf("local sequence %d exists \n", local_sequence);


					//Iterate through the whole sentQueue, and print all the sequence values for the sent packets		
					for (PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); ++itor)
					{
						printf(" + %d \n", itor->sequence);
					}
				}


				//Create a temp container defined as a packet, and store the message properties from 
				//  the send command that was just executed
				PacketData data;
				data.sequence = local_sequence;
				data.time = 0.0f;
				data.size = size;


				//Add the most recent packet data to the sentQueue, and increment the counters, thus recording
				//	the send operation as successful
				sentQueue.push_back(data);
				pendingAckQueue.push_back(data);
				sent_packets++;
				local_sequence++;


				//max_sequence is defined in the reliabilitySystems constructor as 0xFFFFFFFF
				//Loop back to zero, once our local sequence value has passed the max
				if (local_sequence > max_sequence) local_sequence = 0;
			}

			/*
			*  METHOD        : PacketReceived
			*  DESCRIPTION   : DEBUG
			*  PARAMETERS    : Defined below,
			* 	unsigned int sequence : DEBUG
			*	int size : DEBUG
			*  RETURNS       : void : Returns no values
			*/
			void PacketReceived(unsigned int sequence, int size)
			{
				recv_packets++;
				if (receivedQueue.exists(sequence)) return;
				PacketData data;
				data.sequence = sequence;
				data.time = 0.0f;
				data.size = size;
				receivedQueue.push_back(data);
				if (sequence_more_recent(sequence, remote_sequence, max_sequence))
					remote_sequence = sequence;
			}

			unsigned int GenerateAckBits()
			{
				return generate_ack_bits(GetRemoteSequence(), receivedQueue, max_sequence);
			}

			void ProcessAck(unsigned int ack, unsigned int ack_bits)
			{
				process_ack(ack, ack_bits, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, max_sequence);
			}

			void Update(float deltaTime)
			{
				acks.clear();
				AdvanceQueueTime(deltaTime);
				UpdateQueues();
				UpdateStats();
			}

			void Validate()
			{
				sentQueue.verify_sorted(max_sequence);
				receivedQueue.verify_sorted(max_sequence);
				pendingAckQueue.verify_sorted(max_sequence);
				ackedQueue.verify_sorted(max_sequence);
			}


			// utility functions
			static bool sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence)
			{
				return (s1 > s2) && (s1 - s2 <= max_sequence / 2) || (s2 > s1) && (s2 - s1 > max_sequence / 2);
			}

			static int bit_index_for_sequence(unsigned int sequence, unsigned int ack, unsigned int max_sequence)
			{
				if (sequence > ack) { return ack + (max_sequence - sequence); }
				else { return ack - 1 - sequence; }
			}

			static unsigned int generate_ack_bits(unsigned int ack, const PacketQueue & received_queue, unsigned int max_sequence)
			{
				unsigned int ack_bits = 0;
				for (PacketQueue::const_iterator itor = received_queue.begin(); itor != received_queue.end(); itor++)
				{
					if (itor->sequence == ack || sequence_more_recent(itor->sequence, ack, max_sequence))
						break;
					int bit_index = bit_index_for_sequence(itor->sequence, ack, max_sequence);
					if (bit_index <= 31)
						ack_bits |= 1 << bit_index;
				}
				return ack_bits;
			}

			static void process_ack(unsigned int ack, unsigned int ack_bits,
				PacketQueue & pending_ack_queue, PacketQueue & acked_queue,
				std::vector<unsigned int> & acks, unsigned int & acked_packets,
				float & rtt, unsigned int max_sequence)
			{
				if (pending_ack_queue.empty())return;

				PacketQueue::iterator itor = pending_ack_queue.begin();
				while (itor != pending_ack_queue.end())
				{
					bool acked = false;

					if (itor->sequence == ack)
					{
						acked = true;
					}
					else if (!sequence_more_recent(itor->sequence, ack, max_sequence))
					{
						int bit_index = bit_index_for_sequence(itor->sequence, ack, max_sequence);
						if (bit_index <= 31)acked = (ack_bits >> bit_index) & 1;
					}

					if (acked)
					{
						rtt += (itor->time - rtt) * 0.1f;
						acked_queue.insert_sorted(*itor, max_sequence);
						acks.push_back(itor->sequence);
						acked_packets++;
						itor = pending_ack_queue.erase(itor);
					}
					else
						++itor;
				}
			}

			// data accessors		
			unsigned int GetLocalSequence() const { return local_sequence; }
			unsigned int GetRemoteSequence() const { return remote_sequence; }
			unsigned int GetMaxSequence() const { return max_sequence; }
			unsigned int GetSentPackets() const { return sent_packets; }
			unsigned int GetReceivedPackets() const { return recv_packets; }
			unsigned int GetLostPackets() const { return lost_packets; }
			unsigned int GetAckedPackets() const { return acked_packets; }
			float GetSentBandwidth() const { return sent_bandwidth; }
			float GetAckedBandwidth() const { return acked_bandwidth; }
			float GetRoundTripTime() const { return rtt; }
			int GetHeaderSize() const { return 12; }
			void GetAcks(unsigned int ** acks, int & count) { *acks = &this->acks[0]; count = (int)this->acks.size(); }

		protected:

			void AdvanceQueueTime(float deltaTime)
			{
				for (PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); itor++)
					itor->time += deltaTime;

				for (PacketQueue::iterator itor = receivedQueue.begin(); itor != receivedQueue.end(); itor++)
					itor->time += deltaTime;

				for (PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); itor++)
					itor->time += deltaTime;

				for (PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); itor++)
					itor->time += deltaTime;
			}

			void UpdateQueues()
			{
				const float epsilon = 0.001f;

				while (sentQueue.size() && sentQueue.front().time > rtt_maximum + epsilon)
					sentQueue.pop_front();

				if (receivedQueue.size())
				{
					const unsigned int latest_sequence = receivedQueue.back().sequence;
					const unsigned int minimum_sequence = latest_sequence >= 34 ? (latest_sequence - 34) : max_sequence - (34 - latest_sequence);
					while (receivedQueue.size() && !sequence_more_recent(receivedQueue.front().sequence, minimum_sequence, max_sequence))
						receivedQueue.pop_front();
				}

				while (ackedQueue.size() && ackedQueue.front().time > rtt_maximum * 2 - epsilon)
					ackedQueue.pop_front();

				while (pendingAckQueue.size() && pendingAckQueue.front().time > rtt_maximum + epsilon)
				{
					pendingAckQueue.pop_front();
					lost_packets++;
				}
			}

			void UpdateStats()
			{
				int sent_bytes_per_second = 0;
				for (PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); ++itor)
					sent_bytes_per_second += itor->size;
				int acked_packets_per_second = 0;
				int acked_bytes_per_second = 0;
				for (PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor)
				{
					if (itor->time >= rtt_maximum)
					{
						acked_packets_per_second++;
						acked_bytes_per_second += itor->size;
					}
				}
				sent_bytes_per_second /= (int)rtt_maximum;
				acked_bytes_per_second /= (int)rtt_maximum;
				sent_bandwidth = sent_bytes_per_second * (8 / 1000.0f);
				acked_bandwidth = (float)acked_bytes_per_second * (8 / 1000.0f);
			}

		private:

			unsigned int max_sequence;			// maximum sequence value before wrap around (used to test sequence wrap at low # values)
			unsigned int local_sequence;		// local sequence number for most recently sent packet
			unsigned int remote_sequence;		// remote sequence number for most recently received packet

			unsigned int sent_packets;			// total number of packets sent
			unsigned int recv_packets;			// total number of packets received
			unsigned int lost_packets;			// total number of packets lost
			unsigned int acked_packets;			// total number of packets acked

			float sent_bandwidth;				// approximate sent bandwidth over the last second
			float acked_bandwidth;				// approximate acked bandwidth over the last second
			float rtt;							// estimated round trip time
			float rtt_maximum;					// maximum expected round trip time (hard coded to one second for the moment)

			std::vector<unsigned int> acks;		// acked packets from last set of packet receives. cleared each update!

			PacketQueue sentQueue;				// sent packets used to calculate sent bandwidth (kept until rtt_maximum)
			PacketQueue pendingAckQueue;		// sent packets which have not been acked yet (kept until rtt_maximum * 2 )
			PacketQueue receivedQueue;			// received packets for determining acks to send (kept up to most recent recv sequence - 32)
			PacketQueue ackedQueue;				// acked packets (kept until rtt_maximum * 2)
		};
#pragma endregion