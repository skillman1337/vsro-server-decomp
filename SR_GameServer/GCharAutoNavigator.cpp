/**
 * ============================================================================
 * Silkroad Online - Character Auto Navigator
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GCharAutoNavigator.cpp
 * ============================================================================
 */

#include "GCharAutoNavigator.h"
#include "GObjChar.h"
#include "../JMX_Library/BSLib/BSLog.h"

/*
================
CGCharAutoNavigator::CGCharAutoNavigator
[RECONSTRUCTED - 0x004B02D0] (18 bytes)

Only +0x04, +0x08 and +0x18 are written; the rest is established by StartPursuit.
================
*/
CGCharAutoNavigator::CGCharAutoNavigator()
	: m_pOwner(nullptr)
	, m_dwTargetID(0)
	, m_wMinRange(0)
	, m_wMaxRange(0)
	, m_byState(0)
	, m_dwLastPlanTick(0)
	, m_dwPursuerID(0) {
}

/*
================
CGCharAutoNavigator::~CGCharAutoNavigator
[RECONSTRUCTED - 0x004B0310] (13 bytes)
================
*/
CGCharAutoNavigator::~CGCharAutoNavigator() {
	Reset();
}

/*
================
CGCharAutoNavigator::Reset
[RECONSTRUCTED - 0x004B0320] (26 bytes)
================
*/
void CGCharAutoNavigator::Reset() {
	m_pOwner = nullptr;
	m_dwTargetID = 0;
	m_wMinRange = 0;
	m_wMaxRange = 0;
	m_byState = 0;
	m_dwLastPlanTick = 0;
	m_dwPursuerID = 0;
}

/*
================
CGCharAutoNavigator::StartPursuit
[RECONSTRUCTED - 0x004B0350] (147 bytes)
================
*/
int32_t CGCharAutoNavigator::StartPursuit(uint16_t wMaxRange, uint32_t dwTargetID, uint16_t wMinRange) {
	if (m_pOwner->m_bActionLocked != 0) {
		return 0;
	}

	CGObjChar* pTarget = ObjMgr_FindByID(dwTargetID);
	if (pTarget == nullptr) {
		return 0;
	}

	StopPursuit();

	if (pTarget->IsPlayer()) {
		pTarget->m_AutoNavigator.m_dwPursuerID = m_pOwner->GetGlobalID();
	}

	m_dwTargetID = dwTargetID;
	m_wMinRange = wMinRange;
	m_wMaxRange = wMaxRange;
	if (wMaxRange == wMinRange && wMinRange < 10) {
		m_wMaxRange = static_cast<uint16_t>(wMaxRange + 2);
	}
	m_dwLastPlanTick = 0;
	return 1;
}

/*
================
CGCharAutoNavigator::Stop
[PARTIAL - 0x004B03F0] (86 bytes)

Native: when the owner is moving it calls owner slot 300 (CGObjChar 0x004A9430, stop move and
broadcast 0xB023) with the value of the active movement controller's slot 7 (0x0048B840).
Neither is ported yet, so only the pursuit half runs.
================
*/
void CGCharAutoNavigator::Stop(int32_t bFlag) {
	(void)bFlag;
	if (m_pOwner->IsMoving()) {
		// [PARTIAL] owner slot 300 (0x004A9430) not ported.
	}
	StopPursuit();
}

/*
================
CGCharAutoNavigator::StopPursuit
[RECONSTRUCTED - 0x004B0450] (55 bytes)
================
*/
void CGCharAutoNavigator::StopPursuit() {
	if (m_dwTargetID == 0) {
		return;
	}
	CGObjChar* pTarget = ObjMgr_FindByID(m_dwTargetID);
	if (pTarget != nullptr && pTarget->IsChar()) {
		pTarget->m_AutoNavigator.m_dwPursuerID = 0;
	}
	m_dwTargetID = 0;
}

/*
================
CGCharAutoNavigator::IsTargetAlive
[RECONSTRUCTED - 0x004B0B20] (21 bytes)
================
*/
int32_t CGCharAutoNavigator::IsTargetAlive() const {
	return (ObjMgr_FindByID(m_dwTargetID) != nullptr) ? 1 : 0;
}

/*
================
CGCharAutoNavigator::Update
[STUB - 0x004B0490] (1667 bytes)

Native: with a target, re-plans at most every 100 ms; stops when the target vanished or the
owner is dead; otherwise computes the approach point and posts 0x7021 (or 0x70C5 while riding a
transport) through the owner's command source, and 0x704F when the owner must stand up.
Requires the movement controllers and the 0x7021 handler, which are not ported yet.
================
*/
void CGCharAutoNavigator::Update() {
	if (m_dwTargetID == 0) {
		return;
	}
	if (IsTargetAlive() == 0) {
		Stop(1);
	}
}
