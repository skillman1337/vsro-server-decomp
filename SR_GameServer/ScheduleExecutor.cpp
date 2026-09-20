/**
 * ============================================================================
 * Silkroad Online - NewSchedule Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\ScheduleExecutor.cpp
 *                  D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Game_ScheduleExecutor.cpp
 * ============================================================================
 */

#include "ScheduleExecutor.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <ctime>

namespace NewSchedule {

/*
================
IScheduleJob::IScheduleJob
Native 0x00674AD0
================
*/
IScheduleJob::IScheduleJob()
    : m_strJobName()
    , m_dwDayOfWeek(0)
    , m_dwStartHour(0)
    , m_dwStartMinute(0)
    , m_dwDurationMin(0)
    , m_dwIntervalMin(0)
    , m_strScheduleRule() {
}

/*
================
IScheduleJob::~IScheduleJob
Native 0x00674C20
================
*/
IScheduleJob::~IScheduleJob() {
}

/*
================
IScheduleJob::SetTarget
Native 0x00674B40
================
*/
void IScheduleJob::SetTarget(const std::string& strTarget) {
    m_strScheduleRule = strTarget;
}

/*
================
IScheduleJob::InitScheduleRule
Native 0x00976DD0 / 0x00975700
================
*/
int32_t IScheduleJob::InitScheduleRule(const void* pRuleData) {
    if (!pRuleData) return 0;
    // Copies schedule metadata
    return 1;
}

void IScheduleJob::OnPrepare() {}
void IScheduleJob::OnStart() {}
void IScheduleJob::OnStop() {}
void IScheduleJob::OnReset() {}
void IScheduleJob::OnUpdate() {}
void IScheduleJob::OnNotification() {}
void IScheduleJob::OnCustomAction() {}

/*
================
CScheduleExecutor::CScheduleExecutor
Native 0x00975750
================
*/
CScheduleExecutor::CScheduleExecutor()
    : m_mapJobs()
    , m_pJobContainer(nullptr) {
}

/*
================
CScheduleExecutor::~CScheduleExecutor
Native 0x00674CB0
================
*/
CScheduleExecutor::~CScheduleExecutor() {
}

/*
================
CScheduleExecutor::Tick
Native 0x009759D0
================
*/
void CScheduleExecutor::Tick() {
    BSLib::CTime cCurrentTime;
    cCurrentTime.SetTime(::time(nullptr));

    for (auto& pair : m_mapJobs) {
        IScheduleJob* pJob = pair.second;
        if (pJob != nullptr) {
            pJob->OnCheckSchedule(&cCurrentTime);
        }
    }
}

} // namespace NewSchedule

/*
================
IGame_ScheduleJob::IGame_ScheduleJob
Native 0x00674ED0
================
*/
IGame_ScheduleJob::IGame_ScheduleJob()
    : NewSchedule::IScheduleJob()
    , m_strEventName()
    , m_timeStart()
    , m_dwState(0) {
}

/*
================
IGame_ScheduleJob::~IGame_ScheduleJob
Native 0x00674F80
================
*/
IGame_ScheduleJob::~IGame_ScheduleJob() {
}

/*
================
IGame_ScheduleJob::OnCheckSchedule
Native purecall Slot 6 (+0x18)
================
*/
int32_t IGame_ScheduleJob::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    if (!pCurrentTime) return 0;
    if (pCurrentTime->GetTime() >= m_timeStart.GetTime() && m_dwState == 0) {
        m_dwState = 1;
        OnStart();
        return 1;
    }
    return 0;
}

// ============================================================================
// Concrete Scheduled Job Implementations
// ============================================================================

/*
================
CGameSJ_SpecialGoodsSelling
Native 0x00674DD0 / 0x00679FD0
Specialty goods trade selling hours
================
*/
CGameSJ_SpecialGoodsSelling::CGameSJ_SpecialGoodsSelling() {
    m_strJobName = "CGameSJ_SpecialGoodsSelling";
    m_strEventName = "SpecialGoodsSelling";
}

