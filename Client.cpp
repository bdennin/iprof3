

#include "UDPSocket.cpp"
#include "../MQ2Plugin.h"

#undef UNICODE

#include <map>
#include <vector>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1

static constexpr uint32_t max_buffs = NUM_BUFF_SLOTS;

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

class Client
{
public:
	std::map<std::string, SpawnData> m_clients;

	Client(uint32_t ip_addr, uint16_t port)
		: m_p_socket(nullptr)
	{
		m_p_socket = new UDPSocket(ip_addr, port);
	}

	~Client()
	{
		delete m_p_socket;
		m_p_socket = nullptr;
	}

	void Receive()
	{
		char bytes[buffer_size];
		int32_t num_bytes = 0;
		SpawnData arg;

		num_bytes = m_p_socket->Receive(bytes, buffer_size);

		WriteChatf("Received %d bytes for %.2f clients.", num_bytes, static_cast<FLOAT>(num_bytes / struct_size));

		for(int32_t i = 0; i < num_bytes; i += struct_size)
		{
			// Copy directly from memory into object
			memcpy(&arg, bytes + i, struct_size);

			// Get the name and copy to dictionary
			m_clients[arg.name] = arg;
		}
	}

	void Send()
	{
		char bytes[buffer_size];
		int32_t num_bytes  = 0;
		int32_t buff_count = 0;
		SpawnData* p_arg;

		PCHARINFO p_char   = GetCharInfo();
		PCHARINFO2 p_char2 = GetCharInfo2();
		if(nullptr != p_char && nullptr != p_char2)
		{
			PSPAWNINFO p_spawn = p_char->pSpawn;
			if(nullptr != p_spawn)
			{
				p_arg = &m_clients[p_spawn->DisplayedName];

				strcpy_s(p_arg->name, p_spawn->DisplayedName);

				p_arg->spawn_id = static_cast<uint16_t>(p_spawn->SpawnID);
				p_arg->level    = static_cast<uint8_t>(p_spawn->Level);
				p_arg->zone_id  = static_cast<uint16_t>(p_char->zoneId);
				p_arg->zoning   = gbInZone ? 0 : 1;

				p_arg->heading = static_cast<int16_t>(p_spawn->Heading * 0.7 + 0.5);
				p_arg->angle   = static_cast<int16_t>(p_spawn->CameraAngle + 0.5);
				p_arg->x       = static_cast<int16_t>(p_spawn->X + 0.5);
				p_arg->y       = static_cast<int16_t>(p_spawn->Y + 0.5);
				p_arg->z       = static_cast<int16_t>(p_spawn->Z + 0.5);

				p_arg->hp_percent    = static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->HPCurrent) / static_cast<FLOAT>(p_spawn->HPMax) * 100);
				p_arg->mana_percent  = static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->ManaCurrent) / static_cast<FLOAT>(p_spawn->ManaMax) * 100);
				p_arg->aggro_percent = static_cast<uint8_t>(pAggroInfo->aggroData[AD_Player].AggroPct + 0.5);

				for(uint32_t i = 0; i < max_buffs; i++)
				{
					p_arg->buff_ids[i] = static_cast<int32_t>(p_char2->Buff[i].SpellID);
				}

				auto p_pet = ((PEQPETINFOWINDOW)pPetInfoWnd);
				for(uint32_t i = 0; i < max_buffs; i++)
				{
					p_arg->pet_buff_ids[i] = static_cast<int32_t>(p_pet->Buff[i]); // Stored as spell ID's
				}

				// Copy structure directly to memory
				memcpy(bytes, p_arg, struct_size);

				m_p_socket->Send(bytes, struct_size);
			}
		}
	}

private:
	static constexpr uint8_t max_clients  = 64;
	static constexpr uint32_t struct_size = sizeof(SpawnData);
	static constexpr uint32_t buffer_size = struct_size * max_clients;

	UDPSocket* m_p_socket;
};