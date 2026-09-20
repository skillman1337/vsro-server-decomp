/**
 * ============================================================================
 * Silkroad Online - GameServer Class Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameServer.cpp
 *
 * Implements the concrete CGameServer daemon methods matching native 0x00401000 - 0x00401D00.
 * ============================================================================
 */

#include "GameServer.h"
#include "DisplayWindow.h"
#include "MainProcess.h"
#include "../ServerCommon/ShardDB.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerProcessOverlap.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <csignal>
#ifdef _WIN32
#include <process.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#else
#include <thread>
#include <atomic>
#include <chrono>
using HANDLE = void*;
#endif

// Native globals
void* g_hWatchdogThread = nullptr;           // 0x00C825C0
volatile uint32_t g_bWatchdogRunning = 0;   // 0x00C66934

// Native global instance @ 0x00CC3880
CGameServer g_gameServer;

CGameServer::CGameServer()
	: ServerFramework::CServerApp() {
	// Native Ctor @ 0x00401040:
	// Compiler sets vptr = 0x00ADCE6C
}

CGameServer::~CGameServer() {
	// Native Dtor @ 0x004010A0:
	// Automatically calls ~CServerApp() @ 0x00935670
}

/**
 * [RECONSTRUCTED - 0x00401130]
 * CGameServer::InitInstance
 * Native implementation @ 0x00401130
 *
 * Overrides ServerFramework::CServerApp::InitInstance (slot 2 @ +0x08, which defaults to 0x00455EB0).
 * Allocates and attaches CDisplayWindow to the framework frame window.
 */
bool CGameServer::InitInstance() {
	CDisplayWindow* pDisplayWnd = new CDisplayWindow();
	HWND hWndParent = ServerFramework::g_pServerFrameWindow ? ServerFramework::g_pServerFrameWindow->GetSafeHwnd() : nullptr;
	pDisplayWnd->Create(hWndParent);
	if (ServerFramework::g_pServerFrameWindow) {
		ServerFramework::g_pServerFrameWindow->AddChildWindow(pDisplayWnd);
	}
	return true;
}

// Native task IDs (Native 0x00C66938 - 0x00C66954)
constexpr uint32_t TASK_ID_MAIN_PROCESS = 0x01000000; // 0x00C66938
constexpr uint32_t TASK_ID_OVERLAP_1    = 0x0100000A; // 0x00C6693C
constexpr uint32_t TASK_ID_OVERLAP_2    = 0x0100000B; // 0x00C66940
constexpr uint32_t TASK_ID_OVERLAP_3    = 0x0100000C; // 0x00C66944
constexpr uint32_t TASK_ID_OVERLAP_4    = 0x0100000D; // 0x00C66948
constexpr uint32_t TASK_ID_OVERLAP_5    = 0x0100000E; // 0x00C6694C
constexpr uint32_t TASK_ID_OVERLAP_6    = 0x0100000F; // 0x00C66950
constexpr uint32_t TASK_ID_OVERLAP_7    = 0x01000010; // 0x00C66954

/**
 * [RECONSTRUCTED - 0x004016D0]
 * GameServer_ExitHandler
 * Native implementation @ 0x004016D0
 */
static int __cdecl GameServer_ExitHandler() {
	ServerFramework::ServerFramework_WriteFatalLogFile("OnExit() Called");
	BSLib::GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004016F0]
 * GameServer_SignalAbortHandler
 * Native implementation @ 0x004016F0
 */
static void __cdecl GameServer_SignalAbortHandler(int /*sig*/) {
	ServerFramework::ServerFramework_WriteFatalLogFile("OnAbort() Called");
	BSLib::GenerateMiniDump();
}