int32_t CGameSJ_SpecialGoodsSelling::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    if (!pCurrentTime) return 0;
    if (pCurrentTime->GetTime() > m_timeStart.GetTime() && m_dwState == 0) {
        m_dwState = 1;
        // Native 0x00679FD0: Broadcast opcode 0x300C and log state transition
        BSLib::Log_Printf(0x2000001, "Started Selling Special Goods");
    }
    return 1;
}

CGameSJ_SiegeRequestPeriod::CGameSJ_SiegeRequestPeriod() {
    m_strJobName = "CGameSJ_SiegeRequestPeriod";
    m_strEventName = "SiegeRequestPeriod";
}
int32_t CGameSJ_SiegeRequestPeriod::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_SiegePeriod::CGameSJ_SiegePeriod() {
    m_strJobName = "CGameSJ_SiegePeriod";
    m_strEventName = "SiegePeriod";
}
int32_t CGameSJ_SiegePeriod::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_SiegeTaxPeriod::CGame_SiegeTaxPeriod() {
    m_strJobName = "CGame_SiegeTaxPeriod";
    m_strEventName = "SiegeTaxPeriod";
}
int32_t CGame_SiegeTaxPeriod::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_Roc::CGameSJ_Roc() {
    m_strJobName = "CGameSJ_Roc";
    m_strEventName = "Roc";
}
int32_t CGameSJ_Roc::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_ChinsTomb::CGameSJ_ChinsTomb() {
    m_strJobName = "CGameSJ_ChinsTomb";
    m_strEventName = "ChinsTomb";
}
int32_t CGameSJ_ChinsTomb::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_TempleOfSelkisAndNeithGate::CGameSJ_TempleOfSelkisAndNeithGate() {
    m_strJobName = "CGameSJ_TempleOfSelkisAndNeithGate";
    m_strEventName = "TempleOfSelkisAndNeithGate";
}
int32_t CGameSJ_TempleOfSelkisAndNeithGate::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_TempleOfAnubisAndIsisGate::CGameSJ_TempleOfAnubisAndIsisGate() {
    m_strJobName = "CGameSJ_TempleOfAnubisAndIsisGate";
    m_strEventName = "TempleOfAnubisAndIsisGate";
}
int32_t CGameSJ_TempleOfAnubisAndIsisGate::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_TempleOfHaroerisGate::CGameSJ_TempleOfHaroerisGate() {
    m_strJobName = "CGameSJ_TempleOfHaroerisGate";
    m_strEventName = "TempleOfHaroerisGate";
}
int32_t CGameSJ_TempleOfHaroerisGate::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_FlagWorldParticipation::CGameSJ_FlagWorldParticipation() {
    m_strJobName = "CGameSJ_FlagWorldParticipation";
    m_strEventName = "FlagWorldParticipation";
}
int32_t CGameSJ_FlagWorldParticipation::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGameSJ_FlagWorld::CGameSJ_FlagWorld() {
    m_strJobName = "CGameSJ_FlagWorld";
    m_strEventName = "FlagWorld";
}
int32_t CGameSJ_FlagWorld::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaRandomParticipation::CGame_BattleArenaRandomParticipation() {
    m_strJobName = "CGame_BattleArenaRandomParticipation";
    m_strEventName = "BattleArenaRandomParticipation";
}
int32_t CGame_BattleArenaRandomParticipation::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaRandom::CGame_BattleArenaRandom() {
    m_strJobName = "CGame_BattleArenaRandom";
    m_strEventName = "BattleArenaRandom";
}
int32_t CGame_BattleArenaRandom::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaPartyParticipation::CGame_BattleArenaPartyParticipation() {
    m_strJobName = "CGame_BattleArenaPartyParticipation";
    m_strEventName = "BattleArenaPartyParticipation";
}
int32_t CGame_BattleArenaPartyParticipation::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaParty::CGame_BattleArenaParty() {
    m_strJobName = "CGame_BattleArenaParty";
    m_strEventName = "BattleArenaParty";
}
int32_t CGame_BattleArenaParty::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaGuildParticipation::CGame_BattleArenaGuildParticipation() {
    m_strJobName = "CGame_BattleArenaGuildParticipation";
    m_strEventName = "BattleArenaGuildParticipation";
}
int32_t CGame_BattleArenaGuildParticipation::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaGuild::CGame_BattleArenaGuild() {
    m_strJobName = "CGame_BattleArenaGuild";
    m_strEventName = "BattleArenaGuild";
}
int32_t CGame_BattleArenaGuild::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaJobParticipation::CGame_BattleArenaJobParticipation() {
    m_strJobName = "CGame_BattleArenaJobParticipation";
    m_strEventName = "BattleArenaJobParticipation";
}
int32_t CGame_BattleArenaJobParticipation::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGame_BattleArenaJob::CGame_BattleArenaJob() {
    m_strJobName = "CGame_BattleArenaJob";
    m_strEventName = "BattleArenaJob";
}
int32_t CGame_BattleArenaJob::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

