/**
 * ============================================================================
 * Silkroad Online - Character Auto Navigator (pursuit of a target object)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GCharAutoNavigator.h
 *
 * CGCharAutoNavigator
 *   - Native VTable @ 0x00AECD1C, RTTI .?AVCGCharAutoNavigator@@
 *   - Embedded at CGObjChar + 0x1BF4
 *   - Constructor @ 0x004B02D0, destructor @ 0x004B0310, Reset @ 0x004B0320
 *   - StartPursuit @ 0x004B0350, Stop @ 0x004B03F0, StopPursuit @ 0x004B0450
 *   - Update @ 0x004B0490 (called from CGObjChar::OnTick 0x004A89E9)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GCHARAUTONAVIGATOR_H_
#define _SR_GAMESERVER_GCHARAUTONAVIGATOR_H_

#include <cstdint>

class CGObjChar;

class CGCharAutoNavigator {
public:
	// [RECONSTRUCTED - 0x004B02D0] (eax = this)
	CGCharAutoNavigator();

	// [RECONSTRUCTED - 0x004B0310]
	virtual ~CGCharAutoNavigator();

	// [RECONSTRUCTED - 0x004B0320] (eax = this)
	void Reset();

	// [RECONSTRUCTED - inlined by CGObjChar construction]
	void SetOwner(CGObjChar* pOwner) { m_pOwner = pOwner; }

	// [RECONSTRUCTED - 0x004B0350] (ecx = this, eax = wMaxRange, stack dwTargetID, wMinRange)
	// Starts following dwTargetID until it is between wMinRange and wMaxRange away.
	int32_t StartPursuit(uint16_t wMaxRange, uint32_t dwTargetID, uint16_t wMinRange);

	// [PARTIAL - 0x004B03F0] (eax = this, stack bFlag)
	// Stops the owner's current movement and the pursuit.
	void Stop(int32_t bFlag);

	// [RECONSTRUCTED - 0x004B0450] (edi = this)
	void StopPursuit();

	// [RECONSTRUCTED - 0x004B0B20] (eax = this)
	int32_t IsTargetAlive() const;

	// [STUB - 0x004B0490] (ebx = this)
	// Re-plans the approach every 100 ms and posts 0x7021 / 0x70C5 / 0x704F move commands.
	void Update();

	uint32_t GetTargetID() const { return m_dwTargetID; }

public:
	CGObjChar* m_pOwner;          // +0x04
	uint32_t   m_dwTargetID;      // +0x08
	uint16_t   m_wMinRange;       // +0x0C
	uint16_t   m_wMaxRange;       // +0x0E
	uint8_t    m_byState;         // +0x10
	uint32_t   m_dwLastPlanTick;  // +0x14
	uint32_t   m_dwPursuerID;     // +0x18: id of the object pursuing this navigator's owner
};

#endif // _SR_GAMESERVER_GCHARAUTONAVIGATOR_H_
