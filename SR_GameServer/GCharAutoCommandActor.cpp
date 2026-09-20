/**
 * ============================================================================
 * Silkroad Online - Character Auto Command Actor (client action queue)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GCharAutoCommandActor.cpp
 *
 * CORRECTION (Claude): rewritten from the machine code of 0x004AC910 - 0x004AF810. The previous
 * version sent the 0x7070 action as a client packet, deleted records the native code keeps,
 * collapsed the six-entry handler table into a switch, acknowledged with type 0 instead of 1 and
 * skipped the skill validation, learned-skill and pursuit steps.
 * ============================================================================
 */

#include "GCharAutoCommandActor.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "GItem.h"
#include "Formulae.h"
#include "GlobalPos.h"
#include "SkillManager.h"
#include "GameWorldMgr.h"
#include "GameWorld.h"
#include "skill/SkillGlobal.h"
#include "../ServerCommon/ReferenceData.h"
#include "../Common/Framework/CmdSource.h"
#include "../JMX_Library/BSLib/Msg.h"
#include "../JMX_Library/BSLib/Packet.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/ChunkAllocator.h"
#include <windows.h>
#include <cmath>

// Native static pool @ 0x00CCF0B0, constructed by 0x004AF630.
static CChunkAllocatorST<sAutoCommand> s_poolAutoCommand;

// ============================================================================
// sAutoCommand
// ============================================================================

/*
================
sAutoCommand::sAutoCommand
[RECONSTRUCTED - inlined by the block constructor 0x004AFD20]
================
*/
sAutoCommand::sAutoCommand()
	: m_bAllocated(0)
	, m_byType(0)
	, m_pRefSkill(nullptr)
	, m_dwDispelParam(0)
	, m_dwStateTick(0)
	, m_fMinRange(0.0f)
	, m_fMaxRange(0.0f)
	, m_dwState(0)
	, m_pChainSkill(nullptr)
	, m_dwChainStepTime(0)
	, m_dwLatencyBudget(500)
	, m_dwLatencyUsed(0)
	, m_pPreEngage(nullptr) {
}

/*
================
sAutoCommand::~sAutoCommand
[RECONSTRUCTED - 0x004AC980]
================
*/
sAutoCommand::~sAutoCommand() {
}

/*
================
sAutoCommand::Allocate
[RECONSTRUCTED - 0x004AC910] (12 bytes)
================
*/
sAutoCommand* sAutoCommand::Allocate() {
	sAutoCommand* pCommand = s_poolAutoCommand.Alloc();
	pCommand->m_bAllocated = 1;
	return pCommand;
}

/*
================
sAutoCommand::Release
[RECONSTRUCTED - 0x004AC920] (81 bytes)
================
*/
void sAutoCommand::Release(sAutoCommand*& pCommand) {
	if (pCommand == nullptr) {
		return;
	}
	if (pCommand->m_bAllocated != 0) {
		pCommand->m_bAllocated = 0;
		if (pCommand->m_pPreEngage != nullptr) {
			Skill::sSkillPreEngageData::Release(pCommand->m_pPreEngage);
		}
		pCommand->m_pRefSkill = nullptr;
		pCommand->m_dwDispelParam = 0;
		pCommand->m_pChainSkill = nullptr;
		pCommand->m_dwLatencyBudget = 500;
		pCommand->m_dwChainStepTime = 0;
		pCommand->m_dwLatencyUsed = 0;
		s_poolAutoCommand.Free(pCommand);
	}
	pCommand = nullptr;
}

/*
================
sAutoCommand::SetState
[RECONSTRUCTED - 0x004AC9E0] (13 bytes)
================
*/
void sAutoCommand::SetState(uint32_t dwState) {
	m_dwState = dwState;
	m_dwStateTick = ::GetTickCount();
}

// ============================================================================
// CGCharAutoCommandActor
// ============================================================================

/*
================
CGCharAutoCommandActor::CGCharAutoCommandActor
[RECONSTRUCTED - 0x004AC9F0] (152 bytes)
================
*/
CGCharAutoCommandActor::CGCharAutoCommandActor()
	: m_listCommands()
	, m_pOwner(nullptr)
	, m_dwLastUpdateTick(0)
	, m_dwLastCommandTick(::GetTickCount()) {
	m_apfnHandler[AUTO_COMMAND_ATTACK - 1] = &CGCharAutoCommandActor::Handler_Attack;
	m_apfnHandler[AUTO_COMMAND_PICKUP - 1] = &CGCharAutoCommandActor::Handler_Pickup;
	m_apfnHandler[AUTO_COMMAND_TRACE - 1]  = &CGCharAutoCommandActor::Handler_Trace;
	m_apfnHandler[AUTO_COMMAND_CAST - 1]   = &CGCharAutoCommandActor::Handler_Cast;
	m_apfnHandler[AUTO_COMMAND_DISPEL - 1] = &CGCharAutoCommandActor::Handler_Dispel;
	m_apfnHandler[5] = nullptr;
}

/*
================
CGCharAutoCommandActor::~CGCharAutoCommandActor
[RECONSTRUCTED - 0x004ACA90]
================
*/
CGCharAutoCommandActor::~CGCharAutoCommandActor() {
	for (std::list<sAutoCommand*>::iterator it = m_listCommands.begin(); it != m_listCommands.end(); ++it) {
		sAutoCommand::Release(*it);
	}
	m_listCommands.clear();
}

