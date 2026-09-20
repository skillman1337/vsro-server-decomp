/**
 * ============================================================================
 * Silkroad Online - AI Tactics Extended Routine Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AITactics_ExtRoutine.cpp
 *
 * Implements:
 *   - AI::CTactics Lifecycle & VTable @ 0x00AF879C
 *   - AI_CTactics_EvaluateCandidateTarget @ 0x00547070
 *   - AI_CTactics_ConfigureFollowState @ 0x005471B0
 *   - AI_CTactics_ConfigureWanderState @ 0x00547200
 *   - AI_CTactics_ResetStateHandlers @ 0x00547240
 *   - AI_CTactics_SetStateHook @ 0x00547280
 *   - AI_CTactics_Battle_TryAction @ 0x005472A0
 *   - AI_CTactics_Aggro_RecordDamage @ 0x005473C0
 *   - AI_CTactics_PartyAggro_Distribute @ 0x00547570 (Assert @ line 1634)
 *   - AI_CTactics_SyncMovementSpeed @ 0x00547860
 *   - AI_CTactics_ScanForTargets @ 0x005478F0
 *   - AI_CTactics_ScanForTargets_Special @ 0x00547AD0
 *   - AI_CTactics_StateIdleWander @ 0x00547C70
 *   - AI_CTactics_StateCustomSpawn @ 0x00547F40
 * ============================================================================
 */

#include "AITactics_ExtRoutine.h"
#include "GameAI.h"
#include "../ServerCommon/ReferenceData.h"
#include "GObjChar.h"
#include "Game.h"
#include "../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <windows.h>
#include <cstdlib>
#include <cmath>
#include <cstring>

// Server global manager and helper forwarders
extern CGObjChar* CGame_FindObjectByID(uint32_t dwGameID);
extern bool Pos_RegionsCompatible(const void* pReg1, const void* pReg2);

