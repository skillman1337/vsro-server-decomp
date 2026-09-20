/**
 * ============================================================================
 * Silkroad Online - GameServer Core Game Simulation Class
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Game.h
 *
 * Implements CGame:
 *   - Native VTable @ 0x00AE07F0 (RTTI: .?AVCGame@@)
 *   - Native Ctor @ 0x00412610, Dtor @ 0x004127A0
 *   - Native singleton pointer g_pGame @ 0x00D6A8E0
 *   - Native Tick @ 0x00413840 (Frame heartbeat: 30 Hz / 33.33ms)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAME_H_
#define _SR_GAMESERVER_GAME_H_

#include <cstdint>
#include <map>
#include "GItem.h"
#include "GObj.h"
#include "TrijobMgr.h"
#include "GObjChar.h"
#include "ScheduledCallbacker.h"
#include "Lobby.h"
#include "Map.h"

class CGame;

// Global singleton pointer matching native 0x00D6A8E0
extern CGame* g_pGame;

// Global accumulators for game ticking (Native 0x00D6A8EC, 0x00D6A8F0)
extern float    g_fScheduledTickTime; // 0x00D6A8EC: Time accumulated for ScheduleTick
extern float    g_fWatchdogTime;      // 0x00D6A8F0: Time accumulated for periodic watchdog (3.0s)
extern uint32_t g_dwLastWatchdogTick; // 0x00C669B0: Last watchdog execution GetTickCount()
extern uint32_t g_bDisplayWindowEnabled; // 0x00C82534: Flag indicating display window is active

/**
 * tagFPSProfiler / tagPerfSampler
 * Native layout @ 0x00C669AC (size 28 bytes)
 * Sampled in CFPSProfiler_Sample @ 0x0041D3C0
 */
struct tagFPSProfiler {
	uint32_t dwReserved;        // +0x00: Status / reset flag
	uint32_t dwLastTick;        // +0x04: Last sampled GetTickCount()
	uint32_t dwAccumulatedMs;   // +0x08: Accumulated elapsed ms
	uint32_t dwSampleCount;     // +0x0C: Number of frames sampled
	uint32_t dwAverageFrameMs;  // +0x10: Computed average frame time in ms
	uint32_t dwMaxSamples;      // +0x14: Sample window (e.g. 100)
	uint32_t dwCachedFPS;       // +0x18: Cached FPS / avg frame time
};

extern tagFPSProfiler g_fpsProfiler; // Native 0x00C669AC

class CGame {
public:
	// [RECONSTRUCTED - Native 0x00412610]
	CGame();

	// [RECONSTRUCTED - Native 0x004127A0]
	virtual ~CGame();

	// [RECONSTRUCTED - Native 0x00412C20]
	// Subsystem initialization (locales, abuse filter, object pools, world partitions)
	int32_t Initialize();

	// [RECONSTRUCTED - Native 0x00414260]
	// Primary client game packet dispatcher
	int32_t ProcessClientGamePacket(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00413590]
	// Registers diagnostic performance counters ("Obj Count", "DBQuery Count", etc.)
	void RegisterPerformanceCounters();

	// [RECONSTRUCTED - Native 0x00412BE0]
	// Sets environmental weather state and broadcasts update to active regions
	void SetWeather(uint8_t byWeather);

	// [RECONSTRUCTED - Native 0x00413FB0]
	// Advances player status, per-character timers, and environment time
	void TickPlayers();

	// [RECONSTRUCTED - Native 0x00414120]
	// Advances global world manager regions and geography
	void TickWorldManager();

	// [RECONSTRUCTED - Native 0x00414140]
	// Drives global event daemon scheduler and timers
	void TickEventDaemon();

	// [RECONSTRUCTED - Native 0x00414890]
	// Routes subsystem packets (Party 0xB010, Spawn 0x300C, Target 0xAA00)
	int32_t DispatchSubsystemPacket(BSLib::CPacket* pMsg, void* pContext);

	// [RECONSTRUCTED - Native 0x004154D0]
	// Processes Game Master administrative commands (MoveToPlayer, Recall, SetTime)
	int32_t ProcessGMCommand(BSLib::CPacket* pMsg, void* pContext, uint32_t dwParam);

