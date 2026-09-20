/**
 * ============================================================================
 * Silkroad Online - NewSchedule Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\ScheduleExecutor.h
 *                  D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Game_ScheduleExecutor.h
 *
 * Implements the global scheduled event executor:
 *   - NewSchedule::IScheduleJob @ 0x00674AD0 (vftable @ 0x00B072E4)
 *   - IGame_ScheduleJob @ 0x00674ED0 (vftable @ 0x00B07310)
 *   - NewSchedule::CScheduleExecutor @ 0x00B07308
 *   - CGame_ScheduleExecutor @ 0x00D161F4 (vftable @ 0x00B07340)
 *   - CGame_ScheduleExecutor_Tick @ 0x009759D0 (alias CGameEventManager_Tick)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SCHEDULEEXECUTOR_H_
#define _SR_GAMESERVER_SCHEDULEEXECUTOR_H_

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include "../JMX_Library/BSLib/BSObj.h"
#include "../JMX_Library/BSLib/Util/TimeSpan.h"

// Forward declaration
class CGame_ScheduleExecutor;

namespace NewSchedule {

/**
 * [RECONSTRUCTED - 0x00674AD0]
 * IScheduleJob
 * VTable @ 0x00B072E4
 */
class IScheduleJob {
public:
    IScheduleJob();
    virtual ~IScheduleJob();

    // VTable Slot 0 (+0x00)
    virtual void SetTarget(const std::string& strTarget);
    // VTable Slot 1 (+0x04)
    virtual int32_t InitScheduleRule(const void* pRuleData);
    // VTable Slot 2 (+0x08)
    virtual void OnPrepare();
    // VTable Slot 3 (+0x0C)
    virtual void OnStart();
    // VTable Slot 4 (+0x10)
    virtual void OnStop();
    // VTable Slot 5 (+0x14)
    virtual void OnReset();
    // VTable Slot 6 (+0x18)
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) = 0;
    // VTable Slot 7 (+0x1C)
    virtual void OnUpdate();
    // VTable Slot 8 (+0x20)
    virtual void OnNotification();
    // VTable Slot 9 (+0x24)
    virtual void OnCustomAction();

public:
    std::string m_strJobName;      // +0x04: Job identifier name
    uint32_t    m_dwDayOfWeek;     // +0x20: Day of week mask/index
    uint32_t    m_dwStartHour;     // +0x24: Scheduled start hour
    uint32_t    m_dwStartMinute;   // +0x28: Scheduled start minute
    uint32_t    m_dwDurationMin;   // +0x2C: Duration in minutes
    uint32_t    m_dwIntervalMin;   // +0x30: Interval in minutes
    std::string m_strScheduleRule; // +0x34: Recurrence / schedule rule text
};

/**
 * [RECONSTRUCTED - 0x00B07308]
 * CScheduleExecutor
 */
class CScheduleExecutor {
public:
    CScheduleExecutor();
    virtual ~CScheduleExecutor();

    virtual void Tick();

public:
    std::map<uint32_t, IScheduleJob*> m_mapJobs;        // +0x04 (Head at +0x08, Size at +0x0C)
    void*                             m_pJobContainer;  // +0x10: Container / list allocator
};

} // namespace NewSchedule

/**
 * [RECONSTRUCTED - 0x00674ED0]
 * IGame_ScheduleJob
 * VTable @ 0x00B07310
 */
class IGame_ScheduleJob : public NewSchedule::IScheduleJob {
public:
    IGame_ScheduleJob();
    virtual ~IGame_ScheduleJob() override;

    // VTable Slot 6 (+0x18)
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;

public:
    std::string  m_strEventName; // +0x54: Event display / config name
    BSLib::CTime m_timeStart;    // +0x70: Next scheduled start time
    uint32_t     m_dwState;      // +0x98: Active status (0 = idle, 1 = active/running)
};

