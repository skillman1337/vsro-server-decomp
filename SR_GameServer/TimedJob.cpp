/**
 * ============================================================================
 * Silkroad Online - Game Server Timed Jobs Subsystem
 *
 * Implements:
 *   - CTimedJob         (portable shared projection; native base ctor 0x00650930)
 *   - CTJ_SkillKeeper   (Native VTable @ 0x00B04F2C)
 *   - CTimedJobManager  (Native VTable @ 0x00B04EBC, embedded in CGObjPC at +0x1DFC)
 *   - CreateOwnerTimedJob (Native @ 0x0049A390)
 * ============================================================================
 */

#include "TimedJob.h"
#include "GObjChar.h"
#include "SkillManager.h"
#include "GObjPC.h"
#include "skill/SkillGlobal.h"
#include "../ServerCommon/ReferenceData.h"
#include <algorithm>
#include <sstream>
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdio>
#include <ctime>
#include <cmath>

// Forward declaration for native async DB query submission (0x004420C0)
extern bool CGameServer_SubmitAsyncDBQuery(const char* szSql);

class TimedJobTrigger final : public Skill::RetirementTrigger {
public:
	CTimedJob* job;
	explicit TimedJobTrigger(CTimedJob* value) : job(value) {}
	void Notify(int32_t reason, tagActiveSkillInstance*) override {
		if (job) job->RequestRetirement(reason);
	}
};

// ============================================================================
// CTimedJob Implementation
// ============================================================================

CTimedJob::CTimedJob()
	: m_trigger(std::make_shared<TimedJobTrigger>(this))
	, m_pOwner(nullptr)
	, m_pRecord(nullptr)
	, m_fCheckpointElapsed(0.0f)
	, m_pServices(nullptr)
	, m_dwActiveMarker(1)
	, m_dwSecondaryGuard(0)
	, m_fAccrued(0.0f) {
}

CTimedJob::~CTimedJob() {
	Dispose();
}

/**
 * [RECONSTRUCTED - Native 0x006514A0] (160 bytes)
 * CTimedJob::BuildCheckpointQuery
 */
bool CTimedJob::BuildCheckpointQuery(std::string& strOut) {
	if (!m_pRecord || m_pRecord->GetID() == 0) {
		return false;
	}
	if (m_dwActiveMarker != 0) {
		return BuildUpdateQuery(strOut);
	}

	char szBuf[128];
	std::snprintf(szBuf, sizeof(szBuf), "DELETE _TIMEDJOB WHERE ID = %u", m_pRecord->GetID());
	strOut = szBuf;
	return true;
}

/**
 * [PORTABLE PROJECTION - Native 0x00650C90] (59 bytes)
 * Uses the timed-job descriptor's verified mutable column groups. This is a
 * semantic SQL projection, not a byte-identical generic DB formatter.
 * CTimedJob::BuildUpdateQuery
 */
bool CTimedJob::BuildUpdateQuery(std::string& strOut) {
	strOut.clear();
	if (!m_pRecord || m_pRecord->GetID() == 0) {
		return false;
	}

	return m_pRecord->BuildDirtyUpdateQuery(strOut);
}

/**
 * [PORTABLE PROJECTION - Native 0x00650C50] (53 bytes)
 * Binds services, owner and record. Native +0x34 is the post-bind callback;
 * CTJ_SkillKeeper uses the empty implementation at 0x0066B100. Installation
 * is a separate virtual +0x30 operation at 0x00650E70.
 * CTimedJob::Initialize
 */
bool CTimedJob::Initialize(CTimedJobManager* pMgr, CGObjChar* pOwner, CInstanceTimedJob* pRecord, int32_t bActive) {
	(void)bActive;
	if (m_pRecord || !pRecord) return false;
	m_pServices = pMgr;
	m_pOwner = pOwner;
	m_pRecord = pRecord;
	return true;
}

bool CTimedJob::Activate() {
	m_fAccrued = 0.0f;

	if (!m_pRecord) {
		return false;
	}

	// If Kind is 1 (absolute time) and already expired, mark inactive immediately
	if (m_pRecord->GetData2() == 1) {
		time_t now = std::time(nullptr);
		if (m_pRecord->GetTimeToKeep() <= static_cast<uint32_t>(now)) {
			m_dwActiveMarker = 0;
			return true;
		}
	} else if (m_pRecord->GetData2() == 0) {
		if (m_pRecord->GetTimeToKeep() == 0) {
			m_dwActiveMarker = 0;
			return true;
		}
	}

	m_dwActiveMarker = 1;
	m_dwSecondaryGuard = 0;
	return true;
}