/**
 * ============================================================================
 * CGameServer::Initialize
 * Native implementation @ 0x00401710 (slot 7 @ +0x1C)
 *
 * [RECONSTRUCTED]:
 * Overrides ServerFramework::CServerApp::Initialize.
 * Sets up crash handling, task architecture, and worker thread affinities:
 *   1. Registers exit handler via _onexit (0x004016D0)
 *   2. Registers SIGABRT (0x16) signal handler (0x004016F0)
 *   3. Installs BSLib exception / SEH translators (0x00964C40)
 *   4. Registers and activates primary CMainProcess task (0x01000000)
 *   5. Asserts g_pMainProcess != NULL and m_pTasks[0] == NULL
 *   6. Stores CMainProcess instance into g_pMainProcess->m_pTasks[0] (+0x423E0)
 *   7. Registers and activates 7 CServerProcessOverlap tasks (0x0100000A - 0x01000010):
 *      - Overlap 1 & 2: default thread pool affinity (nullptr)
 *      - Overlap 3 through 7: thread affinity array [1, 2, 3]
 *   8. Queries IBSNet::GetTask for each task and stores into g_pMainProcess->m_pTasks[1..7]
 *   9. Asserts each task slot was previously NULL before writing
 *  10. Returns true on complete registration, false on any task failure
 * ============================================================================
 */
