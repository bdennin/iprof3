
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
	//, m_p_send(nullptr)
	, m_p_data_index(nullptr)
	, m_p_self(nullptr)
	, m_p_target(nullptr)
	, m_p_client(nullptr)
	, m_p_spell_queue(nullptr)
	, m_p_chase(nullptr)
	, m_clients()
	, m_time_counts()
	, m_time_start(0)
	, m_last_tenth(0)
	, m_last_half_second(0)
	, m_last_second(0)
	, m_last_tick(0)
	, m_xp_percent(0.0)
	, m_xp_aa_percent(0.0)
	, m_force_refresh(false)
	, m_drag_enable(false)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	//m_p_send = FindNetSendFunction();

	m_p_self = &m_clients[GetCharInfo()->Name];

	m_p_client      = new Client(0xC0A8006A, 44444);
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
}

MQ2Stats::~MQ2Stats()
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	delete m_p_chase;
	delete m_p_spell_queue;
    delete m_p_client;
}

void MQ2Stats::Pulse()
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	if(gGameState == GAMESTATE_INGAME)
	{
		//if(nullptr == m_p_send)
		//{
		//	m_p_send = FindNetSendFunction();
		//}

		// Cannot do anything without server distribution
		//if(nullptr != m_p_send)
		//{
		std::clock_t now = std::clock();

		if(now > m_last_tenth + 100)
		{
			m_last_tenth = now;

            HandleNetwork();

			//PublishStats(false);

			HandleCasting();
		}

		if(now > m_last_half_second + 500)
		{
			m_last_half_second = now;

			//PublishLocation(false);

			HandleMovement();
		}

		if(now > m_last_second + 1000)
		{
			m_last_second = now;

			//PublishZone(false);

			HandleCorpse();
			HandleDrag();
			HandleEvents();
		}
		//}
	}
}

