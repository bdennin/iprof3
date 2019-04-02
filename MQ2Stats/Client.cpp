
#include "UDPSocket.cpp"
#include "../MQ2Plugin.h"

#undef UNICODE

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1

class Client
{
public:
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
	};

	std::map<std::string, SpawnData> m_clients;

	Client(uint32_t ip_addr, uint16_t port)
		: m_p_socket(nullptr)
		, m_is_running(false)
		, m_p_send_thread(nullptr)
		, m_p_recv_thread(nullptr)
	{
		m_p_socket   = new UDPSocket(ip_addr, port);
		m_is_running = true;

		m_p_recv_thread = new std::thread(&Client::recv_thread, this);
		m_p_send_thread = new std::thread(&Client::send_thread, this);
	}

	~Client()
	{
		m_is_running = false;

		m_p_send_thread->join();
		m_p_recv_thread->join();

		delete m_p_send_thread;
		delete m_p_recv_thread;
		delete m_p_socket;
	}

	void recv_thread()
	{
		char bytes[max_size];
		int32_t num_bytes = 0;
		SpawnData arg;

		while(m_is_running)
		{
			WriteChatf("Receiving");

			num_bytes = m_p_socket->Receive(bytes, max_size);

            WriteChatf("Done receiving");

			for(int32_t i = 0; i < num_bytes; i += struct_size)
			{
				// Copy directly from memory into object
				memcpy(&arg, bytes + i, struct_size);

				// Get the name and copy to dictionary
				m_clients[arg.name] = arg;
			}
		}
	}

	void send_thread()
	{
		const auto send_delay_ms = std::chrono::milliseconds(100);

		char bytes[max_size];
		int32_t num_bytes = 0;
		SpawnData* p_arg;

		while(m_is_running)
		{
			// Sleep briefly
			std::this_thread::sleep_for(send_delay_ms);

            WriteChatf("attemping send");

            // Collect local data
			PCHARINFO p_char   = GetCharInfo();
			PSPAWNINFO p_spawn = p_char->pSpawn;

			if(nullptr != p_spawn)
			{
				p_arg = &m_clients[p_spawn->Name];

                strcpy_s(p_arg->name, p_spawn->Name);

				p_arg->spawn_id      = static_cast<uint16_t>(p_spawn->SpawnID);
				p_arg->zone_id       = static_cast<uint16_t>(p_char->zoneId);
				p_arg->level         = static_cast<uint8_t>(p_spawn->Level);
				p_arg->heading       = static_cast<int16_t>(p_spawn->Heading * 0.7 + 0.5);
				p_arg->angle         = static_cast<int16_t>(p_spawn->CameraAngle + 0.5);
				p_arg->x             = static_cast<int16_t>(p_spawn->X + 0.5);
				p_arg->y             = static_cast<int16_t>(p_spawn->Y + 0.5);
				p_arg->z             = static_cast<int16_t>(p_spawn->Z + 0.5);
				p_arg->hp_percent    = static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->HPCurrent) / static_cast<FLOAT>(p_spawn->HPMax) * 100);
				p_arg->mana_percent  = static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->ManaCurrent) / static_cast<FLOAT>(p_spawn->ManaMax) * 100);
				p_arg->aggro_percent = static_cast<uint8_t>(pAggroInfo->aggroData[AD_Player].AggroPct + 0.5);
				p_arg->zoning        = gbInZone ? 0 : 1;

                // Copy directly to memory
				memcpy(bytes, p_arg, struct_size);

                WriteChatf("Sending");

				m_p_socket->Send(bytes, struct_size);
			}
		}
	}

private:
	static constexpr uint32_t max_size    = 4096;
	static constexpr uint32_t struct_size = sizeof(SpawnData);

	std::atomic<bool> m_is_running;
	std::thread* m_p_send_thread;
	std::thread* m_p_recv_thread;
	UDPSocket* m_p_socket;
};