	// [RECONSTRUCTED - Native 0x004168F0]
	// Handles special goods trade merchant notifications (Opcode 0xBC00)
	void OnSpecialGoodsTradeNotice(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00416DB0]
	// Handles special goods trade schedule state (Opcodes 0x0C08 - 0x0C0A)
	void OnSpecialGoodsTradeSchedule(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x004169D0]
	void OnGuildMemberUpdate(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00416AD0]
	void OnGuildNoticeUpdate(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00416BD0]
	void OnGuildWarUpdate(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00416D00]
	void OnGuildDisband(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00417A80]
	// Notifies all connected clients of server shutdown/disconnection
	void NotifyAllClientsDisconnect();

	// [RECONSTRUCTED - Native 0x00417E10]
	// Flushes all dirty character states and saves character records
	void FlushAllCharacters();

	// [RECONSTRUCTED - Native 0x00417E80]
	// Schedules bulk asynchronous database backup for all character data
	void BackupCharacterData();

	// [RECONSTRUCTED - Native 0x00418110]
	// Validates and applies character teleport transitions
	void ApplyTeleportTransition(CGObjChar* pChar, uint32_t dwTeleportID, void* pContext);

	// [RECONSTRUCTED - Native 0x00413840]
	// Primary simulation frame heartbeat
	// Advances world scheduler, timers, entities, and performance metrics
	int32_t Tick(float fDeltaSeconds);

	// [RECONSTRUCTED - Native 0x004140B0]
	// Periodic 3-second watchdog and status update
	void PeriodicWatchdogTick();

	// [RECONSTRUCTED - Native 0x004139B0]
	// Updates all active ground items, progresses expiration timers, and despawns expired items
	void UpdateActiveItems(float fDeltaSec);

	// [RECONSTRUCTED - Native 0x00413B30]
	// Primary world entities simulation tick:
	//   Phase 1: Pumps character network packets, cleans up dead characters
	//   Phase 2: Advances character simulation ticks, cleans up dead characters
	//   Phase 3: Advances active skill object timers and despawns expired skills
	//   Phase 4: Advances event daemon timers and updates daemon lifecycles
	//   Phase 5: Advances world structures and siege building simulation
	//   Phase 6: Collects sliding-window performance profiler metrics
	uint32_t TickWorldObjects();

	// [RECONSTRUCTED - Native 0x004177D0]
	// Unregisters an active player character from the world and sessions
	uint32_t UnregisterPlayer(CGObjChar* pChar);

	// [RECONSTRUCTED - Native 0x00417200]
	// Registers a newly spawned game object into active world partitions
	void RegisterObject(CGObj* pObj);

	// [RECONSTRUCTED - Native 0x00417090]
	// Despawns and destroys an active game object from world partitions
	void DestroyObject(CGObj* pObj);

	// [RECONSTRUCTED - Native 0x00417090]
	// Despawns and destroys an item object, dispatches despawn event, and unregisters it
	void DestroyObject(CGItem* pItem);

	// [RECONSTRUCTED - Native 0x00417DB0]
	// Removes an item from the active item update map
	void RemoveItemFromUpdateList(CGItem* pItem);

	// [RECONSTRUCTED - Native 0x00417240]
	// Ready-to-play world entry handler (enters player into active world from lobby)
	bool EnterWorld(class CLobbyEntry* pEntry);

	// [RECONSTRUCTED - Native 0x0041CCF0]
	// Finds active character / entity by unique GameID
	CGObjChar* FindPlayer(uint32_t dwPlayerID);
	CGObjChar* FindObjectByID(uint32_t dwObjectID);

	// [RECONSTRUCTED - Native 0x004117E0]
	// Registers active character / entity into object registry map
	bool RegisterObject(CGObjChar* pChar);

	// [RECONSTRUCTED - Native 0x00418100]
	// Registers active player trade caravan
	Caravan* RegisterCaravan(uint32_t dwPlayerID);

	// [RECONSTRUCTED - Native 0x00414250]
	// Periodic scheduled environment and weather condition broadcast handler
	void OnScheduledEnvironmentUpdate();

	// [RECONSTRUCTED - Native 0x00416E90]
	// Inter-server link connected handler
	void OnServerConnected(void* pNode, void* pLink);

	// [RECONSTRUCTED - Native 0x00416F80]
	// Inter-server link disconnected handler
	void OnServerDisconnected(void* pNode);

	float GetTimeStep() const;
	uint32_t GetTargetFPS() const;

	// [RECONSTRUCTED - Native 0x0040B360] (eax = this, edi = session id, push pCmdSrc)
	// Inserts the network command source under its session id; 0 when the id is already registered.
	int32_t RegisterCmdSrcNet(uint32_t dwSessionID, class CCmdSrcNet* pCmdSrc);