bool CTimedJob::BuildInsertQuery(std::string& out) const {
	if (!m_pRecord || m_pRecord->GetID() || !m_pRecord->GetCharID()) return false;
	const auto& r = *m_pRecord;
	std::ostringstream sql;
	sql << "{?=CALL _ADDTIMEDJOB (" << int32_t(r.GetCharID()) << ','
	    << unsigned(r.GetCategory()) << ',' << int32_t(r.GetJobID()) << ','
	    << int32_t(r.GetTimeToKeep()) << ',' << int32_t(r.GetData1()) << ','
	    << int32_t(r.GetData2()) << ',' << int32_t(r.GetData3()) << ','
	    << int32_t(r.GetData4()) << ',' << int32_t(r.GetData5()) << ','
	    << int32_t(r.GetData6()) << ',' << int32_t(r.GetData7()) << ','
	    << int32_t(r.GetData8()) << ',' << r.GetSerial64() << ',' << int32_t(r.GetJID()) << ")}";
	out = sql.str(); return true;
}

bool CTimedJob::BuildDeleteQuery(std::string& out) const {
	if (!m_pRecord || !m_pRecord->GetID()) return false;
	out = "DELETE _TIMEDJOB WHERE ID = " + std::to_string(m_pRecord->GetID());
	return true;
}

void CTimedJob::SetRecordID(uint32_t id) {
	if (!m_pRecord || m_pRecord->GetID() || !id) return;
	m_pRecord->SetID(id);
}

bool CTimedJob::Checkpoint() {
	std::string sql;
	if (!BuildCheckpointQuery(sql)) return false;
	if (sql.empty()) return true;
	return m_pServices
		? static_cast<CTimedJobManager*>(m_pServices)->SubmitQuery(std::move(sql), false, {})
		: ShardQuery::Submit(std::move(sql));
}

void CTimedJob::DetachOwner() {
	m_pOwner = nullptr;
	m_pServices = nullptr;
	m_trigger->job = nullptr;
}

/**
 * [PARTIAL - Native 0x006511D0] (681 bytes)
 * CTimedJob::Tick
 *
 * Advances timer, handles 300-second periodic database checkpoint updates,
 * and handles countdown expiration.
 */
uint32_t CTimedJob::Tick(float fDelta) {
	m_fCheckpointElapsed += fDelta;
	m_fAccrued += fDelta;

	if (m_fAccrued > 1.0f) {
		uint32_t dwElapsedSec = static_cast<uint32_t>(m_fAccrued);

		if (m_pRecord != nullptr) {
			if (m_pRecord->GetData2() == 1) { // Absolute epoch time
				time_t now = std::time(nullptr);
				if (m_pRecord->GetTimeToKeep() <= static_cast<uint32_t>(now)) {
					// Expired
					m_dwActiveMarker = 0;
					m_fCheckpointElapsed = 0.0f;
					m_fAccrued = 0.0f;
				} else {
					m_fAccrued -= static_cast<float>(dwElapsedSec);

					// Check 300s DB checkpoint
					if (m_fCheckpointElapsed >= 300.0f) {
						m_fCheckpointElapsed -= 300.0f;
						Checkpoint();
					}
				}
			} else if (m_pRecord->GetData2() == 0) { // Relative countdown in seconds
				if (m_pRecord->GetTimeToKeep() <= dwElapsedSec) {
					// Expired
					m_pRecord->SetTimeToKeep(0);
					m_dwActiveMarker = 0;
					m_fCheckpointElapsed = 0.0f;
					m_fAccrued = 0.0f;
				} else {
					m_pRecord->SetTimeToKeep(m_pRecord->GetTimeToKeep() - dwElapsedSec);
					m_fAccrued -= static_cast<float>(dwElapsedSec);

					// Check 300s DB checkpoint
					if (m_fCheckpointElapsed >= 300.0f) {
						m_fCheckpointElapsed -= 300.0f;
						Checkpoint();
					}
				}
			}
		}
	}

	return m_dwActiveMarker;
}