CGmaeSJ_EUBusinessEventUpdate::CGmaeSJ_EUBusinessEventUpdate() {
    m_strJobName = "CGmaeSJ_EUBusinessEventUpdate";
    m_strEventName = "EUBusinessEventUpdate";
}
int32_t CGmaeSJ_EUBusinessEventUpdate::OnCheckSchedule(const BSLib::CTime* pCurrentTime) {
    return IGame_ScheduleJob::OnCheckSchedule(pCurrentTime);
}

// ============================================================================
// CGame_ScheduleExecutor Implementation
// ============================================================================

/*
================
CGame_ScheduleExecutor::GetInstance
Native singleton at 0x00D161F4
================
*/
CGame_ScheduleExecutor* CGame_ScheduleExecutor::GetInstance() {
    static CGame_ScheduleExecutor s_instance;
    return &s_instance;
}

/*
================
CGame_ScheduleExecutor::CGame_ScheduleExecutor
Native 0x00975750
================
*/
CGame_ScheduleExecutor::CGame_ScheduleExecutor()
    : NewSchedule::CScheduleExecutor()
    , m_vecJobs() {
}

/*
================
CGame_ScheduleExecutor::~CGame_ScheduleExecutor
Native 0x00674CB0
================
*/
CGame_ScheduleExecutor::~CGame_ScheduleExecutor() {
    ClearJobs();
}

/*
================
CGame_ScheduleExecutor::RegisterJob
Native 0x00975A80
================
*/
bool CGame_ScheduleExecutor::RegisterJob(IGame_ScheduleJob* pJob) {
    if (!pJob) {
        return false;
    }
    uint32_t dwJobId = static_cast<uint32_t>(m_vecJobs.size() + 1);
    m_mapJobs[dwJobId] = pJob;
    m_vecJobs.push_back(pJob);
    return true;
}

/*
================
CGame_ScheduleExecutor::ClearJobs
Native 0x00975890
================
*/
void CGame_ScheduleExecutor::ClearJobs() {
    for (IGame_ScheduleJob* pJob : m_vecJobs) {
        delete pJob;
    }
    m_vecJobs.clear();
    m_mapJobs.clear();
}