void MQ2Stats::CommandChase(PCHAR p_line)
{
	FunctionTimer timer(m_time_counts[__FUNCTION__]);

	std::vector<std::string> args;

	Split(std::string(p_line), args);

	static std::string off = "off";
	static std::string on  = "on";

	if(args.size() > 0)
	{
		WriteChatf("Have chase command");
		if(Contains(args[0], off))
		{
			ClearMovement();
		}
		else if(IsClient(args[0]))
		{
			WriteChatf("Set client to %s", args[0].c_str());
			m_p_target = GetClient(args[0]);
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
		PSPAWNINFO p_spawn = (PSPAWNINFO)GetSpawnByID(std::stoi(args[0]));

		if(nullptr != p_spawn)
		{
			DebugSpewAlwaysFile("JumpStrength: %d", (int32_t)p_spawn->JumpStrength);
			DebugSpewAlwaysFile("SwimStrength: %d", (int32_t)p_spawn->SwimStrength);
			DebugSpewAlwaysFile("SpeedMultiplier: %d", (int32_t)p_spawn->SpeedMultiplier);
			DebugSpewAlwaysFile("AreaFriction: %d", (int32_t)p_spawn->AreaFriction);
			DebugSpewAlwaysFile("AccelerationFriction: %d", (int32_t)p_spawn->AccelerationFriction);
			DebugSpewAlwaysFile("CollidingType: %d", (int32_t)p_spawn->CollidingType);
			DebugSpewAlwaysFile("FloorHeight: %d", (int32_t)p_spawn->FloorHeight);
			DebugSpewAlwaysFile("bSinksInWater: %d", (int32_t)p_spawn->bSinksInWater);
			DebugSpewAlwaysFile("PlayerTimeStamp: %d", (int32_t)p_spawn->PlayerTimeStamp);
			DebugSpewAlwaysFile("LastTimeIdle: %d", (int32_t)p_spawn->LastTimeIdle);
			DebugSpewAlwaysFile("Lastname: %d", (int32_t)p_spawn->Lastname);
			DebugSpewAlwaysFile("AreaHPRegenMod: %d", (int32_t)p_spawn->AreaHPRegenMod);
			DebugSpewAlwaysFile("AreaEndRegenMod: %d", (int32_t)p_spawn->AreaEndRegenMod);
			DebugSpewAlwaysFile("AreaManaRegenMod: %d", (int32_t)p_spawn->AreaManaRegenMod);
			DebugSpewAlwaysFile("Y: %d", (int32_t)p_spawn->Y);
			DebugSpewAlwaysFile("X: %d", (int32_t)p_spawn->X);
			DebugSpewAlwaysFile("Z: %d", (int32_t)p_spawn->Z);
			DebugSpewAlwaysFile("SpeedY: %d", (int32_t)p_spawn->SpeedY);
			DebugSpewAlwaysFile("SpeedX: %d", (int32_t)p_spawn->SpeedX);
			DebugSpewAlwaysFile("SpeedZ: %d", (int32_t)p_spawn->SpeedZ);
			DebugSpewAlwaysFile("SpeedRun: %d", (int32_t)p_spawn->SpeedRun);
			DebugSpewAlwaysFile("Heading: %d", (int32_t)p_spawn->Heading);
			DebugSpewAlwaysFile("Angle: %d", (int32_t)p_spawn->Angle);
			DebugSpewAlwaysFile("AccelAngle: %d", (int32_t)p_spawn->AccelAngle);
			DebugSpewAlwaysFile("SpeedHeading: %d", (int32_t)p_spawn->SpeedHeading);
			DebugSpewAlwaysFile("CameraAngle: %d", (int32_t)p_spawn->CameraAngle);
			DebugSpewAlwaysFile("UnderWater: %d", (int32_t)p_spawn->UnderWater);
			DebugSpewAlwaysFile("LastBodyEnvironmentType: %d", (int32_t)p_spawn->LastBodyEnvironmentType);
			DebugSpewAlwaysFile("LastFeetEnvironmentType: %d", (int32_t)p_spawn->LastFeetEnvironmentType);
			DebugSpewAlwaysFile("HeadWet: %d", (int32_t)p_spawn->HeadWet);
			DebugSpewAlwaysFile("FeetWet: %d", (int32_t)p_spawn->FeetWet);
			DebugSpewAlwaysFile("BodyWet: %d", (int32_t)p_spawn->BodyWet);
			DebugSpewAlwaysFile("LastBodyWet: %d", (int32_t)p_spawn->LastBodyWet);
			DebugSpewAlwaysFile("Name: %d", (int32_t)p_spawn->Name);
			DebugSpewAlwaysFile("DisplayedName: %d", (int32_t)p_spawn->DisplayedName);
			DebugSpewAlwaysFile("PossiblyStuck: %d", (int32_t)p_spawn->PossiblyStuck);
			DebugSpewAlwaysFile("Type: %d", (int32_t)p_spawn->Type);
			DebugSpewAlwaysFile("BodyType: %d", (int32_t)p_spawn->BodyType);
			DebugSpewAlwaysFile("CharPropFiller: %d", (int32_t)p_spawn->CharPropFiller);
			DebugSpewAlwaysFile("AvatarHeight: %d", (int32_t)p_spawn->AvatarHeight);
			DebugSpewAlwaysFile("Height: %d", (int32_t)p_spawn->Height);
			DebugSpewAlwaysFile("Width: %d", (int32_t)p_spawn->Width);
			DebugSpewAlwaysFile("Length: %d", (int32_t)p_spawn->Length);
			DebugSpewAlwaysFile("SpawnID: %d", (int32_t)p_spawn->SpawnID);
			DebugSpewAlwaysFile("PlayerState: %d", (int32_t)p_spawn->PlayerState);
			DebugSpewAlwaysFile("Vehicle: %d", (int32_t)p_spawn->Vehicle);
			DebugSpewAlwaysFile("Mount: %d", (int32_t)p_spawn->Mount);
			DebugSpewAlwaysFile("Rider: %d", (int32_t)p_spawn->Rider);
			DebugSpewAlwaysFile("Unknown0x015c: %d", (int32_t)p_spawn->Unknown0x015c);
			DebugSpewAlwaysFile("Targetable: %d", (int32_t)p_spawn->Targetable);
			DebugSpewAlwaysFile("bTargetCyclable: %d", (int32_t)p_spawn->bTargetCyclable);
			DebugSpewAlwaysFile("bClickThrough: %d", (int32_t)p_spawn->bClickThrough);
			DebugSpewAlwaysFile("bBeingFlung: %d", (int32_t)p_spawn->bBeingFlung);
			DebugSpewAlwaysFile("FlingActiveTimer: %d", (int32_t)p_spawn->FlingActiveTimer);
			DebugSpewAlwaysFile("FlingTimerStart: %d", (int32_t)p_spawn->FlingTimerStart);
			DebugSpewAlwaysFile("bFlingSomething: %d", (int32_t)p_spawn->bFlingSomething);
			DebugSpewAlwaysFile("FlingY: %d", (int32_t)p_spawn->FlingY);
			DebugSpewAlwaysFile("FlingX: %d", (int32_t)p_spawn->FlingX);
			DebugSpewAlwaysFile("FlingZ: %d", (int32_t)p_spawn->FlingZ);
			DebugSpewAlwaysFile("bFlingSnapToDest: %d", (int32_t)p_spawn->bFlingSnapToDest);
			DebugSpewAlwaysFile("SplineID: %d", (int32_t)p_spawn->SplineID);
			DebugSpewAlwaysFile("SplineRiderID: %d", (int32_t)p_spawn->SplineRiderID);
			DebugSpewAlwaysFile("LastIntimidateUse: %d", (int32_t)p_spawn->LastIntimidateUse);
			//DebugSpewAlwaysFilMovementStat: e("%d", (int32_t)&p_spawn->MovementStats);
			DebugSpewAlwaysFile("WhoFollowing: %d", (int32_t)p_spawn->WhoFollowing);
			DebugSpewAlwaysFile("GroupAssistNPC: %d", (int32_t)p_spawn->GroupAssistNPC);
			DebugSpewAlwaysFile("RaidAssistNPC: %d", (int32_t)p_spawn->RaidAssistNPC);
			DebugSpewAlwaysFile("GroupMarkNPC: %d", (int32_t)p_spawn->GroupMarkNPC);
			DebugSpewAlwaysFile("RaidMarkNPC: %d", (int32_t)p_spawn->RaidMarkNPC);
			DebugSpewAlwaysFile("TargetOfTarget: %d", (int32_t)p_spawn->TargetOfTarget);
			DebugSpewAlwaysFile("Unknown0x0e70: %d", (int32_t)p_spawn->Unknown0x0e70);
			//DebugSpewAlwaysFilmActorClien: e("%d", (int32_t)&p_spawn->mActorClient);
			DebugSpewAlwaysFile("pAnimation: %d", (int32_t)p_spawn->pAnimation);
			DebugSpewAlwaysFile("MeleeRadius: %d", (int32_t)p_spawn->MeleeRadius);
			DebugSpewAlwaysFile("CollisionCounter: %d", (int32_t)p_spawn->CollisionCounter);
			DebugSpewAlwaysFile("CachedFloorLocationY: %d", (int32_t)p_spawn->CachedFloorLocationY);
			DebugSpewAlwaysFile("CachedFloorLocationX: %d", (int32_t)p_spawn->CachedFloorLocationX);
			DebugSpewAlwaysFile("CachedFloorLocationZ: %d", (int32_t)p_spawn->CachedFloorLocationZ);
			DebugSpewAlwaysFile("CachedFloorHeight: %d", (int32_t)p_spawn->CachedFloorHeight);
			DebugSpewAlwaysFile("CachedCeilingLocationY: %d", (int32_t)p_spawn->CachedCeilingLocationY);
			DebugSpewAlwaysFile("CachedCeilingLocationX: %d", (int32_t)p_spawn->CachedCeilingLocationX);
			DebugSpewAlwaysFile("CachedCeilingLocationZ: %d", (int32_t)p_spawn->CachedCeilingLocationZ);
			DebugSpewAlwaysFile("CachedCeilingHeight: %d", (int32_t)p_spawn->CachedCeilingHeight);
			//DebugSpewAlwaysFilStaticCollisio: e("%d", (int32_t)p_spawn->StaticCollision);
			//DebugSpewAlwaysFilmPhysicsEffect: e("%d", (int32_t)p_spawn->mPhysicsEffects);
			//DebugSpewAlwaysFilPhysicsEffectsUpdate: e("%d", (int32_t)p_spawn->PhysicsEffectsUpdated);
			DebugSpewAlwaysFile("Animation: %d", (int32_t)p_spawn->Animation);
			DebugSpewAlwaysFile("NextAnim: %d", (int32_t)p_spawn->NextAnim);
			DebugSpewAlwaysFile("CurrLowerBodyAnim: %d", (int32_t)p_spawn->CurrLowerBodyAnim);
			DebugSpewAlwaysFile("NextLowerBodyAnim: %d", (int32_t)p_spawn->NextLowerBodyAnim);
			DebugSpewAlwaysFile("CurrLowerAnimVariation: %d", (int32_t)p_spawn->CurrLowerAnimVariation);
			DebugSpewAlwaysFile("CurrAnimVariation: %d", (int32_t)p_spawn->CurrAnimVariation);
			DebugSpewAlwaysFile("CurrAnimRndVariation: %d", (int32_t)p_spawn->CurrAnimRndVariation);
			DebugSpewAlwaysFile("Loop3d_SoundID: %d", (int32_t)p_spawn->Loop3d_SoundID);
			DebugSpewAlwaysFile("Step_SoundID: %d", (int32_t)p_spawn->Step_SoundID);
			DebugSpewAlwaysFile("CurLoop_SoundID: %d", (int32_t)p_spawn->CurLoop_SoundID);
			DebugSpewAlwaysFile("Idle3d1_SoundID: %d", (int32_t)p_spawn->Idle3d1_SoundID);
			DebugSpewAlwaysFile("Idle3d2_SoundID: %d", (int32_t)p_spawn->Idle3d2_SoundID);
			DebugSpewAlwaysFile("Jump_SoundID: %d", (int32_t)p_spawn->Jump_SoundID);
			DebugSpewAlwaysFile("Hit1_SoundID: %d", (int32_t)p_spawn->Hit1_SoundID);
			DebugSpewAlwaysFile("Hit2_SoundID: %d", (int32_t)p_spawn->Hit2_SoundID);
			DebugSpewAlwaysFile("Hit3_SoundID: %d", (int32_t)p_spawn->Hit3_SoundID);
			DebugSpewAlwaysFile("Hit4_SoundID: %d", (int32_t)p_spawn->Hit4_SoundID);
			DebugSpewAlwaysFile("Gasp1_SoundID: %d", (int32_t)p_spawn->Gasp1_SoundID);
			DebugSpewAlwaysFile("Gasp2_SoundID: %d", (int32_t)p_spawn->Gasp2_SoundID);
			DebugSpewAlwaysFile("Drown_SoundID: %d", (int32_t)p_spawn->Drown_SoundID);
			DebugSpewAlwaysFile("Death_SoundID: %d", (int32_t)p_spawn->Death_SoundID);
			DebugSpewAlwaysFile("Attk1_SoundID: %d", (int32_t)p_spawn->Attk1_SoundID);
			DebugSpewAlwaysFile("Attk2_SoundID: %d", (int32_t)p_spawn->Attk2_SoundID);
			DebugSpewAlwaysFile("Attk3_SoundID: %d", (int32_t)p_spawn->Attk3_SoundID);
			DebugSpewAlwaysFile("Walk_SoundID: %d", (int32_t)p_spawn->Walk_SoundID);
			DebugSpewAlwaysFile("Run_SoundID: %d", (int32_t)p_spawn->Run_SoundID);
			DebugSpewAlwaysFile("Crouch_SoundID: %d", (int32_t)p_spawn->Crouch_SoundID);
			DebugSpewAlwaysFile("Swim_SoundID: %d", (int32_t)p_spawn->Swim_SoundID);
			DebugSpewAlwaysFile("TreadWater_SoundID: %d", (int32_t)p_spawn->TreadWater_SoundID);
			DebugSpewAlwaysFile("Climb_SoundID: %d", (int32_t)p_spawn->Climb_SoundID);
			DebugSpewAlwaysFile("Sit_SoundID: %d", (int32_t)p_spawn->Sit_SoundID);
			DebugSpewAlwaysFile("Kick_SoundID: %d", (int32_t)p_spawn->Kick_SoundID);
			DebugSpewAlwaysFile("Bash_SoundID: %d", (int32_t)p_spawn->Bash_SoundID);
			DebugSpewAlwaysFile("FireBow_SoundID: %d", (int32_t)p_spawn->FireBow_SoundID);
			DebugSpewAlwaysFile("MonkAttack1_SoundID: %d", (int32_t)p_spawn->MonkAttack1_SoundID);
			DebugSpewAlwaysFile("MonkAttack2_SoundID: %d", (int32_t)p_spawn->MonkAttack2_SoundID);
			DebugSpewAlwaysFile("MonkSpecial_SoundID: %d", (int32_t)p_spawn->MonkSpecial_SoundID);
			DebugSpewAlwaysFile("PrimaryBlunt_SoundID: %d", (int32_t)p_spawn->PrimaryBlunt_SoundID);
			DebugSpewAlwaysFile("PrimarySlash_SoundID: %d", (int32_t)p_spawn->PrimarySlash_SoundID);
			DebugSpewAlwaysFile("PrimaryStab_SoundID: %d", (int32_t)p_spawn->PrimaryStab_SoundID);
			DebugSpewAlwaysFile("Punch_SoundID: %d", (int32_t)p_spawn->Punch_SoundID);
			DebugSpewAlwaysFile("Roundhouse_SoundID: %d", (int32_t)p_spawn->Roundhouse_SoundID);
			DebugSpewAlwaysFile("SecondaryBlunt_SoundID: %d", (int32_t)p_spawn->SecondaryBlunt_SoundID);
			DebugSpewAlwaysFile("SecondarySlash_SoundID: %d", (int32_t)p_spawn->SecondarySlash_SoundID);
			DebugSpewAlwaysFile("SecondaryStab_SoundID: %d", (int32_t)p_spawn->SecondaryStab_SoundID);
			DebugSpewAlwaysFile("SwimAttack_SoundID: %d", (int32_t)p_spawn->SwimAttack_SoundID);
			DebugSpewAlwaysFile("TwoHandedBlunt_SoundID: %d", (int32_t)p_spawn->TwoHandedBlunt_SoundID);
			DebugSpewAlwaysFile("TwoHandedSlash_SoundID: %d", (int32_t)p_spawn->TwoHandedSlash_SoundID);
			DebugSpewAlwaysFile("TwoHandedStab_SoundID: %d", (int32_t)p_spawn->TwoHandedStab_SoundID);
			DebugSpewAlwaysFile("SecondaryPunch_SoundID: %d", (int32_t)p_spawn->SecondaryPunch_SoundID);
			DebugSpewAlwaysFile("JumpAcross_SoundID: %d", (int32_t)p_spawn->JumpAcross_SoundID);
			DebugSpewAlwaysFile("WalkBackwards_SoundID: %d", (int32_t)p_spawn->WalkBackwards_SoundID);
			DebugSpewAlwaysFile("CrouchWalk_SoundID: %d", (int32_t)p_spawn->CrouchWalk_SoundID);
			DebugSpewAlwaysFile("LastWalkTime: %d", (int32_t)p_spawn->LastWalkTime);
			DebugSpewAlwaysFile("RightHolding: %d", (int32_t)p_spawn->RightHolding);
			DebugSpewAlwaysFile("LeftHolding: %d", (int32_t)p_spawn->LeftHolding);
			DebugSpewAlwaysFile("LastBubblesTime: %d", (int32_t)p_spawn->LastBubblesTime);
			DebugSpewAlwaysFile("LastColdBreathTime: %d", (int32_t)p_spawn->LastColdBreathTime);
			DebugSpewAlwaysFile("LastParticleUpdateTime: %d", (int32_t)p_spawn->LastParticleUpdateTime);
			DebugSpewAlwaysFile("MercID: %d", (int32_t)p_spawn->MercID);
			DebugSpewAlwaysFile("ContractorID: %d", (int32_t)p_spawn->ContractorID);
			DebugSpewAlwaysFile("CeilingHeightAtCurrLocation: %d", (int32_t)p_spawn->CeilingHeightAtCurrLocation);
			DebugSpewAlwaysFile("MobileEmitter: %d", (int32_t)p_spawn->MobileEmitter);
			DebugSpewAlwaysFile("bInstantHPGaugeChange: %d", (int32_t)p_spawn->bInstantHPGaugeChange);
			DebugSpewAlwaysFile("LastUpdateReceivedTime: %d", (int32_t)p_spawn->LastUpdateReceivedTime);
			DebugSpewAlwaysFile("MaxSpeakDistance: %d", (int32_t)p_spawn->MaxSpeakDistance);
			DebugSpewAlwaysFile("WalkSpeed: %d", (int32_t)p_spawn->WalkSpeed);
			DebugSpewAlwaysFile("Unknown0x11cc: %d", (int32_t)p_spawn->Unknown0x11cc);
			DebugSpewAlwaysFile("AssistName: %d", (int32_t)p_spawn->AssistName);
			DebugSpewAlwaysFile("InvitedToGroup: %d", (int32_t)p_spawn->InvitedToGroup);
			DebugSpewAlwaysFile("Unknown0x1211: %d", (int32_t)p_spawn->Unknown0x1211);
			DebugSpewAlwaysFile("GroupMemberTargeted: %d", (int32_t)p_spawn->GroupMemberTargeted);
			DebugSpewAlwaysFile("bRemovalPending: %d", (int32_t)p_spawn->bRemovalPending);
			DebugSpewAlwaysFile("pCorpse: %d", (int32_t)p_spawn->pCorpse);
			DebugSpewAlwaysFile("EmitterScalingRadius: %d", (int32_t)p_spawn->EmitterScalingRadius);
			DebugSpewAlwaysFile("DefaultEmitterID: %d", (int32_t)p_spawn->DefaultEmitterID);
			DebugSpewAlwaysFile("bDisplayNameSprite: %d", (int32_t)p_spawn->bDisplayNameSprite);
			DebugSpewAlwaysFile("bIdleAnimationOff: %d", (int32_t)p_spawn->bIdleAnimationOff);
			DebugSpewAlwaysFile("bIsInteractiveObject: %d", (int32_t)p_spawn->bIsInteractiveObject);
			DebugSpewAlwaysFile("InteractiveObjectModelName: %d", (int32_t)p_spawn->InteractiveObjectModelName);
			DebugSpewAlwaysFile("InteractiveObjectOtherName: %d", (int32_t)p_spawn->InteractiveObjectOtherName);
			DebugSpewAlwaysFile("InteractiveObjectName: %d", (int32_t)p_spawn->InteractiveObjectName);
			//DebugSpewAlwaysFilPhysicsBeforeLastPor: e("%d", (int32_t)p_spawn->PhysicsBeforeLastPort);
			//DebugSpewAlwaysFilFellowshi: e("%d", (int32_t)p_spawn->Fellowship);
			DebugSpewAlwaysFile("CampfireY: %d", (int32_t)p_spawn->CampfireY);
			DebugSpewAlwaysFile("CampfireX: %d", (int32_t)p_spawn->CampfireX);
			DebugSpewAlwaysFile("CampfireZ: %d", (int32_t)p_spawn->CampfireZ);
			DebugSpewAlwaysFile("CampfireZoneID: %d", (int32_t)p_spawn->CampfireZoneID);
			DebugSpewAlwaysFile("CampfireTimestamp: %d", (int32_t)p_spawn->CampfireTimestamp);
			DebugSpewAlwaysFile("FellowShipID: %d", (int32_t)p_spawn->FellowShipID);
			DebugSpewAlwaysFile("CampType: %d", (int32_t)p_spawn->CampType);
			DebugSpewAlwaysFile("Campfire: %d", (int32_t)p_spawn->Campfire);
			//DebugSpewAlwaysFilEquipmen: e("%d", (int32_t)p_spawn->Equipment);
			DebugSpewAlwaysFile("bIsPlacingItem: %d", (int32_t)p_spawn->bIsPlacingItem);
			DebugSpewAlwaysFile("bGMCreatedNPC: %d", (int32_t)p_spawn->bGMCreatedNPC);
			DebugSpewAlwaysFile("ObjectAnimationID: %d", (int32_t)p_spawn->ObjectAnimationID);
			DebugSpewAlwaysFile("bInteractiveObjectCollidable: %d", (int32_t)p_spawn->bInteractiveObjectCollidable);
			DebugSpewAlwaysFile("InteractiveObjectType: %d", (int32_t)p_spawn->InteractiveObjectType);
			DebugSpewAlwaysFile("SoundIDs: %d", (int32_t)p_spawn->SoundIDs);
			DebugSpewAlwaysFile("LastHistorySentTime: %d", (int32_t)p_spawn->LastHistorySentTime);
			//DebugSpewAlwaysFilBardTwistSpell: e("%d", (int32_t)p_spawn->BardTwistSpells);
			DebugSpewAlwaysFile("CurrentBardTwistIndex: %d", (int32_t)p_spawn->CurrentBardTwistIndex);
			//DebugSpewAlwaysFilmPlayerPhysicsClien: e("%d", (int32_t)p_spawn->mPlayerPhysicsClient);
			DebugSpewAlwaysFile("]: %d", (int32_t)p_spawn->SpawnStatus[6]);
			DebugSpewAlwaysFile("BannerIndex0: %d", (int32_t)p_spawn->BannerIndex0);
			DebugSpewAlwaysFile("BannerIndex1: %d", (int32_t)p_spawn->BannerIndex1);
			//DebugSpewAlwaysFilBannerTint: e("%d", (int32_t)p_spawn->BannerTint0);
			//DebugSpewAlwaysFilBannerTint: e("%d", (int32_t)p_spawn->BannerTint1);
			DebugSpewAlwaysFile("MountAnimationRelated: %d", (int32_t)p_spawn->MountAnimationRelated);
			DebugSpewAlwaysFile("bGuildShowAnim: %d", (int32_t)p_spawn->bGuildShowAnim);
			DebugSpewAlwaysFile("bWaitingForPort: %d", (int32_t)p_spawn->bWaitingForPort);
			DebugSpewAlwaysFile("IsPassenger: %d", (int32_t)p_spawn->IsPassenger);
			DebugSpewAlwaysFile("ViewHeight: %d", (int32_t)p_spawn->ViewHeight);
			DebugSpewAlwaysFile("bOfflineMode: %d", (int32_t)p_spawn->bOfflineMode);
			DebugSpewAlwaysFile("RealEstateID: %d", (int32_t)p_spawn->RealEstateID);
			DebugSpewAlwaysFile("TitleVisible: %d", (int32_t)p_spawn->TitleVisible);
			DebugSpewAlwaysFile("bStationary: %d", (int32_t)p_spawn->bStationary);
			DebugSpewAlwaysFile("CharClass: %d", (int32_t)p_spawn->CharClass);
			DebugSpewAlwaysFile("EnduranceCurrent: %d", (int32_t)p_spawn->EnduranceCurrent);
			DebugSpewAlwaysFile("MinuteTimer: %d", (int32_t)p_spawn->MinuteTimer);
			DebugSpewAlwaysFile("LoginRelated: %d", (int32_t)p_spawn->LoginRelated);
			DebugSpewAlwaysFile("Mercenary: %d", (int32_t)p_spawn->Mercenary);
			DebugSpewAlwaysFile("FallingStartZ: %d", (int32_t)p_spawn->FallingStartZ);
			DebugSpewAlwaysFile("Zone: %d", (int32_t)p_spawn->Zone);
			DebugSpewAlwaysFile("Light: %d", (int32_t)p_spawn->Light);
			DebugSpewAlwaysFile("LastSecondaryUseTime: %d", (int32_t)p_spawn->LastSecondaryUseTime);
			DebugSpewAlwaysFile("IsAttacking: %d", (int32_t)p_spawn->IsAttacking);
			DebugSpewAlwaysFile("BearingToTarget: %d", (int32_t)p_spawn->BearingToTarget);
			DebugSpewAlwaysFile("GMRank: %d", (int32_t)p_spawn->GMRank);
			DebugSpewAlwaysFile("AnimationSpeedRelated: %d", (int32_t)p_spawn->AnimationSpeedRelated);
			DebugSpewAlwaysFile("Type2: %d", (int32_t)p_spawn->Type2);
			DebugSpewAlwaysFile("bBuffTimersOnHold: %d", (int32_t)p_spawn->bBuffTimersOnHold);
			DebugSpewAlwaysFile("LastRangedUsedTime: %d", (int32_t)p_spawn->LastRangedUsedTime);
			DebugSpewAlwaysFile("pRaceGenderInfo: %d", (int32_t)p_spawn->pRaceGenderInfo);
			DebugSpewAlwaysFile("LastAttack: %d", (int32_t)p_spawn->LastAttack);
			DebugSpewAlwaysFile("LastPrimaryUseTime: %d", (int32_t)p_spawn->LastPrimaryUseTime);
			DebugSpewAlwaysFile("CurrIOState: %d", (int32_t)p_spawn->CurrIOState);
			DebugSpewAlwaysFile("LastRefresh: %d", (int32_t)p_spawn->LastRefresh);
			DebugSpewAlwaysFile("GuildStatus: %d", (int32_t)p_spawn->GuildStatus);
			DebugSpewAlwaysFile("LastTick: %d", (int32_t)p_spawn->LastTick);
			DebugSpewAlwaysFile("Level: %d", (int32_t)p_spawn->Level);
			DebugSpewAlwaysFile("bAttackRelated: %d", (int32_t)p_spawn->bAttackRelated);
			DebugSpewAlwaysFile("LoginSerial: %d", (int32_t)p_spawn->LoginSerial);
			DebugSpewAlwaysFile("SomethingElse: %d", (int32_t)p_spawn->SomethingElse);
			DebugSpewAlwaysFile("GM: %d", (int32_t)p_spawn->GM);
			DebugSpewAlwaysFile("FishingETA: %d", (int32_t)p_spawn->FishingETA);
			DebugSpewAlwaysFile("Sneak: %d", (int32_t)p_spawn->Sneak);
			DebugSpewAlwaysFile("bAnimationOnPop: %d", (int32_t)p_spawn->bAnimationOnPop);
			DebugSpewAlwaysFile("LastResendAddPlayerPacket: %d", (int32_t)p_spawn->LastResendAddPlayerPacket);
			DebugSpewAlwaysFile("pViewPlayer: %d", (int32_t)p_spawn->pViewPlayer);
			DebugSpewAlwaysFile("ACounter: %d", (int32_t)p_spawn->ACounter);
			DebugSpewAlwaysFile("CameraOffset: %d", (int32_t)p_spawn->CameraOffset);
			DebugSpewAlwaysFile("Meditating: %d", (int32_t)p_spawn->Meditating);
			DebugSpewAlwaysFile("FindBits: %d", (int32_t)p_spawn->FindBits);
			DebugSpewAlwaysFile("SecondaryTintIndex: %d", (int32_t)p_spawn->SecondaryTintIndex);
			//DebugSpewAlwaysFilLastCollisio: e("%d", (int32_t)p_spawn->LastCollision);
			DebugSpewAlwaysFile("PetID: %d", (int32_t)p_spawn->PetID);
			DebugSpewAlwaysFile("Anon: %d", (int32_t)p_spawn->Anon);
			DebugSpewAlwaysFile("RespawnTimer: %d", (int32_t)p_spawn->RespawnTimer);
			DebugSpewAlwaysFile("LastCastNum: %d", (int32_t)p_spawn->LastCastNum);
			DebugSpewAlwaysFile("AARank: %d", (int32_t)p_spawn->AARank);
			DebugSpewAlwaysFile("LastCastTime: %d", (int32_t)p_spawn->LastCastTime);
			//DebugSpewAlwaysFileqc_inf: e("%d", (int32_t)p_spawn->eqc_info);
			DebugSpewAlwaysFile("MerchantGreed: %d", (int32_t)p_spawn->MerchantGreed);
			DebugSpewAlwaysFile("pTouchingSwitch: %d", (int32_t)p_spawn->pTouchingSwitch);
			DebugSpewAlwaysFile("TimeStamp: %d", (int32_t)p_spawn->TimeStamp);
			DebugSpewAlwaysFile("HPMax: %d", (int32_t)p_spawn->HPMax);
			DebugSpewAlwaysFile("bSummoned: %d", (int32_t)p_spawn->bSummoned);
			DebugSpewAlwaysFile("InPvPArea: %d", (int32_t)p_spawn->InPvPArea);
			DebugSpewAlwaysFile("HPCurrent: %d", (int32_t)p_spawn->HPCurrent);
			DebugSpewAlwaysFile("SpellGemETA: %d", (int32_t)p_spawn->SpellGemETA);
			DebugSpewAlwaysFile("GetMeleeRangeVar1: %d", (int32_t)p_spawn->GetMeleeRangeVar1);
			DebugSpewAlwaysFile("Trader: %d", (int32_t)p_spawn->Trader);
			DebugSpewAlwaysFile("SomeData: %d", (int32_t)p_spawn->SomeData);
			DebugSpewAlwaysFile("bSwitchMoved: %d", (int32_t)p_spawn->bSwitchMoved);
			DebugSpewAlwaysFile("MyWalkSpeed: %d", (int32_t)p_spawn->MyWalkSpeed);
			DebugSpewAlwaysFile("HoldingAnimation: %d", (int32_t)p_spawn->HoldingAnimation);
			DebugSpewAlwaysFile("HideMode: %d", (int32_t)p_spawn->HideMode);
			DebugSpewAlwaysFile("Stuff: %d", (int32_t)p_spawn->Stuff);
			DebugSpewAlwaysFile("AltAttack: %d", (int32_t)p_spawn->AltAttack);
			DebugSpewAlwaysFile("PotionTimer: %d", (int32_t)p_spawn->PotionTimer);
			DebugSpewAlwaysFile("HmmWhat: %d", (int32_t)p_spawn->HmmWhat);
			DebugSpewAlwaysFile("PvPFlag: %d", (int32_t)p_spawn->PvPFlag);
			DebugSpewAlwaysFile("GuildID: %d", (int32_t)p_spawn->GuildID);
			DebugSpewAlwaysFile("RunSpeed: %d", (int32_t)p_spawn->RunSpeed);
			DebugSpewAlwaysFile("ManaMax: %d", (int32_t)p_spawn->ManaMax);
			DebugSpewAlwaysFile("LastMealTime: %d", (int32_t)p_spawn->LastMealTime);
			DebugSpewAlwaysFile("StandState: %d", (int32_t)p_spawn->StandState);
			DebugSpewAlwaysFile("LastTimeStoodStill: %d", (int32_t)p_spawn->LastTimeStoodStill);
			DebugSpewAlwaysFile("DoSpecialMelee: %d", (int32_t)p_spawn->DoSpecialMelee);
			DebugSpewAlwaysFile("CombatSkillTicks: %d", (int32_t)p_spawn->CombatSkillTicks);
			DebugSpewAlwaysFile("MissileRangeToTarget: %d", (int32_t)p_spawn->MissileRangeToTarget);
			DebugSpewAlwaysFile("SitStartTime: %d", (int32_t)p_spawn->SitStartTime);
			//DebugSpewAlwaysFilrealEstateItemGui: e("%d", (int32_t)p_spawn->realEstateItemGuid);
			DebugSpewAlwaysFile("bShowHelm: %d", (int32_t)p_spawn->bShowHelm);
			DebugSpewAlwaysFile("MasterID: %d", (int32_t)p_spawn->MasterID);
			DebugSpewAlwaysFile("CombatSkillUsed: %d", (int32_t)p_spawn->CombatSkillUsed);
			DebugSpewAlwaysFile("Handle: %d", (int32_t)p_spawn->Handle);
			DebugSpewAlwaysFile("FD: %d", (int32_t)p_spawn->FD);
			DebugSpewAlwaysFile("PrimaryTintIndex: %d", (int32_t)p_spawn->PrimaryTintIndex);
			DebugSpewAlwaysFile("AFK: %d", (int32_t)p_spawn->AFK);
			DebugSpewAlwaysFile("EnduranceMax: %d", (int32_t)p_spawn->EnduranceMax);
			DebugSpewAlwaysFile("NextIntimidateTime: %d", (int32_t)p_spawn->NextIntimidateTime);
			DebugSpewAlwaysFile("SpellCooldownETA: %d", (int32_t)p_spawn->SpellCooldownETA);
			DebugSpewAlwaysFile("Linkdead: %d", (int32_t)p_spawn->Linkdead);
			DebugSpewAlwaysFile("Buyer: %d", (int32_t)p_spawn->Buyer);
			DebugSpewAlwaysFile("LastTrapDamageTime: %d", (int32_t)p_spawn->LastTrapDamageTime);
			DebugSpewAlwaysFile("bTempPet: %d", (int32_t)p_spawn->bTempPet);
			DebugSpewAlwaysFile("Title: %d", (int32_t)p_spawn->Title);
			DebugSpewAlwaysFile("FishingEvent: %d", (int32_t)p_spawn->FishingEvent);
			DebugSpewAlwaysFile("Suffix: %d", (int32_t)p_spawn->Suffix);
			DebugSpewAlwaysFile("RealEstateItemId: %d", (int32_t)p_spawn->RealEstateItemId);
			DebugSpewAlwaysFile("WarCry: %d", (int32_t)p_spawn->WarCry);
			DebugSpewAlwaysFile("Blind: %d", (int32_t)p_spawn->Blind);
			DebugSpewAlwaysFile("bBetaBuffed: %d", (int32_t)p_spawn->bBetaBuffed);
			DebugSpewAlwaysFile("NextSwim: %d", (int32_t)p_spawn->NextSwim);
			DebugSpewAlwaysFile("CorpseDragCount: %d", (int32_t)p_spawn->CorpseDragCount);
			DebugSpewAlwaysFile("IntimidateCount: %d", (int32_t)p_spawn->IntimidateCount);
			DebugSpewAlwaysFile("berserker: %d", (int32_t)p_spawn->berserker);
			DebugSpewAlwaysFile("StunTimer: %d", (int32_t)p_spawn->StunTimer);
			DebugSpewAlwaysFile("LFG: %d", (int32_t)p_spawn->LFG);
			//DebugSpewAlwaysFilCastingDat: e("%d", (int32_t)p_spawn->CastingData);
			DebugSpewAlwaysFile("ManaCurrent: %d", (int32_t)p_spawn->ManaCurrent);
			DebugSpewAlwaysFile("DragNames: %d", (int32_t)p_spawn->DragNames);
			DebugSpewAlwaysFile("Deity: %d", (int32_t)p_spawn->Deity);
			DebugSpewAlwaysFile("NpcTintIndex: %d", (int32_t)p_spawn->NpcTintIndex);
			DebugSpewAlwaysFile("ppUDP: %d", (int32_t)p_spawn->ppUDP);
			DebugSpewAlwaysFile("bAlwaysShowAura: %d", (int32_t)p_spawn->bAlwaysShowAura);
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

bool MQ2Stats::GetMember(MQ2VARPTR p_var, PCHAR Member, PCHAR index, MQ2TYPEVAR& Dest)
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
			Split(std::string(index), args, ',');
			if(args.size() > 0)
			{
				range = std::stoi(args[0]);
			}

			GetAggroed(&DataTypeTemp[0], MAX_STRING, range);

			Dest.Type = pStringType;
			Dest.Ptr  = &DataTypeTemp[0];
			return true;
		}
		case CorpseList:
		{
			DWORD range     = 100;
			CorpseType type = NPC;
			std::vector<std::string> args;
			Split(std::string(index), args, ',');
			if(args.size() == 1)
			{
				range = std::stoi(args[0]);
			}
			else if(args.size() == 2)
			{
				range = std::stoi(args[0]);
				type  = static_cast<CorpseType>(std::stoi(args[1]));
			}

			GetCorpses(&DataTypeTemp[0], MAX_STRING, range, type);

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

void CommandChase(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandChase(p_line);
	}
}

void CommandDrag(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandDrag(p_line);
	}
}

void CommandDumpSpawn(PSPAWNINFO p_char, char* p_line)
{
	if(nullptr != p_stats)
	{
		p_stats->CommandDumpSpawn(p_line);
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

	AddCommand("/chase", CommandChase);
	AddCommand("/drag", CommandDrag);
	AddCommand("/dumpspawn", CommandDumpSpawn);
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
	RemoveCommand("/chase");

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

PLUGIN_API VOID OnPulse(VOID)
{
	if(nullptr != p_stats)
	{
		p_stats->Pulse();
	}
}

// This is called when we receive the EQ_BEGIN_ZONE packet is received
PLUGIN_API VOID OnBeginZone(VOID)
{
	if(nullptr != p_stats)
	{
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

//PLUGIN_API VOID OnNetBotEVENT(PCHAR p_char)
//{
//	if(nullptr != p_stats)
//	{
//		WriteChatf("Received event %s", p_char);
//
//		if(!strncmp(p_char, "NBQUIT=", 7))
//		{
//		}
//		else if(!strncmp(p_char, "NBJOIN=", 7))
//		{
//			WriteChatf("Received request");
//			p_stats->PublishRequest();
//		}
//		else if(!strncmp(p_char, "NBEXIT", 6))
//		{
//		}
//	}
//}
//
//PLUGIN_API VOID OnNetBotMSG(PCHAR name, PCHAR message)
//{
//	if(nullptr != p_stats)
//	{
//		p_stats->ParseMessage(name, message);
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
