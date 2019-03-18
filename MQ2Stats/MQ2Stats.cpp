
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
	, m_p_cast(nullptr)
	, m_p_data_index(nullptr)
	, m_p_self(nullptr)
	, m_p_target(nullptr)
	, m_p_spell_queue(nullptr)
	, m_p_chase(nullptr)
	, m_clients()
	, m_time_counts()
	, m_time_start(0)
	, m_last_tenth(0)
	, m_last_second(0)
	, m_xp_percent(0.0)
	, m_xp_aa_percent(0.0)
	, m_force_refresh(false)
	, m_drag_enable(false)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	m_p_self = &m_clients[GetCharInfo()->Name];

	m_p_spell_queue = new SpellQueue();
	m_p_chase       = new Chase();

	m_time_start = std::clock();

	// Indexables
	TypeMember(ID);
	TypeMember(ZoneID);
	TypeMember(InZone);
	TypeMember(Zoning);
	TypeMember(Level);
	TypeMember(Heading);
	TypeMember(Angle);
	TypeMember(X);
	TypeMember(Y);
	TypeMember(Z);
	TypeMember(Distance);
	TypeMember(Visible);
	TypeMember(HP);
	TypeMember(Mana);
	TypeMember(Aggro);

	// Globals
	TypeMember(Names);
	TypeMember(Active);
	TypeMember(Combat);
	TypeMember(Following);
	TypeMember(EngageID);
	TypeMember(LowestID);
	TypeMember(FurthestID);
	TypeMember(GroupHP);
	TypeMember(AggroList);
	TypeMember(CorpseList);
	TypeMember(RunTimes);
	TypeMember(DumpData);
}

MQ2Stats::~MQ2Stats()
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	delete m_p_chase;
	delete m_p_spell_queue;
}

void MQ2Stats::Pulse()
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

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
				m_p_send = (NetSendFunction)GetProcAddress(p_plugin->hModule, "NetBotSendMsg");
			}
		}

		if(nullptr == m_p_cast)
		{
			PMQPLUGIN p_plugin = pPlugins;
			while(p_plugin && _strnicmp(p_plugin->szFilename, "MQ2Cast", 7))
			{
				p_plugin = p_plugin->pNext;
			}

			if(nullptr != p_plugin)
			{
				m_p_cast = (CastFunction)GetProcAddress(p_plugin->hModule, "CastCommand");
			}
		}

		// Cannot do anything without server distribution
		if(nullptr != m_p_send)
		{
			std::clock_t now = std::clock();

			if(now > m_last_tenth + 100)
			{
				m_last_tenth = now;

				SpawnData current;
				Populate(current);

				SpawnData cached = m_clients[GetCharInfo()->Name];

				if(Compare(current, cached) != 0 || m_force_refresh)
				{
					PublishMessage(current);

					m_force_refresh = false;
				}

				HandleCasting();
			}

			if(now > m_last_second + 1000)
			{
				m_last_second = now;

				HandleCorpse();
				HandleDrag();
				HandleEvents();
			}
		}
	}
}

void MQ2Stats::CommandDrag(PCHAR p_line)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	std::vector<std::string> args;

	Split(std::string(p_line), args);

	static std::string on  = "on";
	static std::string off = "off";

	if(args.size() > 0)
	{
		if(Contains(args[0], on))
		{
			snprintf(m_command_buffer, MAX_STRING, "/bcaa //consent %s", GetCharInfo()->Name);
			DoCommand(GetCharInfo()->pSpawn, m_command_buffer);

			m_drag_enable = true;
		}
		else
		{
			m_drag_enable = false;
		}
	}
}