/*
================
CGCharAutoCommandActor::ProcessCommand
[RECONSTRUCTED - 0x004ACC40] (998 bytes)

0x7074: [request][type][skill id: cast, dispel][dispel param: dispel with Lnks][pre-engage data]
================
*/
void CGCharAutoCommandActor::ProcessCommand(CMsg* pMsg, int32_t bRefuse) {
	if (bRefuse != 0) {
		pMsg->Consume();
		SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
		return;
	}

	uint8_t byRequest = 0;
	*pMsg >> byRequest;

	if (byRequest == AUTO_COMMAND_REQUEST_CANCEL) {
		size_t nCount = m_listCommands.size();
		if (nCount == 0) {
			CancelActiveAction(1);
			SendResponse(AUTO_COMMAND_RESPONSE_FINISHED, 0);
			return;
		}
		if (nCount != 1) {
			sAutoCommand* pQueued = m_listCommands.back();
			m_listCommands.pop_back();
			sAutoCommand::Release(pQueued);
			SendResponse(AUTO_COMMAND_RESPONSE_FINISHED, 0);
			return;
		}
		const sAutoCommand* pFront = m_listCommands.front();
		if (pFront->m_byType == AUTO_COMMAND_CAST &&
			(pFront->m_dwState == AUTO_COMMAND_STATE_KEEP_UP || pFront->m_pChainSkill != nullptr)) {
			SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
			return;
		}
		CancelActiveAction(1);
		SendResponse(AUTO_COMMAND_RESPONSE_FINISHED, 0);
		return;
	}

	if (byRequest == AUTO_COMMAND_REQUEST_CANCEL_AND_EXECUTE) {
		CancelActiveAction(0);
	}

	uint8_t byType = 0;
	*pMsg >> byType;
	if (byType > AUTO_COMMAND_DISPEL) {
		ASSERT(false);
		pMsg->Consume();
		SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
		return;
	}

	if ((byType == AUTO_COMMAND_ATTACK || byType == AUTO_COMMAND_CAST) && m_pOwner->IsRidingTransport()) {
		pMsg->Consume();
		SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
		return;
	}

	sAutoCommand* pCommand = sAutoCommand::Allocate();
	pCommand->m_byType = byType;
	m_dwLastCommandTick = ::GetTickCount();

	const tagRefSkill* pRefSkill = nullptr;
	if (byType == AUTO_COMMAND_CAST || byType == AUTO_COMMAND_DISPEL) {
		uint32_t dwSkillID = 0;
		*pMsg >> dwSkillID;

		if (byType == AUTO_COMMAND_CAST) {
			pRefSkill = m_pOwner->GetSkillManager()->GetSkillData(dwSkillID);
		} else {
			ASSERT(g_pRefData != nullptr);
			pRefSkill = g_pRefData->FindSkill(dwSkillID);
			if (pRefSkill != nullptr && pRefSkill->pLnks != nullptr) {
				*pMsg >> pCommand->m_dwDispelParam;
			}
		}

		if (pRefSkill == nullptr) {
			sAutoCommand::Release(pCommand);
			SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
			return;
		}
		pCommand->m_pRefSkill = pRefSkill;
	}

	pCommand->m_pPreEngage = Skill::sSkillPreEngageData::Allocate();
	pCommand->m_pPreEngage->ReadFromMsg(pMsg, 1);
	if ((pCommand->m_pPreEngage->m_byTargetFlags & Skill::SKILL_TARGET_FLAG_OBJECT) != 0) {
		ASSERT(pCommand->m_pPreEngage->m_vecTargets.size() <= 1);
	}
	pCommand->SetState(AUTO_COMMAND_STATE_QUEUED);

	if (m_pOwner->GetBodyMode() == 6) {
		pCommand->m_pPreEngage->m_byTargetFlags |= Skill::SKILL_TARGET_FLAG_BODY_MODE_6;
	}

	if (pCommand->m_byType == AUTO_COMMAND_CAST) {
		if (pRefSkill->pBuffType == 0) {
			uint16_t wError = CheckSkillPreEngageCondition(m_pOwner, pCommand->m_pPreEngage, 0x37, pRefSkill);
			if (wError != 0) {
				m_pOwner->GetSkillManager()->SendSkillErrorResponseB070(wError);
				sAutoCommand::Release(pCommand);
				SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
				return;
			}
		}

		if (!m_listCommands.empty()) {
			const sAutoCommand* pFront = m_listCommands.front();
			if (pFront->m_byType == AUTO_COMMAND_CAST && pFront->m_pRefSkill->dwSkillID == pRefSkill->dwSkillID) {
				uint32_t dwState = pFront->m_dwState;
				if (dwState == AUTO_COMMAND_STATE_QUEUED || dwState == AUTO_COMMAND_STATE_APPROACH ||
					dwState == AUTO_COMMAND_STATE_EXECUTE || dwState == AUTO_COMMAND_STATE_KEEP_UP) {
					sAutoCommand::Release(pCommand);
					SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
					return;
				}
			}
		}
	} else if (pCommand->m_byType == AUTO_COMMAND_PICKUP) {
		if (!m_listCommands.empty()) {
			const sAutoCommand* pFront = m_listCommands.front();
			if (pFront->m_byType == AUTO_COMMAND_PICKUP &&
				pFront->m_pPreEngage->m_dwTargetObjID == pCommand->m_pPreEngage->m_dwTargetObjID) {
				sAutoCommand::Release(pCommand);
				if (m_pOwner->IsPlayer()) {
					SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, 0x4004);
				}
				return;
			}
		}
	}

	// ExecuteAction owns the record from here on: a refused record has already been released
	// (the native caller's second Release at 0x004ACFEB is a no-op on the cleared pool item).
	uint16_t wError = ExecuteAction(pCommand);
	if (wError != 0) {
		SendResponse(AUTO_COMMAND_RESPONSE_REFUSED, wError);
		return;
	}

	if (m_pOwner->IsPlayer()) {
		SendResponse(AUTO_COMMAND_RESPONSE_ACCEPTED, 0);
	}
}

/*
================
CGCharAutoCommandActor::SendResponse
[RECONSTRUCTED - 0x004AD270] (136 bytes)

0xB074: [type][queued command count][error code when refused]
================
*/
void CGCharAutoCommandActor::SendResponse(uint8_t byType, uint16_t wErrorCode) {
	if (!m_pOwner->IsPlayer()) {
		ASSERT(false);
		return;
	}

	CPacket* pPacket = m_pOwner->AllocMsgForPeer(0xB074);
	pPacket->WriteUint8(byType);
	pPacket->WriteUint8(static_cast<uint8_t>(m_listCommands.size()));
	if (byType == AUTO_COMMAND_RESPONSE_REFUSED) {
		pPacket->WriteUint16(wErrorCode);
	}
	m_pOwner->SendMsgToPeer(pPacket);
}

/*
================
CGCharAutoCommandActor::CancelActiveAction
[RECONSTRUCTED - 0x004AD320] (105 bytes)
================
*/
void CGCharAutoCommandActor::CancelActiveAction(int32_t bStopMovement) {
	if (!m_listCommands.empty()) {
		for (std::list<sAutoCommand*>::iterator it = m_listCommands.begin(); it != m_listCommands.end(); ++it) {
			sAutoCommand::Release(*it);
		}
		m_listCommands.clear();

		if (m_pOwner != nullptr && m_pOwner->IsPlayer()) {
			SendResponse(AUTO_COMMAND_RESPONSE_FINISHED, 0);
		}
	}

	if (m_pOwner != nullptr && m_pOwner->GetCmdSource() != nullptr) {
		m_pOwner->GetCmdSource()->OnCommandEvent(0x10, 0, 0);
		if (bStopMovement != 0) {
			m_pOwner->m_AutoNavigator.Stop(1);
		}
	}
}

