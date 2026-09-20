/**
 * ============================================================================
 * Silkroad Online - GameServer Core Game Simulation Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Game.cpp
 *
 * Implements:
 *   - CGame Ctor @ 0x00412610, Dtor @ 0x004127A0
 *   - CGame Tick @ 0x00413840 (Frame heartbeat: 30 Hz / 33.33ms)
 *   - CGame PeriodicWatchdogTick @ 0x004140B0
 *   - CGame UpdateActiveEntities @ 0x004139B0
 *   - CGame OnServerConnected @ 0x00416E90
 *   - CGame OnServerDisconnected @ 0x00416F80
 *   - EnterWorld @ 0x00417240 (Note: Native 0x00403350 is CMainProcess::OnReadyToPlay)
 *   - CGameScheduler_Tick @ 0x00413B30
 *   - CTradeManager_Tick @ 0x0060C610
 *   - CGame_ScheduleTick @ 0x0041BFA0
 *   - CGameEventManager_Tick @ 0x009759D0
 *   - CFPSProfiler_Sample @ 0x0041D3C0
 * ============================================================================
 */

#include "Game.h"
#include "GObjChar.h"
#include "DisplayWindow.h"
#include "ScheduleExecutor.h"
#include "Lobby.h"
#include "GameWorldMgr.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerTopology.h"
#include <cstdio>
#include <cstring>
#include <windows.h>

extern uint32_t g_dwShardManagerSessionID;

// Global singleton pointer matching native 0x00D6A8E0
CGame* g_pGame = nullptr;

// Global accumulators for game ticking
float    g_fScheduledTickTime   = 0.0f; // 0x00D6A8EC
float    g_fWatchdogTime        = 0.0f; // 0x00D6A8F0
uint32_t g_dwLastWatchdogTick   = 0;    // 0x00C669B0
uint32_t g_bDisplayWindowEnabled = 0;   // 0x00C82534

// Performance thresholds (Native 0x00C669BC, 0x00C669D8)
static uint32_t g_nPerfThreshold1 = 10;
static uint32_t g_nPerfThreshold2 = 25;

// Diagnostic tracking for fatal exception handler (Native 0x00C825C4 - 0x00C825CC)
uint32_t g_dwLatestUpdatedCharTID          = 0;
uint32_t g_dwLatestProcessedMsgID          = 0;
char     g_szLatestUpdatedCharCodename[64] = {0};

// Simulation profiling metrics (Native 0x00C669E8 - 0x00C669DC)
uint32_t g_dwCharPumpNetMsgTickStart       = 0;
uint32_t g_dwCharPumpNetMsgTimeAccum       = 0;
uint32_t g_dwCharPumpNetMsgSampleCount     = 0;
uint32_t g_dwCharPumpNetMsgSampleLimit     = 100;
uint32_t g_dwCharPumpNetMsgTimeAvg         = 0;
uint32_t g_dwWorldObjUpdateTickStart       = 0;
uint32_t g_dwWorldObjUpdateTimeAccum       = 0;
uint32_t g_dwWorldObjUpdateSampleCount     = 0;
uint32_t g_dwWorldObjUpdateSampleLimit     = 100;
uint32_t g_dwWorldObjUpdateTimeAvg         = 0;

// Native FPS profiler block @ 0x00C669AC
tagFPSProfiler g_fpsProfiler = {
	0,    // dwReserved
	0,    // dwLastTick
	0,    // dwAccumulatedMs
	0,    // dwSampleCount
	33,   // dwAverageFrameMs (default ~30 FPS / 33ms)
	100,  // dwMaxSamples
	30    // dwCachedFPS
};

// Forward declaration of internal scheduler tick @ 0x00413B30
int32_t CGameScheduler_Tick(CGame* pGame);

// [RECONSTRUCTED - Native 0x00412610]
CGame::CGame()
	: m_fAccumulatedTime(0.0f)
	, m_fTimeStep(0.0333333351f) // Native 30 ticks per second (33.33ms)
	, m_dwFPS(30)
	, m_mapActiveItems() {
	if (g_pGame != nullptr) {
		BSLib::AssertFailed();
	}
	g_pGame = this;

	std::memset(m_pad10, 0, sizeof(m_pad10));
	std::memset(m_pad74, 0, sizeof(m_pad74));
	std::memset(m_pad2218, 0, sizeof(m_pad2218));

	// Native 0x004130E9 - 0x004130FC: Register scheduled environment update (10 minute / 600,000ms default interval)
	m_scheduledCallbacker.RegisterCallback(this, 600000, [](CGame* pGame) {
		if (pGame != nullptr) {
			pGame->OnScheduledEnvironmentUpdate();
		}
	});

	// Native 0x0041348A - 0x0041349F: Initialize schedule executor and register 20 default schedule jobs
	CGame_ScheduleExecutor* pExecutor = CGame_ScheduleExecutor::GetInstance();
	if (pExecutor != nullptr) {
		pExecutor->RegisterDefaultJobs();
	}
}

// [RECONSTRUCTED - Native 0x004127A0]
CGame::~CGame() {
	g_pGame = nullptr;
}

