
#include "Client.cpp"

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
	void CommandChase(PCHAR p_line);
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

	inline void ClearMovement()
	{
		m_p_target = nullptr;
		m_p_chase->ClearAll();
		m_p_chase->DoStop();
	}

	inline void ParseMessage(std::string name, std::string message)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		std::vector<std::string> tokens;

		Split(message, tokens);

		SpawnData& spawn = m_clients[name];
		spawn.name       = name;

		switch(tokens.size())
		{
		case 0:
			PublishLocation(true);
			PublishStats(true);
			PublishZone(true);

			WriteChatf("Publishing all.");
			break;

		case 3:
			spawn.spawn_id = std::stoi(tokens[0]);
			spawn.zone_id  = std::stoi(tokens[1]);
			spawn.level    = std::stoi(tokens[2]);
			break;

		case 4:
			spawn.hp_percent    = std::stoi(tokens[0]);
			spawn.mana_percent  = std::stoi(tokens[1]);
			spawn.aggro_percent = std::stoi(tokens[2]);
			spawn.zoning        = std::stoi(tokens[3]);
			break;

		case 5:
			spawn.heading = std::stoi(tokens[0]);
			spawn.angle   = std::stoi(tokens[1]);
			spawn.x       = std::stoi(tokens[2]);
			spawn.y       = std::stoi(tokens[3]);
			spawn.z       = std::stoi(tokens[4]);
			break;

		default:

			WriteChatf("Unexpected message type received.");
			break;
		}
	}

	inline void PublishRequest()
	{
		if(nullptr != m_p_send)
		{
			sprintf_s(m_send_buffer,
					  MAX_STRING,
					  "[NS]|");

			m_p_send(m_send_buffer);
		}
	}