/*
================
CGCharAutoCommandActor::OnActionFinish
[RECONSTRUCTED - 0x004AD390] (80 bytes)
================
*/
void CGCharAutoCommandActor::OnActionFinish() {
	if (m_listCommands.empty()) {
		return;
	}

	sAutoCommand* pFinished = m_listCommands.front();
	m_listCommands.pop_front();
	sAutoCommand::Release(pFinished);

	StopMovement(1);
	if (m_pOwner->IsPlayer()) {
		SendResponse(AUTO_COMMAND_RESPONSE_FINISHED, 0);
	}
}

/*
================
CGCharAutoCommandActor::ExecuteAction
[RECONSTRUCTED - 0x004AD630] (322 bytes)

At most two records are kept: the running one and the one queued behind it.
================
*/
uint16_t CGCharAutoCommandActor::ExecuteAction(sAutoCommand* pCommand) {
	ASSERT(pCommand != nullptr);

	uint16_t wError = 0;
	if (CheckAndDispatchAction(pCommand, &wError) != 0) {
		sAutoCommand::Release(pCommand);
		return wError;
	}

	size_t nCount = m_listCommands.size();
	if (nCount == 0) {
		m_listCommands.push_back(pCommand);
		return 0;
	}

	const sAutoCommand* pFront = m_listCommands.front();
	if (pFront->m_byType == AUTO_COMMAND_TRACE) {
		m_pOwner->m_AutoNavigator.Stop(1);
		for (std::list<sAutoCommand*>::iterator it = m_listCommands.begin(); it != m_listCommands.end(); ++it) {
			sAutoCommand::Release(*it);
		}
		m_listCommands.clear();
		m_listCommands.push_back(pCommand);
		return 0;
	}

	switch (pFront->m_dwState) {
	case AUTO_COMMAND_STATE_QUEUED:
	case AUTO_COMMAND_STATE_APPROACH:
		for (std::list<sAutoCommand*>::iterator it = m_listCommands.begin(); it != m_listCommands.end(); ++it) {
			sAutoCommand::Release(*it);
		}
		m_listCommands.clear();
		m_listCommands.push_back(pCommand);
		return 0;

	case AUTO_COMMAND_STATE_EXECUTE:
	case AUTO_COMMAND_STATE_KEEP_UP:
		if (nCount == 2) {
			sAutoCommand* pQueued = m_listCommands.back();
			m_listCommands.pop_back();
			sAutoCommand::Release(pQueued);
		}
		m_listCommands.push_back(pCommand);
		return 0;

	default:
		// Native returns 1 without releasing; the caller releases (0x004ACFEB / 0x004AEDDF).
		sAutoCommand::Release(pCommand);
		return 1;
	}
}

/*
================
CGCharAutoCommandActor::CheckAndDispatchAction
[RECONSTRUCTED - 0x004AD870] (106 bytes)

Skills that are not cast at a target (+0x65 != 2) run immediately: BEGIN, then EXECUTE posts 0x7070.
Returning 1 makes ExecuteAction drop the record instead of queueing it.
================
*/
int32_t CGCharAutoCommandActor::CheckAndDispatchAction(sAutoCommand* pCommand, uint16_t* pwErrorCode) {
	if (pCommand->m_byType != AUTO_COMMAND_CAST) {
		return 0;
	}
	if (pCommand->m_pRefSkill == nullptr) {
		*pwErrorCode = 0x4002;
		return 1;
	}
	if (pCommand->m_pRefSkill->byCastType == 2) {
		return 0;
	}

	pCommand->SetState(AUTO_COMMAND_STATE_BEGIN);
	PFN_HANDLER pfnHandler = m_apfnHandler[pCommand->m_byType - 1];
	if ((this->*pfnHandler)(pCommand, pwErrorCode) == 0) {
		*pwErrorCode = 0x4004;
		return 1;
	}

	pCommand->SetState(AUTO_COMMAND_STATE_EXECUTE);
	pfnHandler = m_apfnHandler[pCommand->m_byType - 1];
	(this->*pfnHandler)(pCommand, pwErrorCode);
	return 1;
}

/*
================
CGCharAutoCommandActor::Update
[RECONSTRUCTED - 0x004AD7A0] (205 bytes)
================
*/
int32_t CGCharAutoCommandActor::Update(uint16_t* pwErrorCode) {
	if (m_listCommands.empty()) {
		return 0;
	}
	if (::GetTickCount() - m_dwLastUpdateTick < 30) {
		return 1;
	}
	m_dwLastUpdateTick = ::GetTickCount();

	if (m_pOwner->GetMotionState() == 0x0B) {
		return 1;
	}

	sAutoCommand* pCommand = m_listCommands.front();

	if (pCommand->m_dwState == AUTO_COMMAND_STATE_QUEUED) {
		// 0x004AD7F8 writes the state directly; the state tick keeps the queue time.
		pCommand->m_dwState = AUTO_COMMAND_STATE_BEGIN;
		if (pCommand->m_pPreEngage == nullptr) {
			OnActionFinish();
			return 0;
		}
		if (pCommand->m_byType < AUTO_COMMAND_ATTACK || pCommand->m_byType > 6 ||
			m_apfnHandler[pCommand->m_byType - 1] == nullptr) {
			OnActionFinish();
			return 0;
		}
		if ((this->*m_apfnHandler[pCommand->m_byType - 1])(pCommand, pwErrorCode) == 0) {
			OnActionFinish();
			return 2;
		}
	}

	if (pCommand->m_pPreEngage == nullptr || pCommand->m_byType > AUTO_COMMAND_DISPEL) {
		OnActionFinish();
		return 0;
	}
	if ((this->*m_apfnHandler[pCommand->m_byType - 1])(pCommand, pwErrorCode) == 0) {
		OnActionFinish();
		return 2;
	}
	return 1;
}