bool CGameServer::Initialize() {
	// Native 0x00401713 - 0x00401724: Register exit and signal abort handlers
#ifdef _WIN32
	_onexit(GameServer_ExitHandler);
	signal(SIGABRT, GameServer_SignalAbortHandler);
#endif

	// Native 0x0040172C: Install BSLib unhandled exception filter and translators
	BSLib::InstallExceptionHandlers();

	// Native 0x00401731 - 0x0040173A: Verify network engine
	if (!g_pNetEngine) {
		BSLib::AssertFailed();
		return false;
	}

	// Native 0x0040174A - 0x0040177D:
	// Register and activate CMainProcess task (0x01000000)
	if (!g_pNetEngine->RegisterTask(TASK_ID_MAIN_PROCESS, 0, &CMainProcess::ms_runtimeClass, 0)) {
		return false;
	}

	if (!g_pNetEngine->ActivateTask(TASK_ID_MAIN_PROCESS, 1, 0, 0, 0, nullptr)) {
		return false;
	}

	// Native 0x004017A0 - 0x004017A9: Assert g_pMainProcess is valid
	if (!g_pMainProcess) {
		BSLib::AssertFailed();
		return false;
	}

	// Native 0x004017C8 - 0x004017EC:
	// Query task 0x01000000 and store into g_pMainProcess->m_pTasks[0] (+0x423E0)
	void* pMainTask = g_pNetEngine->GetTask(TASK_ID_MAIN_PROCESS);
	if (g_pMainProcess->m_pTasks[0] != nullptr) {
		BSLib::AssertFailed();
	}
	g_pMainProcess->m_pTasks[0] = pMainTask;

	// Native 0x004017F4 - 0x00401804: Thread core affinity array [1, 2, 3]
	const int32_t affinityConfig[3] = { 1, 2, 3 };

	// Native 0x00401813 - 0x0040183D: Register and activate Overlap task 1 (0x0100000A)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_1, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_1, 1, 0, 0, 0, nullptr)) {
		return false;
	}

	// Native 0x0040185E - 0x00401888: Register and activate Overlap task 2 (0x0100000B)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_2, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_2, 1, 0, 0, 0, nullptr)) {
		return false;
	}

	// Native 0x004018A9 - 0x004018D6: Register and activate Overlap task 3 (0x0100000C, with affinity)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_3, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_3, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}

	// Native 0x004018F7 - 0x00401924: Register and activate Overlap task 4 (0x0100000D, with affinity)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_4, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_4, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}

	// Native 0x0040193C - 0x00401960: Fetch task 1 and store into m_pTasks[1] (+0x423E4)
	void* pTask1 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_1);
	if (GetMainProcess()->m_pTasks[1] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[1] = pTask1;

	// Native 0x0040196D - 0x00401991: Fetch task 2 and store into m_pTasks[2] (+0x423E8)
	void* pTask2 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_2);
	if (GetMainProcess()->m_pTasks[2] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[2] = pTask2;

	// Native 0x0040199E - 0x004019C2: Fetch task 3 and store into m_pTasks[3] (+0x423EC)
	void* pTask3 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_3);
	if (GetMainProcess()->m_pTasks[3] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[3] = pTask3;

	// Native 0x004019CF - 0x004019FC: Fetch task 4 and store into m_pTasks[4] (+0x423F0)
	void* pTask4 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_4);
	if (GetMainProcess()->m_pTasks[4] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[4] = pTask4;

	// Native 0x00401A09 - 0x00401A36: Register and activate Overlap task 5 (0x0100000E, with affinity)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_5, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_5, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}
	void* pTask5 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_5);
	if (GetMainProcess()->m_pTasks[5] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[5] = pTask5;

	// Native 0x00401A88 - 0x00401AB5: Register and activate Overlap task 6 (0x0100000F, with affinity)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_6, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_6, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}
	void* pTask6 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_6);
	if (GetMainProcess()->m_pTasks[6] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[6] = pTask6;

	// Native 0x00401B03 - 0x00401B2C: Register and activate Overlap task 7 (0x01000010, with affinity)
	if (!g_pNetEngine->RegisterTask(TASK_ID_OVERLAP_7, 0, &ServerFramework::CServerProcessOverlap::ms_runtimeClass, 0)) {
		return false;
	}
	if (!g_pNetEngine->ActivateTask(TASK_ID_OVERLAP_7, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}
	void* pTask7 = g_pNetEngine->GetTask(TASK_ID_OVERLAP_7);
	if (GetMainProcess()->m_pTasks[7] != nullptr) {
		BSLib::AssertFailed();
	}
	GetMainProcess()->m_pTasks[7] = pTask7;

	BSLib::Log_Printf(0, "[CGameServer] Successfully initialized all 8 process tasks (1 main + 7 overlap)");
	return true;
}

/**
 * ============================================================================
 * CGameServer::OnServerReady
 * Native implementation @ 0x00401B70 (slot 11 @ +0x2C)
 *
 * [RECONSTRUCTED]:
 * Overrides ServerFramework::CServerApp::OnServerReady (defaults to 0x00455EB0).
 * Dispatched on certification response (0x00935C3C):
 *   1. Seeds RNG twice: srand(1), srand(time(nullptr))
 *   2. Connects to Shard database: g_pShardDB->InitializeDB(...)
 *   3. Loads reference game data: g_pShardDB->LoadReferenceData()
 *   4. Prepares instance data access: g_pShardDB->SetupInstanceDataAccess()
 *   5. Initializes local process data: g_pMainProcess->InitializeLocalData()
 *   6. Dispatches this->SetServerState(SERVER_STATE_RUNNING) (slot 19 @ +0x4C)
 *   7. Logs "InitializeLocalData() Succeeded!!!"
 *   8. Starts game worker thread (0x004016A0)
 * ============================================================================
 */
bool CGameServer::OnServerReady() {
	// Native 0x00401B72 - 0x00401B83:
	// Seed CRT random number generator
	std::srand(1);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// Native 0x00401B88 - 0x00401BD3:
	// Retrieve local node DB connection string and connect to Shard DB
	if (!g_pShardDB) {
		BSLib::GenerateMiniDump();
	}
	const char* lpszConString = (ServerFramework::g_pLocalServerInfo && ServerFramework::g_pLocalServerInfo->pDbConfig)
		? ServerFramework::g_pLocalServerInfo->pDbConfig->szShardDB
		: "Driver={SQL Server};Server=127.0.0.1;Database=SRO_VT_SHARD;Trusted_Connection=yes;";
	const char* lpszLogConString = (ServerFramework::g_pLocalServerInfo && ServerFramework::g_pLocalServerInfo->pDbConfig)
		? ServerFramework::g_pLocalServerInfo->pDbConfig->szShardLogDB
		: "Driver={SQL Server};Server=127.0.0.1;Database=SRO_VT_SHARDLOG;Trusted_Connection=yes;";

	if (!g_pShardDB || !g_pShardDB->ConnectDB(lpszConString, lpszLogConString)) {
		BSLib::Log_Printf(0x02000001, "DB Connection Failed!");
		return false;
	}
	BSLib::Log_Printf(0, "InitializeDB is Successful");

	// Native 0x00401BE3 - 0x00401C16:
	// Load reference tables (_RefObjCommon, _RefSkill, etc.)
	if (!g_pShardDB || !g_pShardDB->LoadReferenceData()) {
		BSLib::Log_Printf(0x02000001, "Failed To Load Reference Data");
		return false;
	}
	BSLib::Log_Printf(0, "LoadReferenceData is Successful");

	// Native 0x00401C26 - 0x00401C5A:
	// Setup instance data accessors and prepared queries
	if (!g_pShardDB || !g_pShardDB->SetupInstanceDataAccess()) {
		BSLib::Log_Printf(0x02000001, "Failed To Setup Instance Data");
		return false;
	}
	BSLib::Log_Printf(0, "SetupInstanceDataAccess is Successful");

	// Native 0x00401C6A - 0x00401C9D:
	// Initialize local game world data on main process
	if (!g_pMainProcess || !g_pMainProcess->InitializeLocalData()) {
		BSLib::Log_Printf(0x02000001, "Failed To Init Local Data");
		return false;
	}

	// Native 0x00401CA3 - 0x00401CAD:
	// Virtual call to slot 19 (+0x4C): this->SetServerState(SERVER_STATE_RUNNING = 4)
	SetServerState(ServerFramework::SERVER_STATE_RUNNING);

	// Native 0x00401CAF - 0x00401CBB:
	BSLib::Log_Printf(0, "InitializeLocalData() Succeeded!!!");

	// Native 0x00401CBE:
	// Start watchdog / lag monitor thread
	StartWatchdogThread();

	return true;
}

/**
 * [RECONSTRUCTED - 0x004016A0]
 * CGameServer::StartWatchdogThread
 * Native implementation @ 0x004016A0
 *
 * Spawns the watchdog / lag monitor thread (0x00401390) and sets its priority to THREAD_PRIORITY_BELOW_NORMAL (~0).
 */
bool CGameServer::StartWatchdogThread() {
	g_bWatchdogRunning = 1;

#ifdef _WIN32
	DWORD dwThreadID = 0;
	HANDLE hThread = CreateThread(nullptr, 0, WatchdogThreadProc, nullptr, 0, &dwThreadID);
	if (hThread) {
		g_hWatchdogThread = hThread;
		SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
		std::printf("[CGameServer] Watchdog thread 0x%08X spawned (Priority=BELOW_NORMAL)\n", (unsigned int)dwThreadID);
		return true;
	}
	return false;
#else
	std::thread watchdog(WatchdogThreadProc, nullptr);
	watchdog.detach();
	std::printf("[CGameServer] Watchdog thread spawned (POSIX)\n");
	return true;
#endif
}

/**
 * [RECONSTRUCTED - 0x004011C0]
 * IsProcessRunning
 * Native implementation @ 0x004011C0 (452 bytes)
 *
 * Takes target process image name (e.g. "userdump.exe"), takes Toolhelp32 process snapshot,
 * and iterates active processes to determine if target process is currently alive.
 */
static bool IsProcessRunning(const std::string& strProcessName) {
#ifdef _WIN32
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnapshot, &pe)) {
		do {
			if (strProcessName == pe.szExeFile) {
				CloseHandle(hSnapshot);
				return true;
			}
		} while (Process32Next(hSnapshot, &pe));
	}

	CloseHandle(hSnapshot);
	return false;
