
#include "../MQ2Plugin.h"

#include <map>
#include <string>

class SpellQueue
{
public:
	enum class SpellType
	{
		Feign,
		Evacuate,   // Gate
		Arrest,     // Mesmerize, fear, root, stun
		Resistance, // Malo, tashani, scent
		Charm,
		Blur,
		Slow,
		HealGroup,
		Heal,
		HoT,
		Resurrect,
		PointBlank,
		CureDisease,
		CurePoison,
		CureCurse,
		Cripple,
		PetHeal,
		PetSummon,

		Movement, // Spirit of the wolf, selo's
		Clarity,  // Tranquility, chorus,
		Cannibalize,
		Haste,
		Aegolism,
		Symbol,
		Protection,
		Focus,
		Toughness, // remove
		Statistic, // AC, charisma, wisdom, etc.
		DoT,
		Nuke,
		SummonCorpse,
		Calm,
		Invisibility,
		EnduringBreath,
		Bind,
		Illusion,
		Alliance,
		Unknown,
	};

	struct SpellData
	{
		SpellType type;
		PSPELL p_spell;
		std::string target_name;
		std::clock_t time_ms;

		bool operator<(SpellData& other)
		{
			return type < other.type;
		}
	};

	SpellQueue();
	~SpellQueue();

	SpellData* Front();
	void Push(SpellType& type, std::string& target_name);
	void Push(std::string& type_name, std::string& target_name);
	void Pop();
	void Remove(SpellType& type, std::string& target_name);
	void Remove(std::string& type_name, std::string& target_name);

private:
	enum class SpellCategory
	{
		Unknown            = 0,
		CreateItem         = 18,
		DamageOverTime     = 20,
		DirectDamage       = 25,
		Heals              = 42,
		HPBuffs            = 45,
		Pet                = 69,
		Regen              = 79,
		StatisticBuffs     = 95,
		Taps               = 114,
		Transport          = 123,
		UtilityBeneficial  = 125,
		UtilityDetrimental = 126,
		Traps              = 131,
		Auras              = 132,
	};

	enum class SpellSubCategory
	{
		Unknown         = 0,
		Aegolism        = 1,
		Agility         = 2,
		Alliance        = 3,
		Antonica        = 5,
		ArmorClass      = 6,
		Attack          = 7,
		Block           = 10,
		Calm            = 11,
		Charisma        = 12,
		Charm           = 13,
		Cold            = 14,
		CombatInnates   = 16,
		Conversions     = 17,
		CreateItem      = 18,
		Cure            = 19,
		DamageShield    = 21,
		Discord         = 28,
		Disease         = 29,
		Disempower      = 30,
		Dispel          = 31,
		DurationHeals   = 32,
		DurationTap     = 33,
		EnchantMetal    = 34,
		Enthrall        = 35,
		Faydwer         = 36,
		Fear            = 37,
		Fire            = 38,
		Haste           = 41,
		Heals           = 42,
		Health          = 43,
		HealthMana      = 44,
		HPTypeOne       = 46,
		HPTypeTwo       = 47,
		IllusionOther   = 48,
		IllusionSelf    = 49,
		ImbueGem        = 50,
		Invisibility    = 51,
		Invulnerability = 52,
		Jolt            = 53,
		Kunark          = 54,
		Levitate        = 55,
		Luclin          = 57,
		Magic           = 58,
		Mana            = 59,
		ManaDrain       = 60,
		ManaFlow        = 61,
		MeleeGuard      = 62,
		Miscellaneous   = 64,
		Movement        = 65,
		Odus            = 67,
		PetHaste        = 70,
		PetBuff         = 71,
		Physical        = 72,
		Poison          = 75,
		PowerTap        = 76,
		QuickHeal       = 77,
		Reflection      = 78,
		Regen           = 79,
		ResistBuff      = 80,
		ResistDebuff    = 81,
		Resurrection    = 82,
		Root            = 83,
		Rune            = 84,
		ShadowStep      = 86,
		Shielding       = 87,
		Slow            = 88,
		Snare           = 89,
		SpellFocus      = 91,
		Stamina         = 94,
		Strength        = 96,
		Stun            = 97,
		SummonAir       = 98,
		SummonAnimation = 99,
		SummonFamiliar  = 101,
		SummonUndead    = 103,
		SummonWarder    = 104,
		SummonWeapon    = 110,
		Summoned        = 111,
		Symbol          = 112,
		Taelosia        = 113,
		ThePlanes       = 116,
		Undead          = 124,
		Velious         = 127,
		Visage          = 128,
		Vision          = 129,
		SerpentSpine    = 134,
		SummonSwarm     = 139,
		Chromatic       = 137,
		Swarm           = 139,
		HasteSpellFocus = 145,
	};

	inline void IntializeStructures();
	inline void ParseSpellBook();
	inline void AddSpell(PSPELL p_spell);
	inline void PrintNack(PSPELL p_spell);

	//
	// Fetches a spell from a spell type.
	//
	inline PSPELL ToSpell(SpellType& type);

	//
	// Fetches a spell type from a type name. Unknown if not found.
	//
	inline SpellType ToType(std::string& type_name);

	//
	// Determines whether this spell is already queued for this target
	//
	inline bool IsQueued(SpellType& type, std::string& target_name);

	std::multimap<SpellType, SpellData*> m_queued; // Spells waiting to be cast
	std::map<std::string, SpellType> m_types;      // In-bound command takes a generic term
	std::map<SpellType, PSPELL> m_spells;          // Translates generic in-bound to specific spell
};