/*
================
CGCharAutoCommandActor::ResolveActionSkill
[RECONSTRUCTED - 0x004AD030] (492 bytes)

Re-validates the command against the current world: the attack/skill targets, the pick-up
target and the traced player. Skill refusals are reported only while the record is in BEGIN.
================
*/
int32_t CGCharAutoCommandActor::ResolveActionSkill(sAutoCommand* pCommand) {
	Skill::sSkillPreEngageData* pPreEngage = pCommand->m_pPreEngage;
	if (pPreEngage == nullptr) {
		return 0;
	}

	switch (pCommand->m_byType) {
	case AUTO_COMMAND_ATTACK:
	case AUTO_COMMAND_CAST: {
		const tagRefSkill* pRefSkill = nullptr;
		if (pCommand->m_byType == AUTO_COMMAND_ATTACK) {
			uint32_t dwSkillID = m_pOwner->GetSkillManager()->GetDefaultAttackSkillByWeapon();
			pPreEngage->m_dwSkillID = dwSkillID;
			pRefSkill = g_pRefData->FindSkill(dwSkillID);
		} else {
			pPreEngage->m_dwSkillID = pCommand->m_pRefSkill->dwSkillID;
			pRefSkill = pCommand->m_pRefSkill;
		}

		uint16_t wError = TargetValidation_ValidateAllTargets(m_pOwner, pPreEngage, pRefSkill);
		if (wError == 0) {
			return 1;
		}
		if (pCommand->m_dwState == AUTO_COMMAND_STATE_BEGIN) {
			m_pOwner->GetSkillManager()->SendSkillErrorResponseB070(wError);
		}
		return 0;
	}

	case AUTO_COMMAND_PICKUP: {
		if (pPreEngage->m_vecTargets.empty()) {
			return 0;
		}
		CGObj* pTarget = ObjMgr_FindByID(pPreEngage->m_vecTargets.front().dwGlobalID);
		if (pTarget == nullptr || !pTarget->IsItem()) {
			return 0;
		}
		// [PARTIAL] native continues: the item must not be taken (+0x14C == 0) and its region (+0x84)
		// must be compatible with the owner's. The port's CGItem is not a CGObj yet and
		// ObjMgr_FindByID only indexes characters, so a ground item cannot be resolved: fail closed.
		return 0;
	}

	case AUTO_COMMAND_TRACE: {
		if (pPreEngage->m_vecTargets.empty()) {
			return 0;
		}
		CGObjChar* pTarget = ObjMgr_FindByID(pPreEngage->m_vecTargets.front().dwGlobalID);
		if (pTarget == nullptr || !pTarget->IsPlayer()) {
			return 0;
		}
		if (pTarget->GetLifeState() == 3 || pTarget->GetLifeState() == 2) {
			return 0;
		}
		return Pos_RegionsCompatible(pTarget->m_wRegionID, m_pOwner->m_wRegionID) ? 1 : 0;
	}

	default:
		return 1;
	}
}

/*
================
CGCharAutoCommandActor::HasLearnedSkill
[RECONSTRUCTED - 0x004AD230] (51 bytes)
================
*/
int32_t CGCharAutoCommandActor::HasLearnedSkill(sAutoCommand* pCommand) {
	if (pCommand->m_byType != AUTO_COMMAND_CAST || pCommand->m_pRefSkill == nullptr) {
		return 1;
	}
	return (m_pOwner->GetSkillManager()->FindSkillByID(pCommand->m_pRefSkill->dwSkillID) != nullptr) ? 1 : 0;
}

/*
================
CGCharAutoCommandActor::IsTargetInRange
[RECONSTRUCTED - 0x004AD4A0] (386 bytes)

Fortress structures are measured from the approach point a player was assigned (+0x2284).
================
*/
int32_t CGCharAutoCommandActor::IsTargetInRange(uint32_t dwTargetID, int32_t nRange) {
	CGObjChar* pTarget = ObjMgr_FindByID(dwTargetID);
	if (pTarget == nullptr || !pTarget->IsChar()) {
		return 0;
	}
	if (!Pos_RegionsCompatible(pTarget->m_wRegionID, m_pOwner->m_wRegionID)) {
		return 0;
	}

	SRO_Vector3D vOwnerPos(m_pOwner->m_fLocalPosX, m_pOwner->m_fLocalPosY, m_pOwner->m_fLocalPosZ);
	SRO_Vector3D vRelative;
	if (pTarget->IsFortressStructure() && m_pOwner->IsPlayer()) {
		SRO_Vector3D vApproach(m_pOwner->m_StructureApproachPos.fPosX, m_pOwner->m_StructureApproachPos.fPosY,
			m_pOwner->m_StructureApproachPos.fPosZ);
		Pos_Relative3D(&vRelative, m_pOwner->m_wRegionID, &vOwnerPos, m_pOwner->m_StructureApproachPos.wRegionID, &vApproach);
	} else {
		SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
		Pos_Relative3D(&vRelative, m_pOwner->m_wRegionID, &vOwnerPos, pTarget->m_wRegionID, &vTargetPos);
	}

	float fDistance = std::sqrt(vRelative.x * vRelative.x + vRelative.y * vRelative.y + vRelative.z * vRelative.z);
	return (static_cast<int32_t>(fDistance) <= nRange + 2) ? 1 : 0;
}

/*
================
CGCharAutoCommandActor::GetDistanceToEntity
[RECONSTRUCTED - 0x004AD3E0] (184 bytes)
================
*/
float CGCharAutoCommandActor::GetDistanceToEntity(CGObjChar* pTarget) {
	if (pTarget == nullptr) {
		ASSERT(false);
		return 0.0f;
	}

	SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
	SRO_Vector3D vOwnerPos(m_pOwner->m_fLocalPosX, m_pOwner->m_fLocalPosY, m_pOwner->m_fLocalPosZ);
	SRO_Vector3D vRelative;
	Pos_Relative3D(&vRelative, m_pOwner->m_wRegionID, &vOwnerPos, pTarget->m_wRegionID, &vTargetPos);
	return std::sqrt(vRelative.x * vRelative.x + vRelative.y * vRelative.y + vRelative.z * vRelative.z);
}

/*
================
CGCharAutoCommandActor::StopMovement
[RECONSTRUCTED - 0x004AD780] (17 bytes)
================
*/
void CGCharAutoCommandActor::StopMovement(int32_t bFlag) {
	m_pOwner->m_AutoNavigator.Stop(bFlag);
}

/*
================
CGCharAutoCommandActor::ApplyWeaponStructureRange
[RECONSTRUCTED - 0x004AF3C0] (85 bytes)

Weapon kind = TID >> 11. Kinds 6 (bow) and 12 (crossbow) engage a structure from 91..130,
kinds 10, 11, 14, 15 from 70..100; the rest keep the computed range. (table 0x004AF424)
================
*/
void CGCharAutoCommandActor::ApplyWeaponStructureRange(sAutoCommand* pCommand) {
	if (pCommand == nullptr) {
		return;
	}

	static const uint8_t s_abyRangeClass[10] = { 0, 2, 2, 2, 1, 1, 0, 2, 1, 1 };

	uint32_t dwKind = static_cast<uint32_t>(m_pOwner->GetEquippedPrimaryWeaponTID()) >> 11;
	if (dwKind - 6 > 9) {
		return;
	}
	switch (s_abyRangeClass[dwKind - 6]) {
	case 0:
		pCommand->m_fMinRange = 91.0f;
		pCommand->m_fMaxRange = 130.0f;
		break;
	case 1:
		pCommand->m_fMinRange = 70.0f;
		pCommand->m_fMaxRange = 100.0f;
		break;
	default:
		break;
	}
}