/**
 * [RECONSTRUCTED - Native 0x00651480] (24 bytes)
 * CTimedJob::RequestRetirement
 */
int32_t CTimedJob::RequestRetirement(int32_t nReason) {
	if (nReason == 1) {
		m_dwActiveMarker = 0;
		m_dwSecondaryGuard = 1;
	}
	return 0;
}

void CTimedJob::Dispose() {
	DetachOwner();
	if (m_pRecord) {
		delete m_pRecord;
		m_pRecord = nullptr;
	}
	m_pOwner = nullptr;
	m_dwActiveMarker = 0;
}

// ============================================================================
// CTJ_SkillKeeper Implementation
// ============================================================================

bool CTJ_SkillKeeper::Activate() {
	if (!CTimedJob::Activate()) return false;
	if (!m_dwActiveMarker) return true;
	if (!m_pOwner || !g_pRefData) return false;
	const auto* ref = g_pRefData->FindSkill(m_pRecord->GetJobID());
	if (!ref || !ref->Param(0x280)) return false;
	auto* manager = m_pOwner->GetSkillManager();
	if (!manager) return false;
	auto* instance = tagActiveSkillInstance::Allocate();
	auto* command = Skill::sSkillPreEngageData::Allocate();
	instance->m_pCommand = command;
	instance->m_pExecution = tagSkillExecutionContext::Allocate();
	instance->m_pExecution->m_pRefSkill = ref;
	command->m_byTargetFlags = 0x0C;
	command->m_dwSkillID = ref->dwSkillID;
	command->m_dwTargetObjID = command->m_dwTargetObjID14 = 0;
	uint32_t seconds = m_pRecord->GetTimeToKeep();
	if (m_pRecord->GetData2() == 1) seconds -= static_cast<uint32_t>(std::time(nullptr));
	command->m_dwDuration = seconds * 1000u;
	if (ref->m_Basic_Code != "SKILL_MALL_PRE_APRU_4W_01")
		command->m_dwDuration = std::min(command->m_dwDuration, ref->Param(0x280)[0]);
	command->m_retirementTrigger = m_trigger;
	instance->m_wStatus = 0x3000;
	instance->m_dwStartTime = ::GetTickCount();
	instance->m_dwMode = 2;
	manager->AddActiveSkill(instance);
	manager->SendCastBeginB070(instance, instance->m_wStatus);
	manager->SendStageEndB071(1, 0, instance->m_pExecution->m_dwContextID);
	// Allocate the effect identity before recycling the transient cast identity.
	auto* effect = tagSkillExecutionContext::Allocate();
	effect->m_pRefSkill = ref;
	tagSkillExecutionContext::Release(instance->m_pExecution);
	instance->m_pExecution = effect;
	manager->SendEffectAddedB0BD(instance);
	BSLib::CPacket packet;
	packet.SetOpcode(0x3206);
	const auto tid = m_pOwner->GetTID();
	const uint8_t kind = tid.IsCOS() && ((tid.wType >> 11) == 3 || (tid.wType >> 11) == 4) ? 8 : 0;
	packet.Write(&kind, 1);
	if (kind == 8) packet.WriteUint32(m_pOwner->GetGameID());
	packet.WriteUint32(effect->m_dwContextID);
	packet.WriteUint32(ref->dwSkillID);
	m_pOwner->SendMsgToPeer(&packet);
	instance->m_dwStartTime = ::GetTickCount();
	if (ref->dwPackedStates) manager->ChangeStates(ref->dwPackedStates, false);
	if (const auto* ovl2 = ref->Param(0x37C)) manager->ChangeStates(*ovl2, false);
	SkillCombat_EngageSkill(m_pOwner, instance);
	return true;
}

/**
 * [PARTIAL - Native 0x006511D0 override in CTJ_SkillKeeper @ VTable 0x00B04F2C]
 * Expiry forces retirement (651427), independent of voluntary cancellation
 * eligibility. The scheduler service's 0x6F notification remains unimplemented.
 */
