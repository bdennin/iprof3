

#include "UDPSocket.cpp"

#include <atomic>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG 
#undef DEBUG

//template <typename T, U>
//class Distributable
//{
//	static Serialize(char* p_bytes, uint32_t max_bytes, T* p_type)
//	{
//		if(max_bytes >= struct_size)
//		{
//			// Copy directly from memory into byte array
//			memcpy(p_bytes, p_type, struct_size);
//		}
//	}
//
//	static Deserialize(T* p_type, const char* p_bytes, uint32_t size_bytes)
//	{
//		if(size_bytes >= struct_size)
//		{
//			// Copy directly from memory into object
//			memcpy(p_type, p_bytes, struct_size);
//		}
//	}
//
//	virtual U GetIdentifier() = 0;
//
//protected:
//	Distributable(uint32_t rate_ms);
//	~Distributable() = 0;
//
//private:
//	static constexpr struct_size = sizeof(T);
//
//	uint32_t m_rate_ms;
//}

static constexpr uint32_t max_buffs = 0x61;

struct SpawnData
{
	char name[16];
	uint16_t spawn_id;
	uint16_t zone_id;
	uint8_t level;
	int16_t heading;
	int16_t angle;
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t hp_percent;
	uint8_t mana_percent;
	uint8_t aggro_percent;
	uint8_t zoning;
	int32_t buff_ids[max_buffs];
	int32_t pet_buff_ids[max_buffs];
};

void PrintArg(SpawnData& data)
{
	std::cout << "<" << data.name << "> " <<
		"spawn_id: " << data.spawn_id << " "
		"zone_id: " << data.zone_id << " "
		"level: " << +data.level << " "
		"heading: " << data.heading << " "
		"angle: " << data.angle << " "
		"x: " << data.x << " "
		"y: " << data.y << " "
		"z: " << data.z << " "
		"hp_percent: " << +data.hp_percent << " "
		"mana_percent: " << +data.mana_percent << " "
		"aggro_percent: " << +data.aggro_percent << " "
		"zoning: " << +data.zoning << " ";

	std::cout << "buff_ids: ";

	for(uint32_t i = 0; i < max_buffs; i++)
	{
		std::cout << data.buff_ids[i] << " ";
	}

	std::cout << std::endl;
}

class Server
{
public:

	Server(uint16_t port)
		: m_p_socket(nullptr)
		, m_is_running(false)
		, m_p_thread(nullptr)
	{
		m_p_socket = new UDPSocket(port);
		m_is_running = true;

		m_p_thread = new std::thread(&Server::TransThread, this);
	}

	~Server()
	{
		m_is_running = false; // Signal thread to stop iterating

		m_p_thread->join(); // Wait for join

		delete m_p_thread;
		delete m_p_socket;
	}

	bool IsRunning()
	{
		return m_is_running;
	}

	void TransThread()
	{
		char bytes[buffer_size];
		int32_t num_bytes = 0;
		SpawnData arg;
		sockaddr_in reply_addr;

		while(m_is_running)
		{
			// Block until data is read
			num_bytes = m_p_socket->Receive(bytes, buffer_size, &reply_addr);

			if(num_bytes >= num_bytes)
			{
				// Cache
				for(int32_t i = 0; i < num_bytes; i += struct_size)
				{
					// Copy directly from memory into object
					memcpy(&arg, bytes + i, struct_size);

					// Get the name and copy to dictionary
					m_clients[arg.name] = arg;
#ifdef DEBUG
					PrintArg(arg);
#endif
				}

				// Accumulate
				int32_t i = 0;
				for(auto it = m_clients.begin(); it != m_clients.end(); it++, i++)
				{
					memcpy(bytes + i * struct_size, &it->second, struct_size);
				}

#ifdef DEBUG
				std::cout << "sending client data for " << m_clients.size() << " clients.\n";
#endif 

				// Reply
				m_p_socket->Send(bytes, m_clients.size() * struct_size, reply_addr);
			}
#ifdef DEBUG
			else
			{

				std::cout << "  invalid packet received.\n";
			}
#endif
		}
	}

private:

	static constexpr uint8_t max_clients = 64;
	static constexpr uint32_t struct_size = sizeof(SpawnData);
	static constexpr uint32_t buffer_size = struct_size * max_clients;

	UDPSocket* m_p_socket;
	std::thread* m_p_thread;
	std::atomic<bool> m_is_running;
	std::map<std::string, SpawnData> m_clients;
};

int main()
{
	Server serv(44444);

	std::cout << "Server started." << std::endl;

	while(serv.IsRunning())
	{
		Sleep(500);
	}
}