/*
================
CGCharAutoCommandActor::ApplyWeaponRangeScale
[RECONSTRUCTED - 0x004AF430] (77 bytes)
================
*/
void CGCharAutoCommandActor::ApplyWeaponRangeScale(sAutoCommand* pCommand) {
	uint32_t dwKind = static_cast<uint32_t>(m_pOwner->GetEquippedPrimaryWeaponTID()) >> 11;
	if (dwKind == 6) {
		double dScale = 0.699999988079071; // 0x00B45EF8
		pCommand->m_fMinRange = static_cast<float>(pCommand->m_fMinRange * dScale);
		pCommand->m_fMaxRange = static_cast<float>(pCommand->m_fMaxRange * dScale);
	} else if (dwKind == 12) {
		double dScale = 0.6000000238418579; // 0x00B45BF8
		pCommand->m_fMinRange = static_cast<float>(pCommand->m_fMinRange * dScale);
		pCommand->m_fMaxRange = static_cast<float>(pCommand->m_fMaxRange * dScale);
	}
}

/*
================
CGCharAutoCommandActor::AddSkillEngageRange
[RECONSTRUCTED - inlined in 0x004ADA88, 0x004ADF13 and 0x004AE82E]

range = wTargetRange (+0x92), or the cast range bonus (param 0x21, 0x004AC890) plus Range
(+0x250); plus the two skill modifiers keyed by +0x50C and +0x4E8; scaled by (1 - param 0xB7 / 100).
Up to 4 it is added whole, below 10 it is shortened by 2, above 10 it counts 70 %; exactly 10 adds
nothing (0x004AE971 falls through both tests).
================
*/
void CGCharAutoCommandActor::AddSkillEngageRange(sAutoCommand* pCommand, const tagRefSkill* pRefSkill) {
	CSkillManager* pSkillManager = m_pOwner->GetSkillManager();

	float fRange = 0.0f;
	if (pRefSkill->wTargetRange != 0) {
		fRange = static_cast<float>(static_cast<int32_t>(pRefSkill->wTargetRange));
	} else {
		fRange = static_cast<float>(static_cast<int32_t>(m_pOwner->GetCastRangeBonus()));
		if (pRefSkill->pRangeModifier != nullptr) {
			fRange = static_cast<float>(static_cast<double>(*pRefSkill->pRangeModifier) + fRange);
		}
	}

	if (pRefSkill->pParam50C != nullptr) {
		const tagSkillModifier* pModifier = pSkillManager->GetSkillModifier(*pRefSkill->pParam50C);
		if (pModifier != nullptr) {
			fRange = static_cast<float>(static_cast<double>(pModifier->dwValue) + fRange);
		}
	}
	if (pRefSkill->pParam4E8 != nullptr) {
		const tagSkillModifier* pModifier = pSkillManager->GetSkillModifier(*pRefSkill->pParam4E8);
		if (pModifier != nullptr) {
			fRange = static_cast<float>(static_cast<double>(pModifier->dwValue) + fRange);
		}
	}

	double dReduction = static_cast<double>(m_pOwner->m_paramKeeper.GetParamFloat(0xB7)) / 100.0;
	fRange = static_cast<float>((1.0 - dReduction) * fRange);

	if (fRange <= 4.0f) {
		pCommand->m_fMinRange += fRange;
		pCommand->m_fMaxRange += fRange;
	} else if (fRange < 10.0f) {
		float fAdd = static_cast<float>(fRange - 2.0);
		pCommand->m_fMinRange += fAdd;
		pCommand->m_fMaxRange += fAdd;
	} else if (fRange > 10.0f) {
		float fAdd = static_cast<float>(0.699999988079071 * fRange);
		pCommand->m_fMinRange += fAdd;
		pCommand->m_fMaxRange += fAdd;
	}
}

/*
================
CGCharAutoCommandActor::PostSkillActionMsg
[RECONSTRUCTED - inlined in 0x004ADD6F and 0x004AEBB4]

Serialises the pre-engage data into 0x7070 and posts it to the owner's command source, so the
skill manager executes it on the next message pump. On a full queue the message is consumed
and freed and the handler reports failure.
================
*/
int32_t CGCharAutoCommandActor::PostSkillActionMsg(sAutoCommand* pCommand) {
	CCmdSource* pCmdSource = m_pOwner->GetCmdSource();
	CMsg* pMsg = pCmdSource->AllocMsg(0x7070);
	pCommand->m_pPreEngage->WriteToMsg(pMsg);
	if (pCmdSource->PostCommand(pMsg) == 0) {
		pMsg->Consume();
		pCmdSource->FreeMsg(pMsg);
		return 0;
	}
	return 1;
}

