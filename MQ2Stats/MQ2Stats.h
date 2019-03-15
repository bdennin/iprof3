
#include "../MQ2Plugin.h"

#include "Chase.h"
#include "SpellQueue.h"

#include <map>
#include <string>
#include <vector>

class MQ2Stats : public MQ2Type
{
public:
	static size_t Split(std::string& message, std::vector<std::string>& tokens, const char delineator = ' ');
	static bool Contains(std::string& words, std::string& match);

	static const std::clock_t CLOCK_TICK = 100;

	MQ2Stats();
	~MQ2Stats();
	void Pulse();
	void SetClientIndex(PCHAR index);
	void Command(PCHAR p_line);
	bool GetMember(MQ2VARPTR p_var, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest);
	bool ToString(MQ2VARPTR p_var, PCHAR p_dest);
	bool FromData(MQ2VARPTR& p_var, MQ2TYPEVAR& src);
	bool FromString(MQ2VARPTR& p_var, PCHAR p_src);

	inline void ParseMessage(std::string name, std::string message)
	{
		message = message.substr(5); // Remove header [NB]|

		std::vector<std::string> tokens;

		Split(message, tokens);

		if(tokens.size() >= 11)
		{
			SpawnData& spawn = m_clients[name];

			spawn.name          = name;
			spawn.spawn_id      = std::stoi(tokens[0]);
			spawn.zone_id       = std::stoi(tokens[1]);
			spawn.zoning        = std::stoi(tokens[2]);
			spawn.level         = std::stoi(tokens[3]);
			spawn.heading       = std::stof(tokens[4]);
			spawn.angle         = std::stof(tokens[5]);
			spawn.x             = std::stof(tokens[6]);
			spawn.y             = std::stof(tokens[7]);
			spawn.z             = std::stof(tokens[8]);
			spawn.hp_percent    = std::stoi(tokens[9]);
			spawn.hp_missing    = std::stoi(tokens[10]);
			spawn.mana_percent  = std::stoi(tokens[11]);
			spawn.aggro_percent = std::stoi(tokens[12]);
		}
		else if(tokens.size() >= 3)
		{
			RequestType type = static_cast<RequestType>(std::stoi(tokens[0]));
			switch(type)
			{
			case RequestType::Push:

				m_p_spell_queue->Push(tokens[1], tokens[2]);

			case RequestType::Pop:

				m_p_spell_queue->Remove(tokens[1], tokens[2]);

			default:

				WriteChatf("Unknown spell command received: %s", message.c_str());
			}
		}
	}

	inline void ClearMovement()
	{
		m_p_target = nullptr;
		m_p_chase->ClearAll();
		m_p_chase->DoStop();
	}

private:
	struct SpawnData
	{
		std::string name;
		DWORD spawn_id;
		DWORD zone_id;
		bool zoning;
		DWORD level;
		FLOAT heading;
		FLOAT angle;
		FLOAT x;
		FLOAT y;
		FLOAT z;
		DWORD hp_percent;
		DWORD hp_missing;
		DWORD mana_percent;
		DWORD aggro_percent;
	};

	struct Position
	{
		FLOAT x;
		FLOAT y;
		FLOAT z;
		FLOAT heading;
		FLOAT look;
	};

	enum StatType
	{
		ID,
		ZoneID,
		InZone,
		Zoning,
		Level,
		Heading,
		Angle,
		X,
		Y,
		Z,
		Distance,
		Visible,
		HP,
		Mana,
		Aggro,

		DumpSpell,
		Names,
		Active,
		Combat,
		Following,
		EngageID,
		LowestID,
		FurthestID,
		GroupHP,
		AggroList,
		AggroCount,
		CorpseList,
		CorpseCount,
	};

	enum RequestType
	{
		Push,
		Pop
	};

	inline void Populate(SpawnData& client)
	{
		PSPAWNINFO p_spawn = GetCharInfo()->pSpawn; // Populate with local information

		client.spawn_id      = p_spawn->SpawnID;
		client.zone_id       = GetCharInfo()->zoneId;
		client.zoning        = !gbInZone;
		client.level         = p_spawn->Level;
		client.heading       = p_spawn->Heading;
		client.angle         = p_spawn->CameraAngle;
		client.x             = p_spawn->X;
		client.y             = p_spawn->Y;
		client.z             = p_spawn->Z;
		client.hp_percent    = static_cast<DWORD>(static_cast<FLOAT>(p_spawn->HPCurrent) / static_cast<FLOAT>(p_spawn->HPMax) * 100);
		client.hp_missing    = p_spawn->HPCurrent - p_spawn->HPMax;
		client.mana_percent  = static_cast<DWORD>(static_cast<FLOAT>(p_spawn->ManaCurrent) / static_cast<FLOAT>(p_spawn->ManaMax) * 100);
		client.aggro_percent = (nullptr != pAggroInfo) ? pAggroInfo->aggroData[AD_Player].AggroPct : 0;
	}

	inline void PublishMessage(SpawnData& client)
	{
		if(nullptr != m_p_send)
		{
			sprintf_s(m_send_buffer,
					  MAX_STRING,
					  "[NS]|%d %d %d %d %.2f %.2f %.2f %.2f %.2f %d %d %d %d",
					  client.spawn_id,
					  client.zone_id,
					  client.zoning ? 1 : 0,
					  client.level,
					  client.heading,
					  client.angle,
					  client.x,
					  client.y,
					  client.z,
					  client.hp_percent,
					  client.hp_missing,
					  client.mana_percent,
					  client.aggro_percent);

			m_p_send(m_send_buffer);
		}
	}