private:
	struct SpawnData
	{
		std::string name;
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
	};

	enum CorpseType
	{
		NPC,
		PC,
	};

	inline void PublishLocation(bool force)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != m_p_send)
		{
			PCHARINFO p_char   = GetCharInfo();
			PSPAWNINFO p_spawn = p_char->pSpawn;

			int16_t heading = (nullptr != p_spawn) ? static_cast<int16_t>(p_spawn->Heading * 0.7 + 0.5) : m_p_self->heading;
			int16_t angle   = (nullptr != p_spawn) ? static_cast<int16_t>(p_spawn->CameraAngle + 0.5) : m_p_self->angle;
			int16_t x       = (nullptr != p_spawn) ? static_cast<int16_t>(p_spawn->X + 0.5) : m_p_self->x;
			int16_t y       = (nullptr != p_spawn) ? static_cast<int16_t>(p_spawn->Y + 0.5) : m_p_self->y;
			int16_t z       = (nullptr != p_spawn) ? static_cast<int16_t>(p_spawn->Z + 0.5) : m_p_self->z;

			if(force
			   || abs(heading - m_p_self->heading) >= 1.0
			   || abs(angle - m_p_self->angle) >= 1.0
			   || abs(x - m_p_self->x) >= 1.0
			   || abs(y - m_p_self->y) >= 1.0
			   || abs(z - m_p_self->z) >= 1.0)
			{
				sprintf_s(m_send_buffer,
						  MAX_STRING,
						  "[NS]|%hd %hd %hd %hd %hd",
						  heading,
						  angle,
						  x,
						  y,
						  z);

				m_p_send(m_send_buffer);
			}
		}
	}

	inline void PublishStats(bool force)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != m_p_send)
		{
			PCHARINFO p_char   = GetCharInfo();
			PSPAWNINFO p_spawn = p_char->pSpawn;

			uint8_t hp_percent    = (nullptr != p_spawn) ? static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->HPCurrent) / static_cast<FLOAT>(p_spawn->HPMax) * 100) : m_p_self->hp_percent;
			uint8_t mana_percent  = (nullptr != p_spawn) ? static_cast<uint8_t>(static_cast<FLOAT>(p_spawn->ManaCurrent) / static_cast<FLOAT>(p_spawn->ManaMax) * 100) : m_p_self->mana_percent;
			uint8_t aggro_percent = (nullptr != pAggroInfo) ? static_cast<uint8_t>(pAggroInfo->aggroData[AD_Player].AggroPct + 0.5) : m_p_self->aggro_percent;
			uint8_t zoning        = gbInZone ? 0 : 1;

			if(force
			   || hp_percent != m_p_self->hp_percent
			   || mana_percent != m_p_self->mana_percent
			   || aggro_percent != m_p_self->aggro_percent
			   || zoning != m_p_self->zoning)
			{
				sprintf_s(m_send_buffer,
						  MAX_STRING,
						  "[NS]|%hhu %hhu %hhu %hhu",
						  hp_percent,
						  mana_percent,
						  aggro_percent,
						  zoning);

				m_p_send(m_send_buffer);
			}
		}
	}

	inline void PublishZone(bool force)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		if(nullptr != m_p_send)
		{
			PCHARINFO p_char   = GetCharInfo();
			PSPAWNINFO p_spawn = p_char->pSpawn;

			uint16_t spawn_id = (nullptr != p_spawn) ? static_cast<uint16_t>(p_spawn->SpawnID) : 0;
			uint16_t zone_id  = (nullptr != p_char) ? static_cast<uint16_t>(p_char->zoneId) : 0;
			uint8_t level     = (nullptr != p_spawn) ? static_cast<uint8_t>(p_spawn->Level) : m_p_self->level;

			if(force
			   || spawn_id != m_p_self->spawn_id
			   || zone_id != m_p_self->zone_id
			   || level != m_p_self->level)
			{
				sprintf_s(m_send_buffer,
						  MAX_STRING,
						  "[NS]|%hu %hu %hhu",
						  spawn_id,
						  zone_id,
						  level);

				m_p_send(m_send_buffer);
			}
		}
	}

	inline void HandleCasting()
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		return;
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

			for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < 90; i++)
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

		DOUBLE xp_percent    = nullptr != p_spawn ? (static_cast<DOUBLE>(p_char->Exp) / 3.3) + (static_cast<DOUBLE>(p_spawn->Level) * 100) : m_xp_percent;
		DOUBLE delta_xp      = xp_percent - m_xp_percent;
		m_xp_percent         = xp_percent;
		DOUBLE xp_aa_percent = nullptr != p_spawn ? (static_cast<DOUBLE>(p_char->AAExp) / 3.3) + (static_cast<DOUBLE>(GetCharInfo2()->AAPoints + GetCharInfo2()->AAPointsSpent) * 100) : m_xp_aa_percent;
		DOUBLE delta_xp_aa   = xp_aa_percent - m_xp_aa_percent;
		m_xp_aa_percent      = xp_aa_percent;

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

			// Turn on AA XP when level too high
			if(m_xp_percent >= 7050.0)
			{
				if(nullptr != p_char && p_char->PercentEXPtoAA < 100)
				{
					WriteChatf("Setting aa to 100");
					DoCommand(p_spawn, "/alt on 100");
				}
			}
		}

		if(delta_xp_aa != 0)
		{
			WriteChatf("You gained %.2f%% AA experience.", delta_xp_aa);

			// Turn off AA XP when level too low
			if(m_xp_percent <= 7050.0)
			{
				if(nullptr != p_char && p_char->PercentEXPtoAA > 0)
				{
					WriteChatf("Setting aa to 0");
					DoCommand(p_spawn, "/alt off");
				}
			}
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

		// Capitalize first letter
		std::toupper(name[0]);

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

		memset(aggro_list, 0, aggro_len);

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

	inline DWORD GetCorpses(char* corpse_list, size_t corpse_len, DWORD range = 100, CorpseType type = NPC)
	{
		FunctionTimer timer(m_time_counts[__FUNCTION__]);

		DWORD count = 0;

		PSPAWNINFO p_self = GetCharInfo()->pSpawn;

		memset(corpse_list, 0, corpse_len);

		for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < range; i++)
		{
			PSPAWNINFO p_spawn = (PSPAWNINFO)EQP_DistArray[i].VarPtr.Ptr;

			if(nullptr != p_self && nullptr != p_spawn)
			{
				if(fabs(p_self->Z - p_spawn->Z) < 20.0) // Only count corpses on this plane
				{
					switch(p_spawn->Type)
					{
					case SPAWN_CORPSE:

						switch(type)
						{
						case NPC:

							if(p_spawn->Deity == 0)
							{
								snprintf(corpse_list + strlen(corpse_list), corpse_len, "%d ", p_spawn->SpawnID);
							}

							break;

						case PC:

							if(p_spawn->Deity > 0)
							{
								snprintf(corpse_list + strlen(corpse_list), corpse_len, "%d ", p_spawn->SpawnID);
							}

							break;
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

	NetSendFunction FindNetSendFunction()
	{
		PMQPLUGIN p_plugin = pPlugins;
		while(p_plugin && _strnicmp(p_plugin->szFilename, "MQ2EQBC", 8))
		{
			p_plugin = p_plugin->pNext;
		}

		if(nullptr != p_plugin)
		{
			return (NetSendFunction)GetProcAddress(p_plugin->hModule, "NetBotSendMsg");
		}

		return nullptr;
	}

	char m_send_buffer[MAX_STRING];
	char m_command_buffer[MAX_STRING];
	NetSendFunction m_p_send;
	SpawnData* m_p_data_index;
	SpawnData* m_p_self;
	SpawnData* m_p_target;
	SpellQueue* m_p_spell_queue;
	Chase* m_p_chase;
	client* m_p_client;
	std::map<std::string, SpawnData> m_clients;
	std::map<std::string, std::clock_t> m_time_counts; // Function run time count
	std::clock_t m_time_start;
	std::clock_t m_last_tenth;
	std::clock_t m_last_half_second;
	std::clock_t m_last_second;
	std::clock_t m_last_tick;
	DOUBLE m_xp_percent;
	DOUBLE m_xp_aa_percent;
	bool m_force_refresh;
	bool m_drag_enable;
};