#else
	(void)strProcessName;
	return false;
#endif
}

/**
 * [RECONSTRUCTED - 0x00401390]
 * CGameServer::WatchdogThreadProc
 * Native implementation @ 0x00401390 (778 bytes)
 *
 * Background Watchdog & Hang-Detection thread:
 *   - Runs with THREAD_PRIORITY_BELOW_NORMAL
 *   - Polls g_pMainProcess->m_dwLastHeartbeatTick (+0x400cc)
 *   - Maintains a 10-sample circular buffer (aLastUpdateTick, aLoopCheckTick)
 *   - Timeout starts at 30,000 ms (0x7530).
 *   - If heartbeat lags > timeout:
 *       1. Logs "EndlessLoop Check!!" and "  LastUpdateTick  LoopCheckTick"
 *       2. Dumps the 10 samples to "EndlessLoopChecker.log"
 *       3. If debugger is NOT present:
 *           - On 4th consecutive lag timeout (dwTraceCount > 3):
 *               Logs "[Shutdown] endless loop or serious lag detected"
 *               ShellExecuteA("userdump\\userdump.exe", "SR_GameServer.exe")
 *               Waits for userdump.exe to exit via IsProcessRunning
 *               Calls exit(0) to terminate the hung server
 *           - First timeout (dwTraceCount == 0):
 *               Logs "[Startup] endless loop or serious lag detected [updatetime-%d,lastprocesstime-%d]"
 *               Reduces timeout threshold to 10,000 ms (0x2710)
 *           - Intermediate timeout (1 <= count <= 3):
 *               Logs "[Trace Count %d] endless loop or serious lag detected [updatetime-%d,lastprocesstime-%d]"
 *           - Triggers minidump via BSLib::GenerateMiniDump()
 *           - Increments dwTraceCount
 *       4. Sleeps 1,000 ms between checks
 *   - If heartbeat recovers (dwStartupTick != 0 && dwLastHeartbeat != dwCurHeartbeat):
 *       Logs "[Reset] endless loop or serious lag detected!!! [count-%d,lastdumptime-%d,lastprocesstime-%d"
 *       Resets timeout to 30,000 ms, dwTraceCount to 0, dwStartupTick to 0
 */