/*
================
CGCharAutoCommandActor::Handler_Attack
[RECONSTRUCTED - 0x004AD8E0] (1987 bytes)
================
*/
int32_t CGCharAutoCommandActor::Handler_Attack(sAutoCommand* pCommand, uint16_t* /*pwErrorCode*/) {
	if (!m_pOwner->IsPlayer() || pCommand->m_dwState > AUTO_COMMAND_STATE_KEEP_UP) {
		ASSERT(false);
		return 0;
	}

	Skill::sSkillPreEngageData* pPreEngage = pCommand->m_pPreEngage;
	CSkillManager* pSkillManager = m_pOwner->GetSkillManager();

	switch (pCommand->m_dwState) {
	case AUTO_COMMAND_STATE_BEGIN: {
		if (m_pOwner->IsAttackLocked()) {
			return 0;
		}
		if (ResolveActionSkill(pCommand) == 0) {
			return 0;
		}
		if (m_pOwner->GetMotionState() == 0x0F) {
			return 0;
		}

		pPreEngage->m_dwSkillID = pSkillManager->GetDefaultAttackSkillByWeapon();
		const tagRefSkill* pRefSkill = g_pRefData->FindSkill(pPreEngage->m_dwSkillID);
		if (pRefSkill == nullptr) {
			pSkillManager->SendSkillErrorResponseB070(0x3003);
			return 0;
		}

		uint16_t wError = CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x64, nullptr);
		if (wError != 0) {
			pSkillManager->SendSkillErrorResponseB070(wError);
			return 0;
		}

		CGObjChar* pTarget = ObjMgr_FindByID(pPreEngage->m_dwTargetObjID);
		if (pTarget == nullptr) {
			return 0;
		}

		if (pTarget->IsFortressStructure()) {
			pCommand->m_fMinRange = 0.0f;
			pCommand->m_fMaxRange = 0.0f;
			if (m_pOwner->AssignStructureApproachPos(pTarget->GetFortressStructureRefID()) == 0) {
				return 0;
			}
		} else {
			pCommand->m_fMinRange = static_cast<float>(m_pOwner->GetCollisionRadius() + pTarget->GetCollisionRadius());
			pCommand->m_fMaxRange = static_cast<float>(m_pOwner->GetCollisionRadius() + pTarget->GetCollisionRadius());
		}

		AddSkillEngageRange(pCommand, pRefSkill);
		if (pTarget->IsFortressStructure()) {
			ApplyWeaponStructureRange(pCommand);
		}

		pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
		return 1;
	}

	case AUTO_COMMAND_STATE_APPROACH: {
		if (m_listCommands.size() == 2 && m_listCommands.back()->m_byType == AUTO_COMMAND_CAST) {
			return 0;
		}
		if (ResolveActionSkill(pCommand) == 0) {
			OnActionFinish();
			return 0;
		}

		uint16_t wError = CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x24, nullptr);
		if (wError != 0) {
			pSkillManager->SendSkillErrorResponseB070(wError);
			return 0;
		}

		if (IsTargetInRange(pPreEngage->m_dwTargetObjID, static_cast<int32_t>(pCommand->m_fMaxRange)) != 0) {
			StopMovement(0);
			if (CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x81, nullptr) == 0) {
				pCommand->SetState(AUTO_COMMAND_STATE_EXECUTE);
			}
			return 1;
		}

		if (m_pOwner->m_AutoNavigator.GetTargetID() != pPreEngage->m_dwTargetObjID) {
			m_pOwner->m_AutoNavigator.StartPursuit(static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMaxRange)),
				pPreEngage->m_dwTargetObjID, static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMinRange)));
		}
		return 1;
	}

	case AUTO_COMMAND_STATE_EXECUTE: {
		tagActiveSkillInstance* pAttackState = m_pOwner->m_pAttackState;
		if (pAttackState != nullptr) {
			pAttackState->m_pExecution->m_pPeriodicDamage->m_dwStartedAt = ::GetTickCount();
			pAttackState->m_pExecution->m_pPeriodicDamage->m_dwTargetID = pPreEngage->m_dwTargetObjID;
		}

		if (PostSkillActionMsg(pCommand) == 0) {
			return 0;
		}
		pCommand->SetState(AUTO_COMMAND_STATE_KEEP_UP);
		return 1;
	}

	case AUTO_COMMAND_STATE_KEEP_UP: {
		if (pPreEngage == nullptr) {
			return 0;
		}

		const tagRefSkill* pRefSkill = pSkillManager->GetSkillData(pPreEngage->m_dwSkillID);
		if (pRefSkill == nullptr) {
			BSLib::Log_Printf(0x2000001, "[HANDLE_ATTACK-KeepUp] Name:%s,SkilID:%d,DeafultID:%d",
				m_pOwner->GetName(), pPreEngage->m_dwSkillID, pSkillManager->GetDefaultAttackSkillByWeapon());
			return 0;
		}

		if (CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x01, nullptr) != 0) {
			return 1;
		}
		if (m_listCommands.size() == 2) {
			return 0;
		}

		CGObjChar* pTarget = ObjMgr_FindByID(pPreEngage->m_dwTargetObjID);
		if (pTarget == nullptr || !pTarget->IsChar()) {
			return 0;
		}

		pCommand->m_fMinRange = static_cast<float>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius());
		pCommand->m_fMaxRange = static_cast<float>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius());
		AddSkillEngageRange(pCommand, pRefSkill);
		if (pTarget->IsFortressStructure()) {
			ApplyWeaponStructureRange(pCommand);
		}

		pPreEngage->m_byTargetFlags &= static_cast<uint8_t>(~Skill::SKILL_TARGET_FLAG_BODY_MODE_6);
		pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
		return 1;
	}

	default:
		return 0;
	}
}

/*
================
CGCharAutoCommandActor::Handler_Pickup
[RECONSTRUCTED - 0x004AE0C0] (755 bytes)
================
*/
int32_t CGCharAutoCommandActor::Handler_Pickup(sAutoCommand* pCommand, uint16_t* pwErrorCode) {
	switch (pCommand->m_dwState) {
	case AUTO_COMMAND_STATE_BEGIN: {
		uint16_t wResult = m_pOwner->CanPickupItem(pCommand->m_pPreEngage->m_dwTargetObjID);
		*pwErrorCode = wResult;
		if (wResult != 1 && wResult != 4) {
			m_pOwner->GetCmdSource()->OnCommandEvent(0x10, reinterpret_cast<uintptr_t>(pCommand), wResult);
			if (m_pOwner->IsPlayer()) {
				static_cast<CGObjPC*>(m_pOwner)->SendErrorResponse(0xB034, wResult);
			}
			return 0;
		}
		pCommand->m_fMinRange = 2.0f;
		pCommand->m_fMaxRange = 10.0f;
		pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
		return 1;
	}

	case AUTO_COMMAND_STATE_APPROACH: {
		CGObj* pTarget = ObjMgr_FindByID(pCommand->m_pPreEngage->m_dwTargetObjID);
		if (pTarget == nullptr) {
			// 0x004AE197 returns with the lookup result (null) in eax.
			*pwErrorCode = 3;
			return 0;
		}
		// [PARTIAL] native continues: a taken item (+0x14C != 0) fails with 0x1812; within 10 of the
		// item StopMovement(1) and EXECUTE, otherwise StartPursuit(max, item, min) when the navigator is
		// idle; return 1. The port's CGItem is not a CGObj yet (see ResolveActionSkill): fail closed.
		return 0;
	}

	case AUTO_COMMAND_STATE_EXECUTE: {
		if (ResolveActionSkill(pCommand) == 0) {
			*pwErrorCode = 3;
			return 0;
		}

		CCmdSource* pCmdSource = m_pOwner->GetCmdSource();
		CMsg* pMsg = pCmdSource->AllocMsg(0x7034);
		if (m_pOwner->IsPlayer()) {
			*pMsg << static_cast<uint8_t>(6);
		} else {
			ASSERT(m_pOwner->IsPickPetCOS());
			*pMsg << static_cast<uint8_t>(0x11);
			*pMsg << m_pOwner->GetGlobalID();
		}
		*pMsg << pCommand->m_pPreEngage->m_dwTargetObjID;

		if (pCmdSource->PostCommand(pMsg) == 0) {
			*pwErrorCode = 2;
			pMsg->Consume();
			pCmdSource->FreeMsg(pMsg);
			return 0;
		}

		pCmdSource->OnCommandEvent(0x11, reinterpret_cast<uintptr_t>(&pCommand), 0);
		pCommand->SetState(AUTO_COMMAND_STATE_KEEP_UP);
		return 1;
	}

	case AUTO_COMMAND_STATE_KEEP_UP:
		return 0;

	default:
		ASSERT(false);
		return 0;
	}
}

