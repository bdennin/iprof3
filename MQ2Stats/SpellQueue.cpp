
#include "SpellQueue.h"

SpellQueue::SpellQueue()
	: m_p_cast(nullptr)
	, m_cast_x(0.0)
	, m_cast_y(0.0)
	, m_cast_z(0.0)
	, m_is_casting(false)
	, m_queued()
	, m_types()
	, m_spells()
{
	InitTypes();
	InitSpells();

	ParseSpellBook();
}

SpellQueue::~SpellQueue()
{
	while(!m_queued.empty())
	{
		Pop();
	}
}

SpellQueue::SpellData* SpellQueue::Front()
{
	return m_queued.size() ? m_queued.begin()->second : nullptr;
}

void SpellQueue::Push(std::string& type_name, std::string& target_name)
{
	SpellType type = ToType(type_name);
	PSPAWNINFO p_target = (PSPAWNINFO)GetSpawnByName(const_cast<char*>(target_name.c_str()));

	Push(type, p_target);
}

void SpellQueue::Push(SpellType& type, PSPAWNINFO p_target)
{
	PSPELL p_spell = ToSpell(type);

	if(nullptr != p_spell && nullptr != p_target && !IsQueued(type, p_target))
	{
		SpellData* spell_data = new SpellData();

		spell_data->type = type;
		spell_data->p_spell = p_spell;
		spell_data->p_target = p_target;
		spell_data->time_ms = std::clock();

		m_queued.insert(std::pair<SpellType, SpellData*>(type, spell_data));
	}
}

void SpellQueue::Pop()
{
	if(!m_queued.empty())
	{
		SpellQueue::SpellData* p_start = m_queued.begin()->second;
		m_queued.erase(m_queued.begin());
		delete p_start;
	}
}

void SpellQueue::Remove(std::string& type_name, std::string& target_name)
{
	SpellType type = ToType(type_name);
	PSPAWNINFO p_target = (PSPAWNINFO)GetSpawnByName(const_cast<char*>(target_name.c_str()));

	Remove(type, p_target);
}

void SpellQueue::Remove(SpellType& type, PSPAWNINFO p_target)
{
	typedef std::multimap<SpellType, SpellData*>::iterator map_it;

	std::pair<map_it, map_it> range = m_queued.equal_range(type);

	for(map_it it = range.first; it != range.second; it++)
	{
		SpellData* p_data = it->second;

		if(nullptr != p_data && p_target == p_data->p_target)
		{
			m_queued.erase(it);
			delete p_data;
		}
	}
}

void SpellQueue::InitTypes()
{
	// Translates word send over network to enum type
	m_types["feign"] = SpellType::Feign;
	m_types["evacuate"] = SpellType::Evacuate;
	m_types["charm"] = SpellType::Charm;
	m_types["slow"] = SpellType::Slow;
	m_types["healgroup"] = SpellType::HealGroup;
	m_types["heal"] = SpellType::Heal;
	m_types["hot"] = SpellType::HoT;
	m_types["resurrect"] = SpellType::Resurrect;
	m_types["curedisease"] = SpellType::CureDisease;
	m_types["curepoison"] = SpellType::CurePoison;
	m_types["curecurse"] = SpellType::CureCurse;
	m_types["cripple"] = SpellType::Cripple;
	m_types["summonpet"] = SpellType::PetSummon;
	m_types["movement"] = SpellType::Movement;
	m_types["clarity"] = SpellType::Clarity;
	m_types["cannibalize"] = SpellType::Cannibalize;
	m_types["haste"] = SpellType::Haste;
	m_types["aegolism"] = SpellType::Aegolism;
	m_types["statistic"] = SpellType::Statistic;
	m_types["dot"] = SpellType::DoT;
	m_types["nuke"] = SpellType::Nuke;
	m_types["summoncorpse"] = SpellType::SummonCorpse;
	m_types["calm"] = SpellType::Calm;
	m_types["invisibility"] = SpellType::Invisibility;
	m_types["enduringbreath"] = SpellType::EnduringBreath;
	m_types["bind"] = SpellType::Bind;
	m_types["illusion"] = SpellType::Illusion;
	m_types["alliance"] = SpellType::Alliance;
	m_types["unknown"] = SpellType::Unknown;
}