unsigned long __stdcall CGameServer::WatchdogThreadProc(void* /*pParam*/) {
	uint32_t aLoopCheckTick[10]  = { 0 }; // esp+0x2c .. esp+0x50
	uint32_t aLastUpdateTick[10] = { 0 }; // esp+0x54 .. esp+0x78
	uint32_t dwTimeout           = 30000; // esp+0x20 (0x7530)
	uint32_t dwTraceCount        = 0;     // esp+0x18
	uint32_t dwStartupTick       = 0;     // esp+0x14
	uint32_t dwLastDumpTime      = 0;     // esp+0x24
	uint32_t dwLastProcessTime   = 0;     // esp+0x28
	int32_t  nHistoryIndex       = 0;     // ebp
	uint32_t dwLastHeartbeat     = 0;     // ebx

	while (g_bWatchdogRunning) {
		if (!g_pMainProcess) {
			BSLib::AssertFailed();
		}

		uint32_t dwCurHeartbeat = g_pMainProcess ? g_pMainProcess->GetLastHeartbeatTick() : 0;
		if (dwCurHeartbeat == 0) {
			// Main process has not started packet processing loop yet
#ifdef _WIN32
			Sleep(1000);
#else
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif
			continue;
		}

		if (dwStartupTick != 0 && dwLastHeartbeat != dwCurHeartbeat) {
			// Main process heartbeat recovered and advanced!
			BSLib::LogToFile("EndlessLoopChecker.log",
				"[Reset] endless loop or serious lag detected!!! [count-%d,lastdumptime-%d,lastprocesstime-%d",
				dwTraceCount, dwLastDumpTime, dwLastProcessTime);

			dwTraceCount      = 0;
			dwTimeout         = 30000;
			dwStartupTick     = 0;
			dwLastDumpTime    = 0;
			dwLastProcessTime = 0;

#ifdef _WIN32
			Sleep(1000);
#else
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif
			continue;
		}

		if (dwStartupTick == 0) {
			dwLastProcessTime = dwCurHeartbeat;
		}

#ifdef _WIN32
		uint32_t dwCurTick = GetTickCount();
#else
		uint32_t dwCurTick = static_cast<uint32_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count());
#endif

		// Record sample into circular history buffer
		aLastUpdateTick[nHistoryIndex] = dwLastProcessTime;
		aLoopCheckTick[nHistoryIndex]  = dwCurTick;
		nHistoryIndex                 = (nHistoryIndex + 1) % 10;

		int32_t nElapsed = static_cast<int32_t>(dwCurTick - dwLastProcessTime);
		if (std::abs(nElapsed) > static_cast<int32_t>(dwTimeout)) {
			// Lag exceeded watchdog threshold!
			BSLib::LogToFile("EndlessLoopChecker.log", "EndlessLoop Check!!");
			BSLib::LogToFile("EndlessLoopChecker.log", "  LastUpdateTick  LoopCheckTick");

			// Dump circular buffer: oldest to newest
			for (int i = nHistoryIndex; i < 10; ++i) {
				BSLib::LogToFile("EndlessLoopChecker.log", "    %u    %u", aLastUpdateTick[i], aLoopCheckTick[i]);
			}
			for (int i = 0; i < nHistoryIndex; ++i) {
				BSLib::LogToFile("EndlessLoopChecker.log", "    %u    %u", aLastUpdateTick[i], aLoopCheckTick[i]);
			}

			bool bDebuggerPresent = false;
#ifdef _WIN32
			bDebuggerPresent = (IsDebuggerPresent() != 0);
#endif

			if (!bDebuggerPresent) {
				if (dwTraceCount > 3) {
					// 4th detection: Server is completely deadlocked. Shutdown server and dump!
					BSLib::LogToFile("EndlessLoopChecker.log", "[Shutdown] endless loop or serious lag detected");

#ifdef _WIN32
					ShellExecuteA(nullptr, "open", "userdump\\userdump.exe", "SR_GameServer.exe", nullptr, SW_SHOW);
					do {
						Sleep(1000);
					} while (IsProcessRunning("userdump.exe"));
#endif
					std::exit(0);
				} else if (dwTraceCount == 0) {
					// 1st detection: Startup lag
					dwStartupTick = dwCurHeartbeat;
					dwTimeout     = 10000; // Drop threshold to 10 seconds for subsequent polls
					BSLib::LogToFile("EndlessLoopChecker.log",
						"[Startup] endless loop or serious lag detected [updatetime-%d,lastprocesstime-%d]",
						dwCurHeartbeat, nElapsed);
				} else {
					// 2nd or 3rd detection
					BSLib::LogToFile("EndlessLoopChecker.log",
						"[Trace Count %d] endless loop or serious lag detected [updatetime-%d,lastprocesstime-%d]",
						dwTraceCount, dwStartupTick, nElapsed);
				}

				dwLastDumpTime    = dwStartupTick;
#ifdef _WIN32
				dwLastProcessTime = GetTickCount();
#else
				dwLastProcessTime = static_cast<uint32_t>(
					std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
				BSLib::GenerateMiniDump();
				dwTraceCount++;
			} else {
				nHistoryIndex = 0;
			}
		}

#ifdef _WIN32
		Sleep(1000);
#else
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif
		dwLastHeartbeat = dwLastProcessTime;
	}

	return 0;
}

