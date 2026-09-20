/**
 * ============================================================================
 * Silkroad Online - Expendable Item Entity
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItemExpendable.h
 *
 * Implements:
 *   - CGItemExpendable
 *   - Primary VTable @ 0x00AEB2B4 (325 virtual slots, RTTI: .?AVCGItemExpendable@@)
 *   - Secondary VTable @ 0x00AEB7E4 (for CBase at offset +0x04)
 *   - Chunk Pool & Block Queue Management (0x004A0CE0 - 0x004A19E0)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GITEMEXPENDABLE_H_
#define _SR_GAMESERVER_GITEMEXPENDABLE_H_

#include "GItem.h"
#include <cstdint>

class CGObjChar;
class CGObjPC;

class CGItemExpendable : public CGItem {
public:
	// Native 0x0049A8E0: Constructor
	CGItemExpendable();

	// Native 0x0049A940: Destructor (Scalar deleting destructor)
	virtual ~CGItemExpendable() override;

	// [RECONSTRUCTED - Native 0x0049A880 / VTable Slot 213]
	// Deletes instance back to thread-safe memory pool
	virtual void DeleteInstance();

	// [RECONSTRUCTED - Native 0x0049A9A0 / VTable Slot 214]
	// Releases permanent data block
	virtual void ReleasePermanentData();

	// [RECONSTRUCTED - Native 0x00559C70 / VTable Slot 300]
	// Expendable items do not have magic options
	virtual bool HasMagicOptions();

	// [RECONSTRUCTED - Native 0x0049A140 / VTable Slot 301]
	// Cannot unequip expendable item from normal equip slots
	virtual bool CanUnequip(uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x0049A140 / VTable Slot 302]
	// Cannot equip expendable item in normal equip slots (arrows equip via Slot 317)
	virtual bool Equip(uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x0049ACA0 / VTable Slot 306]
	// Performs action job execution
	virtual int32_t DoJob(int32_t* pResult, int32_t nActionID, int32_t nArg);

	// [RECONSTRUCTED - Native 0x0049B9F0 / VTable Slot 307]
	// Uses expendable item (potions, scrolls, summon scrolls)
	virtual int32_t UseItem(int32_t nParam1, float fParam2, int32_t nParam3, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - Native 0x0049B710 / VTable Slot 308]
	// Validates whether item can be used
	virtual int16_t* CanUseItem(int16_t* pwResult);

	// [RECONSTRUCTED - Native 0x0049C2B0 / VTable Slot 309]
	// Executes return / teleport scroll action
	virtual int16_t* ExecuteReturnScroll(int16_t* pwResult, int16_t* pArg2, int32_t* pArg3);

	// [RECONSTRUCTED - Native 0x0049EDB0 / VTable Slot 312]
	// Splits current stack and returns a new expendable item
	virtual CGItemExpendable* SplitStack(int32_t nCount);

	// [RECONSTRUCTED - Native 0x0049EE80 / VTable Slot 313]
	// Decrements stock count with assertion check
	virtual int32_t DecrementStock(int32_t nCount);

	// [RECONSTRUCTED - Native 0x0049A150 / VTable Slot 314]
	// Returns current stack count from m_pDataPermanent (+0x38)
	virtual int32_t GetCount() const;

	// [RECONSTRUCTED - Native 0x0049A160 / VTable Slot 315]
	// Sets current stack count into m_pDataPermanent (+0x38)
	virtual int32_t SetCount(int32_t nCount);

	// [RECONSTRUCTED - Native 0x0049A9B0 / VTable Slot 316]
	// Offsets stock count clamped between 0 and max stack
	virtual int32_t OffsetStock(int32_t nDelta);

	// [RECONSTRUCTED - Native 0x0049EEE0 / VTable Slot 317]
	// Checks if ammo/arrows can be equipped into slot 7 (quiver slot)
	virtual bool CanEquip(CGObjChar* pChar, uint8_t bySlot, uint8_t byTargetSlot, uint16_t* pwErrorCode);

public:
	// Static & Helper methods matching native logic
	static void CalculatePotionSteps(int32_t* pSteps, int32_t nTotal);
	static void CalculateRecoveryAmount(int32_t* pnRecoveryHP, float fRatioHP, float fRatioMP, int32_t* pnParam, int32_t* pnRecoveryMP);
	static void GetAbnormalStatusParams(void* pItem, int32_t* pParams);

	bool IsStackFull() const;

	// High-level wrapper matching original Silkroad engine usage
	virtual bool UseItem(CGObjPC* pUser);

	// Convenience accessors
	uint32_t GetStackCount() const { return static_cast<uint32_t>(GetCount()); }
	void DecrementStack() { DecrementStock(1); }
};

#endif // _SR_GAMESERVER_GITEMEXPENDABLE_H_