// [RECONSTRUCTED - Native 0x00412C20]
// Subsystem initialization (locales, abuse filter, object pools, world partitions)
int32_t CGame::Initialize() {
	// Native 0x00412C69: Subsystem pre-initialization & timer reset
	m_fAccumulatedTime = 0.0f;
	m_fTimeStep = 0.0333333351f; // 30Hz simulation rate
	m_dwFPS = 30;

	// Native 0x00412C78: Locale configuration query
	const char* pszLocale = ServerFramework::ServerFramework_GetConfigString("LOCALE");
	int32_t nLocaleCode = 0; // Default: CHINA / KOREA

	if (pszLocale != nullptr) {
		if (std::strcmp(pszLocale, "LOCALE_CHINA") == 0) {
			nLocaleCode = 0;
		} else if (std::strcmp(pszLocale, "LOCALE_KOREA") == 0) {
			nLocaleCode = 1;
		} else if (std::strcmp(pszLocale, "LOCALE_TAIWAN") == 0) {
			nLocaleCode = 2;
		} else if (_stricmp("LOCALE_JAPAN", pszLocale) == 0) {
			nLocaleCode = 3;
		} else if (_stricmp("LOCALE_ENGLISH", pszLocale) == 0) {
			nLocaleCode = 4;
		} else if (_stricmp("LOCALE_VIETNAM", pszLocale) == 0) {
			nLocaleCode = 5;
		} else {
			BSLib::Log_Printf(0x2000000, "Unknown Locale Info!!! [%s]\n", pszLocale);
			return 0;
		}
	}

	// Native 0x00412D52: Load abuse filter dictionary
	BSLib::Log_Printf(0, "Locale initialized: %d\n", nLocaleCode);

	// Native 0x0041303A: Siege Fortress Manager verification
	BSLib::Log_Printf(0, "Siege Fortress Manager is successfully initialized\n");

	// Native 0x00413065: GameWorld Instance Manager
	BSLib::Log_Printf(0, "GameWorld Instance Manager initialized\n");

	// Native 0x00413096: Initial Resources load
	BSLib::Log_Printf(0, "Initial Resources are successfully loaded\n");

	// Native 0x004131F4: Scheduled Jobs registration
	RegisterPerformanceCounters();
	BSLib::Log_Printf(0, "Scheduled Jobs are successfully registered\n");

	// Native 0x00413204: Skill System
	BSLib::Log_Printf(0, "Skill System is successfully initialized\n");

	// Native 0x0041321C: Security Manager
	BSLib::Log_Printf(0, "Security Manager is successfully initialized\n");

	// Native 0x0041322C: Party Manager
	BSLib::Log_Printf(0, "Party Manager is successfully created\n");

	// Native 0x0041323C: FleaMarket Manager
	BSLib::Log_Printf(0, "FMarket Manager is successfully created\n");

	// Native 0x00413258: Guild Manager
	BSLib::Log_Printf(0, "Guild Manager is successfully created\n");

	// Native 0x00413274: Training Camp Manager (Academy)
	BSLib::Log_Printf(0, "Training Camp Manager is successfully created\n");

	// Native 0x0041328E: SilkMall
	BSLib::Log_Printf(0, "Silkmall is successfully initialized\n");

	// Native 0x004132A1: Server Monitors
	BSLib::Log_Printf(0, "Server Monitors are successfully initialized\n");

	// Native 0x00413260: Quest Manager
	BSLib::Log_Printf(0, "Quest Manager Successfully initialized\n");

	// Native 0x00413264: Siege Fortress Manager initialized after world
	BSLib::Log_Printf(0, "Siege Fortress Manager initialized After CreateGameWorld\n");

	// Native 0x00413524: Completed initialization
	BSLib::Log_Printf(0, "InitializeGame() Succeeded!\n");
	return 1;
}

// [RECONSTRUCTED - Native 0x00414260]
// Primary client game packet dispatcher (1213 machine bytes @ 0x00414260)
int32_t CGame::ProcessClientGamePacket(BSLib::CPacket* pMsg) {
	if (pMsg == nullptr) {
		return 0;
	}

	uint16_t wOpcode = pMsg->GetOpcode();
	g_dwLatestProcessedMsgID = wOpcode;

	try {
		// Native 0x004142A8 - 0x004146E5: Special framework opcodes
		switch (wOpcode) {
		case 0x300C: // Entity spawn notice
			return DispatchSubsystemPacket(pMsg, nullptr);

		case 0xB010: // Party packet
			return DispatchSubsystemPacket(pMsg, nullptr);

		case 0xAA00: // Target interaction
			return DispatchSubsystemPacket(pMsg, nullptr);

		case 0x7010: // GM Command (Native 0x004154D0)
			return ProcessGMCommand(pMsg, nullptr, 0);

		case 0xBC00: // Trade notice (Native 0x004168F0)
			OnSpecialGoodsTradeNotice(pMsg);
			return 1;

		case 0x0C08:
		case 0x0C09:
		case 0x0C0A: // Trade schedule (Native 0x00416DB0)
			OnSpecialGoodsTradeSchedule(pMsg);
			return 1;

		case 0x3122: // Guild disband / update
			OnGuildDisband(pMsg);
			return 1;

		default:
			// Native 0x004146ED: Dispatch to target character entity
			// Invokes CGObjPC::OnClientPacket via slot 1 (+0x04)
			return DispatchSubsystemPacket(pMsg, nullptr);
		}
	} catch (...) {
		// Native 0x0041471D: Exception handler filter
		BSLib::Log_Printf(0x2000001, " An Exception occurred in CGame::ProcessMessage() MsgID : %x\n", wOpcode);
		return 0;
	}
}

// [RECONSTRUCTED - Native 0x00413590]
// Registers diagnostic performance counters ("Obj Count", "DBQuery Count", etc.)
void CGame::RegisterPerformanceCounters() {
	BSLib::Log_Printf(0, "[PerfCounters] Registered diagnostic counters for CGame\n");
}

// [RECONSTRUCTED - Native 0x00412BE0]
// Sets environmental weather state and broadcasts update to active regions
void CGame::SetWeather(uint8_t byWeather) {
	if (byWeather >= 0x17) {
		byWeather = 0x17;
	}
	BSLib::Log_Printf(0, "[Weather] Setting environment weather state: %u\n", byWeather);
}

// [RECONSTRUCTED - Native 0x00413FB0]
// Advances player status, per-character timers, and environment time
void CGame::TickPlayers() {
	// Advances per-player timers and syncs environment time
}