uint32_t CTJ_SkillKeeper::Tick(float fDelta) {
	uint32_t dwResult = CTimedJob::Tick(fDelta);

	// When expired and not yet finalized, notify CSkillManager
	if (m_dwActiveMarker == 0 && m_dwSecondaryGuard == 0) {
		if (m_pOwner != nullptr && m_pRecord != nullptr) {
			CSkillManager* pSkillMgr = m_pOwner->GetSkillManager();
			if (pSkillMgr != nullptr) {
				if (auto* pInstance = pSkillMgr->FindActiveBuffBySkillID(m_pRecord->GetJobID(), 0)) {
					pInstance->RequestRetirement(true);
				}
			}
		}
	}

	return dwResult;
}

// ============================================================================
// CTimedJobManager Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - Native 0x00655970] (112 bytes)
 * CTimedJobManager Constructor
 */
CTimedJobManager::CTimedJobManager()
	: m_pOwner(nullptr)
	, m_setJobs() {
}

/**
 * [RECONSTRUCTED - Native 0x00655A50] (108 bytes)
 * CTimedJobManager Destructor
 */
CTimedJobManager::~CTimedJobManager() {
	Clear();
}

/**
 * [RECONSTRUCTED - Native 0x00655BE0] (441 bytes)
 * CTimedJobManager::Create
 */
bool CTimedJobManager::Create(uint8_t byCategory, uint32_t dwSkillID, uint32_t dwTimeToKeep,
                              uint32_t dwData1, uint32_t dwData2, uint32_t dwData3,
                              uint32_t dwData4, uint32_t dwData5, uint32_t dwSerialLow,
                              uint32_t dwSerialHigh, uint32_t dwJID) {
	auto pJob = std::shared_ptr<CTimedJob>(CreateJob(byCategory, dwSkillID));
	if (!pJob) {
		return false;
	}

	CInstanceTimedJob* pRecord = CreateRecord();
	if (!pRecord) {
		return false;
	}

	uint32_t dwOwnerCharID = m_pOwner && m_pOwner->GetDataPermanent()
		? m_pOwner->GetDataPermanent()->m_dwCharID : 0;
	pRecord->SetCharID(dwOwnerCharID);
	pRecord->SetCategory(byCategory);
	pRecord->SetJobID(dwSkillID);
	pRecord->SetTimeToKeep(dwTimeToKeep);
	pRecord->SetData1(dwData1);
	pRecord->SetData2(dwData2);
	pRecord->SetData3(dwData3);
	pRecord->SetData4(dwData4);
	pRecord->SetData5(dwData5);
	pRecord->SetData6(0);
	pRecord->SetData7(0);
	pRecord->SetData8(0);
	pRecord->SetSerial64(dwSerialLow, dwSerialHigh);
	pRecord->SetJID(dwJID);

	if (!pJob->Initialize(this, m_pOwner, pRecord, 1)) {
		ReleaseRecord(pRecord);
		return false;
	}
	std::string insert;
	if (!pJob->BuildInsertQuery(insert) || !pJob->Activate() || !pJob->CanSchedule()) return false;
	auto lifetime = std::weak_ptr<int>(m_lifetime);
	if (!SubmitQuery(std::move(insert), true, [this, lifetime, pJob](const ShardQuery::Result& result) {
		if (!result.ok) {
			if (!lifetime.expired()) OnPersistenceFailure(result);
			return;
		}
		pJob->SetRecordID(static_cast<uint32_t>(result.value));
		// Detached/expired jobs can outlive the manager while their insert is
		// in flight. Complete their final checkpoint/delete without a raw owner.
		if (lifetime.expired() || !m_setJobs.contains(pJob)) pJob->Checkpoint();
	})) {
		pJob->RequestRetirement();
		if (auto* manager = m_pOwner->GetSkillManager()) {
			if (auto* effect = manager->FindActiveBuffBySkillID(dwSkillID, 0)) effect->RequestRetirement(true);
		}
		return false;
	}
	m_setJobs.insert(std::move(pJob));
	return true;
}

/**
 * [PORTABLE PROJECTION - Native 0x00655F10]
 * CTimedJobManager::LoadFromDB
 */