void MQ2Stats::CommandDumpSpawn(PCHAR p_line)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	std::vector<std::string> args;
	Split(std::string(p_line), args);

	if(args.size() > 0)
	{
		WriteChatf("Attempting to dump spawn ID: %d", std::stoi(args[0]));
		PSPAWNINFO p_spawn = (PSPAWNINFO)GetSpawnByID(std::stoi(args[0]));

		if(nullptr != p_spawn)
		{
			WriteChatf("%d", (uint32_t)p_spawn->JumpStrength);
			WriteChatf("%d", (uint32_t)p_spawn->SwimStrength);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedMultiplier);
			WriteChatf("%d", (uint32_t)p_spawn->AreaFriction);
			WriteChatf("%d", (uint32_t)p_spawn->AccelerationFriction);
			WriteChatf("%d", (uint32_t)p_spawn->CollidingType); /* ok finally had time to get this one right, when we collide with something this gets set. */
			WriteChatf("%d", (uint32_t)p_spawn->FloorHeight);
			WriteChatf("%d", (uint32_t)p_spawn->bSinksInWater);
			WriteChatf("%d", (uint32_t)p_spawn->PlayerTimeStamp); /* doesn't update when on a Vehicle (mounts/boats etc) */
			WriteChatf("%d", (uint32_t)p_spawn->LastTimeIdle);
			WriteChatf("%d", (uint32_t)p_spawn->AreaHPRegenMod); /*from guild hall pools etc. */
			WriteChatf("%d", (uint32_t)p_spawn->AreaEndRegenMod);
			WriteChatf("%d", (uint32_t)p_spawn->AreaManaRegenMod);
			WriteChatf("%d", (uint32_t)p_spawn->Y);
			WriteChatf("%d", (uint32_t)p_spawn->X);
			WriteChatf("%d", (uint32_t)p_spawn->Z);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedY);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedX);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedZ);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedRun);
			WriteChatf("%d", (uint32_t)p_spawn->Heading);
			WriteChatf("%d", (uint32_t)p_spawn->Angle);
			WriteChatf("%d", (uint32_t)p_spawn->AccelAngle);
			WriteChatf("%d", (uint32_t)p_spawn->SpeedHeading);
			WriteChatf("%d", (uint32_t)p_spawn->CameraAngle);
			WriteChatf("%d", (uint32_t)p_spawn->UnderWater); /*LastHeadEnvironmentType */
			WriteChatf("%d", (uint32_t)p_spawn->LastBodyEnvironmentType);
			WriteChatf("%d", (uint32_t)p_spawn->LastFeetEnvironmentType);
			WriteChatf("%d", (uint32_t)p_spawn->HeadWet); /*these really are environment related, like lava as well for example */
			WriteChatf("%d", (uint32_t)p_spawn->FeetWet);
			WriteChatf("%d", (uint32_t)p_spawn->BodyWet);
			WriteChatf("%d", (uint32_t)p_spawn->LastBodyWet);
			WriteChatf("%d", (uint32_t)p_spawn->PossiblyStuck); /* never seen this be 1 so maybe it was used a a point but not now... */
			WriteChatf("%d", (uint32_t)p_spawn->Type);
			WriteChatf("%d", (uint32_t)p_spawn->AvatarHeight); /* height of avatar from groundwhen standing*/
			WriteChatf("%d", (uint32_t)p_spawn->Height);
			WriteChatf("%d", (uint32_t)p_spawn->Width);
			WriteChatf("%d", (uint32_t)p_spawn->Length);
			WriteChatf("%d", (uint32_t)p_spawn->SpawnID);
			WriteChatf("%d", (uint32_t)p_spawn->PlayerState); /* 0=Idle 1=Open 2=WeaponSheathed 4=Aggressive 8=ForcedAggressive 0x10=InstrumentEquipped 0x20=Stunned 0x40=PrimaryWeaponEquipped 0x80=SecondaryWeaponEquipped */
			WriteChatf("%d", (uint32_t)p_spawn->Vehicle);     /* NULL until you collide with a vehicle (boat,airship etc) */
			WriteChatf("%d", (uint32_t)p_spawn->Mount);       /* NULL if no mount present */
			WriteChatf("%d", (uint32_t)p_spawn->Rider);       /* _SPAWNINFO of mount's rider */
			WriteChatf("%d", (uint32_t)p_spawn->Unknown0x015c);
			WriteChatf("%d", (uint32_t)p_spawn->Targetable); /* true if mob is targetable */
			WriteChatf("%d", (uint32_t)p_spawn->bTargetCyclable);
			WriteChatf("%d", (uint32_t)p_spawn->bClickThrough);
			WriteChatf("%d", (uint32_t)p_spawn->bBeingFlung);
			WriteChatf("%d", (uint32_t)p_spawn->FlingActiveTimer);
			WriteChatf("%d", (uint32_t)p_spawn->FlingTimerStart);
			WriteChatf("%d", (uint32_t)p_spawn->bFlingSomething);
			WriteChatf("%d", (uint32_t)p_spawn->FlingY);
			WriteChatf("%d", (uint32_t)p_spawn->FlingX);
			WriteChatf("%d", (uint32_t)p_spawn->FlingZ);
			WriteChatf("%d", (uint32_t)p_spawn->bFlingSnapToDest);
			WriteChatf("%d", (uint32_t)p_spawn->SplineID);
			WriteChatf("%d", (uint32_t)p_spawn->SplineRiderID);
			WriteChatf("%d", (uint32_t)p_spawn->LastIntimidateUse);
			WriteChatf("%d", (uint32_t)p_spawn->TargetOfTarget);
			WriteChatf("%s", (char*)p_spawn->Unknown0x0e70);
			WriteChatf("%d", (uint32_t)p_spawn->MeleeRadius); // used by GetMeleeRange
			WriteChatf("%d", (uint32_t)p_spawn->CollisionCounter);
			WriteChatf("%d", (uint32_t)p_spawn->CachedFloorLocationY);
			WriteChatf("%d", (uint32_t)p_spawn->CachedFloorLocationX);
			WriteChatf("%d", (uint32_t)p_spawn->CachedFloorLocationZ);
			WriteChatf("%d", (uint32_t)p_spawn->CachedFloorHeight);
			WriteChatf("%d", (uint32_t)p_spawn->CachedCeilingLocationY);
			WriteChatf("%d", (uint32_t)p_spawn->CachedCeilingLocationX);
			WriteChatf("%d", (uint32_t)p_spawn->CachedCeilingLocationZ);
			WriteChatf("%d", (uint32_t)p_spawn->CachedCeilingHeight);
			WriteChatf("%d", (uint32_t)p_spawn->Animation);
			WriteChatf("%d", (uint32_t)p_spawn->NextAnim);
			WriteChatf("%d", (uint32_t)p_spawn->CurrLowerBodyAnim);
			WriteChatf("%d", (uint32_t)p_spawn->NextLowerBodyAnim);
			WriteChatf("%d", (uint32_t)p_spawn->CurrLowerAnimVariation);
			WriteChatf("%d", (uint32_t)p_spawn->CurrAnimVariation);
			WriteChatf("%d", (uint32_t)p_spawn->CurrAnimRndVariation);
			WriteChatf("%d", (uint32_t)p_spawn->Loop3d_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->Step_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->CurLoop_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Idle3d1_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Idle3d2_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Jump_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Hit1_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Hit2_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Hit3_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->Hit4_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Gasp1_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Gasp2_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Drown_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Death_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Attk1_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Attk2_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Attk3_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->Walk_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Run_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Crouch_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Swim_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->TreadWater_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Climb_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Sit_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Kick_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->Bash_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->FireBow_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->MonkAttack1_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->MonkAttack2_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->MonkSpecial_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->PrimaryBlunt_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->PrimarySlash_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->PrimaryStab_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->Punch_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->Roundhouse_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->SecondaryBlunt_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->SecondarySlash_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->SecondaryStab_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->SwimAttack_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->TwoHandedBlunt_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->TwoHandedSlash_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->TwoHandedStab_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->SecondaryPunch_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->JumpAcross_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->WalkBackwards_SoundID);
			WriteChatf("%d", (uint32_t)p_spawn->CrouchWalk_SoundID);

			WriteChatf("%d", (uint32_t)p_spawn->RightHolding); //119c see 514929
			WriteChatf("%d", (uint32_t)p_spawn->LeftHolding);  //1 holding 0 not holding 11a0 see 514946
			WriteChatf("%d", (uint32_t)p_spawn->LastBubblesTime);
			WriteChatf("%d", (uint32_t)p_spawn->LastColdBreathTime);
			WriteChatf("%d", (uint32_t)p_spawn->LastParticleUpdateTime);
			WriteChatf("%d", (uint32_t)p_spawn->MercID);       //if the spawn is player and has a merc up this is it's spawn ID -eqmule 16 jul 2014
			WriteChatf("%d", (uint32_t)p_spawn->ContractorID); //if the spawn is a merc this is its contractor's spawn ID -eqmule 16 jul 2014
			WriteChatf("%d", (uint32_t)p_spawn->CeilingHeightAtCurrLocation);
			WriteChatf("%d", (uint32_t)p_spawn->MobileEmitter); //todo: change and map to EqMobileEmitter*
			WriteChatf("%d", (uint32_t)p_spawn->bInstantHPGaugeChange);
			WriteChatf("%d", (uint32_t)p_spawn->LastUpdateReceivedTime);
			WriteChatf("%d", (uint32_t)p_spawn->MaxSpeakDistance);
			WriteChatf("%d", (uint32_t)p_spawn->WalkSpeed);
			WriteChatf("%s", (char*)p_spawn->Unknown0x11cc);
			WriteChatf("%d", (uint32_t)p_spawn->InvitedToGroup);
			WriteChatf("%d", (char*)p_spawn->Unknown0x1211);
			WriteChatf("%d", (uint32_t)p_spawn->GroupMemberTargeted); // 0xFFFFFFFF if no target, else 1 through 5
			WriteChatf("%d", (uint32_t)p_spawn->bRemovalPending);
			WriteChatf("%s", (char*)p_spawn->pCorpse); //look into 0x12f4 for sure!
			WriteChatf("%d", (uint32_t)p_spawn->DefaultEmitterID);
			WriteChatf("%d", (uint32_t)p_spawn->bDisplayNameSprite);
			WriteChatf("%d", (uint32_t)p_spawn->bIdleAnimationOff);
			WriteChatf("%d", (uint32_t)p_spawn->bIsInteractiveObject);
			/*0x136b*/
			WriteChatf("%d", (uint32_t)p_spawn->CampfireY);
			WriteChatf("%d", (uint32_t)p_spawn->CampfireX);
			WriteChatf("%d", (uint32_t)p_spawn->CampfireZ);
			WriteChatf("%d", (uint32_t)p_spawn->CampfireZoneID);    // zone ID where campfire is
			WriteChatf("%d", (uint32_t)p_spawn->CampfireTimestamp); // CampfireTimestamp-FastTime()=time left on campfire
			WriteChatf("%d", (uint32_t)p_spawn->FellowShipID);
			WriteChatf("%d", (uint32_t)p_spawn->CampType);
			WriteChatf("%d", (uint32_t)p_spawn->Campfire); // do we have a campfire up?
			WriteChatf("%d", (uint32_t)p_spawn->bGMCreatedNPC);
			WriteChatf("%d", (uint32_t)p_spawn->ObjectAnimationID);
			WriteChatf("%d", (uint32_t)p_spawn->bInteractiveObjectCollidable);
			WriteChatf("%d", (uint32_t)p_spawn->InteractiveObjectType);
			WriteChatf("%d", (uint32_t)p_spawn->LastHistorySentTime); //for sure 1e8c see 5909C7
			WriteChatf("%d", (uint32_t)p_spawn->CurrentBardTwistIndex);
			WriteChatf("%s", (char*)p_spawn->SpawnStatus); //todo: look closer at these i think they can show like status of mobs slowed, mezzed etc, but not sure
			WriteChatf("%d", (uint32_t)p_spawn->BannerIndex1);
			WriteChatf("%d", (uint32_t)p_spawn->MountAnimationRelated);
			WriteChatf("%d", (uint32_t)p_spawn->bGuildShowAnim);  //or sprite? need to check
			WriteChatf("%d", (uint32_t)p_spawn->bWaitingForPort); //check this
		}
	}
}