	inline int Compare(SpawnData& left, SpawnData& right)
	{
		if(left.spawn_id != right.spawn_id
		   || left.zone_id != right.zone_id
		   || left.zoning != right.zoning
		   || left.level != right.level
		   || fabs(left.heading - right.heading) > 0.5
		   || fabs(left.angle - right.angle) > 0.5
		   || fabs(left.x - right.x) > 0.5
		   || fabs(left.y - right.y) > 0.5
		   || fabs(left.z - right.z) > 0.5
		   || left.hp_percent != right.hp_percent
		   || left.mana_percent != right.mana_percent
		   || left.aggro_percent != right.aggro_percent)
		{
			return -1;
		}

		return 0;
	}

	inline void HandleEvents()
	{
		PSPAWNINFO p_char         = GetCharInfo()->pSpawn;
		PEQTRADEWINDOW p_trade    = (PEQTRADEWINDOW)pTradeWnd;
		CSidlScreenWnd* p_group   = (CSidlScreenWnd*)FindMQ2Window("GroupWindow");
		CSidlScreenWnd* p_confirm = (CSidlScreenWnd*)FindMQ2Window("ConfirmationDialogBox");

		// Accept group
		if(p_char->InvitedToGroup)
		{
			SendWndClick2(p_group->GetChildItem("GW_FollowButton"), "leftmouseup");
		}

		// Accept trade
		if(nullptr != p_trade && p_trade->HisTradeReady)
		{
			SendWndClick2(pTradeWnd->GetChildItem("TRDW_Trade_Button"), "leftmouseup");
		}

		// Click yes
		if(nullptr != p_confirm && p_confirm->dShow == 1) // Visible
		{
			SendWndClick2(p_confirm->GetChildItem("CD_Yes_Button"), "leftmouseup");
		}
	}

	inline void HandleCasting()
	{
		if(nullptr != m_p_cast)
		{
		}
	}

	inline void HandleMovement()
	{
		if(nullptr != m_p_chase && nullptr != m_p_target)
		{
			//if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
			//    return;
			//
			//         if(FollowState == FOLLOW_FOLLOWING && StatusState == STATUS_ON)
			//	FollowSpawn();
			//
			//MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
			//MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
			//MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
		}
	}

	inline bool IsClientInZone(SpawnData* p_client)
	{
		if(nullptr != p_client)
		{
			PSPAWNINFO p_spawn = (PSPAWNINFO)GetSpawnByID(p_client->spawn_id);

			if(nullptr != p_spawn && !p_spawn->Linkdead && p_client->name.compare(p_spawn->Name) == 0)
			{
				return true;
			}
		}

		return false;
	}

	inline FLOAT DistanceToClient(SpawnData* p_client)
	{
		if(IsClientInZone(p_client))
		{
			PSPAWNINFO p_self = GetCharInfo()->pSpawn;

			return GetDistance3D(p_self->X, p_self->Y, p_self->Z, p_client->x, p_client->y, p_client->z);
		}

		return 0.0;
	}

	inline bool IsClientVisible(SpawnData* p_client)
	{
		if(IsClientInZone(p_client))
		{
			SPAWNINFO client = *GetCharInfo()->pSpawn;
			client.Y         = p_client->y;
			client.X         = p_client->x;
			client.Z         = p_client->z;

			return pCharSpawn->CanSee((EQPlayer*)&client);
		}

		return false;
	}

	inline bool IsClient(std::string& name)
	{
		return m_clients.find(name) != m_clients.end();
	}

	inline DWORD GetAggroed(char* aggro_list, size_t aggro_len, DWORD range = 100)
	{
		DWORD count = 0;
		snprintf(aggro_list, aggro_len, "");

		for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < range; i++)
		{
			PSPAWNINFO p_spawn = (PSPAWNINFO)EQP_DistArray[i].VarPtr.Ptr;

			if(nullptr != p_spawn)
			{
				switch(p_spawn->Type)
				{
				case SPAWN_NPC:

					if(!p_spawn->MasterID && (p_spawn->PlayerState & 0xC) != 0) //Aggressive spawn flag
					{
						snprintf(aggro_list + strlen(aggro_list), aggro_len, "%d ", p_spawn->SpawnID);
						count++;
					}

					break;

				case SPAWN_PLAYER:
				case SPAWN_CORPSE:
				default:
					break;
				}
			}
		}

		return count;
	}

	inline DWORD GetCorpses(char* corpse_list, size_t corpse_len, DWORD range = 100)
	{
		DWORD count = 0;
		snprintf(corpse_list, corpse_len, "");

		PSPAWNINFO p_self = GetCharInfo()->pSpawn;

		for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < range; i++)
		{
			PSPAWNINFO p_spawn = (PSPAWNINFO)EQP_DistArray[i].VarPtr.Ptr;

			if(nullptr != p_spawn)
			{
				if(fabs(p_self->Z - p_spawn->Z) < 20.0) // Only count corpses on this plane
				{
					switch(p_spawn->Type)
					{
					case SPAWN_CORPSE:

						if(p_spawn->Deity == 0)
						{
							snprintf(corpse_list + strlen(corpse_list), corpse_len, "%d ", p_spawn->SpawnID);
						}

						break;

					case SPAWN_PLAYER:
					case SPAWN_NPC:
					default:
						break;
					}
				}
			}
		}

		return count;
	}

	typedef VOID(__cdecl* NetSendFunction)(PCHAR);
	typedef VOID(__cdecl* CastFunction)(PSPAWNINFO, PCHAR);

	char m_send_buffer[MAX_STRING];
	NetSendFunction m_p_send;
	CastFunction m_p_cast;
	SpawnData* m_p_data_index;
	SpellQueue* m_p_spell_queue;
	Chase* m_p_chase;
	SpawnData* m_p_target;
	std::map<std::string, SpawnData> m_clients;
	std::vector<std::clock_t> m_time_counts;
	std::clock_t m_last_pulse;
};