// ============================================================================
// Concrete Game Scheduled Jobs (Registered by Native 0x00676490)
// ============================================================================

class CGameSJ_SpecialGoodsSelling : public IGame_ScheduleJob {
public:
    CGameSJ_SpecialGoodsSelling();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_SiegeRequestPeriod : public IGame_ScheduleJob {
public:
    CGameSJ_SiegeRequestPeriod();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_SiegePeriod : public IGame_ScheduleJob {
public:
    CGameSJ_SiegePeriod();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_SiegeTaxPeriod : public IGame_ScheduleJob {
public:
    CGame_SiegeTaxPeriod();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_Roc : public IGame_ScheduleJob {
public:
    CGameSJ_Roc();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_ChinsTomb : public IGame_ScheduleJob {
public:
    CGameSJ_ChinsTomb();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_TempleOfSelkisAndNeithGate : public IGame_ScheduleJob {
public:
    CGameSJ_TempleOfSelkisAndNeithGate();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_TempleOfAnubisAndIsisGate : public IGame_ScheduleJob {
public:
    CGameSJ_TempleOfAnubisAndIsisGate();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_TempleOfHaroerisGate : public IGame_ScheduleJob {
public:
    CGameSJ_TempleOfHaroerisGate();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_FlagWorldParticipation : public IGame_ScheduleJob {
public:
    CGameSJ_FlagWorldParticipation();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGameSJ_FlagWorld : public IGame_ScheduleJob {
public:
    CGameSJ_FlagWorld();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaRandomParticipation : public IGame_ScheduleJob {
public:
    CGame_BattleArenaRandomParticipation();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaRandom : public IGame_ScheduleJob {
public:
    CGame_BattleArenaRandom();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaPartyParticipation : public IGame_ScheduleJob {
public:
    CGame_BattleArenaPartyParticipation();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaParty : public IGame_ScheduleJob {
public:
    CGame_BattleArenaParty();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaGuildParticipation : public IGame_ScheduleJob {
public:
    CGame_BattleArenaGuildParticipation();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaGuild : public IGame_ScheduleJob {
public:
    CGame_BattleArenaGuild();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaJobParticipation : public IGame_ScheduleJob {
public:
    CGame_BattleArenaJobParticipation();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGame_BattleArenaJob : public IGame_ScheduleJob {
public:
    CGame_BattleArenaJob();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

class CGmaeSJ_EUBusinessEventUpdate : public IGame_ScheduleJob {
public:
    CGmaeSJ_EUBusinessEventUpdate();
    virtual int32_t OnCheckSchedule(const BSLib::CTime* pCurrentTime) override;
};

// ============================================================================
// CGame_ScheduleExecutor (Native singleton at 0x00D161F4)
// ============================================================================

class CGame_ScheduleExecutor : public NewSchedule::CScheduleExecutor {
public:
    static CGame_ScheduleExecutor* GetInstance();

    CGame_ScheduleExecutor();
    virtual ~CGame_ScheduleExecutor() override;

    // [RECONSTRUCTED - 0x00676490]
    // Instantiates and registers the 20 default game schedule jobs
    bool RegisterDefaultJobs();

    // [RECONSTRUCTED - 0x00975A80]
    // Registers a single schedule job into the active map
    bool RegisterJob(IGame_ScheduleJob* pJob);

    // [RECONSTRUCTED - 0x00975890]
    // Clears and frees all registered schedule jobs
    void ClearJobs();

    // [RECONSTRUCTED - 0x009759D0]
    // Core frame tick: iterates jobs and dispatches OnCheckSchedule(&currentTime)
    virtual void Tick() override;

public:
    std::vector<IGame_ScheduleJob*> m_vecJobs; // Job tracking list
};

// Global tick function matching native 0x009759D0
void CGame_ScheduleExecutor_Tick();

// Legacy compatibility alias
void CGameEventManager_Tick();

#endif // _SR_GAMESERVER_SCHEDULEEXECUTOR_H_