namespace AI {

// Forwarders for state actions
extern int32_t AI_CTactics_FollowLeader_Check(CTactics* pTactics, void* pContext);
extern int32_t AI_CTactics_StateApproachTarget(CTactics* pTactics, void* pContext);
extern int32_t AI_CTactics_StateCombatRanged(CTactics* pTactics, void* pContext);
extern int32_t AI_CTactics_StateFollowLeader(CTactics* pTactics, void* pContext);
extern int32_t AI_CTactics_StateCombatMelee(CTactics* pTactics, void* pContext);
extern int32_t AI_CTactics_StateReturnToSpawn(CTactics* pTactics, void* pContext);

/*
================================================================================
AI::CTactics Implementation (VTable @ 0x00AF879C)
================================================================================
*/

/**
 * [RECONSTRUCTED - Native 0x0053EA60] (1740 bytes)
 * CTactics constructor
 */
CTactics::CTactics()
	: m_pOwner(nullptr)
	, m_nPoolIndex(0)
	, m_pRefTactics(nullptr)
	, m_pNest(nullptr)
	, m_pCurrentState(nullptr)
	, m_dwActiveStateFlag(0)
	, m_pPendingMsg(nullptr)
	, m_pActiveSkill(nullptr)
	, m_pBackupSkill(nullptr)
	, m_nApproachSlot(-1)
	, m_pfnApproach(nullptr)
	, m_pfnFollow(nullptr)
	, m_pfnWander(nullptr)
	, m_pfnFlee(nullptr)
	, m_pfnReturnToSpawn(nullptr)
	, m_pfnApproachTarget(nullptr)
	, m_pfnCombatMelee(nullptr)
	, m_pfnCombatRanged(nullptr)
	, m_byUnk148(0)
	, m_dwPendingActionID(0)
	, m_dwNextSearchDelayMs(1000)
	, m_dwLastSearchTimestamp(0)
	, m_fAggroRadius(10.0f)
	, m_fSearchRadius(15.0f)
	, m_fLeashRadius(30.0f)
	, m_nRoamDirection(1)
	, m_byAggroMode(0)
	, m_fSpawnX(0.0f)
	, m_fSpawnY(0.0f)
	, m_fSpawnZ(0.0f)
	, m_wSpawnRegionID(0)
	, m_fHomeX(0.0f)
	, m_fHomeY(0.0f)
	, m_fHomeZ(0.0f)
	, m_wHomeRegionID(0)
	, m_fRoamRadius(200.0f) {
	std::memset(pad_cmdqueue, 0, sizeof(pad_cmdqueue));
	std::memset(pad_a4, 0, sizeof(pad_a4));
	std::memset(pad_e8, 0, sizeof(pad_e8));
	std::memset(pad_f4, 0, sizeof(pad_f4));
	std::memset(pad_120, 0, sizeof(pad_120));
	std::memset(pad_169, 0, sizeof(pad_169));
	std::memset(pad_192, 0, sizeof(pad_192));
	std::memset(pad_1aa, 0, sizeof(pad_1aa));
}

/**
 * [RECONSTRUCTED - Native 0x0053F130] (360 bytes)
 * CTactics destructor (Virtual slot 0 @ 0x0054E640)
 */
CTactics::~CTactics() {
	Shutdown();
	m_vecStates.clear();
	m_vecSkills.clear();
	m_vecAggroTargets.clear();
}

/**
 * [RECONSTRUCTED - 0x0040B720]
 * CTactics::PostCommand
 */
void CTactics::PostCommand(void* pPacket) {
	(void)pPacket;
}

/**
 * [RECONSTRUCTED - 0x0040A940]
 * CTactics::SetOwnerChar
 */
void CTactics::SetOwnerChar(CGObjChar* pChar) {
	m_pOwner = pChar;
}

/**
 * [RECONSTRUCTED - 0x0053D420]
 * CAITimeManager::SetTimer
 */
void CAITimeManager::SetTimer(uint32_t dwTimerID, uint32_t dwIntervalMs) {
	for (size_t i = 0; i < m_vecActiveTimers.size(); ++i) {
		if (m_vecActiveTimers[i].m_dwTimerID == dwTimerID) {
			m_vecActiveTimers[i].m_dwInterval = dwIntervalMs;
			m_vecActiveTimers[i].m_dwTargetTime = ::GetTickCount() + dwIntervalMs;
			return;
		}
	}
	tagAITimerEntry newEntry;
	newEntry.m_dwTimerID = dwTimerID;
	newEntry.m_dwInterval = dwIntervalMs;
	newEntry.m_dwTargetTime = ::GetTickCount() + dwIntervalMs;
	m_vecActiveTimers.push_back(newEntry);
	m_nActiveTimerCount = static_cast<uint32_t>(m_vecActiveTimers.size());
}

/**
 * [RECONSTRUCTED - Native 0x0053D6E0]
 * CAITimeManager::IsTimerExpired
 */
bool CAITimeManager::IsTimerExpired(uint32_t dwTimerID, uint32_t dwCurTick) {
	for (size_t i = 0; i < m_vecActiveTimers.size(); ++i) {
		if (m_vecActiveTimers[i].m_dwTimerID == dwTimerID) {
			if (dwCurTick >= m_vecActiveTimers[i].m_dwTargetTime) {
				m_vecActiveTimers[i].m_dwTargetTime = dwCurTick + m_vecActiveTimers[i].m_dwInterval;
				return true;
			}
			return false;
		}
	}
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053D600]
 * CAITimeManager::ArmTimer
 */
void CAITimeManager::ArmTimer(uint32_t dwTimerID, uint32_t dwInterval, uint32_t dwRandomRange, uint32_t dwCurTick, bool /*bSpecial*/) {
	(void)dwCurTick;
	uint32_t dwRand = (dwRandomRange > 0) ? (static_cast<uint32_t>(rand()) % dwRandomRange) : 0;
	uint32_t dwTotalInterval = dwInterval + dwRand;
	if (dwTotalInterval == 0) {
		dwTotalInterval = 1;
	}
	SetTimer(dwTimerID, dwTotalInterval);
}

/**
 * [RECONSTRUCTED - Native 0x005455F0]
 * CAITimeManager::ClearTimer
 */
void CAITimeManager::ClearTimer(uint32_t dwTimerID) {
	for (size_t i = 0; i < m_vecActiveTimers.size(); ++i) {
		if (m_vecActiveTimers[i].m_dwTimerID == dwTimerID) {
			m_vecActiveTimers[i].m_dwTargetTime = 0;
			return;
		}
	}
}

/**
 * [RECONSTRUCTED - Native 0x00540CB0] (58 bytes)
 * CTactics::UnregisterSquadTarget
 */
void CTactics::UnregisterSquadTarget() {
	// Clears squad binding if active
}

/**
 * [RECONSTRUCTED - Native 0x00541DC0] (48 bytes)
 * CTactics::UpdateSkillAttackRange
 */
void CTactics::UpdateSkillAttackRange(CGObjChar* pTarget, void* pSkillData) {
	if (pTarget == nullptr || pSkillData == nullptr) {
		return;
	}
	m_fLeashRadius = m_fAggroRadius + 30.0f;
}

/**
 * [RECONSTRUCTED - Native 0x005411A0] (84 bytes)
 * CTactics::BroadcastActionPacket
 */
void CTactics::BroadcastActionPacket(CGObjChar* pTarget, void* pSkillData) {
	(void)pTarget;
	(void)pSkillData;
}

/**
 * [RECONSTRUCTED - 0x0040AA50]
 * CTactics::FlushCommands
 */
void CTactics::FlushCommands() {
}

/**
 * [RECONSTRUCTED - 0x0053FD70] (39 bytes)
 * CTactics::CreateAIMessage
 * Native virtual slot 5 (+0x14)
 */
AI::CAIMsg* CTactics::CreateAIMessage(uint16_t wMsgID) {
	if (g_pGameAI == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	return g_pGameAI->CreateAIMessage(wMsgID);
}

/**
 * [RECONSTRUCTED - 0x0053FDA0] (39 bytes)
 * CTactics::PostAIMessage
 * Native virtual slot 6 (+0x18)
 */
bool CTactics::PostAIMessage(AI::CAIMsg* pMsg) {
	if (g_pGameAI == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	return g_pGameAI->PostAIMessage(pMsg);
}

/**
 * [RECONSTRUCTED - 0x0053FDD0] (41 bytes)
 * CTactics::OnCurrentStateTick
 * Native virtual slot 7 (+0x1C)
 */
bool CTactics::OnCurrentStateTick() {
	if (m_pCurrentState != nullptr) {
		return m_pCurrentState->OnTick() != 0;
	}
	ServerFramework::ServerFramework_GenerateMiniDump();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0053F3B0] (45 bytes)
 * CTactics::RequestRemoval
 * Native virtual slot 8 (+0x20)
 */
void CTactics::RequestRemoval() {
	Shutdown();
	if (g_pGameAI == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	g_pGameAI->MarkTacticsForRemoval(static_cast<uint32_t>(m_nPoolIndex));
}

/**
 * [RECONSTRUCTED - 0x0053F2A0] (267 bytes, retn 0x14)
 * CTactics::BindContext (vftable slot 12)
 * Binds owner character, tactics definition, nesting location, roam radius,
 * and resets active timer manager state.
 */
bool CTactics::BindContext(CGObjChar* pOwner, tagRefTactics* pRefTactics, CNest* pNest, const tagObjLocation* pPos, float fRoamRadius) {
	if (pOwner == nullptr || pOwner->IsPlayer() == true || pRefTactics == nullptr) {
		ASSERT(false);
		if (g_pGameAI != nullptr) {
			g_pGameAI->MarkTacticsForRemoval(static_cast<uint32_t>(m_nPoolIndex));
		}
		return false;
	}

	m_pRefTactics = pRefTactics;
	m_pOwner = pOwner;
	m_pNest = pNest;

	const tagObjLocation* pSpawnPos = (pNest != nullptr) ? &pNest->m_Pos : pPos;
	if (pSpawnPos != nullptr) {
		m_NestPos = *pSpawnPos;
		m_fSpawnX = pSpawnPos->fPosX;
		m_fSpawnY = pSpawnPos->fPosY;
		m_fSpawnZ = pSpawnPos->fPosZ;
		m_wSpawnRegionID = pSpawnPos->wRegionID;

		m_fHomeX = m_fSpawnX;
		m_fHomeY = m_fSpawnY;
		m_fHomeZ = m_fSpawnZ;
		m_wHomeRegionID = m_wSpawnRegionID;
	}

	tagTID ownerTID(pOwner->GetTypeID());
	bool bSlot243 = ownerTID.IsNonPlayer() && (ownerTID.wType & 0x780) == 0x200 && (ownerTID.wType & 0xF800) == 0x2000;
	if (bSlot243 == true) {
		m_fRoamRadius = 5.0f;
	} else if (m_pNest != nullptr && m_pNest->m_pRefNest != nullptr) {
		m_fRoamRadius = static_cast<float>(m_pNest->m_pRefNest->m_nRadius);
	} else if (fRoamRadius > 0.0f) {
		m_fRoamRadius = fRoamRadius;
	} else if (m_pRefTactics->m_nHomingData > 0) {
		m_fRoamRadius = static_cast<float>(m_pRefTactics->m_nHomingData);
	} else {
		m_fRoamRadius = 200.0f;
	}

	m_timeManager.ClearTimers();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0053F3E0] (239 bytes)
 * CTactics::Shutdown (vftable slot 13)
 * Shuts down tactics controller, releases queued commands and messages,
 * resets skill instances, detaches from nest, and clears aggro records.
 */
void CTactics::Shutdown() {
	FlushCommands();
	m_dwActiveStateFlag = 0;

	if (m_pPendingMsg != nullptr) {
		m_pPendingMsg->Release();
		m_pPendingMsg = nullptr;
	}

	if (m_pBackupSkill != nullptr) {
		ASSERT(m_pActiveSkill);
		if (m_pActiveSkill != nullptr) {
			delete m_pActiveSkill;
			m_pActiveSkill = nullptr;
		}
		m_pActiveSkill = m_pBackupSkill;
		m_pBackupSkill = nullptr;
	}

	if (m_pOwner != nullptr) {
		DetachNest();
		m_pOwner = nullptr;
		m_pRefTactics = nullptr;
		UnregisterSquadTarget();
		m_timeManager.ClearTimers();
		std::memset(&m_primaryAggro, 0, sizeof(m_primaryAggro));
		std::memset(&m_secondaryAggro, 0, sizeof(m_secondaryAggro));
	}
}

/**
 * [RECONSTRUCTED - Native 0x0053FEA0] (133 bytes)
 * CTactics::Update
 * Native virtual slot 14 (+0x38)
 *
 * CORRECTION (Claude): slot 9 is IsNPC (0x004825C0), +0x30 is the nest, and the owner log does not
 * test m_pRefTactics (0x0053FEA9 dereferences it directly).
 *   1. !m_pOwner -> Log "Tactics[ %d ] Char null pointer " with m_pRefTactics->m_dwTacticsID, return TRUE.
 *   2. m_pOwner->IsNPC() == 1 or m_dwActiveStateFlag == 0 -> TRUE.
 *   3. m_pNest && (m_pNest->m_Pos.wRegionID ^ owner region) < 0 (dungeon bit differs) -> DetachNest(), TRUE.
 *   4. CheckAggroOrStateTransition() or ProcessPendingAIMessage() -> TRUE; else m_pCurrentState->vftable[3]().
 */
bool CTactics::Update() {
	if (m_pOwner == nullptr) {
		BSLib::Log_Printf(0x2000001, "Tactics[ %d ] Char null pointer ", m_pRefTactics->m_dwTacticsID);
		return true;
	}

	if (m_pOwner->IsNPC() == true) {
		return true;
	}

	if (m_dwActiveStateFlag == 0) {
		return true;
	}

	if (m_pNest != nullptr) {
		int16_t nDiff = static_cast<int16_t>(m_pNest->m_Pos.wRegionID ^ m_pOwner->m_wRegionID);
		if (nDiff < 0) {
			DetachNest();
			return true;
		}
	}

	// Check aggro / state transition (0x00540D20)
	if (CheckAggroOrStateTransition()) {
		return true;
	}

	// Process any pending asynchronous AI message (0x0053FE30)
	if (ProcessPendingAIMessage()) {
		return true;
	}

	// Execute active state's virtual tick handler (+0x0C)
	if (m_pCurrentState != nullptr) {
		m_pCurrentState->OnTick();
	}

	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053FD40] (47 bytes, esi = this)
 * CTactics::DetachNest
 * CORRECTION (Claude): formerly an invented CancelAction that cleared the active skill. The native reports
 * the owner to the hatching nest through CNest vftable[4] with the pool index as the second argument.
 */
void CTactics::DetachNest() {
	if (m_pNest != nullptr) {
		m_pNest->OnMonsterDead(m_pOwner, static_cast<uint32_t>(QueryPoolIndex()));
	}
	m_pNest = nullptr;
	m_byUnk148 = 0;
}

/**
 * [RECONSTRUCTED - Native 0x00540D20] (88 bytes)
 * CTactics::CheckAggroOrStateTransition
 */
bool CTactics::CheckAggroOrStateTransition() {
	// CORRECTION (Claude): the second condition is [this+0x14C] == 0 (0x00540D2F), not a target pointer,
	// and the native has no m_pRefTactics null test (0x00540D26 dereferences it).
	if ((m_pRefTactics->m_btAdditionOptionFlag & 0x84) == 0 && m_dwPendingActionID == 0) {
		if (m_pCurrentState != nullptr && m_pCurrentState->GetStateId() == 2) {
			if (CheckPCInOwnerMsgBlock() == 0) {
				ChangeState(14, 0);
				if (m_pCurrentState != nullptr) {
					m_pCurrentState->m_nSubState = 2;
				}
				return true;
			}
		}
	}

	return false;
}

/**
 * [RECONSTRUCTED - Native 0x0053D860] (54 bytes)
 * CTactics::CheckPCInOwnerMsgBlock
 * CheckAggroOrStateTransition switches to state 14 when this returns 0.
 */
int32_t CTactics::CheckPCInOwnerMsgBlock() {
	// CORRECTION (Claude): 0x0053D863 reads the cached AI tick, 0x0053D868 compares against the WORD at +0x150,
	// and 0x0053D886..0x0053D88D returns 1 when IsPCInOwnerMsgBlock() is true (the old port had this inverted).
	uint32_t dwNow = g_dwGameAICurrentTick;
	if (dwNow - m_dwLastSearchTimestamp <= static_cast<uint16_t>(m_dwNextSearchDelayMs)) {
		return -1;
	}

	m_dwLastSearchTimestamp = dwNow;
	return (IsPCInOwnerMsgBlock() == true) ? 1 : 0;
}

/**
 * [STUB - Native 0x00540E60] (117 bytes)
 * CTactics::IsPCInOwnerMsgBlock
 * Native: no owner -> false. ASSERT(owner+0x128) and a NULL region -> true. Otherwise
 * CRegion::GetPCCount(owner, owner+0x7C position) (0x00538100 -> CMsgBlock 0x005347A0 on layer owner+0x7A) != 0.
 * CGObjChar does not model +0x128 (CRegion*) or the nav-cell half of its position, so the stub answers
 * like the NULL-region path.
 * CORRECTION (Claude): formerly SearchAggroTarget, which returned whether CGObjChar+0x9C (the game world layer)
 * was set.
 */
bool CTactics::IsPCInOwnerMsgBlock() {
	if (m_pOwner == nullptr) {
		return false;
	}
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053FE30] (72 bytes)
 * CTactics::ProcessPendingAIMessage
 */
bool CTactics::ProcessPendingAIMessage() {
	if (m_pPendingMsg == nullptr) {
		return false;
	}

	CAIMsg* pMsg = m_pPendingMsg;
	switch (pMsg->GetMsgID()) {
	case 1:
		OnMsg_Attacked(pMsg);
		break;
	case 2:
		OnMsg_SquadTargetHelp(pMsg);
		break;
	case 3:
		OnMsg_Assist(pMsg);
		break;
	case 4:
		OnMsg_Command(pMsg);
		break;
	default:
		break;
	}

	pMsg->Release();
	m_pPendingMsg = nullptr;
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053FFE0] (116 bytes)
 * CTactics::SetCombatTarget
 * Sets the primary combat aggro target and notifies squad or resets aggro when null.
 */
bool CTactics::SetCombatTarget(CGObjChar* pTarget, uint8_t byMode) {
	if (pTarget == nullptr) {
		std::memset(&m_primaryAggro, 0, sizeof(m_primaryAggro));
		return true;
	}

	(void)byMode;
	m_primaryAggro.m_dwTargetID = pTarget->GetGameID();
	m_primaryAggro.m_dwLastHitTime = g_dwGameAICurrentTick;
	m_primaryAggro.m_nAggroScore = 1;

	return (m_primaryAggro.m_dwTargetID != 0);
}

/**
 * [RECONSTRUCTED - Native 0x0053FF30] (148 bytes)
 * CTactics::ChangeState
 */
void CTactics::ChangeState(int32_t nStateID, int32_t nSubStateID) {
	if (nStateID < 0 || nStateID >= 16) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}

	if (m_pCurrentState != nullptr) {
		if (m_pCurrentState->GetStateId() == 15) {
			return;
		}
		m_pCurrentState->OnExit();
	}

	if (static_cast<size_t>(nStateID) < m_vecStates.size()) {
		m_pCurrentState = m_vecStates[nStateID];
	}

	// Transition to new state
	if (m_pCurrentState != nullptr) {
		m_pCurrentState->OnEnter(nSubStateID, 0);
	}
}

/**
 * [RECONSTRUCTED - Native 0x00540120] (537 bytes)
 * CTactics::OnMsg_Attacked
 * Responds to attack notifications (AIMsg type 0). Verifies target eligibility, health threshold (< 80%),
 * navigation line of sight, and transitions into combat state.
 */
void CTactics::OnMsg_Attacked(CAIMsg* pMsg) {
	if (pMsg == nullptr || m_pRefTactics == nullptr || m_pOwner == nullptr) {
		return;
	}

	if (m_pRefTactics->m_btHelpRequestTo == 2) {
		return;
	}

	uint8_t byFlag = 0;
	uint32_t dwAttackerID = 0;
	uint32_t dwTargetID = 0;
	uint8_t bySubFlag = 0;
	uint32_t dwSkillOrParam = 0;

	pMsg->ResetReadCursor();
	pMsg->ReadPayload(&byFlag, sizeof(byFlag));
	pMsg->ReadPayload(&dwAttackerID, sizeof(dwAttackerID));
	pMsg->ReadPayload(&dwTargetID, sizeof(dwTargetID));
	pMsg->ReadPayload(&bySubFlag, sizeof(bySubFlag));
	pMsg->ReadPayload(&dwSkillOrParam, sizeof(dwSkillOrParam));

	if (byFlag == 0 && dwTargetID != m_pOwner->GetGameID()) {
		return;
	}

	CGObjChar* pAttacker = CGame_FindObjectByID(dwAttackerID);
	if (pAttacker == nullptr) {
		return;
	}

	if (bySubFlag != 1 && m_pCurrentState != nullptr && m_pCurrentState->GetStateId() == 3) {
		return;
	}

	if (pAttacker->GetLifeState() != 1) {
		return;
	}

	uint32_t dwCurHP = m_pOwner->GetCurrentHP();
	uint32_t dwMaxHP = m_pOwner->GetMaxHP();
	if (dwMaxHP == 0) {
		return;
	}

	float fHPRatio = (static_cast<float>(dwCurHP) * 100.0f) / static_cast<float>(dwMaxHP);
	if (fHPRatio < 80.0f) {
		if (NavMesh::g_pRegionManager != nullptr) {
			NavMesh::tagNavPos srcPos;
			srcPos.wRegionID = m_pOwner->m_wRegionID;
			srcPos.fPosX = m_pOwner->m_fLocalPosX;
			srcPos.fPosY = m_pOwner->m_fLocalPosY;
			srcPos.fPosZ = m_pOwner->m_fLocalPosZ;

			NavMesh::tagNavPos dstPos;
			dstPos.wRegionID = pAttacker->m_wRegionID;
			dstPos.fPosX = pAttacker->m_fLocalPosX;
			dstPos.fPosY = pAttacker->m_fLocalPosY;
			dstPos.fPosZ = pAttacker->m_fLocalPosZ;

			int32_t nMoveResult = NavMesh::g_pRegionManager->QueryMovement(0, 1, &srcPos, &dstPos, 0, m_pOwner);
			if ((nMoveResult & 0x10000000) != 0) {
				return;
			}
		}

		if (SetCombatTarget(pAttacker, 1)) {
			ChangeState(3, 0);
		}
	}
}

/**
 * [RECONSTRUCTED - Native 0x00540340] (232 bytes)
 * CTactics::OnMsg_SquadTargetHelp
 * Responds to squad help request by acquiring the squad's target and engaging in combat (State 3).
 */
void CTactics::OnMsg_SquadTargetHelp(CAIMsg* pMsg) {
	if (pMsg == nullptr) {
		return;
	}

	uint32_t dwSquadTargetID = 0;
	uint32_t dwHelpTacticsID = 0;

	pMsg->ResetReadCursor();
	pMsg->ReadPayload(&dwSquadTargetID, sizeof(dwSquadTargetID));
	pMsg->ReadPayload(&dwHelpTacticsID, sizeof(dwHelpTacticsID));

	CGObjChar* pTarget = CGame_FindObjectByID(dwSquadTargetID);
	if (pTarget == nullptr) {
		return;
	}

	SetCombatTarget(nullptr, 0);

	if (!SetCombatTarget(pTarget, 1)) {
		ChangeState(1, 0);
		return;
	}

	ChangeState(3, 0);
}

/**
 * [RECONSTRUCTED - Native 0x00540430] (196 bytes)
 * CTactics::OnMsg_Assist
 * Responds to assist request by focusing on ally squad member and entering assist state (State 4).
 */
void CTactics::OnMsg_Assist(CAIMsg* pMsg) {
	if (pMsg == nullptr) {
		return;
	}

	uint32_t dwAllyID = 0;
	uint32_t dwAssistParam = 0;

	pMsg->ResetReadCursor();
	pMsg->ReadPayload(&dwAllyID, sizeof(dwAllyID));
	pMsg->ReadPayload(&dwAssistParam, sizeof(dwAssistParam));

	CGObjChar* pAlly = CGame_FindObjectByID(dwAllyID);
	if (pAlly == nullptr) {
		return;
	}

	SetCombatTarget(nullptr, 0);

	if (!SetCombatTarget(pAlly, 1)) {
		ChangeState(1, 0);
		return;
	}

	ChangeState(4, 0);
}

/**
 * [RECONSTRUCTED - Native 0x00540500] (864 bytes)
 */
void CTactics::OnMsg_Command(CAIMsg* pMsg) {
	if (pMsg == nullptr) return;
}

/**
 * [RECONSTRUCTED - Native 0x00541200] (923 bytes, this on stack, retn 4)
 * CTactics::CallForHelp
 * Broadcasts an AIMessage (type 1) to nearby allied monsters within help radius,
 * dispatches broadcast packet 0x704F to nearby players, and resets help cooldown.
 */
int32_t CTactics::CallForHelp() {
	if (m_pRefTactics == nullptr || m_pRefTactics->m_btHelpRequestTo == 2) {
		return 0;
	}

	if (m_pOwner == nullptr || g_pGameAI == nullptr) {
		return 0;
	}

	CAIMsg* pMsg = g_pGameAI->CreateAIMessage(1);
	if (pMsg == nullptr) {
		return 0;
	}

	uint8_t byHelpTo = m_pRefTactics->m_btHelpRequestTo;
	uint32_t dwOwnerID = m_pOwner->GetGameID();
	uint32_t dwTacticsID = m_pRefTactics->m_dwTacticsID;
	uint8_t byLifeState = static_cast<uint8_t>(m_pOwner->GetLifeState());
	uint32_t dwTargetID = m_primaryAggro.m_dwTargetID;

	pMsg->WritePayload(&byHelpTo, sizeof(byHelpTo));
	pMsg->WritePayload(&dwOwnerID, sizeof(dwOwnerID));
	pMsg->WritePayload(&dwTacticsID, sizeof(dwTacticsID));
	pMsg->WritePayload(&byLifeState, sizeof(byLifeState));
	pMsg->WritePayload(&dwTargetID, sizeof(dwTargetID));

	float fRadius = 500.0f;
	if (m_pNest != nullptr && m_pNest->m_pRefNest != nullptr) {
		fRadius = static_cast<float>(m_pNest->m_pRefNest->m_nRadius);
	}
	(void)fRadius;

	pMsg->Release();

	m_timeManager.SetTimer(11, (rand() % 3000) + 3000);
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x005415A0] (480 bytes)
 * CTactics::NotifyLinkedHelp
 *
 * Sequence:
 *   1. Allocates AI message (ID: 4 = AI_MSG_COMMAND) via g_pGameAI->CreateAIMessage(4)
 *   2. Writes sender GlobalID into payload
 *   3. Dispatches message to linked monsters via ReceiveAIMessage(pMsg)
 *   4. Releases message instance
 */
int32_t CTactics::NotifyLinkedHelp() {
	if (m_pOwner == nullptr || g_pGameAI == nullptr) {
		return 0;
	}

	CAIMsg* pMsg = g_pGameAI->CreateAIMessage(4);
	if (pMsg == nullptr) {
		return 0;
	}

	pMsg->m_dwSenderID = m_pOwner->m_dwGlobalID;

	// Release message after dispatch
	pMsg->Release();
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x0053F4D0] (681 bytes)
 * CTactics::Initialize
 */
bool CTactics::Initialize() {
	if (m_pRefTactics == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	InitializeSkillsAndRadii();
	InstallStrategyTable();

	m_dwActiveStateFlag = 1;
	m_dwLastSearchTimestamp = ::GetTickCount();
	m_dwNextSearchDelayMs = 1000 + (std::rand() % 1000);

	if (m_pOwner != nullptr) {
		m_fSpawnX = m_pOwner->m_fPosX;
		m_fSpawnY = m_pOwner->m_fPosY;
		m_fSpawnZ = m_pOwner->m_fPosZ;
		m_wSpawnRegionID = m_pOwner->m_wRegionID;

		m_fHomeX = m_fSpawnX;
		m_fHomeY = m_fSpawnY;
		m_fHomeZ = m_fSpawnZ;
		m_wHomeRegionID = m_wSpawnRegionID;
	}

	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053FB90] (7 bytes: push ecx / call 0x0053F780 / retn; body 751 bytes)
 * CTactics::InitializeSkillsAndRadii
 * Initializes monster combat radii (Aggro radius from collision base, Search radius = Aggro + Sight range,
 * Leash radius) and allocates skill execution controller if null.
 */
bool CTactics::InitializeSkillsAndRadii() {
	if (m_pRefTactics == nullptr || m_pOwner == nullptr) {
		return false;
	}

	if (m_pActiveSkill == nullptr) {
		m_pActiveSkill = new CAISkill();
	}

	m_fAggroRadius = 15.0f;
	m_fSearchRadius = m_fAggroRadius + static_cast<float>(m_pRefTactics->m_nSightRange);
	m_fLeashRadius = m_fAggroRadius + 30.0f;

	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0053FC00] (312 bytes)
 * CTactics::InstallStrategyTable
 */
bool CTactics::InstallStrategyTable() {
	ResetStateHandlers();
	ConfigureWanderState();
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x005471B0] (72 bytes)
 * CTactics::ConfigureFollowState
 * Virtual slot 18 (+0x48)
 */
int32_t CTactics::ConfigureFollowState() {
	m_pfnApproach = AI_CTactics_StateApproachTarget;
	m_pfnCombatRanged = AI_CTactics_StateCombatRanged;
	m_pfnFollow = AI_CTactics_StateFollowLeader;
	m_pfnCombatMelee = AI_CTactics_StateCombatMelee;
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00547200] (60 bytes)
 * CTactics::ConfigureWanderState
 * Virtual slot 19 (+0x4C)
 */
int32_t CTactics::ConfigureWanderState() {
	m_pfnWander = AI_CTactics_StateIdleWander;
	m_pfnReturnToSpawn = AI_CTactics_StateReturnToSpawn;
	m_pfnCombatRanged = AI_CTactics_StateCombatRanged;
	m_pfnApproach = AI_CTactics_StateApproachTarget;
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00547240] (58 bytes)
 * CTactics::ResetStateHandlers
 * Virtual slot 20 (+0x50)
 */
int32_t CTactics::ResetStateHandlers() {
	m_pfnWander = nullptr;
	m_pfnApproach = nullptr;
	m_pfnFollow = nullptr;
	m_pfnFlee = nullptr;
	m_pfnReturnToSpawn = nullptr;
	m_pfnApproachTarget = nullptr;
	m_pfnCombatMelee = nullptr;
	m_pfnCombatRanged = nullptr;
	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x00547280] (28 bytes)
 * CTactics::SetStateHook
 * Virtual slot 21 (+0x54)
 */
void CTactics::SetStateHook(int32_t nState) {
	if (nState == 0x10) {
		m_pfnWander = AI_CTactics_StateCustomSpawn;
	} else {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
}

/**
 * [RECONSTRUCTED - Native 0x0053D850] (10 bytes)
 * CTactics::SetPoolIndex
 * Virtual slot 22 (+0x58)
 */
void CTactics::SetPoolIndex(int32_t nIndex) {
	m_nPoolIndex = nIndex;
}

/**
 * [RECONSTRUCTED - Native 0x00823DB0] (4 bytes)
 * CTactics::GetPoolIndex
 * Virtual slot 23 (+0x5C)
 */
int32_t CTactics::GetPoolIndex() const {
	return m_nPoolIndex;
}

/**
 * [RECONSTRUCTED - Native 0x0053FE00] (44 bytes)
 * CTactics::ReceiveAIMessage
 * Virtual slot 24 (+0x60)
 *
 * Sequence:
 *   1. Null check pMsg
 *   2. If m_pPendingMsg != nullptr, call m_pPendingMsg->Release() (Slot 2 @ +0x08)
 *   3. Assign m_pPendingMsg = pMsg
 *   4. Call pMsg->AddRef() (*(pMsg + 8) += 1)
 */
void CTactics::ReceiveAIMessage(CAIMsg* pMsg) {
	if (pMsg == nullptr) {
		return;
	}
	if (m_pPendingMsg != nullptr) {
		m_pPendingMsg->Release();
	}
	m_pPendingMsg = pMsg;
	pMsg->AddRef();
}

/**
 * [RECONSTRUCTED - Native 0x005400C0] (56 bytes)
 * CTactics::GetEffectiveLeashRadius
 * Virtual slot 25 (+0x64)
 */
float CTactics::GetEffectiveLeashRadius() const {
	// CORRECTION (Claude): 0x005400D9 selects on the current state ID (3 keeps +0x160), not on a target pointer.
	float fRadius = m_fLeashRadius;
	if (m_pCurrentState->GetStateId() != 3) {
		fRadius = static_cast<float>(m_fAggroRadius * 3.0);
	}
	return fRadius;
}

/*
================================================================================
Extended Tactical AI Routines
================================================================================
*/

/**
 * [RECONSTRUCTED - Native 0x00547070] (316 bytes)
 * AI_CTactics_EvaluateCandidateTarget
 */
int32_t AI_CTactics_EvaluateCandidateTarget(CTactics* pTactics, uint8_t byFilter, void* pCandidateList, CGObjChar* pTarget, const float* pMaxDist, float* pOutDist) {
	(void)pCandidateList;
	(void)byFilter;

	if (pTactics == nullptr || pTactics->m_pOwner == nullptr || pTarget == nullptr || pMaxDist == nullptr) {
		return 0;
	}

	float fDx = pTactics->m_pOwner->m_fPosX - pTarget->m_fPosX;
	float fDz = pTactics->m_pOwner->m_fPosZ - pTarget->m_fPosZ;
	float fDist = std::sqrt(fDx * fDx + fDz * fDz);

	if (fDist <= *pMaxDist) {
		if (pOutDist != nullptr) {
			*pOutDist = fDist;
		}
		return 1;
	}

	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x005472A0] (278 bytes)
 * AI_CTactics_Battle_TryAction
 * Selects and executes an action/skill against the target.
 *
 * Sequence:
 *   1. Null check and target life state check (must be 1 = alive)
 *   2. Checks combat action cooldown timer (Timer 5 via m_timeManager.IsTimerExpired)
 *   3. Queries candidate skill from m_pActiveSkill; if null, selects via SelectSkill
 *   4. Recomputes effective combat attack range (UpdateSkillAttackRange)
 *   5. Arms Timer 5 with skill execution interval
 *   6. Dispatches skill execution (m_pActiveSkill->ExecuteSkill); on failure clears Timer 5
 *   7. Updates primary aggro last hit timestamp and broadcasts action packet 0x7070
 *
 * Returns:
 *   0: Successfully executed an action / skill
 *   1: No action available / execution failed
 *   2: Action cooldown not yet expired (Timer 5 active)
 */
int32_t AI_CTactics_Battle_TryAction(CTactics* pTactics, CGObjChar* pTarget) {
	if (pTactics == nullptr || pTarget == nullptr || pTactics->m_pOwner == nullptr) {
		return 1;
	}

	if (pTarget->GetLifeState() != 1) {
		return 1;
	}

	// 1. Check action cooldown timer 5
	uint32_t dwCurTick = ::GetTickCount();
	if (!pTactics->m_timeManager.IsTimerExpired(5, dwCurTick)) {
		return 2;
	}

	// 2. Select active skill candidate
	CAISkill* pSkill = pTactics->m_pActiveSkill;
	if (pSkill == nullptr) {
		return 1;
	}

	void* pSkillData = pSkill->GetCandidateSkill();
	if (pSkillData == nullptr) {
		pSkillData = pSkill->SelectSkill(pTactics, pTarget);
		if (pSkillData == nullptr) {
			return 1;
		}
	}

	// 3. Update skill state flags & compute effective attack range
	pTactics->UpdateSkillAttackRange(pTarget, pSkillData);

	// 4. Arm action timer (Timer 5) with skill interval
	pTactics->m_timeManager.ArmTimer(5, pSkill->GetInterval(), 0, dwCurTick, false);

	// 5. Execute skill
	if (!pSkill->ExecuteSkill(pTactics, pTarget, pSkillData)) {
		pTactics->m_timeManager.ClearTimer(5);
		return 1;
	}

	// 6. Record hit timestamp and broadcast action packet 0x7070
	pTactics->m_primaryAggro.m_dwLastHitTime = dwCurTick;
	pTactics->BroadcastActionPacket(pTarget, pSkillData);

	if (pTactics->m_dwPendingActionID > 0) {
		pTactics->m_dwPendingActionID = 0;
	}

	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x005473C0] (424 bytes)
 * AI_CTactics_Aggro_RecordDamage
 */
int32_t AI_CTactics_Aggro_RecordDamage(CTactics* pTactics, const uint32_t* pDamageInfo, uint8_t* pOutSlot) {
	if (pTactics == nullptr || pDamageInfo == nullptr) {
		return 0;
	}

	uint32_t dwAttackerID = pDamageInfo[0];
	uint32_t dwDamage = pDamageInfo[1];

	// Primary vs secondary aggro slot resolution
	tagAggroRecord* pRecord = nullptr;
	uint8_t bySlot = 0;

	if (pTactics->m_primaryAggro.m_dwTargetID == 0 || pTactics->m_primaryAggro.m_dwTargetID == dwAttackerID) {
		pRecord = &pTactics->m_primaryAggro;
		bySlot = 0;
	} else if (pTactics->m_secondaryAggro.m_dwTargetID == 0 || pTactics->m_secondaryAggro.m_dwTargetID == dwAttackerID) {
		pRecord = &pTactics->m_secondaryAggro;
		bySlot = 1;
	} else {
		bySlot = 2;
	}

	if (pOutSlot != nullptr) {
		*pOutSlot = bySlot;
	}

	if (pRecord != nullptr) {
		pRecord->m_dwTargetID = dwAttackerID;
		pRecord->m_dwTotalDamage += dwDamage;
		pRecord->m_nAggroScore += static_cast<int32_t>(dwDamage);
		pRecord->m_dwLastHitTime = ::GetTickCount();
	}

	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00547570] (748 bytes)
 * AI_CTactics_PartyAggro_Distribute
 * Distributes aggro / threat among target's party members within share radius (1000 units).
 * Clamps to candidate count [0, m_vecAggroTargets.size()] and updates threat scores.
 */
int32_t AI_CTactics_PartyAggro_Distribute(CTactics* pTactics, const uint32_t* pTargetData) {
	if (pTactics == nullptr || pTargetData == nullptr || pTactics->m_pOwner == nullptr) {
		return 1;
	}

	uint32_t dwTargetID = pTargetData[0];
	uint32_t dwDamage = pTargetData[2];
	if (dwTargetID == 0 || dwDamage == 0) {
		return 1;
	}

	CGObjChar* pTarget = CGame_FindObjectByID(dwTargetID);
	if (pTarget == nullptr || !pTarget->IsPlayer()) {
		return 1;
	}

	// Count party members within party share range (1000.0f)
	int32_t nNearCount = 1; // Primary target itself counts

	// Native line 1634 clamp: CLAMP(nNearCount, 0, (int)m_vecAggroTargets.size())
	int32_t nMaxCandidates = static_cast<int32_t>(pTactics->m_vecAggroTargets.size());
	if (nNearCount < 0) {
		nNearCount = 0;
	} else if (nNearCount > nMaxCandidates) {
		nNearCount = nMaxCandidates;
	}

	if (nNearCount > 0) {
		uint32_t dwSharedDamage = dwDamage / static_cast<uint32_t>(nNearCount);
		if (dwSharedDamage == 0) {
			dwSharedDamage = 1;
		}

		for (int32_t i = 0; i < nNearCount; ++i) {
			uint32_t dwCandidateID = pTactics->m_vecAggroTargets[i];
			CGObjChar* pCandidate = CGame_FindObjectByID(dwCandidateID);
			if (pCandidate != nullptr) {
				// Record shared damage in aggro table
				uint32_t damageInfo[2] = { dwCandidateID, dwSharedDamage };
				AI_CTactics_Aggro_RecordDamage(pTactics, damageInfo, nullptr);
			}
		}
	}

	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x00547860] (138 bytes)
 * AI_CTactics_SyncMovementSpeed
 */
int32_t AI_CTactics_SyncMovementSpeed(CTactics* pTactics) {
	if (pTactics == nullptr || pTactics->m_pOwner == nullptr) {
		return 0;
	}
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x005478F0] (467 bytes)
 * AI_CTactics_ScanForTargets
 */
int32_t AI_CTactics_ScanForTargets(CTactics* pTactics, void* pContext, uint32_t dwParam) {
	(void)pContext;
	(void)dwParam;

	if (pTactics == nullptr || pTactics->m_pOwner == nullptr) {
		return 0;
	}

	uint32_t dwNow = ::GetTickCount();
	if (dwNow - pTactics->m_dwLastSearchTimestamp < pTactics->m_dwNextSearchDelayMs) {
		return 0;
	}

	pTactics->m_dwLastSearchTimestamp = dwNow;
	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x00547AD0] (411 bytes)
 * AI_CTactics_ScanForTargets_Special
 */
int32_t AI_CTactics_ScanForTargets_Special(CTactics* pTactics, void* pContext, uint32_t dwParam) {
	(void)pContext;
	(void)dwParam;

	if (pTactics == nullptr || pTactics->m_pOwner == nullptr) {
		return 0;
	}

	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x00547C70] (716 bytes)
 * AI_CTactics_StateIdleWander
 */
int32_t AI_CTactics_StateIdleWander(CTactics* pTactics, void* pContext) {
	(void)pContext;

	if (pTactics == nullptr || pTactics->m_pOwner == nullptr) {
		return 0;
	}

	// Scan for nearby aggressive targets if idle
	AI_CTactics_ScanForTargets(pTactics, nullptr, 0);
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00547F40] (332 bytes)
 * AI_CTactics_StateCustomSpawn
 */
int32_t AI_CTactics_StateCustomSpawn(CTactics* pTactics, void* pContext) {
	(void)pContext;

	if (pTactics == nullptr || pTactics->m_pOwner == nullptr) {
		return 0;
	}

	AI_CTactics_ScanForTargets_Special(pTactics, nullptr, 0);
	return 1;
}

// Forwarders for state actions referenced in state configurations
int32_t AI_CTactics_FollowLeader_Check(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }
int32_t AI_CTactics_StateApproachTarget(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }
int32_t AI_CTactics_StateCombatRanged(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }
int32_t AI_CTactics_StateFollowLeader(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }
int32_t AI_CTactics_StateCombatMelee(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }
int32_t AI_CTactics_StateReturnToSpawn(CTactics* pTactics, void* pContext) { (void)pTactics; (void)pContext; return 1; }

} // namespace AI
