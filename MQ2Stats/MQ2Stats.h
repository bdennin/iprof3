
#include "../MQ2Plugin.h"

#include "Chase.h"
#include "SpellQueue.h"

#include <map>
#include <string>
#include <vector>

class FunctionTimer
{
public:
	FunctionTimer(std::clock_t& clock_count)
		: m_clock_count(clock_count)
	{
		m_clock_start = std::clock();
	}

	~FunctionTimer()
	{
		m_clock_count += std::clock() - m_clock_start;
	}

private:
	std::clock_t m_clock_start;
	std::clock_t& m_clock_count;
};

class MQ2Stats : public MQ2Type
{
public:
	static size_t Split(std::string& message, std::vector<std::string>& tokens, const char delineator = ' ');
	static bool Contains(std::string& words, std::string& match);

	static const std::clock_t CLOCK_TICK = 100;

	MQ2Stats();
	~MQ2Stats();
	void Pulse();
	void CommandDrag(PCHAR p_line);
	void CommandDumpSpawn(PCHAR p_line);
	void CommandMissing();
	void CommandPush(PCHAR p_line);
	void CommandRunTimes();
	void SetClientIndex(PCHAR index);
	bool GetMember(MQ2VARPTR p_var, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest);
	bool ToString(MQ2VARPTR p_var, PCHAR p_dest);
	bool FromData(MQ2VARPTR& p_var, MQ2TYPEVAR& src);
	bool FromString(MQ2VARPTR& p_var, PCHAR p_src);