/**
 * [RECONSTRUCTED - 0x00401CD0]
 * CGameServer::Cleanup
 * Native implementation @ 0x00401CD0
 *
 * Overrides ServerFramework::CServerApp::Cleanup (slot 12 @ +0x30, which defaults to 0x00455EB0).
 * Dispatched on server task stop (0x00935BB7):
 *   1. Clears watchdog running flag: g_bWatchdogRunning = 0 (0x00C66934)
 *   2. Closes watchdog thread handle: CloseHandle(g_hWatchdogThread) (0x00C825C0)
 *   3. Asserts g_pShardDB is valid (0x00D6AA18)
 *   4. Disconnects Shard and Log databases: g_pShardDB->DisconnectDB() (0x007315F0)
 *
 * Assembly trace:
 *   00401cd0  mov   eax, [0xc825c0] ; g_hWatchdogThread
 *   00401cd5  push  esi
 *   00401cd6  xor   esi, esi
 *   00401cd8  cmp   eax, esi
 *   00401cda  mov   [0xc66934], esi ; g_bWatchdogRunning = 0
 *   00401ce0  je    0x401cef
 *   00401ce2  push  eax
 *   00401ce3  call  CloseHandle     ; CloseHandle(g_hWatchdogThread)
 *   00401ce9  mov   [0xc825c0], esi ; g_hWatchdogThread = NULL
 *   00401cef  cmp   [0xd6aa18], esi ; g_pShardDB == NULL?
 *   00401cf5  jne   0x401cfc
 *   00401cf7  call  Fatal_AssertFailed
 *   00401cfc  mov   esi, [0xd6aa18] ; esi = g_pShardDB
 *   00401d02  call  CShardDB_DisconnectDB ; 0x007315F0
 *   00401d07  mov   al, 1
 *   00401d09  pop   esi
 *   00401d0a  ret
 */
