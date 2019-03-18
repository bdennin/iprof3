
#include "../MQ2Plugin.h"

#include <map>
#include <string>

class SpellQueue
{
public:
	enum class SpellType
	{
		Feign,
		Evacuate,
		AreaStun,
		Mesmerize,
		Malo,
		Tashani,
		Scent,
		Charm,
		MemoryBlur,
		Slow,
		HealGroup,
		Heal,
		HoT,
		Portal,
		Resurrect,
		AreaDamage,
		CureDisease,
		CurePoison,
		CureCurse,
		Cripple,
		PetHeal,
		PetSummon,
		Movement,
		Clarity,
		Cannibalize,
		Haste,
		Aegolism,
		Symbol,
		Protection,
		Focus,
		Statistic,
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
		PSPAWNINFO p_target;
		std::clock_t time_ms;

		bool operator<(SpellData& other)
		{
			return type < other.type;
		}
	};

	SpellQueue();
	~SpellQueue();

	SpellData* Front();
	void Push(std::string& type_name, std::string& target_name);
	void Push(SpellType& type, PSPAWNINFO p_target);
	void Pop();
	void Remove(std::string& type_name, std::string& target_name);
	void Remove(SpellType& type, PSPAWNINFO p_target);

private:
	enum class SpellCategory
	{
		Unknown = 0,
		CreateItem = 18,
		DamageOverTime = 20,
		DirectDamage = 25,
		Heals = 42,
		HPBuffs = 45,
		Pet = 69,
		Regen = 79,
		StatisticBuffs = 95,
		Taps = 114,
		Transport = 123,
		UtilityBeneficial = 125,
		UtilityDetrimental = 126,
		Traps = 131,
		Auras = 132,
	};

	enum class SpellSubCategory
	{
		Unknown = 0,
		Aegolism = 1, // Aegolism
		Agility = 2, // Statistic
		Alliance = 3, // Alliance
		Antonica = 5, // Portal
		ArmorClass = 6, // ArmorClass
		Attack = 7, // Attack
		Block = 10, // ?
		Calm = 11, // Calm
		Charisma = 12, // Charisma
		Charm = 13, // Charm
		Cold = 14, // Nuke/Dot
		CombatInnates = 16, // Panther
		Conversions = 17, // Cannibalize, Lich
		CreateItem = 18, // ?
		Cure = 19, // CurePoison, CureDisease, CureCurse
		DamageShield = 21, // DamageShield
		Discord = 28, // Portal
		Disease = 29, // Nuke/Dot
		Disempower = 30, // Cripple
		Dispel = 31, // ?
		DurationHeals = 32, // HoT
		DurationTap = 33, // ?
		EnchantMetal = 34, // ?
		Enthrall = 35, // Mesmerize
		Faydwer = 36, // Portal
		Fear = 37, // ?
		Fire = 38, // Nuke/DoT
		Haste = 41, // Haste
		Heals = 42, // Heal
		Health = 43, // ?? Regeneration
		HealthMana = 44, // HMRegeneration
		HPTypeOne = 46, // ? Symbol ?
		HPTypeTwo = 47, // ? Skin ?
		IllusionOther = 48, // ? 
		IllusionSelf = 49, // ?
		ImbueGem = 50, // ?
		Invisibility = 51, // Invisibility
		Invulnerability = 52, // Invulnerability
		Jolt = 53, // Jolt
		Kunark = 54, // Portal
		Levitate = 55, // Levitate
		Luclin = 57, // Portal
		Magic = 58, // Nuke/DoT
		Mana = 59, // Clarity
		ManaDrain = 60, // ?
		ManaFlow = 61, // ?
		MeleeGuard = 62, // ?
		Miscellaneous = 64, // ?
		Movement = 65, // Movement
		Odus = 67, // Portal
		PetHaste = 70, // PetBuff
		PetBuff = 71, // PetBuff
		Physical = 72, // Nuke/DoT
		Poison = 75, // Nuke/DoT
		PowerTap = 76, // ?
		QuickHeal = 77, // ?
		Reflection = 78, // ?
		Regen = 79, // ?? Regeneration
		ResistBuff = 80, // ResistFire, ResistCold, ResistPoison, ResistDisease, ResistMagic
		ResistDebuff = 81, // Malo, Tashani, Scent
		Resurrection = 82, // Resurrection
		Root = 83, // Root
		Rune = 84, // Absorption
		ShadowStep = 86, // Shadowstep
		Shielding = 87, // Talisman / Shielding
		Slow = 88, // Slow
		Snare = 89, // Snare
		SpellFocus = 91, // Focus
		Stamina = 94, // Stamina
		Strength = 96, // Strength
		Stun = 97, // Stun, AreaStun
		SummonAir = 98, // Pet
		SummonAnimation = 99, // Pet
		SummonFamiliar = 101, // Pet
		SummonUndead = 103, // Pet
		SummonWarder = 104, // Pet
		SummonWeapon = 110, // Pet
		Summoned = 111, // Nuke/DoT
		Symbol = 112, // Symbol
		Taelosia = 113, // Portal
		ThePlanes = 116, // Portal
		Undead = 124, // Nuke/DoT
		Velious = 127, // Portal
		Visage = 128, // ?
		Vision = 129, // ?
		SerpentSpine = 134, // Portal
		SummonSwarm = 139, // Swarm
		Chromatic = 137, // Nuke/DoT
		Swarm = 139, // Swarm
		HasteSpellFocus = 145,
	};

	inline void InitTypes();
	inline void InitSpells();
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
	inline bool IsQueued(SpellType& type, PSPAWNINFO p_target);

	enum class SpellFailure
	{
		None,
		Barterting, // Merchant
		Crouching,
		Interrupted,
		Invisible,
		Moving,
		Reading,    // Book open
		Trading,	// Player

		Fizzled,
		Memorizing,
		NotMemorized,
		NotReady,
		Timeout,

		LevelTooLow,
		BookClosed,
		NotStack,
		NotInZone,
		NotVisible,
		OutOfRange,
		OutOfMana,
		IsCorpse,
		IsNotCorpse,
		UndefinedSelf,
		UndefinedSpell,
		UndefinedTarget,
		Unknown,
	};

	inline SpellFailure Cast();
	inline SpellFailure Memorize(PSPELL p_spell, PSPAWNINFO p_caster);
	inline SpellFailure IsCastable(PSPELL p_spell, PSPAWNINFO p_caster, PSPAWNINFO p_target);

	typedef VOID(__cdecl* CastFunction)(PSPAWNINFO, PCHAR);

	CastFunction m_p_cast;
	FLOAT m_cast_x;
	FLOAT m_cast_y;
	FLOAT m_cast_z;
	size_t m_gem_count;
	std::multimap<SpellType, SpellData*> m_queued;
	std::map<std::string, SpellType> m_types;
	std::map<SpellType, PSPELL> m_spells;
};