bool CTimedJobManager::LoadFromDB(CGObjChar* pOwner, std::list<CInstanceTimedJob*>& records) {
	m_pOwner = pOwner;
	while (!records.empty()) {
		auto* record = records.front();
		if (!record) return false;
		auto job = std::shared_ptr<CTimedJob>(CreateJob(record->GetCategory(), record->GetJobID()));
		if (!job || !job->Initialize(this, pOwner, record, 0)) return false;
		records.pop_front(); // binding transfers ownership, activation is separate
		m_setJobs.insert(std::move(job));
	}
	return true;
}

bool CTimedJobManager::ActivateLoadedJobs() {
	// 656095..6560AC always advances to the next job after virtual +0x30.
	// Portable activation can additionally reject an unsupported prerequisite;
	// report that failure without suppressing activation of subsequent records.
	bool success = true;
	for (const auto& job : m_setJobs) {
		if (!job->Activate()) success = false;
	}
	return success;
}

bool CTimedJobManager::Checkpoint() {
	bool success = true;
	for (const auto& job : m_setJobs)
		if (job->GetRecord()->GetID() && !job->Checkpoint()) success = false;
	return success;
}

bool CTimedJobManager::SubmitQuery(std::string sql, bool returnsID, ShardQuery::Completion completion) {
	return ShardQuery::Submit(std::move(sql), returnsID, std::move(completion));
}

void CTimedJobManager::OnPersistenceFailure(const ShardQuery::Result& result) {
	BSLib::Log_Printf(0x2000000, "Timed-job persistence failed: %s", result.error.c_str());
	if (m_pOwner && m_pOwner->IsPlayer()) static_cast<CGObjPC*>(m_pOwner)->Recall(4);
}

CTimedJob* CTimedJobManager::CreateJob(uint8_t byCategory, uint32_t dwSkillID) {
	// Category 0 = Buff / Skill Keeper
	if (byCategory == 0) {
		const auto* ref = g_pRefData ? g_pRefData->FindSkill(dwSkillID) : nullptr;
		if (!ref || !ref->Param(0x280)) return nullptr;
		return new CTJ_SkillKeeper();
	}
	BSLib::Log_Printf(0x2000000, "Unsupported native timed-job category %u (job %u)", unsigned(byCategory), dwSkillID);
	return nullptr;
}

CInstanceTimedJob* CTimedJobManager::CreateRecord() {
	return new CInstanceTimedJob();
}

void CTimedJobManager::ReleaseRecord(CInstanceTimedJob* pRecord) {
	delete pRecord;
}

void CTimedJobManager::Tick(float fDelta) {
	auto it = m_setJobs.begin();
	while (it != m_setJobs.end()) {
		auto pJob = *it;
		if (pJob != nullptr) {
			uint32_t dwMarker = pJob->Tick(fDelta);
			if (dwMarker == 0) {
				std::string sql;
				if (pJob->BuildDeleteQuery(sql) && !SubmitQuery(std::move(sql), false, {}))
					OnPersistenceFailure({false, 0, "Failed to enqueue timed-job deletion"});
				it = m_setJobs.erase(it);
				pJob->DetachOwner();
				continue;
			}
		}
		++it;
	}
}

void CTimedJobManager::Clear() {
	for (const auto& pJob : m_setJobs) {
		pJob->DetachOwner();
	}
	m_setJobs.clear();
	m_lifetime.reset();
	m_lifetime = std::make_shared<int>(0);
}

// ============================================================================
// Global Helper Function
// ============================================================================

/**
 * [RECONSTRUCTED - Native 0x0049A390] (102 bytes)
 * CreateOwnerTimedJob
 */
bool CreateOwnerTimedJob(CGObjChar* pOwner, uint8_t byCategory, uint32_t dwSkillID,
                         uint32_t dwTimeToKeep, uint32_t dwData1, uint32_t dwData2,
                         uint32_t dwData3, uint32_t dwData4, uint32_t dwData5,
                         uint32_t dwSerialLow, uint32_t dwSerialHigh, uint32_t dwJID) {
	if (!pOwner) {
		return false;
	}

	CTimedJobManager* pMgr = pOwner->GetTimedJobManager();
	if (!pMgr) {
		return false;
	}

	return pMgr->Create(byCategory, dwSkillID, dwTimeToKeep, dwData1, dwData2,
	                    dwData3, dwData4, dwData5, dwSerialLow, dwSerialHigh, dwJID);
}