/*
================
CGame_ScheduleExecutor::RegisterDefaultJobs
Native 0x00676490
Exact Joymax 20 default schedule jobs registration sequence
================
*/
bool CGame_ScheduleExecutor::RegisterDefaultJobs() {
    struct JobEntry {
        const char* pszName;
        IGame_ScheduleJob* (*pfnCreate)();
    };

    static const JobEntry s_defaultJobs[] = {
        { "CGameSJ_SpecialGoodsSelling",              []() -> IGame_ScheduleJob* { return new CGameSJ_SpecialGoodsSelling(); } },
        { "CGameSJ_SiegeRequestPeriod",              []() -> IGame_ScheduleJob* { return new CGameSJ_SiegeRequestPeriod(); } },
        { "CGameSJ_SiegePeriod",                     []() -> IGame_ScheduleJob* { return new CGameSJ_SiegePeriod(); } },
        { "CGame_SiegeTaxPeriod",                    []() -> IGame_ScheduleJob* { return new CGame_SiegeTaxPeriod(); } },
        { "CGameSJ_Roc",                             []() -> IGame_ScheduleJob* { return new CGameSJ_Roc(); } },
        { "CGameSJ_ChinsTomb",                        []() -> IGame_ScheduleJob* { return new CGameSJ_ChinsTomb(); } },
        { "CGameSJ_TempleOfSelkisAndNeithGate",      []() -> IGame_ScheduleJob* { return new CGameSJ_TempleOfSelkisAndNeithGate(); } },
        { "CGameSJ_TempleOfAnubisAndIsisGate",       []() -> IGame_ScheduleJob* { return new CGameSJ_TempleOfAnubisAndIsisGate(); } },
        { "CGameSJ_TempleOfHaroerisGate",            []() -> IGame_ScheduleJob* { return new CGameSJ_TempleOfHaroerisGate(); } },
        { "CGameSJ_FlagWorldParticipation",          []() -> IGame_ScheduleJob* { return new CGameSJ_FlagWorldParticipation(); } },
        { "CGameSJ_FlagWorld",                       []() -> IGame_ScheduleJob* { return new CGameSJ_FlagWorld(); } },
        { "CGame_BattleArenaRandomParticipation",     []() -> IGame_ScheduleJob* { return new CGame_BattleArenaRandomParticipation(); } },
        { "CGame_BattleArenaRandom",                  []() -> IGame_ScheduleJob* { return new CGame_BattleArenaRandom(); } },
        { "CGame_BattleArenaPartyParticipation",      []() -> IGame_ScheduleJob* { return new CGame_BattleArenaPartyParticipation(); } },
        { "CGame_BattleArenaParty",                   []() -> IGame_ScheduleJob* { return new CGame_BattleArenaParty(); } },
        { "CGame_BattleArenaGuildParticipation",      []() -> IGame_ScheduleJob* { return new CGame_BattleArenaGuildParticipation(); } },
        { "CGame_BattleArenaGuild",                   []() -> IGame_ScheduleJob* { return new CGame_BattleArenaGuild(); } },
        { "CGame_BattleArenaJobParticipation",        []() -> IGame_ScheduleJob* { return new CGame_BattleArenaJobParticipation(); } },
        { "CGame_BattleArenaJob",                     []() -> IGame_ScheduleJob* { return new CGame_BattleArenaJob(); } },
        { "CGmaeSJ_EUBusinessEventUpdate",            []() -> IGame_ScheduleJob* { return new CGmaeSJ_EUBusinessEventUpdate(); } },
    };

    for (const auto& entry : s_defaultJobs) {
        IGame_ScheduleJob* pJob = entry.pfnCreate();
        if (!RegisterJob(pJob)) {
            // Native 0x00676490 log format
            BSLib::Log_Printf(0x2000001, "ScheduleExecutor : %s Default ScheduleJob Register Failed !!!", entry.pszName);
        }
    }

    return true;
}

/*
================
CGame_ScheduleExecutor::Tick
Native 0x009759D0
================
*/
void CGame_ScheduleExecutor::Tick() {
    BSLib::CTime cCurrentTime;
    cCurrentTime.SetTime(::time(nullptr));

    for (auto it = m_mapJobs.begin(); it != m_mapJobs.end(); ++it) {
        NewSchedule::IScheduleJob* pJob = it->second;
        if (pJob != nullptr) {
            pJob->OnCheckSchedule(&cCurrentTime);
        }
    }
}

/*
================
CGame_ScheduleExecutor_Tick
Native 0x009759D0
Global tick routine called every main game loop frame (0x0041392D)
================
*/
void CGame_ScheduleExecutor_Tick() {
    CGame_ScheduleExecutor* pExecutor = CGame_ScheduleExecutor::GetInstance();
    if (pExecutor != nullptr) {
        pExecutor->Tick();
    }
}

/*
================
CGameEventManager_Tick
Legacy alias for CGame_ScheduleExecutor_Tick
================
*/
void CGameEventManager_Tick() {
    CGame_ScheduleExecutor_Tick();
}
