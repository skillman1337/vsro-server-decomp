/**
 * ============================================================================
 * Silkroad Online - Generic Scheduled Callback Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\ScheduledCallbacker.h
 *
 * Implements the generic interval-based periodic timer callback system used by:
 *   - CGame @ 0x004126C2: CScheduledCallbacker<CGame, unsigned long> (vftable @ 0x00AE07F8)
 *   - CGameAI @ 0x0054D810: CScheduledCallbacker<CGameAI, float> (vftable @ 0x00AF969C)
 *   - CGObjChar @ 0x004AB3D0: CScheduledCallbacker<CGObjChar, float> (vftable @ 0x00AECCB4)
 *   - CGEventDaemon @ 0x00611EB0: CScheduledCallbacker<CGEventDaemon, unsigned long> (vftable @ 0x00B0288C)
 *   - CMaintainer @ 0x009835F0: CScheduledCallbacker<CMaintainer, unsigned long> (vftable @ 0x00B42E58)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SCHEDULEDCALLBACKER_H_
#define _SR_GAMESERVER_SCHEDULEDCALLBACKER_H_

#include <vector>
#include <cstdint>
#include "../JMX_Library/BSLib/BSLog.h"

/**
 * [RECONSTRUCTED - 0x0041BF50 / 0x0054D8D0]
 * tagScheduleItem: Scheduled callback descriptor
 * Size: 24 bytes (0x18)
 *
 * Element layout in CScheduledCallbacker:
 *   +0x00: TTime    fElapsedTime - Accumulated elapsed time
 *   +0x04: TTime    fInterval    - Scheduled trigger interval threshold
 *   +0x08: TTarget* pTarget      - Target object context (this pointer)
 *   +0x0C: uint32_t pad0C        - Reserved / member pointer alignment
 *   +0x10: PFN     pfnCallback  - Function pointer
 *   +0x14: int32_t  nThisDelta   - this-pointer adjustment delta
 */
template <typename TTarget, typename TTime>
struct tagScheduleItem {
	TTime    fElapsedTime = 0;       // +0x00: Current elapsed time
	TTime    fInterval    = 0;       // +0x04: Scheduled trigger interval
	TTarget* pTarget      = nullptr; // +0x08: Target object context
	uint32_t pad0C        = 0;       // +0x0C: Padding / unused
	void   (*pfnCallback)(TTarget*) = nullptr; // +0x10: Function pointer callback
	int32_t  nThisDelta   = 0;       // +0x14: this-pointer adjustment delta
};

/**
 * [RECONSTRUCTED - 0x0041BE90 / 0x0054D810]
 * CScheduledCallbacker<TTarget, TTime>
 *
 * VTable references:
 *   - CGame:          0x00AE07F8
 *   - CGameAI:        0x00AF969C
 *   - CGObjChar:      0x00AECCB4
 *   - CGEventDaemon:  0x00B0288C
 *   - CMaintainer:    0x00B42E58
 *
 * Native Memory Layout:
 *   +0x00: VTable pointer (4 bytes)
 *   +0x04: std::vector<tagScheduleItem<TTarget, TTime>> m_vecSchedules (16/24 bytes)
 */
template <typename TTarget, typename TTime>
class CScheduledCallbacker {
public:
	typedef void (*PFN_SCHEDULED_CALLBACK)(TTarget*);

	// [RECONSTRUCTED - 0x0041BE90 / 0x0054D810]
	CScheduledCallbacker()
		: m_vecSchedules() {
	}

	// [RECONSTRUCTED - 0x0041BEE0 / 0x0054D860 / 0x0041D560 / 0x0054E6A0]
	virtual ~CScheduledCallbacker() {
		m_vecSchedules.clear();
	}

	// [RECONSTRUCTED - 0x0041BF50 / 0x0054D8D0]
	// Registers a periodic callback on a target instance
	bool RegisterCallback(TTarget* pTarget, TTime fInterval, PFN_SCHEDULED_CALLBACK pfnCallback, int32_t nThisDelta = 0) {
		if (fInterval <= static_cast<TTime>(0)) {
			BSLib::AssertFailed();
			return false;
		}

		tagScheduleItem<TTarget, TTime> item;
		item.fElapsedTime = static_cast<TTime>(0);
		item.fInterval = fInterval;
		item.pTarget = pTarget;
		item.pad0C = 0;
		item.pfnCallback = pfnCallback;
		item.nThisDelta = nThisDelta;

		m_vecSchedules.push_back(item);
		return true;
	}

	// [RECONSTRUCTED - 0x0041BFA0 / 0x0054D920]
	// Advances timers and executes triggered callbacks
	void Update(TTime fDeltaTime) {
		if (fDeltaTime <= static_cast<TTime>(0)) {
			return;
		}

		for (auto& item : m_vecSchedules) {
			item.fElapsedTime += fDeltaTime;
			if (item.fElapsedTime >= item.fInterval) {
				item.fElapsedTime -= item.fInterval;
				if (item.pfnCallback != nullptr && item.pTarget != nullptr) {
					TTarget* pAdjusted = reinterpret_cast<TTarget*>(
						reinterpret_cast<uintptr_t>(item.pTarget) + item.nThisDelta);
					item.pfnCallback(pAdjusted);
				}
			}
		}
	}

public:
	std::vector<tagScheduleItem<TTarget, TTime>> m_vecSchedules; // +0x04
};

#endif // _SR_GAMESERVER_SCHEDULEDCALLBACKER_H_
