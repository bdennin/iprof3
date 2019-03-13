
#include "MQ2Stats.h"

PreSetup("MQ2Stats");
PLUGIN_VERSION(1.0);

#include <sstream>

class MQ2Stats* p_stats = nullptr;

inline size_t MQ2Stats::Split(std::string& message, std::vector<std::string>& tokens, const char delineator)
{
	std::istringstream iss(message);
	std::string token;
	uint32_t count = 0;

	while(std::getline(iss, token, delineator))
	{
		tokens.push_back(token);
		count++;
	}

	return count;
}

inline bool MQ2Stats::Contains(std::string& words, std::string& match)
{
	if(words.empty() || match.empty())
	{
		return false;
	}

	std::transform(words.begin(), words.end(), words.begin(), tolower);
	std::transform(match.begin(), match.end(), match.begin(), tolower);

	return words.find(match) != std::string::npos;
}

MQ2Stats::MQ2Stats()
	: MQ2Type("Stats")
	, m_p_send(nullptr)
	, m_p_data_index(nullptr)
	, m_p_target(nullptr)
	, m_last_pulse(0)
	, m_clients()
{
	TypeMember(ZoneID);
	TypeMember(InZone);
	TypeMember(Zoning);
	TypeMember(Level);
	TypeMember(X);
	TypeMember(Y);
	TypeMember(Z);
	TypeMember(Distance);
	TypeMember(Visible);
	TypeMember(HP);
	TypeMember(Mana);
	TypeMember(Aggro);

	TypeMember(DumpSpell);
	TypeMember(Names);
	TypeMember(Active);
	TypeMember(Combat);
	TypeMember(FollowID);
	TypeMember(EngageID);
	TypeMember(LowestID);
	TypeMember(FurthestID);
	TypeMember(GroupHP);
	TypeMember(AggroList);
	TypeMember(AggroCount);
	TypeMember(CorpseList);
	TypeMember(CorpseCount);
}

MQ2Stats::~MQ2Stats()
{
}

void MQ2Stats::Pulse()
{
	if(gGameState == GAMESTATE_INGAME)
	{
		if(nullptr == m_p_send)
		{
			PMQPLUGIN p_plugin = pPlugins;
			while(p_plugin && _strnicmp(p_plugin->szFilename, "MQ2EQBC", 8))
			{
				p_plugin = p_plugin->pNext;
			}

			if(nullptr != p_plugin)
			{
				m_p_send = (SendFunction)GetProcAddress(p_plugin->hModule, "NetBotSendMsg");
			}
		}

		if(nullptr != m_p_send)
		{
			if(std::clock() > m_last_pulse + CLOCK_TICK)
			{
				m_last_pulse = std::clock();

				SpawnData current;
				Populate(current);
				
                SpawnData cached = m_clients[GetCharInfo()->Name];

                if(Compare(current, cached) != 0)
                {
                    PublishMessage(current);
                }
                
				HandleEvents();
			}
		}
	}
}

void MQ2Stats::SetClientIndex(PCHAR index)
{
	if(m_clients.find(index) != m_clients.end())
	{
		m_p_data_index = &m_clients[index];
	}
	else
	{
		m_p_data_index = nullptr;
	}
}