/*
================
CGCharAutoCommandActor::Handler_Trace
[RECONSTRUCTED - 0x004AE3D0] (326 bytes)
================
*/
int32_t CGCharAutoCommandActor::Handler_Trace(sAutoCommand* pCommand, uint16_t* /*pwErrorCode*/) {
	switch (pCommand->m_dwState) {
	case AUTO_COMMAND_STATE_BEGIN: {
		if (ResolveActionSkill(pCommand) == 0) {
			return 0;
		}
		// ResolveActionSkill has just verified the traced player exists.
		CGObjChar* pTarget = ObjMgr_FindByID(pCommand->m_pPreEngage->m_dwTargetObjID);
		pCommand->m_fMinRange = static_cast<float>(static_cast<double>(m_pOwner->GetCollisionRadius() + pTarget->GetCollisionRadius()) + 50.0);
		pCommand->m_fMaxRange = static_cast<float>(static_cast<double>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius()) + 80.0);
		pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
		return 1;
	}

	case AUTO_COMMAND_STATE_APPROACH: {
		if (ResolveActionSkill(pCommand) == 0) {
			return 0;
		}
		if (m_pOwner->m_AutoNavigator.GetTargetID() != 0) {
			return 1;
		}
		return (m_pOwner->m_AutoNavigator.StartPursuit(static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMaxRange)),
			pCommand->m_pPreEngage->m_dwTargetObjID, static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMinRange))) != 0) ? 1 : 0;
	}

	default:
		ASSERT(false);
		return 0;
	}
}