void SpellQueue::InitSpells()
{
	// Actual spells that these names point to
	m_spells[SpellType::Feign] = nullptr;
	m_spells[SpellType::Evacuate] = nullptr;
	m_spells[SpellType::Charm] = nullptr;
	m_spells[SpellType::Slow] = nullptr;
	m_spells[SpellType::HealGroup] = nullptr;
	m_spells[SpellType::Heal] = nullptr;
	m_spells[SpellType::HoT] = nullptr;
	m_spells[SpellType::Resurrect] = nullptr;
	m_spells[SpellType::CureDisease] = nullptr;
	m_spells[SpellType::CurePoison] = nullptr;
	m_spells[SpellType::CureCurse] = nullptr;
	m_spells[SpellType::Cripple] = nullptr;
	m_spells[SpellType::PetSummon] = nullptr;
	m_spells[SpellType::Movement] = nullptr;
	m_spells[SpellType::Clarity] = nullptr;
	m_spells[SpellType::Cannibalize] = nullptr;
	m_spells[SpellType::Haste] = nullptr;
	m_spells[SpellType::Aegolism] = nullptr;
	m_spells[SpellType::Statistic] = nullptr;
	m_spells[SpellType::DoT] = nullptr;
	m_spells[SpellType::Nuke] = nullptr;
	m_spells[SpellType::SummonCorpse] = nullptr;
	m_spells[SpellType::Calm] = nullptr;
	m_spells[SpellType::Invisibility] = nullptr;
	m_spells[SpellType::EnduringBreath] = nullptr;
	m_spells[SpellType::Bind] = nullptr;
	m_spells[SpellType::Illusion] = nullptr;
	m_spells[SpellType::Alliance] = nullptr;
	m_spells[SpellType::Unknown] = nullptr;
}

void SpellQueue::ParseSpellBook()
{
	PCHARINFO2 p_char = GetCharInfo2();
	PSPELL p_curr = nullptr;
	DWORD spell_index = 0;

	if(nullptr != p_char)
	{
		for(DWORD i = 0; i < NUM_BOOK_SLOTS; i++)
		{
			spell_index = p_char->SpellBook[i];
			if(spell_index != 0xFFFFFFFF) // Empty slot
			{
				if(p_curr = GetSpellByID(spell_index))
				{
					AddSpell(p_curr);
				}
			}
		}
	}
}