void MQ2Stats::Command(PCHAR p_line)
{
	std::vector<std::string> args;
	static std::string queue   = "queue";
	static std::string missing = "missing";
	static std::string corpse  = "corpse";

	Split(std::string(p_line), args);

	if(args.size() > 0)
	{
		std::string& command = args[0];

		if(Contains(command, queue) && args.size() >= 3)
		{
			std::string& spell_type  = args[1];
			std::string& target_name = args[2];
		}
		else if(Contains(command, missing))
		{
			char missing[MAX_STRING] = { 0 };
			size_t num_missing       = 0;

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				if(!IsClientVisible(&(it->second)))
				{

					snprintf(missing + strlen(missing), MAX_STRING, "  %s\n", it->first.c_str());
					num_missing++;
				}
			}

			if(num_missing > 0)
			{
				WriteChatf("Missing clients:\n%s", missing);
			}
			else
			{
				WriteChatf("All clients present.");
			}
		}
		else if(Contains(command, corpse))
        {
            char exec[MAX_STRING];

			for(size_t i = 0; i < gSpawnCount && EQP_DistArray[i].Value.Float < 100; i++)
			{
				PSPAWNINFO p_spawn = (PSPAWNINFO)EQP_DistArray[i].VarPtr.Ptr;

				if(nullptr != p_spawn)
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
}

bool MQ2Stats::GetMember(MQ2VARPTR p_var, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
{
	if(PMQ2TYPEMEMBER pMember = MQ2Stats::FindMember(Member))
	{
		if(nullptr != m_p_data_index)
		{
			switch(static_cast<StatType>(pMember->ID))
			{
			case ID:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->spawn_id;
				return true;

			case ZoneID:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->zone_id;
				return true;

			case InZone:

				Dest.Type  = pBoolType;
				Dest.DWord = IsClientInZone(m_p_data_index);
				return true;

			case Zoning:

				Dest.Type  = pBoolType;
				Dest.DWord = m_p_data_index->zoning;
				return true;

			case Level:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->level;
				return true;

			case X:

				Dest.Type  = pFloatType;
				Dest.Float = m_p_data_index->x;
				return true;

			case Y:

				Dest.Type  = pFloatType;
				Dest.Float = m_p_data_index->y;
				return true;

			case Z:

				Dest.Type  = pFloatType;
				Dest.Float = m_p_data_index->z;
				return true;

			case Distance:

				Dest.Type  = pFloatType;
				Dest.Float = DistanceToClient(m_p_data_index);
				return true;

			case Visible:

				Dest.Type  = pBoolType;
				Dest.DWord = IsClientVisible(m_p_data_index);
				return true;

			case HP:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->hp_percent;
				return true;

			case Mana:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->mana_percent;
				return true;

			case Aggro:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->aggro_percent;
				return true;

			default:
				return false;
			}
		}

		switch(static_cast<StatType>(pMember->ID))
		{
		case DumpSpell:

			//PSPELL p_spell = GetSpellByName();

			//   if(GetSpell)
			//{

			//}

			break;
		case Names:
		{
			char names[MAX_STRING] = { 0 };

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				snprintf(names + strlen(names), MAX_STRING, "%s ", it->first.c_str());
			}

			strcpy_s(DataTypeTemp, names);

			Dest.Type = pStringType;
			Dest.Ptr  = &DataTypeTemp[0];
			return true;
		}
		case Active:

			Dest.Type  = pIntType;
			Dest.DWord = gbInForeground ? 1 : 0;
			return true;
			/*
			case FollowID:

				Dest.Type  = pIntType;
				Dest.DWord = m_follow_id;
				return true;

			case EngageID:

				Dest.Type  = pIntType;

				Dest.DWord = m_engage_id;
				return true;
*/

		case Combat:
		{
			bool in_combat = false;

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				if(IsClientInZone(&(it->second)) && it->second.aggro_percent > 0)
				{
					in_combat = true;
				}
			}

			Dest.Type  = pBoolType;
			Dest.DWord = in_combat;
			return true;
		}
		case LowestID:
		{
			DWORD lowest_id = 0;
			DWORD lowest_hp = 100;

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				if(IsClientInZone(&(it->second)) && DistanceToClient(&(it->second)) < 100.0 && it->second.hp_percent < lowest_hp)
				{
					lowest_id = it->second.spawn_id;
					lowest_hp = it->second.hp_percent;
				}
			}

			Dest.Type  = pIntType;
			Dest.DWord = lowest_id;
			return true;
		}
		case FurthestID:
		{
			DWORD furthest_id = 0;
			FLOAT furthest    = 0;
			FLOAT distance    = 0;

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				distance = DistanceToClient(&(it->second));

				if(distance > furthest)
				{
					furthest_id = it->second.spawn_id;
					furthest    = distance;
				}
			}

			Dest.Type  = pIntType;
			Dest.DWord = furthest_id;

			return true;
		}
		case GroupHP:
		{
			PSPAWNINFO p_self = GetCharInfo()->pSpawn;
			DWORD hp_count    = static_cast<DWORD>(static_cast<FLOAT>(p_self->HPCurrent) / static_cast<FLOAT>(p_self->HPMax) * 100);
			DWORD num_in_zone = 1;

			for(auto it = m_clients.begin(); it != m_clients.end(); it++)
			{
				if(IsGroupMember(const_cast<char*>(it->first.c_str())) && DistanceToClient(&(it->second)) < 50.0)
				{
					hp_count += it->second.hp_percent;
					num_in_zone++;
				}
			}

			Dest.Type  = pIntType;
			Dest.DWord = static_cast<DWORD>(static_cast<FLOAT>(hp_count) / static_cast<FLOAT>(num_in_zone));
			return true;
		}
		case AggroList:
		{
			DWORD range = 100;

			std::vector<std::string> args;
			Split(std::string(Index), args);
			if(args.size() > 0)
			{
				range = std::stoi(args[0]);
			}

			GetAggroed(DataTypeTemp, MAX_STRING, range);

			Dest.Type = pStringType;
			Dest.Ptr  = &DataTypeTemp[0];
			return true;
		}
		case AggroCount:
		{
			DWORD range = 100;

			std::vector<std::string> args;
			Split(std::string(Index), args);
			if(args.size() > 0)
			{
				range = std::stoi(args[0]);
			}

			Dest.Type  = pIntType;
			Dest.DWord = GetAggroed(DataTypeTemp, MAX_STRING, range);
			return true;
		}
		case CorpseList:
		{
			DWORD range = 100;

			std::vector<std::string> args;
			Split(std::string(Index), args);
			if(args.size() > 0)
			{
				range = std::stoi(args[0]);
			}

			GetCorpses(DataTypeTemp, MAX_STRING, range);

			Dest.Type = pStringType;
			Dest.Ptr  = &DataTypeTemp[0];
			return true;
		}
		default:
			break;
		}
	}

	return false;
}

