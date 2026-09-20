/**
 * ============================================================================
 * Silkroad Online - Skill pre-engage data
 * Skill::sSkillPreEngageData (see SkillPreEngageData.h)
 * ============================================================================
 */

#include "SkillPreEngageData.h"
#include "../../JMX_Library/BSLib/Msg.h"
#include "../../JMX_Library/BSLib/ChunkAllocator.h"

namespace Skill {

// Native static pool @ 0x00CE2B18, constructed by 0x005AC160.
static CChunkAllocatorST<sSkillPreEngageData> s_poolSkillPreEngageData;

/*
================
sSkillPreEngageData::sSkillPreEngageData
[RECONSTRUCTED - 0x005A9610]
================
*/
sSkillPreEngageData::sSkillPreEngageData()
	: m_bAllocated(0)
	, m_dwSkillID(0)
	, m_byTargetFlags(0)
	, m_dwTargetObjID(0)
	, m_dwTargetObjID14(0)
	, m_dwDuration(0)
	, m_retirementTrigger()
	, m_dw20(0)
	, m_dw24(0)
	, m_dw28(0)
	, m_w2C(0)
	, m_f30(0.0f)
	, m_f34(0.0f)
	, m_f38(0.0f) {
}

/*
================
sSkillPreEngageData::~sSkillPreEngageData
[RECONSTRUCTED - 0x005AFA70]
================
*/
sSkillPreEngageData::~sSkillPreEngageData() {
}

/*
================
sSkillPreEngageData::Allocate
[RECONSTRUCTED - 0x005AA0B0] (12 bytes)
================
*/
sSkillPreEngageData* sSkillPreEngageData::Allocate() {
	sSkillPreEngageData* pData = s_poolSkillPreEngageData.Alloc();
	pData->m_bAllocated = 1;
	return pData;
}

/*
================
sSkillPreEngageData::Release
[RECONSTRUCTED - 0x005AA0C0] (39 bytes)
================
*/
void sSkillPreEngageData::Release(sSkillPreEngageData*& pData) {
	if (pData == nullptr) {
		return;
	}
	if (pData->m_bAllocated != 0) {
		pData->m_bAllocated = 0;
		pData->Clear();
		s_poolSkillPreEngageData.Free(pData);
	}
	pData = nullptr;
}

/*
================
sSkillPreEngageData::Clear
[RECONSTRUCTED - 0x005A95A0] (112 bytes)
================
*/
void sSkillPreEngageData::Clear() {
	m_vecTargets.clear();
	m_vecTargetPos.clear();
	m_byTargetFlags = 0;
	m_dwDuration = 0;
	m_retirementTrigger.reset();
	m_dw20 = 0;
}

/*
================
sSkillPreEngageData::ReadFromMsg
[RECONSTRUCTED - 0x005AA480] (369 bytes)
================
*/
void sSkillPreEngageData::ReadFromMsg(CMsg* pMsg, int32_t bSkipSkillID) {
	if (bSkipSkillID == 0) {
		*pMsg >> m_dwSkillID;
	}
	*pMsg >> m_byTargetFlags;

	m_dwTargetObjID14 = 0;
	m_dwTargetObjID = 0;

	if ((m_byTargetFlags & SKILL_TARGET_FLAG_OBJECT) != 0) {
		m_vecTargets.clear();

		uint32_t dwTargetObjID = 0;
		*pMsg >> dwTargetObjID;
		m_vecTargets.push_back(tagTargetCandidate(dwTargetObjID, 0, 0x64));

		if (m_dwTargetObjID == 0) {
			m_dwTargetObjID = dwTargetObjID;
		}
		if (m_dwTargetObjID14 == 0) {
			m_dwTargetObjID14 = dwTargetObjID;
		}
	}

	if ((m_byTargetFlags & SKILL_TARGET_FLAG_POSITION) != 0) {
		m_vecTargetPos.clear();

		sSkillTargetPos pos;
		pos.m_wRegionID = 0;
		pos.m_fX = 0.0f;
		pos.m_fY = 0.0f;
		pos.m_fZ = 0.0f;
		*pMsg >> pos.m_wRegionID;
		// 0x005B8520 / 0x005B8580 read the three coordinates as one 12-byte block.
		pMsg->ReadRaw(&pos.m_fX, 12);
		m_vecTargetPos.push_back(pos);
	}
}

/*
================
sSkillPreEngageData::WriteToMsg
[RECONSTRUCTED - 0x005AA630] (184 bytes)
================
*/
void sSkillPreEngageData::WriteToMsg(CMsg* pMsg) const {
	*pMsg << m_dwSkillID;
	*pMsg << m_byTargetFlags;

	if ((m_byTargetFlags & SKILL_TARGET_FLAG_OBJECT) != 0 && !m_vecTargets.empty()) {
		*pMsg << m_vecTargets.front().dwGlobalID;
	}

	if ((m_byTargetFlags & SKILL_TARGET_FLAG_POSITION) != 0 && !m_vecTargetPos.empty()) {
		const sSkillTargetPos& pos = m_vecTargetPos.front();
		*pMsg << pos.m_wRegionID;
		pMsg->Write(&pos.m_fX, 12);
	}
}

} // namespace Skill