void SpellQueue::AddSpell(PSPELL p_spell)
{
	SpellCategory category = static_cast<SpellCategory>(p_spell->Category);
	SpellSubCategory subcategory = static_cast<SpellSubCategory>(p_spell->Subcategory);

	switch(category)
	{
	case SpellCategory::Auras:
	{
		switch(subcategory)
		{
		case SpellSubCategory::HasteSpellFocus:
		{
			break;
		}
		case SpellSubCategory::Mana:
		{
			break;
		}
		case SpellSubCategory::MeleeGuard:
		{
			break;
		}
		case SpellSubCategory::Regen:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::CreateItem:
	{
		switch(subcategory)
		{
		case SpellSubCategory::EnchantMetal:
		{
			break;
		}
		case SpellSubCategory::ImbueGem:
		{
			break;
		}
		case SpellSubCategory::Mana:
		{
			break;
		}
		case SpellSubCategory::Miscellaneous:
		{
			break;
		}
		case SpellSubCategory::SummonWeapon:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::DamageOverTime:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Disease:
		{
			break;
		}
		case SpellSubCategory::Fire:
		{
			break;
		}
		case SpellSubCategory::Magic:
		{
			break;
		}
		case SpellSubCategory::Poison:
		{
			break;
		}
		case SpellSubCategory::Snare:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::DirectDamage:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Chromatic:
		{
			break;
		}
		case SpellSubCategory::Cold:
		{
			break;
		}
		case SpellSubCategory::Fire:
		{
			break;
		}
		case SpellSubCategory::Magic:
		{
			break;
		}
		case SpellSubCategory::Physical:
		{
			break;
		}
		case SpellSubCategory::Poison:
		{
			break;
		}
		case SpellSubCategory::Stun:
		{
			break;
		}
		case SpellSubCategory::Summoned: // Only two that actually
		{
			break;
		}
		case SpellSubCategory::Undead:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Heals:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Cure:
		{
			break;
		}
		case SpellSubCategory::DurationHeals:
		{
			break;
		}
		case SpellSubCategory::Heals:
		{
			break;
		}
		case SpellSubCategory::ManaFlow:
		{
			break;
		}
		case SpellSubCategory::QuickHeal:
		{
			break;
		}
		case SpellSubCategory::Resurrection:
		{
			PSPELL& p_curr = m_spells[SpellQueue::SpellType::Resurrect];

			if(p_curr == nullptr || p_curr->ClassLevel > p_spell->ClassLevel) // Highest level resurrection is best
			{
				p_curr = p_spell;
				WriteChatf("  Assigned res spell %s", p_curr->Name);
			}

			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}

		break;
	}
	case SpellCategory::HPBuffs:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Aegolism:
		{
			break;
		}
		case SpellSubCategory::HPTypeOne:
		{
			break;
		}
		case SpellSubCategory::HPTypeTwo:
		{
			break;
		}
		case SpellSubCategory::Shielding:
		{
			break;
		}
		case SpellSubCategory::Symbol:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Pet:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Block:
		{
			break;
		}
		case SpellSubCategory::Heals:
		{
			break;
		}
		case SpellSubCategory::Miscellaneous:
		{
			break;
		}
		case SpellSubCategory::PetHaste:
		{
			break;
		}
		case SpellSubCategory::PetBuff:
		{
			break;
		}
		case SpellSubCategory::SummonAir:
		{
			break;
		}
		case SpellSubCategory::SummonAnimation:
		{
			break;
		}
		case SpellSubCategory::SummonFamiliar:
		{
			break;
		}
		case SpellSubCategory::SummonSwarm:
		{
			break;
		}
		case SpellSubCategory::SummonUndead:
		{
			break;
		}
		case SpellSubCategory::SummonWarder:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Regen:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Health:
		{
			break;
		}
		case SpellSubCategory::HealthMana:
		{
			break;
		}
		case SpellSubCategory::Mana:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::StatisticBuffs:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Agility:
		{
			break;
		}
		case SpellSubCategory::ArmorClass:
		{
			break;
		}
		case SpellSubCategory::Attack:
		{
			break;
		}
		case SpellSubCategory::Charisma:
		{
			break;
		}
		case SpellSubCategory::ResistBuff:
		{
			break;
		}
		case SpellSubCategory::Stamina:
		{
			break;
		}
		case SpellSubCategory::Strength:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Taps:
	{
		switch(subcategory)
		{
		case SpellSubCategory::CreateItem:
		{
			break;
		}
		case SpellSubCategory::DurationTap:
		{
			break;
		}
		case SpellSubCategory::Health:
		{
			break;
		}
		case SpellSubCategory::PowerTap:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Transport:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Antonica:
		{
			break;
		}
		case SpellSubCategory::Discord:
		{
			break;
		}
		case SpellSubCategory::Faydwer:
		{
			break;
		}
		case SpellSubCategory::Kunark:
		{
			break;
		}
		case SpellSubCategory::Luclin:
		{
			break;
		}
		case SpellSubCategory::Miscellaneous:
		{
			break;
		}
		case SpellSubCategory::Odus:
		{
			break;
		}
		case SpellSubCategory::SerpentSpine:
		{
			break;
		}
		case SpellSubCategory::Taelosia:
		{
			break;
		}
		case SpellSubCategory::ThePlanes:
		{
			break;
		}
		case SpellSubCategory::Velious:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Traps:
	{
		switch(subcategory)
		{
			//case SpellCategory::DamageOverTime:
			//{
			//	break;
			//}
		case SpellSubCategory::ResistDebuff:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::UtilityBeneficial:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Alliance:
		{
			break;
		}
		case SpellSubCategory::CombatInnates:
		{
			break;
		}
		case SpellSubCategory::Conversions:
		{
			break;
		}
		case SpellSubCategory::DamageShield:
		{
			break;
		}
		case SpellSubCategory::Haste:
		{
			break;
		}
		case SpellSubCategory::IllusionOther:
		{
			break;
		}
		case SpellSubCategory::IllusionSelf:
		{
			break;
		}
		case SpellSubCategory::Invisibility:
		{
			break;
		}
		case SpellSubCategory::Invulnerability:
		{
			break;
		}
		case SpellSubCategory::Levitate:
		{
			break;
		}
		case SpellSubCategory::MeleeGuard:
		{
			break;
		}
		case SpellSubCategory::Miscellaneous:
		{
			break;
		}
		case SpellSubCategory::Movement:
		{
			break;
		}
		case SpellSubCategory::Rune:
		{
			break;
		}
		case SpellSubCategory::ShadowStep:
		{
			break;
		}
		case SpellSubCategory::SpellFocus:
		{
			break;
		}
		case SpellSubCategory::Undead:
		{
			break;
		}
		case SpellSubCategory::Visage:
		{
			break;
		}
		case SpellSubCategory::Vision:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::UtilityDetrimental:
	{
		switch(subcategory)
		{
		case SpellSubCategory::Calm:
		{
			break;
		}
		case SpellSubCategory::Charm:
		{
			break;
		}
		case SpellSubCategory::Disempower:
		{
			break;
		}
		case SpellSubCategory::Dispel:
		{
			break;
		}
		case SpellSubCategory::Enthrall:
		{
			break;
		}
		case SpellSubCategory::Fear:
		{
			break;
		}
		case SpellSubCategory::Jolt:
		{
			break;
		}
		case SpellSubCategory::ManaDrain:
		{
			break;
		}
		case SpellSubCategory::ResistDebuff:
		{
			break;
		}
		case SpellSubCategory::Root:
		{
			break;
		}
		case SpellSubCategory::Slow:
		{
			break;
		}
		case SpellSubCategory::Snare:
		{
			break;
		}
		case SpellSubCategory::Undead:
		{
			break;
		}
		default:
			PrintNack(p_spell);
			break;
		}
		break;
	}
	case SpellCategory::Unknown:
	default:

		PrintNack(p_spell);
		break;
	}
}

void SpellQueue::PrintNack(PSPELL p_spell)
{
	char* cat = pCDBStr->GetString(p_spell->Category, 5, 0);
	char* subcat = pCDBStr->GetString(p_spell->Subcategory, 5, 0);

	WriteChatf("  Encountered unknown spell: %s (%d [ %s ] /%d [ %s ])", p_spell->Name, p_spell->Category, cat, p_spell->Subcategory, subcat);
}

SpellQueue::SpellType SpellQueue::ToType(std::string& type_name)
{
	auto it = m_types.find(type_name);

	if(it != m_types.end())
	{
		return it->second;
	}

	return SpellQueue::SpellType::Unknown;
}

PSPELL SpellQueue::ToSpell(SpellType& type)
{
	auto it = m_spells.find(type);

	if(it != m_spells.end())
	{
		return it->second;
	}

	return nullptr;
}

bool SpellQueue::IsQueued(SpellType& type, PSPAWNINFO p_target)
{
	typedef std::multimap<SpellType, SpellData*>::iterator map_it;

	std::pair<map_it, map_it> range = m_queued.equal_range(type);

	for(map_it it = range.first; it != range.second; it++)
	{
		SpellData* p_data = it->second;

		if(nullptr != p_data && p_data->p_target == p_target)
		{
			return true;
		}
	}

	return false;
}

SpellQueue::SpellFailure SpellQueue::IsCastable(PSPELL p_spell, PSPAWNINFO p_caster, PSPAWNINFO p_target)
{
	if(nullptr == p_spell)
	{
		return SpellFailure::UndefinedSpell;
	}

	if(nullptr == p_caster)
	{
		return SpellFailure::UndefinedSelf;
	}

	if(nullptr == p_target)
	{
		return SpellFailure::UndefinedTarget;
	}

	if(p_caster->Zone != p_target->Zone)
	{
		return SpellFailure::NotInZone;
	}

	if(p_caster->ManaCurrent < p_spell->ManaCost)
	{
		return SpellFailure::OutOfMana;
	}

	if(p_spell->Range > 0.0 && GetDistance(p_caster, p_target) > p_spell->Range)
	{
		return SpellFailure::OutOfRange;
	}

	return SpellFailure::None;
}

SpellQueue::SpellFailure SpellQueue::Cast()
{
	SpellFailure failure = SpellFailure::Unknown;

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

	if(nullptr != m_p_cast)
	{
		SpellData* p_data = Front();
		if(nullptr != p_data)
		{
			PSPELL p_spell = p_data->p_spell;
			PSPAWNINFO p_target = p_data->p_target;
			PSPAWNINFO p_self = GetCharInfo()->pSpawn;

			if(SpellFailure::None != (failure = Memorize(p_spell, p_self)))
			{
				return failure;
			}

			if(SpellFailure::None != (failure = IsCastable(p_spell, p_self, p_target)))
			{
				return failure;
			}

			m_cast_x = p_self->X;
			m_cast_y = p_self->Y;
			m_cast_z = p_self->Z;

			m_is_casting = true;

			m_p_cast(p_target, p_spell->Name);
		}
	}
}

SpellQueue::SpellFailure SpellQueue::Memorize(PSPELL p_spell, PSPAWNINFO p_caster)
{
	SPELLFAVORITE memorized;

	ZeroMemory(&memorized, sizeof(memorized));
	memorized.inuse = 1;

	// Check window open
	if(!ppSpellBookWnd || !pSpellBookWnd)
	{
		return SpellFailure::BookClosed;
	}

	// Check already memorized
	for(size_t i = 0; i < NUM_SPELL_GEMS; i++)
	{
		memorized.SpellId[i] = GetMemorizedSpell(i);

		if(p_spell->ID == memorized.SpellId[i])
		{
			return SpellFailure::None;
		}
	}

	// Check level requirement
	if(p_caster->Level < p_spell->ClassLevel[p_caster->mActorClient.Class])
	{
		return SpellFailure::LevelTooLow;
	}

	LONG gem_num = -1;

	// Get unused or shortest recast
	for(size_t i = 0; i < NUM_SPELL_GEMS && (gem_num == -1); i++)
	{
		if(memorized.SpellId[i] == 0xFFFFFFFF)
		{
			memorized.SpellId[i] = p_spell->ID;
			gem_num = i;
		}
	}
	
	// No gems are open -- need to drop one
	if(gem_num == -1)
	{
		// Find shortest recast gem

	}

	pSpellBookWnd->MemorizeSet((int*)&memorized, NUM_SPELL_GEMS);

	return SpellFailure::Memorizing;
}

//typedef struct _SPELL
//{
//	/*0x000*/ FLOAT Range;
//	/*0x004*/ FLOAT AERange;
//	/*0x008*/ FLOAT PushBack;
//	/*0x00c*/ FLOAT Unknown0x00c;
//	/*0x010*/ DWORD CastTime;
//	/*0x014*/ DWORD RecoveryTime;
//	/*0x018*/ DWORD RecastTime;
//	/*0x01c*/ DWORD DurationType; //DurationFormula on Lucy
//	/*0x020*/ DWORD DurationCap;
//	/*0x024*/ BYTE Unknown0x24[0x4];
//	/*0x028*/ DWORD ManaCost;
//	/*0x02c*/ LONG Base[0x0c];         //Base1-Base12
//	/*0x05c*/ LONG Base2[0x0c];        //SpellID of spell for added effects
//	/*0x08c*/ LONG Max[0xc];           //Max1-Max12
//	/*0x0bc*/ DWORD ReagentID[0x4];    //ReagentId1-ReagentId4d
//	/*0x0cc*/ DWORD ReagentCount[0x4]; //ReagentCount1-ReagentCount4
//	/*0x0dc*/ BYTE Unknown0xdc[0x10];
//	/*0x0ec*/ LONG Calc[0x0c];  //Calc1-Calc12
//	/*0x11c*/ LONG Attrib[0xc]; //Attrib1-Attrib12
//	/*0x14c*/ DWORD BookIcon;
//	/*0x150*/ DWORD GemIcon;
//	/*0x154*/ DWORD DescriptionIndex;
//	/*0x158*/ DWORD ResistAdj;
//	/*0x15c*/ DWORD Diety;
//	/*0x160*/ DWORD spaindex;
//	/*0x164*/ DWORD SpellAnim;
//	/*0x168*/ DWORD SpellIcon;
//	/*0x16c*/ DWORD DurationParticleEffect;
//	/*0x170*/ DWORD NPCUsefulness;
//	/*0x174*/ DWORD ID;
//	/*0x178*/ DWORD Autocast;    //SpellID of spell to instacast on caster when current spell lands on target
//	/*0x17c*/ DWORD Category;    //Unknown144 from lucy
//	/*0x180*/ DWORD Subcategory; //Subcat to Category. Unknown145 from lucy
//	/*0x184*/ DWORD Subcategory2;
//	/*0x188*/ DWORD HateMod; //Additional hate
//	/*0x18c*/ DWORD ResistPerLevel;
//	/*0x190*/ DWORD ResistCap;
//	/*0x194*/ DWORD EnduranceCost;   //CA Endurance Cost
//	/*0x198*/ DWORD ReuseTimerIndex; //ID of combat timer, i think.
//	/*0x19c*/ DWORD EndurUpkeep;
//	/*0x1a0*/ DWORD HateGenerated;
//	/*0x1a4*/ DWORD HitCountType;
//	/*0x1a8*/ DWORD HitCount;
//	/*0x1ac*/ DWORD ConeStartAngle;
//	/*0x0b0*/ DWORD ConeEndAngle;
//	/*0x1b4*/ DWORD PvPResistBase;
//	/*0x1b8*/ DWORD PvPCalc;
//	/*0x1bc*/ DWORD PvPResistCap;
//	/*0x1c0*/ DWORD PvPDuration;       //DurationType for PVP
//	/*0x1c4*/ DWORD PvPDurationValue1; //DurationValue1 for PVP
//	/*0x1c8*/ DWORD GlobalGroup;
//	/*0x1cc*/ DWORD PCNPCOnlyFlag; // no idea
//	/*0x1d0*/ DWORD NPCMemCategory;
//	/*0x1d4*/ DWORD SpellGroup;
//	/*0x1d8*/ DWORD SpellSubGroup; //unknown237 on Lucy it is checked at 0x76FE18 in jun 11 2014 and if 0 will ask if we want to replace our spell with a rk. x version
//	/*0x1dc*/ DWORD SpellRank;     //Unknown209 on Lucy jun 11 2014 0x76FEE0 Original = 1 , Rk. II = 5 , Rk. III = 10 , I suppose if they add Rk. IV it will be 15 and so on -eqmule
//	/*0x1e0*/ DWORD SpellClass;    //Unknown222 on Lucy
//	/*0x1e4*/ DWORD SpellSubClass; //Unknown223 on Lucy
//	/*0x1e8*/ DWORD SpellReqAssociationID;
//	/*0x1ec*/ DWORD CasterRequirementID;
//	/*0x1f0*/ DWORD MaxResist;
//	/*0x1f4*/ DWORD MinResist;
//	/*0x1f8*/ DWORD MinSpreadTime;
//	/*0x1fc*/ DWORD MaxSpreadTime;
//	/*0x200*/ DWORD SpreadRadius;
//	/*0x204*/ DWORD BaseEffectsFocusCap; //song cap, maybe other things?
//	/*0x208*/ DWORD CritChanceOverride;
//	/*0x20c*/ DWORD MaxTargets; //how many targets a spell will affect
//	/*0x210*/ DWORD AIValidTargets;
//	/*0x214*/ DWORD BaseEffectsFocusOffset;
//	/*0x218*/ FLOAT BaseEffectsFocusSlope;
//	/*0x21c*/ FLOAT DistanceModCloseDist;
//	/*0x220*/ FLOAT DistanceModCloseMult;
//	/*0x224*/ FLOAT DistanceModFarDist;
//	/*0x228*/ FLOAT DistanceModFarMult;
//	/*0x22c*/ FLOAT MinRange;
//	/*0x230*/ BYTE NoNPCLOS;     // NPC skips LOS checks
//	/*0x231*/ BYTE Feedbackable; // nothing uses this
//	/*0x232*/ BYTE Reflectable;
//	/*0x233*/ BYTE NoPartialSave;
//	/*0x234*/ BYTE NoResist;
//	/*0x235*/ BYTE UsesPersistentParticles;
//	/*0x236*/ BYTE SmallTargetsOnly;
//	/*0x237*/ bool DurationWindow; //0=Long, 1=Short
//	/*0x238*/ BYTE Uninterruptable;
//	/*0x239*/ BYTE NotStackableDot;
//	/*0x23a*/ BYTE Deletable;
//	/*0x23b*/ BYTE BypassRegenCheck;
//	/*0x23c*/ BYTE CanCastInCombat;
//	/*0x23d*/ BYTE CanCastOutOfCombat;
//	/*0x23e*/ BYTE NoHealDamageIt` emMod; //disable worn focus bonuses
//	/*0x23f*/ BYTE OnlyDuringFastRegen;
//	/*0x240*/ BYTE CastNotStanding;
//	/*0x241*/ BYTE CanMGB;
//	/*0x242*/ bool NoDisspell;
//	/*0x243*/ BYTE AffectInanimate; //ldon trap spells etc
//	/*0x244*/ bool IsSkill;
//	/*0x245*/ bool ShowDoTMessage;
//	/*0x246*/ BYTE ClassLevel[0x24]; //per class., yes there are allocations for 0x24 see 4B5776 in eqgame dated 12 mar 2014 -eqmule
//	/*0x26a*/ BYTE LightType;
//	/*0x26b*/ BYTE SpellType; //0=detrimental, 1=Beneficial, 2=Beneficial, Group Only
//	/*0x26c*/ BYTE Activated;
//	/*0x26d*/ BYTE Resist;     //0=un 1=mr 2=fr 3=cr 4=pr 5=dr 6=chromatic 7=prismatic 8=physical(skills,etc) 9=corruption
//	/*0x26e*/ BYTE TargetType; //03=Group v1, 04=PB AE, 05=Single, 06=Self, 08=Targeted AE, 0e=Pet, 28=AE PC v2, 29=Group v2, 2a=Directional
//	/*0x26f*/ BYTE FizzleAdj;
//	/*0x270*/ BYTE Skill;
//	/*0x271*/ BYTE ZoneType; //01=Outdoors, 02=dungeons, ff=Any
//	/*0x272*/ BYTE Environment;
//	/*0x273*/ BYTE TimeOfDay; // 0=any, 1=day only, 2=night only
//	/*0x274*/ BYTE CastingAnim;
//	/*0x275*/ BYTE AnimVariation;
//	/*0x276*/ BYTE TargetAnim;
//	/*0x277*/ BYTE TravelType;
//	/*0x278*/ BYTE CancelOnSit;
//	/*0x279*/ BYTE IsCountdownHeld;
//	/*0x27a*/ CHAR Name[0x40];
//	/*0x2ba*/ CHAR Target[0x20];
//	/*0x2da*/ CHAR Extra[0x20]; //This is 'Extra' from Lucy (portal shortnames etc)
//	/*0x2fa*/ CHAR CastByMe[0x60];
//	/*0x35a*/ CHAR CastByOther[0x60]; //cast by other
//	/*0x3ba*/ CHAR CastOnYou[0x60];
//	/*0x41a*/ CHAR CastOnAnother[0x60];
//	/*0x47a*/ CHAR WearOff[0x60];
//	/*0x4da*/ BYTE ShowWearOffMessage;
//	/*0x4db*/ BYTE NPCNoCast;
//	/*0x4dc*/ BYTE SneakAttack;
//	/*0x4dd*/ BYTE NotFocusable; //ignores all(?) focus effects
//	/*0x4de*/ BYTE NoDetrimentalSpellAggro;
//	/*0x4df*/ BYTE StacksWithSelf;
//	/*0x4e0*/ BYTE NoBuffBlock;
//	/*0x4e1*/ BYTE Scribable;
//	/*0x4e2*/ BYTE NoStripOnDeath;
//	/*0x4e3*/ BYTE NoRemove;   // spell can't be clicked off?
//	/*0x3fd*/ int NoOverwrite; //an enum 0 = Can Be overwritten 1 = Can Only be overwritten by itself 2 = Cannot be overwritten, not even by itself
//	/*0x4e8*/ DWORD SpellRecourseType;
//	/*0x4ec*/ BYTE CRC32Marker;
//	/*0x4ed*/
//} SPELL, *PSPELL;
//struct FocusEffectData