bool MQ2Stats::ToString(MQ2VARPTR p_var, PCHAR p_dest)
{
	strcpy_s(p_dest, MAX_STRING, "TRUE");
	return true;
}

bool MQ2Stats::FromData(MQ2VARPTR& p_var, MQ2TYPEVAR& src)
{
	return false;
}

bool MQ2Stats::FromString(MQ2VARPTR& p_var, PCHAR p_src)
{
	return false;
}

void CommandStats(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->Command(p_line);
	}
}

BOOL DataStats(PCHAR index, MQ2TYPEVAR& dest)
{
	if(nullptr != p_stats)
	{
		dest.DWord = 1;
		dest.Type  = p_stats;

		p_stats->SetClientIndex(index);

		return true;
	}

	return false;
}

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	p_stats = new MQ2Stats();

	AddMQ2Data("Stats", DataStats);

	AddCommand("/stats", CommandStats, 0, 1, 1);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/stats");

	RemoveMQ2Data("Stats");

	delete p_stats;
	p_stats = nullptr;
}

//PLUGIN_API VOID OnNetBotEVENT(PCHAR message)
//{
//	if(nullptr != p_stats)
//	{
//		p_stats->ParseEvent(message);
//	}
//}

PLUGIN_API VOID OnNetBotMSG(PCHAR name, PCHAR message)
{
	if(nullptr != p_stats)
	{
		p_stats->ParseMessage(name, message);
	}
}

// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
	if(nullptr != p_stats)
	{
		p_stats->Pulse();
	}
}

//// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
//// It is NOT called for each existing spawn when a plugin shuts down.
//PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
//{
//    WriteChatf("MQ2Stats::OnRemoveSpawn(%s)", pSpawn->Name);
//}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
//PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
//{
//    WriteChatf("MQ2Stats::OnAddSpawn(%s)", pNewSpawn->Name);
//}

//// Called after entering a new zone
//PLUGIN_API VOID OnZoned(VOID)
//{
//  WriteChatf("MQ2Stats::OnZoned()");
//}
//
//// Called once directly before shutdown of the new ui system, and also
//// every time the game calls CDisplay::CleanGameUI()
//PLUGIN_API VOID OnCleanUI(VOID)
//{
//  WriteChatf("MQ2Stats::OnCleanUI()");
//  // destroy custom windows, etc
//}
//
//// Called once directly after the game ui is reloaded, after issuing /loadskin
//PLUGIN_API VOID OnReloadUI(VOID)
//{
//  WriteChatf("MQ2Stats::OnReloadUI()");
//  // recreate custom windows, etc
//}
//
//// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
//PLUGIN_API VOID OnDrawHUD(VOID)
//{
//  // DONT leave in this debugspew, even if you leave in all the others
//  //WriteChatf("MQ2Stats::OnDrawHUD()");
//}

// Called once directly after initialization, and then every time the gamestate changes
//PLUGIN_API VOID SetGameState(DWORD GameState)
//{
//}

//// This is called every time WriteChatColor is called by MQ2Main or any plugin,
//// IGNORING FILTERS, IF YOU NEED THEM MAKE SURE TO IMPLEMENT THEM. IF YOU DONT
//// CALL CEverQuest::dsp_chat MAKE SURE TO IMPLEMENT EVENTS HERE (for chat plugins)
//PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
//{
//  WriteChatf("MQ2Stats::OnWriteChatColor(%s)", Line);
//  return 0;
//}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
//PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
//{
//    WriteChatf("MQ2Stats::OnIncomingChat(%s)", Line);
//
//    return 0;
//}

//// This is called each time a ground item is added to a zone
//// or for each existing ground item when a plugin first initializes
//// NOTE: When you zone, these will come BEFORE OnZoned
//PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
//{
//  WriteChatf("MQ2Stats::OnAddGroundItem(%d)", pNewGroundItem->DropID);
//}

//// This is called each time a ground item is removed from a zone
//// It is NOT called for each existing ground item when a plugin shuts down.
//PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
//{
//  WriteChatf("MQ2Stats::OnRemoveGroundItem(%d)", pGroundItem->DropID);
//}

//// This is called when we receive the EQ_BEGIN_ZONE packet is received
//PLUGIN_API VOID OnBeginZone(VOID)
//{
//}

//// This is called when we receive the EQ_END_ZONE packet is received
//PLUGIN_API VOID OnEndZone(VOID)
//{
//}
//// This is called when pChar!=pCharOld && We are NOT zoning
//// honestly I have no idea if its better to use this one or EndZone (above)
//PLUGIN_API VOID Zoned(VOID)
//{
//}