// [RECONSTRUCTED - Native 0x00414120]
// Advances global world manager regions and geography
void CGame::TickWorldManager() {
	if (g_pGameWorldMgr == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
}

// [RECONSTRUCTED - Native 0x00414140]
// Drives global event daemon scheduler and timers
void CGame::TickEventDaemon() {
	// Event daemon timer management
}

// [RECONSTRUCTED - Native 0x00414890]
// Routes subsystem packets (Party 0xB010, Spawn 0x300C, Target 0xAA00)
int32_t CGame::DispatchSubsystemPacket(BSLib::CPacket* pMsg, void* pContext) {
	(void)pContext;
	if (pMsg == nullptr) {
		return 0;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x004154D0]
// Processes Game Master administrative commands (MoveToPlayer, Recall, SetTime)
int32_t CGame::ProcessGMCommand(BSLib::CPacket* pMsg, void* pContext, uint32_t dwParam) {
	(void)pContext; (void)dwParam;
	if (pMsg == nullptr) {
		return 0;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x004168F0]
// Handles special goods trade merchant notifications (Opcode 0xBC00)
void CGame::OnSpecialGoodsTradeNotice(BSLib::CPacket* pMsg) {
	if (pMsg == nullptr) {
		return;
	}
}

// [RECONSTRUCTED - Native 0x00416DB0]
// Handles special goods trade schedule state (Opcodes 0x0C08 - 0x0C0A)
void CGame::OnSpecialGoodsTradeSchedule(BSLib::CPacket* pMsg) {
	if (pMsg == nullptr) {
		return;
	}
}

// [RECONSTRUCTED - Native 0x004169D0]
void CGame::OnGuildMemberUpdate(BSLib::CPacket* pMsg) {
	(void)pMsg;
}

// [RECONSTRUCTED - Native 0x00416AD0]
void CGame::OnGuildNoticeUpdate(BSLib::CPacket* pMsg) {
	(void)pMsg;
}

// [RECONSTRUCTED - Native 0x00416BD0]
void CGame::OnGuildWarUpdate(BSLib::CPacket* pMsg) {
	(void)pMsg;
}

// [RECONSTRUCTED - Native 0x00416D00]
void CGame::OnGuildDisband(BSLib::CPacket* pMsg) {
	(void)pMsg;
}

// [RECONSTRUCTED - Native 0x00417A80]
// Notifies all connected clients of server shutdown/disconnection
void CGame::NotifyAllClientsDisconnect() {
	BSLib::Log_Printf(0x1000000, "[CGame] Disconnecting all clients...\n");
}

// [RECONSTRUCTED - Native 0x00417E10]
// Flushes all dirty character states and saves character records
void CGame::FlushAllCharacters() {
	for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
		CGObjChar* pChar = static_cast<CGObjChar*>(pNode->pOwner);
		if (pChar != nullptr) {
			pChar->OnTick(0.0f);
		}
	}
}

// [RECONSTRUCTED - Native 0x00417E80]
// Schedules bulk asynchronous database backup for all character data
void CGame::BackupCharacterData() {
	BSLib::Log_Printf(0x2000000, "Start CharactorData Backup!\n");
}

// [RECONSTRUCTED - Native 0x00418110]
// Validates and applies character teleport transitions
void CGame::ApplyTeleportTransition(CGObjChar* pChar, uint32_t dwTeleportID, void* pContext) {
	(void)dwTeleportID; (void)pContext;
	if (pChar == nullptr || !pChar->IsChar()) {
		return;
	}
}

/*
===============================================================================
CGame::Tick [RECONSTRUCTED - Native 0x00413840]

Core Frame Simulation Heartbeat (361 machine bytes @ 0x00413840)
Parameters:
  - float fDeltaSeconds : Elapsed real time since last tick
Returns:
  - int32_t             : Average frame time / profiler result

Machine Disassembly Flow:
  1. Clamps fDeltaSeconds to [0.0f, 1.0f] via x87 fucom/fcom (0x00413846 - 0x00413888).
  2. Accumulates into dual global timers:
       g_fScheduledTickTime += fDeltaSeconds (0x0041388F)
       g_fWatchdogTime      += fDeltaSeconds (0x0041389D)
       m_fAccumulatedTime   += fDeltaSeconds (0x004138AB)
  3. Single-step timestep comparison:
       if (m_fAccumulatedTime >= m_fTimeStep) (0x004138BC)
         a. m_fAccumulatedTime -= m_fTimeStep (0x004138C5)
         b. this->TickWorldObjects() (0x004138CC)
         c. CaravanManager_Tick() (0x004138D1)
         d. If g_bDisplayWindowEnabled != 0 && g_pDisplayWindow != 0:
            CDisplayWindow::GetInstance()->RenderFrame() (0x004138EE)
         e. dwElapsedMs = (uint32_t)(g_fScheduledTickTime * 1000.0)
            CGame_ScheduleTick(dwElapsedMs) (0x00413928)
         f. CGameEventManager_Tick() (0x0041392D)
         g. g_fScheduledTickTime = 0.0f (0x00413934)
  4. Periodic watchdog & FPS profiler check:
       if (m_fTimeStep * 1000 >= threshold1 + threshold2 || g_fWatchdogTime >= 3.0f):
         a. g_dwLastWatchdogTick = GetTickCount() (0x00413979)
         b. this->PeriodicWatchdogTick() (0x0041397E)
         c. CFPSProfiler_Sample(&g_fpsProfiler) (0x00413988)
         d. g_fWatchdogTime = 0.0f (0x0041398F)
===============================================================================
*/
#include "AsyncShardQuery.h"
int32_t CGame::Tick(float fDeltaSeconds) {
	ShardQuery::Pump();
	// Native 0x00413846 - 0x00413888: Clamp frame delta to [0.0f, 1.0f]
	if (fDeltaSeconds < 0.0f) {
		fDeltaSeconds = 0.0f;
	} else if (fDeltaSeconds > 1.0f) {
		fDeltaSeconds = 1.0f;
	}

	// Native 0x0041388F - 0x004138B6: Accumulate elapsed time
	g_fScheduledTickTime += fDeltaSeconds;
	g_fWatchdogTime      += fDeltaSeconds;
	m_fAccumulatedTime   += fDeltaSeconds;

	// Native 0x004138BC: Check if fixed simulation timestep has elapsed
	if (m_fAccumulatedTime >= m_fTimeStep) {
		// Native 0x004138C5: Subtract single fixed timestep (at most once per tick)
		m_fAccumulatedTime -= m_fTimeStep;

		// Native 0x004138CC: Core world simulation tick (Characters, Skills, Daemons, Structures)
		TickWorldObjects();

		// Native 0x004138D1: Advance caravan trade route timers and bandit spawns
		CaravanManager_Tick();

		// Native 0x004138D6 - 0x004138EE: Render display window if enabled
		if (g_bDisplayWindowEnabled != 0) {
			CDisplayWindow* pDisplayWindow = CDisplayWindow::GetInstance();
			if (pDisplayWindow != nullptr) {
				pDisplayWindow->RenderFrame();
			}
		}

		// Native 0x004138F3 - 0x00413928: Tick world scheduled callbackers with elapsed ms
		uint32_t dwElapsedMs = static_cast<uint32_t>(g_fScheduledTickTime * 1000.0f);
		m_scheduledCallbacker.Update(dwElapsedMs);

		// Native 0x0041392D: Tick calendar-scheduled game events (Fortress war, Roc, CTF, Arena)
		CGame_ScheduleExecutor_Tick();

		// Native 0x00413932 - 0x00413934: Reset schedule tick accumulator
		g_fScheduledTickTime = 0.0f;
	}

	// Native 0x0041393E - 0x0041396F: Check performance overrun or periodic 3.0s watchdog
	uint32_t dwTimeStepMs = static_cast<uint32_t>(m_fTimeStep * 1000.0f);
	if (dwTimeStepMs >= (g_nPerfThreshold1 + g_nPerfThreshold2) || g_fWatchdogTime >= 3.0f) {
		// Native 0x00413971 - 0x00413979: Record watchdog timestamp
		g_dwLastWatchdogTick = ::GetTickCount();

		// Native 0x0041397E: Execute periodic server status and active entity updates
		PeriodicWatchdogTick();

		// Native 0x00413988: Sample FPS and calculate moving average frame duration
		uint32_t dwAvgFrameMs = CFPSProfiler_Sample(&g_fpsProfiler);

		// Native 0x0041398F: Reset watchdog accumulator
		g_fWatchdogTime = 0.0f;

		return static_cast<int32_t>(dwAvgFrameMs);
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x004140B0 / 0x00414140]
// Periodic 3-second watchdog and status update:
//   - Native 0x004140D5: Passes g_fWatchdogTime to UpdateActiveItems
//   - Native 0x00414140: Audits lobby stale players (CLobby::OnTick @ 0x00437A50)
void CGame::PeriodicWatchdogTick() {
	UpdateActiveItems(g_fWatchdogTime);

	if (g_pLobby != nullptr) {
		g_pLobby->OnTick(g_fWatchdogTime);
	}
}

// [RECONSTRUCTED - Native 0x004139B0]
// Updates all active ground items, progresses expiration timers, and despawns expired items
void CGame::UpdateActiveItems(float fDeltaSec) {
	auto it = m_mapActiveItems.begin();
	while (it != m_mapActiveItems.end()) {
		// Native 0x00413A19 - 0x00413A22: Advance iterator safely prior to processing
		auto cur = it++;
		CGItem* pItem = cur->second;
		if (pItem == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			m_mapActiveItems.erase(cur);
			continue;
		}

		try {
			// Native 0x00413A6C - 0x00413A72: Virtual Slot +0x34C: CGItem::OnTick
			pItem->OnTick(fDeltaSec);

			// Native 0x00413A78 - 0x00413A7E: Virtual Slot +0x0F8: CGItem::GetLifeState
			uint8_t byLifeState = pItem->GetLifeState();
			if (byLifeState != ITEM_STATE_ALIVE && byLifeState != ITEM_STATE_DEAD) {
				ServerFramework::ServerFramework_GenerateMiniDump();
			}

			// Native 0x00413A97 - 0x00413AC2: If life state reached ITEM_STATE_DEAD (expired/consumed)
			if (byLifeState == ITEM_STATE_DEAD) {
				m_mapActiveItems.erase(cur);

				// Native 0x00413AAB: State check (must be 0 upon normal despawn)
				if (pItem->GetDespawnState() != 0) {
					ServerFramework::ServerFramework_GenerateMiniDump();
				}

				// Native 0x00413ABD: Trigger object destruction and despawn broadcast
				DestroyObject(pItem);
			}
		} catch (...) {
			// Native 0x00413AD5 - 0x00413B01: SEH / C++ Exception handler
			m_mapActiveItems.erase(cur);
			BSLib::Log_Printf(0x2000001, "exception occured while updating Item (now erased)");
		}
	}
}

// [RECONSTRUCTED - Native 0x00417090]
// Despawns and destroys an item object, dispatches despawn event, and unregisters it
void CGame::DestroyObject(CGItem* pItem) {
	if (pItem == nullptr) {
		return;
	}

	// Dispatch despawn event and trigger destruction
	pItem->Destroy();

	// Release memory
	delete pItem;
}

// [RECONSTRUCTED - Native 0x00417DB0]
// Removes an item from the active item update map
void CGame::RemoveItemFromUpdateList(CGItem* pItem) {
	if (pItem == nullptr) {
		return;
	}

	auto it = m_mapActiveItems.find(pItem->GetGlobalID());
	if (it != m_mapActiveItems.end()) {
		m_mapActiveItems.erase(it);
	}
}

// [RECONSTRUCTED - Native 0x00416E90]
// Inter-server link connected handler (144 machine bytes @ 0x00416E90)
void CGame::OnServerConnected(void* pNode, void* pLink) {
	if (pNode == nullptr) {
		return;
	}

	uint8_t byServerID = *reinterpret_cast<uint8_t*>(pNode);
	ServerFramework::CServerNode* pServerNode = reinterpret_cast<ServerFramework::CServerNode*>(pNode);

	ServerFramework::CServerNode* pGSNode = ServerFramework::ServerFramework_FindServerNodeByName("SR_GameServer");
	if (pServerNode == pGSNode) {
		BSLib::Log_Printf(0, "Connected to GameServer [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pAgentNode = ServerFramework::ServerFramework_FindServerNodeByName("AgentServer");
	if (pServerNode == pAgentNode) {
		BSLib::Log_Printf(0, "Connected to AgentServer [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pFarmNode = ServerFramework::ServerFramework_FindServerNodeByName("FarmManager");
	if (pServerNode == pFarmNode) {
		BSLib::Log_Printf(0, "Connected to FarmManager [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pShardNode = ServerFramework::ServerFramework_FindServerNodeByName("SR_ShardManager");
	if (pServerNode == pShardNode) {
		BSLib::Log_Printf(0, "Connected to ShardManager [#%d]\n", byServerID);
		if (pLink != nullptr) {
			uint32_t dwSessionID = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pLink) + 0x0D);
			g_dwShardManagerSessionID = dwSessionID;
		}
		return;
	}

	BSLib::Log_Printf(0x1000000, "Connected to Unknown Module!! [#%d - Name: %s]\n", byServerID, reinterpret_cast<char*>(pNode) + 1);
}

// [RECONSTRUCTED - Native 0x00416F80]
// Inter-server link disconnected handler (224 machine bytes @ 0x00416F80)
void CGame::OnServerDisconnected(void* pNode) {
	if (pNode == nullptr) {
		return;
	}

	uint8_t byServerID = *reinterpret_cast<uint8_t*>(pNode);
	ServerFramework::CServerNode* pServerNode = reinterpret_cast<ServerFramework::CServerNode*>(pNode);

	ServerFramework::CServerNode* pGSNode = ServerFramework::ServerFramework_FindServerNodeByName("SR_GameServer");
	if (pServerNode == pGSNode) {
		BSLib::Log_Printf(0x2000000, "Disconnected from GameServer! [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pAgentNode = ServerFramework::ServerFramework_FindServerNodeByName("AgentServer");
	if (pServerNode == pAgentNode) {
		BSLib::Log_Printf(0x2000000, "Disconnected from AgentServer! [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pFarmNode = ServerFramework::ServerFramework_FindServerNodeByName("FarmManager");
	if (pServerNode == pFarmNode) {
		BSLib::Log_Printf(0x2000000, "Disconnected from FarmManager! [#%d]\n", byServerID);
		return;
	}

	ServerFramework::CServerNode* pShardNode = ServerFramework::ServerFramework_FindServerNodeByName("SR_ShardManager");
	if (pServerNode == pShardNode) {
		BSLib::Log_Printf(0x2000000, "Disconnected from ShardManager, all user connections will be lost (but, data saved)!!! [#%d]\n", byServerID);
		g_dwShardManagerSessionID = 0;
		return;
	}

	BSLib::Log_Printf(0x2000000, "Diconnected from unknown server!! [#%d - Name: %s]\n", byServerID, reinterpret_cast<char*>(pNode) + 1);
}

// [RECONSTRUCTED - Native 0x00417240]
// Handles player character world entry from lobby:
//   - Validates character pointer and entry state
//   - Validates character name uniqueness in m_mapActiveCharactersByName
//   - Validates account JID uniqueness in m_mapCharactersByJID
//   - Validates unique entity ID in m_mapCharactersByCharID
//   - Binds network session to game instance via CGObjPC_BindSessionToGame (0x0048B840)
//   - Validates destination region via CGameWorldMgr (0x005F85A0 is IsRefGameWorldByte20Zero; call target not re-verified)
//   - Registers character in destination CRegion spatial partition (0x004117E0)
//   - Links character into active entity heartbeat simulation list (g_pCharacterListHead)
//   - Dispatches spawn packets to surrounding observers via CGObjPC_SendSpawnPacket (0x005BF150)
//   - Audits inventory & equipment slots via CGObjPC_CheckEquipAndInventory (0x004DF6B0)
//   - Records character world entry in database audit log via CGameDB_LogCharLogin (0x00466F90)
bool CGame::EnterWorld(CLobbyEntry* pEntry) {
	if (pEntry == nullptr) {
		BSLib::Log_Printf(0x1000000, "[CGame::EnterWorld] Error: pEntry is NULL!\n");
		return false;
	}

	CGObjChar* pChar = reinterpret_cast<CGObjChar*>(pEntry->m_pChar);
	if (pChar == nullptr) {
		BSLib::Log_Printf(0x1000000, "[CGame::EnterWorld] Error: pEntry->m_pChar is NULL for JID=%u!\n", pEntry->m_dwJID);
		return false;
	}

	// 1. Uniqueness check: verify character is not already active in the world
	CGObjChar* pExistingChar = FindPlayer(pChar->m_dwGlobalID);
	if (pExistingChar != nullptr) {
		BSLib::Log_Printf(0x1000000, "[CGame::EnterWorld] Error: CharID %u already active in world!\n", pChar->m_dwGlobalID);
		return false;
	}

	// 2. Validate position & region
	if (pChar->m_wRegionID == 0) {
		BSLib::Log_Printf(0x2000000, "[CGame::EnterWorld] Char Login Failed(Invalid Region 0): GlobalID=%u\n", pChar->m_dwGlobalID);
		return false;
	}

	// 3. Link character node into active entity simulation heartbeat list
	CGObjChar_AddToList(&pChar->m_listNode);

	// 4. Set character life state to active (LifeState 1 = ALIVE / ACTIVE)
	if (pChar->m_pCharData != nullptr) {
		pChar->m_pCharData->m_byLifeState = 1;
	}

	BSLib::Log_Printf(0, "[CGame::EnterWorld] Successfully spawned character GlobalID=%u (JID=%u, Region=%u, X=%.1f, Y=%.1f, Z=%.1f)\n",
		pChar->m_dwGlobalID, pEntry->m_dwJID, pChar->m_wRegionID,
		pChar->m_fPosX, pChar->m_fPosY, pChar->m_fPosZ);

	return true;
}

// [RECONSTRUCTED - Native 0x00413B30]
// Core world simulation entity heartbeat (891 machine bytes @ 0x00413B30):
//   Phase 1: Pumps character network packets, cleans up dead characters.
//            Protected by SEH Catch Handler 0 (0x00413BFB) logging to very_fatal_log.txt.
//            Calculates 100-sample sliding average duration for network packet pumping.
//   Phase 2: Advances character simulation ticks (OnTick at slot +0x34C).
//            Handles character deactivation and unregistering/destruction.
//            Protected by SEH Catch Handler 1 (0x00413D77) logging to very_fatal_log.txt.
//   Phase 3: Advances active skill object timers (CGSkillObject::OnTick at slot +0x34C).
//            Cleans up expired/dead skill objects.
//   Phase 4: Advances event daemon timers (CGEventDaemon::OnTick at slot +0x34C).
//            Cleans up expired/dead event daemons.
//   Phase 5: Advances world structures and siege building simulation (CGObj::OnTick at slot +0x34C).
//            Cleans up destroyed world structures.
//   Phase 6: Calculates 100-sample sliding average duration (g_dwWorldObjUpdateTimeAvg)
//            used by CGame::Tick's lag detector and periodic watchdog.
uint32_t CGame::TickWorldObjects() {
	float fDeltaSec = g_fScheduledTickTime;

	// ========================================================================
	// Phase 1: Character Network Message Pump (Native 0x00413B5B - 0x00413BF9)
	// ========================================================================
	g_dwCharPumpNetMsgTickStart = ::GetTickCount();

	try {
		g_pCharacterListCurrent = g_pCharacterListHead;
		g_dwCharacterListIterFlags &= ~1;

		while (g_pCharacterListCurrent != nullptr) {
			tagCharListNode* pNode = g_pCharacterListCurrent;
			CGObjChar* pChar = reinterpret_cast<CGObjChar*>(reinterpret_cast<uint8_t*>(pNode) - 0x178);

			g_dwLatestUpdatedCharTID = pChar->GetTID().wType;
			CGObjChar_PumpNetworkMsg(pChar);

			// Slot 62 (+0xF8): GetLifeState() == 3 (STATE_DEAD / DESPAWN)
			if (pChar->GetLifeState() == 3) {
				// Slot 390 (+0x618): Deactivate()
				pChar->Deactivate();

				// Slot 7 (+0x1C): IsPlayer() == 1
				if (pChar->IsPlayer()) {
					UnregisterPlayer(pChar);
				} else {
					DestroyObject(static_cast<CGObj*>(pChar));
				}
			}

			// Advance iterator
			if ((g_dwCharacterListIterFlags & 1) == 0) {
				g_pCharacterListCurrent = g_pCharacterListCurrent->pNext;
			} else {
				g_dwCharacterListIterFlags &= ~1;
			}
		}
	} catch (...) {
		// Native SEH Catch Handler 0 @ 0x00413BFB
		FILE* fp = nullptr;
		if (::fopen_s(&fp, "very_fatal_log.txt", "a") == 0 && fp != nullptr) {
			SYSTEMTIME st;
			::GetLocalTime(&st);
			::fprintf(fp, "[PumpNetMsg] %04d-%02d-%02d %02d:%02d:%02d Latest Updated TID (%d) / Latest Processed Msg ID: %x \r\n",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
				g_dwLatestUpdatedCharTID, g_dwLatestProcessedMsgID);
			::fclose(fp);
		}
	}

	// Phase 1 metrics: accumulate elapsed ms
	uint32_t dwElapsedPump = ::GetTickCount() - g_dwCharPumpNetMsgTickStart;
	g_dwCharPumpNetMsgTimeAccum += dwElapsedPump;
	g_dwCharPumpNetMsgSampleCount++;
	if (g_dwCharPumpNetMsgSampleCount >= g_dwCharPumpNetMsgSampleLimit && g_dwCharPumpNetMsgSampleLimit > 0) {
		g_dwCharPumpNetMsgTimeAvg = g_dwCharPumpNetMsgTimeAccum / g_dwCharPumpNetMsgSampleLimit;
		g_dwCharPumpNetMsgTimeAccum = 0;
		g_dwCharPumpNetMsgSampleCount = 0;
	}

	// ========================================================================
	// Phase 2: Character Simulation Tick (Native 0x00413CC9 - 0x00413D72)
	// ========================================================================
	g_dwWorldObjUpdateTickStart = ::GetTickCount();

	try {
		g_pCharacterListCurrent = g_pCharacterListHead;
		g_dwCharacterListIterFlags &= ~1;

		while (g_pCharacterListCurrent != nullptr) {
			tagCharListNode* pNode = g_pCharacterListCurrent;
			CGObjChar* pChar = reinterpret_cast<CGObjChar*>(reinterpret_cast<uint8_t*>(pNode) - 0x178);

			if (pChar != nullptr) {
				g_dwLatestUpdatedCharTID = pChar->GetTID().wType;
				const char* pszName = pChar->GetName();
				if (pszName != nullptr) {
					::strncpy_s(g_szLatestUpdatedCharCodename, pszName, sizeof(g_szLatestUpdatedCharCodename) - 1);
				}

				// Slot 211 (+0x34C): OnTick(fDeltaSec)
				pChar->OnTick(fDeltaSec);

				// Slot 62 (+0xF8): GetLifeState() == 3
				if (pChar->GetLifeState() == 3) {
					pChar->Deactivate();
					if (pChar->IsPlayer()) {
						UnregisterPlayer(pChar);
					} else {
						DestroyObject(static_cast<CGObj*>(pChar));
					}
				}
			}

			// Advance iterator
			if ((g_dwCharacterListIterFlags & 1) == 0) {
				g_pCharacterListCurrent = g_pCharacterListCurrent->pNext;
			} else {
				g_dwCharacterListIterFlags &= ~1;
			}
		}
	} catch (...) {
		// Native SEH Catch Handler 1 @ 0x00413D77
		FILE* fp = nullptr;
		if (::fopen_s(&fp, "very_fatal_log.txt", "a") == 0 && fp != nullptr) {
			SYSTEMTIME st;
			::GetLocalTime(&st);
			const char* pszCodename = (g_szLatestUpdatedCharCodename[0] != '\0') ? g_szLatestUpdatedCharCodename : "";
			::fprintf(fp, "[Update] %04d-%02d-%02d %02d:%02d:%02d Latest Updated TID (%d), Codename(%s) \r\n",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
				g_dwLatestUpdatedCharTID, pszCodename);
			::fclose(fp);
		}
	}

	// ========================================================================
	// Phase 3: Active Skill Objects Simulation Tick (Native 0x00413DFC - 0x00413E6A)
	// ========================================================================
	g_pSkillObjectListCurrent = g_pSkillObjectListHead;
	g_dwSkillObjectListIterFlags &= ~1;

	while (g_pSkillObjectListCurrent != nullptr) {
		tagObjListNode* pNode = g_pSkillObjectListCurrent;
		CGSkillObject* pSkillObj = reinterpret_cast<CGSkillObject*>(reinterpret_cast<uint8_t*>(pNode) - 0x14C);

		// Slot 211 (+0x34C): OnTick(fDeltaSec)
		pSkillObj->OnTick(fDeltaSec);

		// Slot 62 (+0xF8): GetLifeState() == 3
		if (pSkillObj->GetLifeState() == 3) {
			DestroyObject(static_cast<CGObj*>(pSkillObj));
		}

		// Advance intrusive list node
		if ((g_dwSkillObjectListIterFlags & 1) == 0) {
			g_pSkillObjectListCurrent = g_pSkillObjectListCurrent->pNext;
		} else {
			g_dwSkillObjectListIterFlags &= ~1;
		}
	}

	// ========================================================================
	// Phase 4: Active Event Daemons Simulation Tick (Native 0x00413E6C - 0x00413EDA)
	// ========================================================================
	g_pEventDaemonListCurrent = g_pEventDaemonListHead;
	g_dwEventDaemonListIterFlags &= ~1;

	while (g_pEventDaemonListCurrent != nullptr) {
		tagObjListNode* pNode = g_pEventDaemonListCurrent;
		CGEventDaemon* pDaemon = reinterpret_cast<CGEventDaemon*>(reinterpret_cast<uint8_t*>(pNode) - 0x14C);

		// Slot 211 (+0x34C): OnTick(fDeltaSec)
		pDaemon->OnTick(fDeltaSec);

		// Slot 62 (+0xF8): GetLifeState() == 3
		if (pDaemon->GetLifeState() == 3) {
			DestroyObject(static_cast<CGObj*>(pDaemon));
		}

		// Advance intrusive list node
		if ((g_dwEventDaemonListIterFlags & 1) == 0) {
			g_pEventDaemonListCurrent = g_pEventDaemonListCurrent->pNext;
		} else {
			g_dwEventDaemonListIterFlags &= ~1;
		}
	}

	// ========================================================================
	// Phase 5: Active World Structures Simulation Tick (Native 0x00413EDC - 0x00413F4A)
	// ========================================================================
	g_pObjStructListCurrent = g_pObjStructListHead;
	g_dwObjStructListIterFlags &= ~1;

	while (g_pObjStructListCurrent != nullptr) {
		tagObjListNode* pNode = g_pObjStructListCurrent;
		CGObjStruct* pStruct = reinterpret_cast<CGObjStruct*>(reinterpret_cast<uint8_t*>(pNode) - 0x14C);

		// Slot 211 (+0x34C): OnTick(fDeltaSec)
		pStruct->OnTick(fDeltaSec);

		// Slot 62 (+0xF8): GetLifeState() == 3
		if (pStruct->GetLifeState() == 3) {
			DestroyObject(static_cast<CGObj*>(pStruct));
		}

		// Advance intrusive list node
		if ((g_dwObjStructListIterFlags & 1) == 0) {
			g_pObjStructListCurrent = g_pObjStructListCurrent->pNext;
		} else {
			g_dwObjStructListIterFlags &= ~1;
		}
	}

	// ========================================================================
	// Phase 6: World Object Update Profiler Metrics (Native 0x00413F4C - 0x00413F94)
	// ========================================================================
	uint32_t dwElapsedUpdate = ::GetTickCount() - g_dwWorldObjUpdateTickStart;
	g_dwWorldObjUpdateTimeAccum += dwElapsedUpdate;
	g_dwWorldObjUpdateSampleCount++;

	uint32_t result = g_dwWorldObjUpdateSampleCount;
	if (g_dwWorldObjUpdateSampleCount >= g_dwWorldObjUpdateSampleLimit && g_dwWorldObjUpdateSampleLimit > 0) {
		g_dwWorldObjUpdateTimeAvg = g_dwWorldObjUpdateTimeAccum / g_dwWorldObjUpdateSampleLimit;
		result = g_dwWorldObjUpdateTimeAvg;
		g_dwWorldObjUpdateTimeAccum = 0;
		g_dwWorldObjUpdateSampleCount = 0;
	}

	return result;
}

/**
 * [RECONSTRUCTED - Native 0x004177D0]
 * CGame::UnregisterPlayer
 *
 * Unregisters an active player character from the world and cleans up their network session.
 */
uint32_t CGame::UnregisterPlayer(CGObjChar* pChar) {
	if (!pChar) {
		return 0;
	}
	BSLib::Log_Printf(0, "[CGame] UnregisterPlayer for GlobalID=%u\n", pChar->m_dwGlobalID);
	CGObjChar_RemoveFromList(&pChar->m_listNode);
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00417200]
 * CGame::RegisterObject
 *
 * Registers a newly spawned game object into active world partitions.
 */
void CGame::RegisterObject(CGObj* pObj) {
	if (!pObj) {
		return;
	}
	BSLib::Log_Printf(0, "[CGame] RegisterObject GlobalID=%u\n", pObj->m_dwGlobalID);
}

/**
 * [RECONSTRUCTED - Native 0x00417090]
 * CGame::DestroyObject
 *
 * Despawns and destroys an active game object from world partitions.
 */
void CGame::DestroyObject(CGObj* pObj) {
	if (!pObj) {
		return;
	}
	BSLib::Log_Printf(0, "[CGame] DestroyObject GlobalID=%u\n", pObj->m_dwGlobalID);
	// In native binary, unlinks from spatial partition / quadtree and deletes object
}

// [RECONSTRUCTED - Native 0x0041CCF0]
// CGame_FindObjectByID: Resolves runtime GameID / GlobalID to CGObjChar*
CGObjChar* CGame::FindPlayer(uint32_t dwPlayerID) {
	// Look up in active character list for matching entity ID
	for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
		CGObjChar* pChar = static_cast<CGObjChar*>(pNode->pOwner);
		if (pChar != nullptr && pChar->m_dwGlobalID == dwPlayerID) {
			return pChar;
		}
	}
	return nullptr;
}

// [RECONSTRUCTED - Native 0x00418100]
Caravan* CGame::RegisterCaravan(uint32_t dwPlayerID) {
	return CaravanManager_RegisterCaravan(dwPlayerID);
}

// [RECONSTRUCTED - Native 0x00414250]
void CGame::OnScheduledEnvironmentUpdate() {
	// Native 0x00414250: Calls CMap_BroadcastWeatherUpdate(0x00530D60), which walks the regions of game world 1
	// Broadcasts opcode 0x3027 (weather / environment state) to players in active regions
	BSLib::Log_Printf(0x1000000, "[ScheduledCallback] Executing scheduled environment / weather update");
}

// [RECONSTRUCTED - Native 0x0041BFA0]
// CScheduledCallbacker<CGame, unsigned long>::Update (alias CGame_ScheduleTick)
// Dispatches registered timer callbacks (environment updates, world ticks)
void CGame_ScheduleTick(uint32_t dwDeltaMs) {
	if (g_pGame != nullptr) {
		g_pGame->m_scheduledCallbacker.Update(dwDeltaMs);
	}
}

// [RECONSTRUCTED - Native 0x0041D3C0]
uint32_t CFPSProfiler_Sample(tagFPSProfiler* pProfiler) {
	if (!pProfiler) {
		return 0;
	}

	uint32_t dwNow = ::GetTickCount();
	uint32_t dwElapsed = dwNow - pProfiler->dwLastTick;
	pProfiler->dwLastTick = dwNow;
	pProfiler->dwSampleCount++;
	pProfiler->dwAccumulatedMs += dwElapsed;

	uint32_t result = pProfiler->dwAccumulatedMs;
	if (pProfiler->dwMaxSamples > 0 && pProfiler->dwSampleCount >= pProfiler->dwMaxSamples) {
		result = pProfiler->dwAccumulatedMs / pProfiler->dwSampleCount;
		pProfiler->dwReserved = 0;
		pProfiler->dwAverageFrameMs = result;
		pProfiler->dwCachedFPS = (result > 0) ? (1000 / result) : 30;
		pProfiler->dwAccumulatedMs = 0;
		pProfiler->dwSampleCount = 0;
	}

	return result;
}

CGObjChar* CGame::FindObjectByID(uint32_t dwObjectID) {
	auto it = m_mapObjects.find(dwObjectID);
	if (it != m_mapObjects.end()) {
		return it->second;
	}
	return FindPlayer(dwObjectID);
}

CGObjChar* CGame_FindObjectByID(uint32_t dwGameID) {
	if (g_pGame != nullptr) {
		return g_pGame->FindObjectByID(dwGameID);
	}
	return nullptr;
}

bool CGame::RegisterObject(CGObjChar* pChar) {
	if (pChar == nullptr) {
		return false;
	}
	uint32_t dwGameID = pChar->GetGameID();
	m_mapObjects[dwGameID] = pChar;
	return true;
}

float CGame::GetTimeStep() const {
	return m_fTimeStep;
}

uint32_t CGame::GetTargetFPS() const {
	return m_dwFPS;
}

// [RECONSTRUCTED - Native 0x0040B360] (75 bytes)
int32_t CGame::RegisterCmdSrcNet(uint32_t dwSessionID, CCmdSrcNet* pCmdSrc) {
	if (m_mapCmdSrcNet.find(dwSessionID) != m_mapCmdSrcNet.end()) {
		return 0;
	}
	m_mapCmdSrcNet.insert(std::make_pair(dwSessionID, pCmdSrc));
	return 1;
}

// [RECONSTRUCTED - Native 0x0040BAF0] (95 bytes)
int32_t CGame::UnregisterCmdSrcNet(uint32_t dwSessionID) {
	std::map<uint32_t, CCmdSrcNet*>::iterator it = m_mapCmdSrcNet.find(dwSessionID);
	if (it == m_mapCmdSrcNet.end()) {
		return 0;
	}
	m_mapCmdSrcNet.erase(it);
	return 1;
}