/*
================
CGCharAutoCommandActor::Handler_Cast
[RECONSTRUCTED - 0x004AE590] (2162 bytes)
================
*/
int32_t CGCharAutoCommandActor::Handler_Cast(sAutoCommand* pCommand, uint16_t* /*pwErrorCode*/) {
	if (!m_pOwner->IsPlayer() || pCommand->m_dwState > AUTO_COMMAND_STATE_KEEP_UP) {
		ASSERT(false);
		return 0;
	}

	CSkillManager* pSkillManager = m_pOwner->GetSkillManager();
	Skill::sSkillPreEngageData* pPreEngage = pCommand->m_pPreEngage;

	switch (pCommand->m_dwState) {
	case AUTO_COMMAND_STATE_BEGIN: {
		const tagRefSkill* pRefSkill = pCommand->m_pRefSkill;
		if (pRefSkill == nullptr) {
			pSkillManager->SendSkillErrorResponseB070(0x3003);
			return 0;
		}
		if (pSkillManager->IsCastingLocked() == 1) {
			pSkillManager->SendSkillErrorResponseB070(0x3030);
			return 0;
		}

		// Casting an active toggle again switches it off.
		if (pRefSkill->pBuffType != 0) {
			tagActiveSkillInstance* pInstance = pSkillManager->FindActiveBuffBySkillID(pRefSkill->dwSkillID, 0);
			if (pInstance != nullptr && pInstance->m_pExecution->m_byMode == 1) {
				pInstance->RequestRetirement(false);
				return 0;
			}
		}

		if (pPreEngage == nullptr) {
			return 0;
		}
		if (m_pOwner->GetMotionState() == 0x0F) {
			return 0;
		}
		if (pRefSkill->pParam594 == 0 && m_pOwner->GetMotionState() == 0x13) {
			return 0;
		}

		pPreEngage->m_dwSkillID = pRefSkill->dwSkillID;
		pCommand->m_dwLatencyUsed = 0;
		uint16_t wError = CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x17, pRefSkill);
		if (wError != 0) {
			pSkillManager->SendSkillErrorResponseB070(wError);
			return 0;
		}

		if (pRefSkill->byCastType != 2) {
			return 1;
		}

		if (pRefSkill->byTargetRequired == 0) {
			pPreEngage->m_byTargetFlags = 0;
			if (!pPreEngage->m_vecTargets.empty()) {
				pPreEngage->m_vecTargets.clear();
			}
		}

		if (ResolveActionSkill(pCommand) == 0 || HasLearnedSkill(pCommand) == 0) {
			return 0;
		}

		if (pRefSkill->byTargetRequired == 0 || (pRefSkill->byTargetRequired == 1 && pRefSkill->byTargetTypeLand == 1)) {
			pCommand->SetState(AUTO_COMMAND_STATE_EXECUTE);
			return 1;
		}

		CGObjChar* pTarget = ObjMgr_FindByID(pPreEngage->m_dwTargetObjID);
		if (pTarget == nullptr) {
			return 0;
		}

		if (pRefSkill->pReqc != nullptr && (*pRefSkill->pReqc & 1) != 0) {
			pCommand->m_fMinRange = 0.0f;
			pCommand->m_fMaxRange = static_cast<float>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius());
			pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
			return 1;
		}

		pCommand->m_fMinRange = static_cast<float>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius());
		pCommand->m_fMaxRange = static_cast<float>(pTarget->GetCollisionRadius() + m_pOwner->GetCollisionRadius());
		AddSkillEngageRange(pCommand, pRefSkill);

		if (pTarget->CanBeAttacked() != 0) {
			ASSERT(g_pGameWorldMgr != nullptr);
			if (g_pGameWorldMgr->ReducesRangedWeaponRange(static_cast<uint16_t>(m_pOwner->GetWorldID())) == 1) {
				ApplyWeaponRangeScale(pCommand);
			}
		}

		pCommand->SetState(AUTO_COMMAND_STATE_APPROACH);
		return 1;
	}

	case AUTO_COMMAND_STATE_APPROACH: {
		if (ResolveActionSkill(pCommand) == 0 || HasLearnedSkill(pCommand) == 0) {
			return 0;
		}
		if (CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x91, nullptr) != 0) {
			return 0;
		}

		if (IsTargetInRange(pPreEngage->m_dwTargetObjID, static_cast<int32_t>(pCommand->m_fMaxRange)) != 0) {
			StopMovement(0);
			pCommand->SetState(AUTO_COMMAND_STATE_EXECUTE);
			return 1;
		}

		if (m_pOwner->m_AutoNavigator.GetTargetID() != pPreEngage->m_dwTargetObjID) {
			m_pOwner->m_AutoNavigator.StartPursuit(static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMaxRange)),
				pPreEngage->m_dwTargetObjID, static_cast<uint16_t>(static_cast<int32_t>(pCommand->m_fMinRange)));
		}
		return 1;
	}

	case AUTO_COMMAND_STATE_EXECUTE: {
		if (ResolveActionSkill(pCommand) == 0) {
			return 0;
		}
		if (pCommand->m_pChainSkill == nullptr && HasLearnedSkill(pCommand) == 0) {
			return 0;
		}
		if (CheckSkillPreEngageCondition(m_pOwner, pPreEngage, 0x91, nullptr) != 0) {
			return 0;
		}

		const tagRefSkill* pStepSkill = nullptr;
		if (pCommand->m_pChainSkill != nullptr) {
			pStepSkill = g_pRefData->FindSkill(pCommand->m_pRefSkill->dwSkillID);
			if (pStepSkill == nullptr) {
				return 0;
			}
		} else {
			pStepSkill = pSkillManager->GetSkillData(pCommand->m_pRefSkill->dwSkillID);
		}

		tagActiveSkillInstance* pAttackState = m_pOwner->m_pAttackState;
		if (pAttackState != nullptr && pStepSkill->pCastFlag != 0) {
			if (pAttackState->m_pExecution->m_pPeriodicDamage == nullptr) {
				m_pOwner->m_pAttackState = nullptr;
			} else {
				pAttackState->m_pExecution->m_pPeriodicDamage->m_dwStartedAt = ::GetTickCount();
				pAttackState->m_pExecution->m_pPeriodicDamage->m_dwTargetID = pPreEngage->m_dwTargetObjID;
			}
		}

		if (PostSkillActionMsg(pCommand) == 0) {
			return 0;
		}
		pCommand->SetState(AUTO_COMMAND_STATE_KEEP_UP);

		// Chained skills (+0x68 = next step id): each step's time (+0x78) first spends the 500 ms
		// latency budget; what was spent is remembered in m_dwLatencyUsed.
		if (pStepSkill->dwCastingDuration == 0) {
			pCommand->m_pChainSkill = nullptr;
			return 1;
		}
		pCommand->m_pChainSkill = pStepSkill;
		pCommand->m_dwChainStepTime = pStepSkill->dwCastTime;
		if (pCommand->m_dwLatencyBudget != 0) {
			if (pStepSkill->dwCastTime > pCommand->m_dwLatencyBudget) {
				pCommand->m_dwLatencyUsed += pCommand->m_dwLatencyBudget;
				pCommand->m_dwChainStepTime = pStepSkill->dwCastTime - pCommand->m_dwLatencyBudget;
				pCommand->m_dwLatencyBudget = 0;
			} else {
				pCommand->m_dwLatencyBudget -= pStepSkill->dwCastTime;
				pCommand->m_dwChainStepTime = 0;
			}
		}
		return 1;
	}

	case AUTO_COMMAND_STATE_KEEP_UP: {
		const tagRefSkill* pRefSkill = pCommand->m_pRefSkill;
		if (pRefSkill == nullptr) {
			return 0;
		}

		uint32_t dwWait = 0;
		if (pCommand->m_pChainSkill != nullptr) {
			dwWait = pCommand->m_dwChainStepTime;
		} else {
			dwWait = pRefSkill->dwCastTime + pRefSkill->dwActionDuration;
			if (pRefSkill->byAutoAttackChain != 0 && m_listCommands.size() == 1) {
				dwWait += pCommand->m_dwLatencyUsed;
			}
		}

		if (m_pOwner->m_pCastingInstance != nullptr && pRefSkill->pBuffType == 0) {
			return 1;
		}
		if (::GetTickCount() - pCommand->m_dwStateTick <= dwWait) {
			return 1;
		}

		if (pCommand->m_pChainSkill != nullptr) {
			pCommand->m_pRefSkill = g_pRefData->FindSkill(pCommand->m_pChainSkill->dwCastingDuration);
			pCommand->SetState(AUTO_COMMAND_STATE_EXECUTE);
			return 1;
		}

		// Skills flagged to continue into a normal attack (+0x90 == 1) queue one on the same target.
		if (pRefSkill->byAutoAttackChain != 1 || pPreEngage->m_vecTargets.empty()) {
			return 0;
		}
		CGObjChar* pTarget = ObjMgr_FindByID(pPreEngage->m_vecTargets.front().dwGlobalID);
		if (pTarget == nullptr || pTarget->GetLifeState() != 1) {
			return 0;
		}
		if (pTarget->IsChar() && pTarget->GetMotionState() == 8) {
			return 0;
		}
		if (m_listCommands.size() != 1) {
			return 0;
		}

		sAutoCommand* pAttack = sAutoCommand::Allocate();
		pAttack->m_byType = AUTO_COMMAND_ATTACK;
		pAttack->m_pPreEngage = pPreEngage;
		pAttack->SetState(AUTO_COMMAND_STATE_QUEUED);
		pCommand->m_pPreEngage = nullptr;
		ExecuteAction(pAttack);
		return 0;
	}

	default:
		return 0;
	}
}

/*
================
CGCharAutoCommandActor::Handler_Dispel
[RECONSTRUCTED - 0x004AE520] (103 bytes)
================
*/
int32_t CGCharAutoCommandActor::Handler_Dispel(sAutoCommand* pCommand, uint16_t* /*pwErrorCode*/) {
	if (!m_pOwner->IsPlayer() || pCommand->m_dwState != AUTO_COMMAND_STATE_BEGIN) {
		ASSERT(false);
		return 0;
	}

	CSkillManager* pSkillManager = m_pOwner->GetSkillManager();
	if (pCommand->m_pRefSkill == nullptr) {
		pSkillManager->SendSkillErrorResponseB070(0x3003);
		return 0;
	}

	tagActiveSkillInstance* pInstance =
		pSkillManager->FindActiveBuffBySkillID(pCommand->m_pRefSkill->dwSkillID, pCommand->m_dwDispelParam);
	if (pInstance != nullptr) {
		pInstance->RequestRetirement(false);
	}
	return 0;
}