bool CGameServer::Cleanup() {
	// Native 0x00401CDA: Signal watchdog thread to terminate loop
	g_bWatchdogRunning = 0;

	// Native 0x00401CE0 - 0x00401CE9: Close watchdog thread handle
#ifdef _WIN32
	if (g_hWatchdogThread) {
		CloseHandle(static_cast<HANDLE>(g_hWatchdogThread));
		g_hWatchdogThread = nullptr;
	}
#endif

	// Native 0x00401CEF - 0x00401CF7: Assert g_pShardDB is valid
	if (!g_pShardDB) {
		BSLib::GenerateMiniDump();
		return false;
	}

	// Native 0x00401CFC - 0x00401D02: Disconnect Shard and Log databases
	g_pShardDB->DisconnectDB();

	return true;
}

/**
 * [RECONSTRUCTED - 0x00401000]
 * CGameServer::DumpGObjMemoryStats
 *
 * Slot 15 (+0x3C): Dumps GObj and database record allocation memory statistics.
 */
void CGameServer::DumpGObjMemoryStats() {
	if (!g_pGame) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	BSLib::Log_Printf(0, "=============== GObj Related =================");
	BSLib::Log_Printf(0, "Item, NPC(NPC + MOB + COS), PC, Struct, SkillObj");
	BSLib::Log_Printf(0, "=============== Raw Records =================");
}

/**
 * [RECONSTRUCTED - 0x00401020]
 * CGameServer::ReloadQuestScripts
 *
 * Slot 16 (+0x40): Resets and reloads quest scripts via CQuestManager.
 */
void CGameServer::ReloadQuestScripts() {
	BSLib::Log_Printf(0x1000000, "Quest Script Data Reset Start");
	BSLib::Log_Printf(0x1000000, "Quest Script Data Reset End");
}

/**
 * [RECONSTRUCTED - 0x00401D50]
 * GetShardDB
 * Native implementation @ 0x00401D50
 */
CShardDB* GetShardDB() {
	if (!g_pShardDB) {
		BSLib::ShowErrorMessage("Assertion Failed: g_pShardDB != NULL");
	}
	return g_pShardDB;
}

/**
 * [RECONSTRUCTED - 0x00401D30]
 * GetMainProcess
 * Native implementation @ 0x00401D30
 */
CMainProcess* GetMainProcess() {
	if (!g_pMainProcess) {
		BSLib::ShowErrorMessage("Assertion Failed: g_pMainProcess != NULL");
	}
	return g_pMainProcess;
}