	inline void ParseMessage(std::string name, std::string message)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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
			spawn.hp_deficit    = std::stoi(tokens[10]);
			spawn.mana_percent  = std::stoi(tokens[11]);
			spawn.aggro_percent = std::stoi(tokens[12]);
		}
		else if(tokens.size() >= 3)
		{
			RequestType type = static_cast<RequestType>(std::stoi(tokens[0]));

			switch(type)
			{
			case RequestType::PopSpell:

				m_p_spell_queue->Remove(tokens[1], tokens[2]);
				break;

			case RequestType::PushSpell:

				m_p_spell_queue->Push(tokens[1], tokens[2]);
				break;

			case RequestType::Refresh:

				m_force_refresh = true;
				break;

			default:

				WriteChatf("Unknown command received: %s", message.c_str());
				break;
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
		DWORD hp_deficit;
		DWORD mana_percent;
		DWORD aggro_percent;
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

		Names,
		Active,
		Combat,
		Following,
		EngageID,
		LowestID,
		FurthestID,
		GroupHP,
		AggroList,
		CorpseList,
		RunTimes,
		DumpData,
	};

	enum RequestType
	{
		PopSpell,
		PushSpell,
		Refresh,
	};

	inline void Populate(SpawnData& client)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		PCHARINFO p_char   = GetCharInfo();
		PSPAWNINFO p_spawn = (nullptr != p_char) ? p_char->pSpawn : nullptr;

		client.spawn_id      = (nullptr != p_spawn) ? p_spawn->SpawnID : 0;
		client.zone_id       = (nullptr != p_char) ? p_char->zoneId : 0;
		client.zoning        = !gbInZone;
		client.level         = (nullptr != p_spawn) ? p_spawn->Level : m_p_self->level;
		client.heading       = (nullptr != p_spawn) ? p_spawn->Heading : m_p_self->heading;
		client.angle         = (nullptr != p_spawn) ? p_spawn->CameraAngle : m_p_self->angle;
		client.x             = (nullptr != p_spawn) ? p_spawn->X : m_p_self->x;
		client.y             = (nullptr != p_spawn) ? p_spawn->Y : m_p_self->y;
		client.z             = (nullptr != p_spawn) ? p_spawn->Z : m_p_self->z;
		client.hp_percent    = (nullptr != p_spawn) ? static_cast<DWORD>(static_cast<FLOAT>(p_spawn->HPCurrent) / static_cast<FLOAT>(p_spawn->HPMax) * 100) : m_p_self->hp_percent;
		client.hp_deficit    = (nullptr != p_spawn) ? p_spawn->HPCurrent - p_spawn->HPMax : m_p_self->hp_deficit;
		client.mana_percent  = (nullptr != p_spawn) ? static_cast<DWORD>(static_cast<FLOAT>(p_spawn->ManaCurrent) / static_cast<FLOAT>(p_spawn->ManaMax) * 100) : m_p_self->mana_percent;
		client.aggro_percent = (nullptr != pAggroInfo) ? pAggroInfo->aggroData[AD_Player].AggroPct : 0;
	}

	inline void PublishMessage(SpawnData& client)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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
					  client.hp_deficit,
					  client.mana_percent,
					  client.aggro_percent);

			m_p_send(m_send_buffer);
		}
	}

	inline int Compare(SpawnData& left, SpawnData& right)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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

	inline void HandleCasting()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != m_p_cast)
		{
		}
	}

	inline void HandleCorpse()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		return;
	}

	inline void HandleDrag()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(m_drag_enable)
		{
			char exec[MAX_STRING];

			for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < 100; i++)
			{
				PSPAWNINFO p_spawn = (PSPAWNINFO)EQP_DistArray[i].VarPtr.Ptr;

				if(EQP_DistArray[i].Value.Float > 10 && nullptr != p_spawn)
				{
					switch(p_spawn->Type)
					{
					case SPAWN_CORPSE:

						if(p_spawn->Deity > 0)
						{
							snprintf(exec, MAX_STRING, "/squelch /target id %d", p_spawn->SpawnID);
							DoCommand(p_spawn, exec);
							DoCommand(p_spawn, "/squelch /corpse");
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
	}

	inline void HandleEvents()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		PCHARINFO p_char          = GetCharInfo();
		PSPAWNINFO p_spawn        = p_char->pSpawn;
		PEQTRADEWINDOW p_trade    = (PEQTRADEWINDOW)pTradeWnd;
		CSidlScreenWnd* p_group   = (CSidlScreenWnd*)FindMQ2Window("GroupWindow");
		CSidlScreenWnd* p_confirm = (CSidlScreenWnd*)FindMQ2Window("ConfirmationDialogBox");
		DOUBLE xp_percent         = (static_cast<DOUBLE>(p_char->Exp) / 3.3) + (static_cast<DOUBLE>(p_spawn->Level) * 100);
		DOUBLE delta_xp           = xp_percent - m_xp_percent;
		m_xp_percent              = xp_percent;
		DOUBLE xp_aa_percent      = (static_cast<DOUBLE>(p_char->AAExp) / 3.3) + (static_cast<DOUBLE>(GetCharInfo2()->AAPoints + GetCharInfo2()->AAPointsSpent) * 100);
		DOUBLE delta_xp_aa        = xp_aa_percent - m_xp_aa_percent;
		m_xp_aa_percent           = xp_aa_percent;

		// Accept group
		if(nullptr != p_spawn && p_spawn->InvitedToGroup)
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

		// Print experience changes
		if(delta_xp != 0)
		{
			WriteChatf("You gained %.2f%% experience.", delta_xp);
		}

		if(delta_xp_aa != 0)
		{
			WriteChatf("You gained %.2f%% AA experience.", delta_xp_aa);
		}
	}

	inline void HandleMovement()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != m_p_chase && nullptr != m_p_target)
		{
			if(IsClientInZone(m_p_target))
			{
				m_p_chase->AddWaypoint(m_p_target->x, m_p_target->y, m_p_target->z, m_p_target->heading, m_p_target->angle);
			}

			m_p_chase->Iterate();
		}
	}

	inline bool IsClientInZone(SpawnData* p_client)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != p_client)
		{
			PSPAWNINFO p_spawn = (PSPAWNINFO)GetSpawnByID(p_client->spawn_id);

			if(nullptr != p_spawn
			   && !p_spawn->Linkdead
			   && Contains(std::string(p_spawn->Name), p_client->name))
			{
				return true;
			}
		}

		return false;
	}

	inline FLOAT DistanceToClient(SpawnData* p_client)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(IsClientInZone(p_client))
		{
			PSPAWNINFO p_self = GetCharInfo()->pSpawn;

			return GetDistance3D(p_self->X, p_self->Y, p_self->Z, p_client->x, p_client->y, p_client->z);
		}

		return 0.0;
	}

	inline bool IsClientVisible(SpawnData* p_client)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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

	inline SpawnData* GetClient(std::string& name)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		auto it = m_clients.find(name);

		if(it != m_clients.end())
		{
			return &it->second;
		}
		else
		{
			return nullptr;
		}
	}

	inline bool IsClient(std::string& name)
	{
		return nullptr != GetClient(name);
	}

	inline DWORD GetAggroed(char* aggro_list, size_t aggro_len, DWORD range = 100)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

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
	char m_command_buffer[MAX_STRING];
	NetSendFunction m_p_send;
	CastFunction m_p_cast;
	SpawnData* m_p_data_index;
	SpawnData* m_p_self;
	SpawnData* m_p_target;
	SpellQueue* m_p_spell_queue;
	Chase* m_p_chase;
	std::map<std::string, SpawnData> m_clients;
	std::map<std::string, std::clock_t> m_time_counts; // Function run time average
	std::clock_t m_time_start;
	std::clock_t m_last_tenth;
	std::clock_t m_last_second;
	DOUBLE m_xp_percent;
	DOUBLE m_xp_aa_percent;
	bool m_force_refresh;
	bool m_drag_enable;
};
