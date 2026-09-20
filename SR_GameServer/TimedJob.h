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

#ifndef _SR_GAMESERVER_TIMEDJOB_H_
#define _SR_GAMESERVER_TIMEDJOB_H_

#include <cstdint>
#include <set>
#include <string>
#include <list>
#include <memory>
#include "skill/SkillPreEngageData.h"
#include "AsyncShardQuery.h"
#include "../ServerCommon/InstanceTimedJob.h"

class CGObjChar;
class CTimedJobManager;
class TimedJobTrigger;

/**
 * [PARTIAL - portable projection of native base and skill-keeper fields]
 * CTimedJob
 *
 * Native 650E70/6511D0 belong to CTJ_SkillKeeper, not the native base.
 * This portable hierarchy currently combines their fields and timer logic;
 * it is not a native layout/vtable certificate for every timed-job category.
 */
class CTimedJob {
public:
	CTimedJob();
	virtual ~CTimedJob();

	// [RECONSTRUCTED - Native 0x006514A0]
	// Active jobs build an update; retired jobs build a delete. ID zero does
	// not produce SQL. This is not the unconditional delete method at 650AF0.
	virtual bool BuildCheckpointQuery(std::string& strOut);

	// [PARTIAL - Native 0x00650C90 delegates to record-driven 0x009742D0]
	// Builds "UPDATE _TIMEDJOB SET ..."
	virtual bool BuildUpdateQuery(std::string& strOut);

	// [PORTABLE PROJECTION - Native 0x00650C50] (53 bytes)
	// Binds services, owner and record; CTJ_SkillKeeper's post-bind hook is empty.
	virtual bool Initialize(CTimedJobManager* pMgr, CGObjChar* pOwner, CInstanceTimedJob* pRecord, int32_t bActive);
	virtual bool Activate(); // 650E70, distinct from Initialize (650C50)
	bool BuildInsertQuery(std::string& out) const; // 650980
	bool BuildDeleteQuery(std::string& out) const; // 650AF0
	void DetachOwner();
	void SetRecordID(uint32_t id);
	bool Checkpoint();

	// [PARTIAL - Native 0x006511D0] (681 bytes); world-entry restoration is open
	// Advances elapsed time, updates DB checkpoints every 300s, and retires on expiry
	virtual uint32_t Tick(float fDelta);

	// [RECONSTRUCTED - Native 0x00651480] (24 bytes)
	// Flags active marker as 0 and secondary guard as 1
	virtual int32_t RequestRetirement(int32_t nReason = 1);

	// Virtual check whether job can be scheduled into the active set
	virtual bool CanSchedule() const { return true; }

	// Virtual disposal / release back to memory pool
	virtual void Dispose();

	// Accessors
	CGObjChar* GetOwner() const { return m_pOwner; }
	CInstanceTimedJob* GetRecord() const { return m_pRecord; }
	uint32_t GetActiveMarker() const { return m_dwActiveMarker; }

protected:
	friend class TimedJobTrigger;
	std::shared_ptr<TimedJobTrigger> m_trigger;
	// Exact struct layout proven from 0x006511D0 / 0x00650E70:
	CGObjChar*          m_pOwner;              // +0x04: Owner character
	CInstanceTimedJob*  m_pRecord;             // +0x08: Underlying DB record
	float               m_fCheckpointElapsed;  // +0x0C: Accumulated time towards 300s DB checkpoint
	void*               m_pServices;           // +0x10: External engine / scheduler service context
	uint32_t            m_dwActiveMarker;      // +0x14: 1 = Active, 0 = Expired / Pending retirement
	uint32_t            m_dwSecondaryGuard;    // +0x18: Guard flag preventing redundant retirements
	float               m_fAccrued;            // +0x1C: Fractional seconds accumulator (ticks when > 1.0f)
};

/**
 * [RECONSTRUCTED - Native VTable @ 0x00B04F2C]
 * CTJ_SkillKeeper
 *
 * Timed job dedicated to buff / skill instance duration tracking and expiration.
 */
class CTJ_SkillKeeper : public CTimedJob {
public:
	CTJ_SkillKeeper() = default;
	virtual ~CTJ_SkillKeeper() override = default;

	// Overrides Tick to manage buff retirement through CSkillManager
	virtual uint32_t Tick(float fDelta) override;
	bool Activate() override;
};

/**
 * [RECONSTRUCTED - Native 0x00655970 / 0x00655BE0]
 * CTimedJobManager
 *
 * Manager for character timed jobs, embedded in CGObjPC at offset +0x1DFC.
 */
class CTimedJobManager {
public:
	// Native 0x00655970: Constructor
	CTimedJobManager();
	virtual ~CTimedJobManager();

	// [RECONSTRUCTED - Native 0x00655BE0] (441 bytes)
	// Allocates a timed job and its DB record, initializes it, and schedules it
	bool Create(uint8_t byCategory, uint32_t dwSkillID, uint32_t dwTimeToKeep,
	            uint32_t dwData1, uint32_t dwData2, uint32_t dwData3,
	            uint32_t dwData4, uint32_t dwData5, uint32_t dwSerialLow,
	            uint32_t dwSerialHigh, uint32_t dwJID);

	// [RECONSTRUCTED - Native 0x00655F10]
	// Restores active timed jobs loaded from the database upon character login
	bool LoadFromDB(CGObjChar* pOwner, std::list<CInstanceTimedJob*>& records);
	bool ActivateLoadedJobs(); // 656060, after character restoration
	bool Checkpoint(); // 656220, periodic backup/logout
	void OnPersistenceFailure(const ShardQuery::Result& result);
	virtual bool SubmitQuery(std::string sql, bool returnsID, ShardQuery::Completion completion);

	// Factory virtual methods (Slots 1 - 3)
	virtual CTimedJob* CreateJob(uint8_t byCategory, uint32_t dwSkillID);
	virtual CInstanceTimedJob* CreateRecord();
	virtual void ReleaseRecord(CInstanceTimedJob* pRecord);

	// Ticks all scheduled jobs
	void Tick(float fDelta);

	// Clears all active jobs
	void Clear();

	void SetOwner(CGObjChar* pOwner) { m_pOwner = pOwner; }
	CGObjChar* GetOwner() const { return m_pOwner; }

private:
	CGObjChar*             m_pOwner;   // +0x04: Owner character
	std::set<std::shared_ptr<CTimedJob>> m_setJobs; // Native +08 owns jobs
	std::shared_ptr<int> m_lifetime = std::make_shared<int>(0);
};

/**
 * [RECONSTRUCTED - Native 0x0049A390] (102 bytes)
 * CreateOwnerTimedJob
 *
 * Global forwarder that accesses pOwner's CTimedJobManager (via virtual slot +0x5D8)
 * and initiates job creation.
 */
bool CreateOwnerTimedJob(CGObjChar* pOwner, uint8_t byCategory, uint32_t dwSkillID,
                         uint32_t dwTimeToKeep, uint32_t dwData1, uint32_t dwData2,
                         uint32_t dwData3, uint32_t dwData4, uint32_t dwData5,
                         uint32_t dwSerialLow, uint32_t dwSerialHigh, uint32_t dwJID);

#endif // _SR_GAMESERVER_TIMEDJOB_H_