void MQ2Stats::CommandMissing()
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

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

void MQ2Stats::CommandPush(PCHAR p_line)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	std::vector<std::string> args;

	Split(std::string(p_line), args);

	if(args.size() >= 2)
	{
		std::string& type_name   = args[1];
		std::string& target_name = args[2];

		if(nullptr != m_p_spell_queue)
		{
			m_p_spell_queue->Push(type_name, target_name);
		}
	}
}

void MQ2Stats::CommandRunTimes()
{
	std::clock_t exec_time  = 0;
	std::clock_t end_time   = std::clock();
	std::clock_t total_time = end_time - m_time_start;

	for(auto it = m_time_counts.begin(); it != m_time_counts.end(); it++)
	{
		exec_time += it->second;
	}

    WriteChatf("%-32s %-8s   %-8s", "FUNCTION", "EXEC", "TOTAL");

	for(auto it = m_time_counts.begin(); it != m_time_counts.end(); it++)
	{
		WriteChatf("%-32s %8.7f  %8.7f", it->first.c_str(), static_cast<FLOAT>(it->second) / exec_time, static_cast<FLOAT>(it->second) / total_time);
	}
}

void MQ2Stats::SetClientIndex(PCHAR index)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	if(m_clients.find(index) != m_clients.end())
	{
		m_p_data_index = &m_clients[index];
	}
	else
	{
		m_p_data_index = nullptr;
	}
}

