/**
 * ============================================================================
 * Silkroad Online - Equipment Item Entity
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItemEquip.h
 *
 * Implements:
 *   - CGItemEquip
 *   - Primary VTable @ 0x00AEA9EC (326 virtual slots, RTTI: .?AVCGItemEquip@@)
 *   - Secondary VTable @ 0x00AEAF08 (for CBase at offset +0x04)
 *   - Memory Pool & Block Queue Management (0x00499390 - 0x004996B0)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GITEMEQUIP_H_
#define _SR_GAMESERVER_GITEMEQUIP_H_

#include "GItem.h"
#include <cstdint>

class CGObjChar;
class CGObjPC;

class CGItemEquip : public CGItem {
public:
	// Native 0x00495A90: Constructor
	CGItemEquip();

	// Native 0x00495AF0: Destructor (Scalar deleting destructor)
	virtual ~CGItemEquip() override;

	// [RECONSTRUCTED - Native 0x00495B40 / VTable Slot 0]
	// Reinitializes combat attributes and resets ground/equip state
	virtual void InitializeCombatStats();

	// [RECONSTRUCTED - Native 0x00483E80 / VTable Slot 54]
	// Checks if equipment can be enchanted with magic options or alchemy
	virtual bool CanEnchant();

	// [RECONSTRUCTED - Native 0x00495C00 / VTable Slot 212]
	// Initializes equipment entity from permanent data record
	virtual int32_t Initialize(int32_t nParam1, int32_t nParam2, void* pParam3, int32_t nParam4, void* pParam5);

	// [RECONSTRUCTED - Native 0x00495A30 / VTable Slot 213]
	// Deletes instance back to thread-safe memory pool
	virtual void DeleteInstance();

	// [RECONSTRUCTED - Native 0x00495BE0 / VTable Slot 214]
	// Releases permanent data block and updates state to 3
	virtual void ReleasePermanentData();

	// [RECONSTRUCTED - Native 0x00495910 / VTable Slot 300]
	// Checks if equipment has active magic options
	virtual bool HasMagicOptions();

	// [RECONSTRUCTED - Native 0x00497510 / VTable Slot 301]
	// Checks if equipment can be unequipped from character slot
	virtual bool CanUnequip(uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x00497630 / VTable Slot 302]
	// Equips item onto character slot and updates stats
	virtual bool Equip(uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x00497F20 / VTable Slot 306]
	// Performs equip action job on character
	virtual int32_t DoJob(int32_t* pResult, int32_t nActionID, int32_t nArg);

	// [RECONSTRUCTED - Native 0x00497070 / VTable Slot 317]
	// Validates whether character meets requirements (level, gender, race, mastery) to equip item
	virtual bool CanEquip(CGObjChar* pChar, uint8_t bySlot, uint8_t byTargetSlot, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x004972C0 / VTable Slot 318]
	// Verifies if two equipment items belong to the same item set
	virtual bool IsCompatibleSet(CGItemEquip* pOther);

	// [RECONSTRUCTED - Native 0x00495960 / VTable Slot 323]
	// Checks if equipment contains socket attachments
	virtual bool HasSockets();

public:
	// Core Equipment Logic
	// Native 0x00495D10: Retrieves float variance for specified stat index (0..11) from 64-bit mask
	float GetStatVariance(uint32_t dwStatIndex) const;

	// Native 0x004973F0: Updates stat variance bitmask for specified stat index
	bool SetStatVariance(uint32_t dwStatIndex, uint32_t dwValue);

	// Native 0x00495D60: Calculates all base combat attributes from RefItem and variances
	void CalculateBaseStats();

	// Native 0x00496A70: Applies magic options and blue stats to combat attributes
	void ApplyMagicOptions();

	// Native 0x00495CA0: Recalculates all combat stats and verifies durability threshold
	void RecalculateStats();

	// Native 0x00496C20: Resolves authentic equipment slot type (0..12) from TID bitmask
	int32_t GetEquipSlot() const;

	// Native 0x00496D90: Offsets durability and handles break/repair transitions
	int32_t OffsetDurability(int32_t nOffset);

	// Native 0x00496E60: Calculates repair gold cost based on durability loss and item price
	int32_t CalculateRepairCost(int32_t nMaxRepairPoints, int32_t* pnPointsRepaired, uint16_t* pwErrorCode);

	// Native 0x00497830: Unequips item and removes ParamKeeper buffs
	void UnequipRemoveParamKeeperItemEntry();

	// Native 0x00498020: Applies opt level (+plus) combat bonuses
	int32_t ApplyOptLevelBonuses(int16_t* pwResult);

	// Native 0x00498050: Removes opt level bonuses
	int32_t RemoveOptLevelBonuses(int16_t* pwResult);

	// Native 0x00498060: Activates timed/avatar equipment
	int16_t* ActivateTimedItem(int16_t* pwResult);

	// Native 0x00498160: Applies opt level stat modifiers
	int32_t ApplyOptLevelModifiers();

	// Native 0x00498370: Attaches ability modifiers from opt level
	int32_t AttachAbilityByItemOptLevel();

	// Native 0x004984C0: Detaches ability modifiers from opt level
	int32_t DetachAbilityByItemOptLevel();

	// Native 0x00498610: Checks if timed item has expired
	bool IsItemExpired();

	// Native 0x00498690: Applies a magic option directly into ParamKeeper
	void ApplyMagicOptionToParamKeeper(int32_t nOptionID, float fValue, int32_t nType, int32_t nOptLevel);

	// Native 0x004991B0: Locates magic parameter in item descriptor
	int32_t FindMagicParam(uint16_t* pwParamID, uint16_t* pwValue1, int32_t* pnValue2);

	// Native 0x004992E0: Dispatches equip/unequip script event
	int32_t DispatchEquipEvent(int32_t nEventType, int32_t nArg);

public:
	// Accessor Helpers
	uint32_t GetCurrentDurability() const { return m_dwCurrentDurability; }
	uint32_t GetMaxDurability() const     { return m_dwMaxDurability; }
	uint32_t GetEquipState() const        { return m_dwEquipState; }

	float GetPhyDefense() const           { return m_fPhyDefense; }
	float GetMagDefense() const           { return m_fMagDefense; }
	float GetHitRate() const              { return m_fHitRate; }
	float GetParryRatio() const           { return m_fParryRatio; }
	float GetBlockRatio() const           { return m_fBlockRatio; }
	float GetCritical() const             { return m_fCritical; }
	float GetMinPhyAttackPower() const    { return m_fMinPhyAttackPower; }
	float GetMaxPhyAttackPower() const    { return m_fMaxPhyAttackPower; }
	float GetMinMagAttackPower() const    { return m_fMinMagAttackPower; }
	float GetMaxMagAttackPower() const    { return m_fMaxMagAttackPower; }
	float GetPhyReinforce() const         { return m_fPhyReinforce; }
	float GetMagReinforce() const         { return m_fMagReinforce; }
	float GetAttackRange() const          { return m_fAttackRange; }

public:
	// Exact struct layout matching native binary bytes (+0x190 to +0x1E8, total size: 0x1E8 = 488 bytes):
	uint32_t m_dwCurrentDurability; // +0x190: Current item durability
	uint32_t m_dwMaxDurability;     // +0x194: Maximum item durability (scaled by stat variance)
	float    m_fPhyDefense;         // +0x198: Physical defense rating
	float    m_fMagDefense;         // +0x19C: Magical defense rating
	float    m_fHitRate;            // +0x1A0: Hit rate bonus
	float    m_fParryRatio;         // +0x1A4: Parry ratio / dodge bonus
	float    m_fBlockRatio;         // +0x1A8: Shield blocking ratio percentage
	float    m_fCritical;           // +0x1AC: Critical strike probability
	float    m_fMinPhyAttackPower;  // +0x1B0: Minimum physical attack power
	float    m_fMaxPhyAttackPower;  // +0x1B4: Maximum physical attack power
	float    m_fMinMagAttackPower;  // +0x1B8: Minimum magical attack power
	float    m_fMaxMagAttackPower;  // +0x1BC: Maximum magical attack power
	float    m_fPhyReinforce;       // +0x1C0: Physical reinforcement percentage
	float    m_fMagReinforce;       // +0x1C4: Magical reinforcement percentage
	float    m_fAttackRange;        // +0x1C8: Attack range distance
	float    m_fMoveSpeed;          // +0x1CC: Movement speed bonus
	float    m_fItemStat_1D0;       // +0x1D0: Additional attribute
	float    m_fItemStat_1D4;       // +0x1D4: Additional attribute
	float    m_fItemStat_1D8;       // +0x1D8: Additional attribute
	float    m_fItemStat_1DC;       // +0x1DC: Additional attribute
	uint32_t m_dwEquipState;        // +0x1E0: Equip/lifecycle state (initialized to 3)
	uint8_t  m_pad1E4[4];           // +0x1E4 - +0x1E7: Alignment padding to 0x1E8
};

#endif // _SR_GAMESERVER_GITEMEQUIP_H_