	// [RECONSTRUCTED - Native 0x0040BAF0] (eax = this + 0x04, push session id)
	// Erases the session id; returns non-zero when it was registered.
	int32_t UnregisterCmdSrcNet(uint32_t dwSessionID);

public:
	// Exact struct layout matching native binary bytes:
	// CORRECTION (Claude): +0x04 is the session-id -> CCmdSrcNet registry used by CCmdSrcNet
	// slots 10/11 (0x0040B6C0 / 0x0040B6F0) and its destructor (0x0040B4C3).
	std::map<uint32_t, class CCmdSrcNet*> m_mapCmdSrcNet; // +0x04 - +0x0F
	uint8_t  m_pad10[0x30];      // +0x10 - +0x3F: Perf timers & subsystems
	float    m_fAccumulatedTime; // +0x40: Accumulated frame time
	float    m_fTimeStep;        // +0x44: Native 0.0333333351f (30 ticks/sec)
	uint32_t m_dwFPS;             // +0x48: Native 0x1E (30 FPS target)
	// CORRECTION (Claude): CMap is a member here; 0x00412677 constructs it at this+0x4C and it sets g_pMap.
	CMap     m_Map;              // +0x4C - +0x73
	uint8_t  m_pad74[0x218C];    // +0x74 - +0x21FF: Subsystems, schedulers

	// Native 0x004126C2 / 0x0041BFA0: Periodic callback scheduler (weather, world events)
	CScheduledCallbacker<CGame, uint32_t> m_scheduledCallbacker; // +0x2200 - +0x2217 (24 bytes)
	uint8_t  m_pad2218[0x1C];    // +0x2218 - +0x2233: Internal scheduler lists

	// Native 0x004139B0 / 0x0041C0A0: Active item update registry
	std::map<uint32_t, CGItem*> m_mapActiveItems; // +0x2234: Active ground items undergoing ticking

	// Native 0x004117E0 / 0x0041CCF0: Active entity registry map
	std::map<uint32_t, CGObjChar*> m_mapObjects; // +0x22B4: Active entities indexed by GameID
};

// Diagnostic tracking for fatal exception handler (Native 0x00C825C4 - 0x00C825CC)
extern uint32_t g_dwLatestUpdatedCharTID;          // @ 0x00C825C4
extern uint32_t g_dwLatestProcessedMsgID;          // @ 0x00C825C8
extern char     g_szLatestUpdatedCharCodename[64]; // @ 0x00C825CC

// Simulation profiling metrics (Native 0x00C669E8 - 0x00C669DC)
extern uint32_t g_dwCharPumpNetMsgTickStart;       // @ 0x00C669E8
extern uint32_t g_dwCharPumpNetMsgTimeAccum;       // @ 0x00C669EC
extern uint32_t g_dwCharPumpNetMsgSampleCount;     // @ 0x00C669F0
extern uint32_t g_dwCharPumpNetMsgSampleLimit;     // @ 0x00C669F8
extern uint32_t g_dwCharPumpNetMsgTimeAvg;         // @ 0x00C669FC
extern uint32_t g_dwWorldObjUpdateTickStart;       // @ 0x00C669CC
extern uint32_t g_dwWorldObjUpdateTimeAccum;       // @ 0x00C669D0
extern uint32_t g_dwWorldObjUpdateSampleCount;     // @ 0x00C669D4
extern uint32_t g_dwWorldObjUpdateSampleLimit;     // @ 0x00C669DC
extern uint32_t g_dwWorldObjUpdateTimeAvg;         // @ 0x00C669D8

// [RECONSTRUCTED - Native 0x0041CCF0]
// CGame_FindObjectByID: Resolves runtime GameID / GlobalID to CGObjChar*
CGObjChar* CGame_FindObjectByID(uint32_t dwGameID);

// [RECONSTRUCTED - Native 0x0041BFA0]
// CGame_ScheduleTick: Dispatches scheduled timer callbacks via CScheduledCallbacker<CGame, uint32_t>
void CGame_ScheduleTick(uint32_t dwDeltaMs);

// [RECONSTRUCTED - Native 0x009759D0]
// CGame_ScheduleExecutor_Tick: Ticks calendar-based scheduled game events (Fortress war, Roc, CTF, Arena)
void CGame_ScheduleExecutor_Tick();

// Legacy compatibility alias
void CGameEventManager_Tick();

// [RECONSTRUCTED - Native 0x0041D3C0]
// Samples average frame time and FPS
uint32_t CFPSProfiler_Sample(tagFPSProfiler* pProfiler);

#endif // _SR_GAMESERVER_GAME_H_