bool MQ2Stats::GetMember(MQ2VARPTR p_var, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	if(PMQ2TYPEMEMBER pMember = MQ2Stats::FindMember(Member))
	{
		if(nullptr != m_p_data_index)
		{
			switch(static_cast<StatType>(pMember->ID))
			{
			case ID:

				Dest.Type  = pIntType;
				Dest.DWord = m_p_data_index->spawn_id;
				break;

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

			case Heading:

				Dest.Type  = pFloatType;
				Dest.Float = m_p_data_index->heading;
				return true;

			case Angle:

				Dest.Type  = pFloatType;
				Dest.Float = m_p_data_index->angle;
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

		case Following:

			return false;

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

void CommandDrag(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandDrag(p_line);
	}
}

void CommandMissing(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandMissing();
	}
}

void CommandPush(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandPush(p_line);
	}
}

void CommandRunTimes(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandRunTimes();
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

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState)
{
	switch(GameState)
	{
	case GAMESTATE_INGAME:
	case GAMESTATE_LOGGINGIN:

		WriteChatf("Create case hit");

		if(nullptr == p_stats)
		{
			WriteChatf("Creating");
			p_stats = new MQ2Stats();
		}

		break;
	
    case GAMESTATE_CHARCREATE:
	case GAMESTATE_CHARSELECT:
	case GAMESTATE_POSTFRONTLOAD:
	case GAMESTATE_PRECHARSELECT:
	case GAMESTATE_SOMETHING:
	case GAMESTATE_UNLOADING:
	default:
		
        WriteChatf("Destroy case hit");
		
		if(nullptr != p_stats)
		{
		    WriteChatf("Destroying");
			delete p_stats;
			p_stats = nullptr;
		}

		break;
	}
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	// Initialize plugin if the game state is in-game
	SetGameState(gGameState);

	AddMQ2Data("Stats", DataStats);

	AddCommand("/drag", CommandDrag);
	AddCommand("/dumpspawn", CommandDrag);
	AddCommand("/missing", CommandMissing);
	AddCommand("/push", CommandPush);
	AddCommand("/runtimes", CommandRunTimes);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/runtimes");
	RemoveCommand("/push");
	RemoveCommand("/missing");
	RemoveCommand("/dumpspawn");
	RemoveCommand("/drag");

	RemoveMQ2Data("Stats");

	WriteChatf("Shutdown");
	// Always destroy on shutdown
	if(nullptr != p_stats)
	{
		WriteChatf("Destroying");
		delete p_stats;
		p_stats = nullptr;
	}
}

PLUGIN_API VOID OnNetBotMSG(PCHAR name, PCHAR message)
{
	if(nullptr != p_stats)
	{
		p_stats->ParseMessage(name, message);
	}
}

PLUGIN_API VOID OnPulse(VOID)
{
	if(nullptr != p_stats)
	{
		p_stats->Pulse();
	}
}

// This is called when we receive the EQ_END_ZONE packet is received
PLUGIN_API void OnEndZone(void)
{
	if(nullptr != p_stats)
	{
		p_stats->ClearMovement();
	}
}

//PLUGIN_API VOID OnNetBotEVENT(PCHAR message)
//{
//	if(nullptr != p_stats)
//	{
//		p_stats->ParseEvent(message);
//	}
//}

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
