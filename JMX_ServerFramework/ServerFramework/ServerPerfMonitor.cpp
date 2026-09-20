/**
 * ============================================================================
 * Silkroad Online - ServerFramework Performance Monitoring Subsystem Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\JMX_ServerFramework\ServerFramework\ServerPerfMonitor.cpp
 *
 * Implements:
 *   - Native ServerFramework_InitPerfDumpConfig        @ 0x0094E7D0
 *   - Native ServerFramework_StartPerfMonitorThread     @ 0x0094E820
 *   - Native ServerFramework_StopMonitorThread          @ 0x0094E870
 *   - Native ServerFramework_PerfMonitorThreadProc      @ 0x0094E960
 *   - Native ServerFramework_RegisterCustomPerformanceCounter  @ 0x0094EAB0
 *   - Native ServerFramework_RegisterBuiltinPerformanceCounter @ 0x0094EBF0
 *   - Native ServerFramework_BuiltinPerformanceCounterCallback @ 0x0094EDB0
 *   - Native ServerFramework_GetOverlapCounters         @ 0x0094FC40
 *   - Global critical section g_csPerfCounterMap        @ 0x00D67970
 *   - Global counter map g_mapPerfCounters              @ 0x00D67964
 * ============================================================================
 */

#include "ServerPerfMonitor.h"
#include "ServerMoniterView.h"
#include "ServerFrameWindow.h"
#include "ServerConfig.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/NetEngine.h"
#include "../../Common/Framework/MassiveMsg.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ServerFramework {

// Native globals
CCriticalSectionBS                      g_csPerfCounterMap("PerfCounterMap"); // 0x00D67970
std::map<uint32_t, tagPerfCounterNode*> g_mapPerfCounters;                    // 0x00D67964

uint32_t                                g_dwMsgCountOfDumpTrigger = 0;        // 0x00C827D0
uint32_t                                g_dwMsgDumpTriggerElapseTick = 10000; // 0x00C827D4 (10000 ms)
CServerMoniterView*                     g_pPerfMonitorWnd = nullptr;          // 0x00C827D8
void*                                   g_hPerfMonitorThread = nullptr;       // 0x00C827DC
void*                                   g_hPerfMonitorEvent = nullptr;        // 0x00C827E0
uint32_t                                g_dwLastMsgDumpTick = 0;              // 0x00D6AA24

uint32_t                                g_dwTotalOverlapContexts = 0;         // 0x00D67954
uint32_t                                g_dwActiveOverlapContexts = 0;        // 0x00D67958
uint32_t                                g_dwAvailableOverlapContexts = 0;     // 0x00D6795C

/**
 * [RECONSTRUCTED - 0x0094E7D0]
 * ServerFramework_InitPerfDumpConfig
 * Native implementation @ 0x0094E7D0 (71 bytes)
 *
 * Loads "MsgCountOfDumpTrigger" and "MsgDumpTriggerElapseTick" config values.
 */
void ServerFramework_InitPerfDumpConfig() {
	const char* pszTrigger = ServerFramework_GetConfigString("MsgCountOfDumpTrigger");
	if (pszTrigger != nullptr) {
		g_dwMsgCountOfDumpTrigger = static_cast<uint32_t>(std::atoi(pszTrigger));
	}

	const char* pszElapse = ServerFramework_GetConfigString("MsgDumpTriggerElapseTick");
	if (pszElapse != nullptr) {
		g_dwMsgDumpTriggerElapseTick = static_cast<uint32_t>(std::atoi(pszElapse));
	} else {
		g_dwMsgDumpTriggerElapseTick = 10000; // 0x2710 (10 seconds default)
	}
}

/**
 * [RECONSTRUCTED - 0x0094E820]
 * ServerFramework_StartPerfMonitorThread
 * Native implementation @ 0x0094E820 (75 bytes)
 *
 * Spawns the background performance and UI status update thread.
 */
void* ServerFramework_StartPerfMonitorThread() {
#ifdef _WIN32
	if (g_pServerFrameWindow != nullptr) {
		g_pPerfMonitorWnd = g_pServerFrameWindow->GetMonitorView();
		ServerFramework_InitPerfDumpConfig();
		g_hPerfMonitorEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
		DWORD dwThreadId = 0;
		g_hPerfMonitorThread = CreateThread(nullptr, 0, ServerFramework_PerfMonitorThreadProc, nullptr, 0, &dwThreadId);
	}
#endif
	return g_hPerfMonitorThread;
}

/**
 * [RECONSTRUCTED - 0x0094E870]
 * ServerFramework_StopMonitorThread
 * Native implementation @ 0x0094E870 (225 bytes)
 *
 * Signals shutdown event, waits indefinitely for monitor thread termination, closes handles,
 * and frees all registered performance counters.
 */
void ServerFramework_StopMonitorThread() {
#ifdef _WIN32
	if (g_hPerfMonitorEvent != nullptr) {
		SetEvent(static_cast<HANDLE>(g_hPerfMonitorEvent));
		if (g_hPerfMonitorThread != nullptr) {
			WaitForSingleObject(static_cast<HANDLE>(g_hPerfMonitorThread), INFINITE);
			CloseHandle(static_cast<HANDLE>(g_hPerfMonitorThread));
			g_hPerfMonitorThread = nullptr;
		}
		CloseHandle(static_cast<HANDLE>(g_hPerfMonitorEvent));
		g_hPerfMonitorEvent = nullptr;
	}
#endif

	g_csPerfCounterMap.Lock();
	for (auto& pair : g_mapPerfCounters) {
		delete pair.second;
	}
	g_mapPerfCounters.clear();
	g_csPerfCounterMap.Unlock();
}

/**
 * [RECONSTRUCTED - 0x0094E960]
 * ServerFramework_PerfMonitorThreadProc
 * Native implementation @ 0x0094E960 (331 bytes)
 *
 * Background thread procedure waking up every 1000ms to invoke each counter callback.
 */
#ifdef _WIN32
DWORD WINAPI ServerFramework_PerfMonitorThreadProc(void* /*lpParam*/) {
	while (WaitForSingleObject(static_cast<HANDLE>(g_hPerfMonitorEvent), 1000) == WAIT_TIMEOUT) {
		g_csPerfCounterMap.Lock();
		for (auto& pair : g_mapPerfCounters) {
			tagPerfCounterNode* pNode = pair.second;
			if (pNode != nullptr) {
				if (pNode->pfnUpdate == nullptr) {
					ServerFramework_GenerateMiniDump();
				} else {
					pNode->pfnUpdate(pNode);
				}
			}
		}
		g_csPerfCounterMap.Unlock();
	}
	return 0;
}
#else
uint32_t ServerFramework_PerfMonitorThreadProc(void* /*lpParam*/) {
	return 0;
}
#endif

/**
 * [RECONSTRUCTED - 0x0094EAB0]
 * RegisterCustomPerformanceCounter
 * Native implementation @ 0x0094EAB0 (308 bytes)
 */
bool RegisterCustomPerformanceCounter(uint32_t dwCounterID, const char* pszName, tagPerfCounterNode* pNode) {
	if (!pNode) {
		return false;
	}

	g_csPerfCounterMap.Lock();

	if (g_pServerFrameWindow == nullptr) {
		g_csPerfCounterMap.Unlock();
		return false;
	}

	pNode->dwSampleCapacity = 100; // 0x64

	auto it = g_mapPerfCounters.find(dwCounterID);
	if (it != g_mapPerfCounters.end()) {
		ServerFramework_GenerateMiniDump();
	} else {
		g_mapPerfCounters[dwCounterID] = pNode;
		if (g_pPerfMonitorWnd != nullptr) {
			g_pPerfMonitorWnd->AddGraphData(static_cast<uint16_t>(dwCounterID), pszName, pNode);
		}
	}

	g_csPerfCounterMap.Unlock();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0094EBF0]
 * RegisterBuiltinPerformanceCounter
 * Native implementation @ 0x0094EBF0 (407 bytes)
 */
bool RegisterBuiltinPerformanceCounter(uint32_t dwCounterID) {
	g_csPerfCounterMap.Lock();

	if (g_pServerFrameWindow == nullptr || g_pPerfMonitorWnd == nullptr) {
		g_csPerfCounterMap.Unlock();
		return false;
	}

	tagPerfCounterNode* pNode = new tagPerfCounterNode();
	pNode->dwSampleCapacity = 0;
	pNode->dwReserved1 = 0;
	pNode->fValue = 0.0f;
	pNode->dwSampleCount = 0;
	pNode->dwCounterID = dwCounterID;
	pNode->dwReserved3 = 0;
	pNode->pfnUpdate = &BuiltinPerformanceCounterCallback;

	const char* pszCounterName = "";
	if (dwCounterID > PERF_COUNTER_MASSIVE_MSG_COUNT) {
		if (dwCounterID == PERF_COUNTER_OVERLAP_COUNT) {
			pszCounterName = "OverlapCount";
		} else {
			ServerFramework_GenerateMiniDump();
		}
	} else if (dwCounterID == PERF_COUNTER_MASSIVE_MSG_COUNT) {
		pszCounterName = "MassiveMsgCount";
	} else if ((dwCounterID - PERF_COUNTER_MSG_COUNT) > 6) {
		ServerFramework_GenerateMiniDump();
	} else {
		switch (dwCounterID) {
		case PERF_COUNTER_MSG_COUNT:
			pszCounterName = "MsgCount";
			break;
		case PERF_COUNTER_SESSION_COUNT:
			pszCounterName = "SessionCount";
			break;
		case PERF_COUNTER_SOCK_TCP_COUNT:
			pszCounterName = "SockTCPCount";
			break;
		case PERF_COUNTER_CLASS_LINK_COUNT:
			pszCounterName = "ClassLinkCount";
			break;
		case PERF_COUNTER_ACCEPT_COUNT:
			pszCounterName = "AcceptCount";
			break;
		case PERF_COUNTER_USED_FILE_COUNT:
			pszCounterName = "UsedFileCount";
			break;
		case PERF_COUNTER_CACHED_FILE_COUNT:
			pszCounterName = "CachedFileCount";
			break;
		default:
			break;
		}
	}

	g_mapPerfCounters[dwCounterID] = pNode;
	g_pPerfMonitorWnd->AddGraphData(static_cast<uint16_t>(dwCounterID), pszCounterName, pNode);

	g_csPerfCounterMap.Unlock();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0094EDB0]
 * BuiltinPerformanceCounterCallback
 * Native implementation @ 0x0094EDB0 (591 bytes)
 *
 * Samples performance counter metric from CNetEngine or IOCP overlap pools.
 * Includes automatic dump trigger on excessive MsgCount threshold.
 */
bool BuiltinPerformanceCounterCallback(tagPerfCounterNode* pNode) {
	if (!pNode || !g_pNetEngine) {
		return false;
	}

	tagNetEngineStats stats = {};

	if (pNode->dwCounterID <= PERF_COUNTER_MASSIVE_MSG_COUNT) {
		if (pNode->dwCounterID == PERF_COUNTER_MASSIVE_MSG_COUNT) {
			uint32_t dwTotal = 0;
			uint32_t dwActive = 0;
			uint32_t dwAvailable = 0;
			ServerFramework_CMassiveMsg_GetCounts(&dwTotal, &dwActive, &dwAvailable);
			pNode->fValue = static_cast<float>(dwActive);
			return true;
		}

		uint32_t idx = pNode->dwCounterID - PERF_COUNTER_MSG_COUNT;
		if (idx > 6) {
			return true;
		}

		switch (idx) {
		case 0: // PERF_COUNTER_MSG_COUNT
			g_pNetEngine->GetStatistics(1, &stats);
			pNode->fValue = static_cast<float>(stats.dwMsgCount);
			{
				DWORD now = ::GetTickCount();
				if (g_dwMsgCountOfDumpTrigger != 0 &&
				    pNode->fValue > static_cast<float>(g_dwMsgCountOfDumpTrigger) &&
				    (now - g_dwLastMsgDumpTick > g_dwMsgDumpTriggerElapseTick)) {
					BSLib::Log_Printf(0x2000001, "============== Msg Usage ================");
					g_pNetEngine->DumpMessageUsage(1, g_dwMsgCountOfDumpTrigger);
					g_dwLastMsgDumpTick = now;
				}
			}
			return true;

		case 1: // PERF_COUNTER_SESSION_COUNT
			g_pNetEngine->GetStatistics(4, &stats);
			pNode->fValue = static_cast<float>(stats.dwSessionCount);
			return true;

		case 2: // PERF_COUNTER_SOCK_TCP_COUNT
			g_pNetEngine->GetStatistics(2, &stats);
			pNode->fValue = static_cast<float>(stats.dwSockTCPCount);
			return true;

		case 3: // PERF_COUNTER_CLASS_LINK_COUNT
			g_pNetEngine->GetStatistics(0x20, &stats);
			pNode->fValue = static_cast<float>(stats.dwClassLinkCount);
			return true;

		case 4: // PERF_COUNTER_ACCEPT_COUNT
			g_pNetEngine->GetStatistics(0x10, &stats);
			pNode->fValue = static_cast<float>(stats.dwAcceptCount);
			return true;

		case 5: // PERF_COUNTER_USED_FILE_COUNT
			g_pNetEngine->GetStatistics(8, &stats);
			pNode->fValue = static_cast<float>(stats.dwUsedFileCount);
			return true;

		case 6: // PERF_COUNTER_CACHED_FILE_COUNT
			g_pNetEngine->GetStatistics(8, &stats);
			pNode->fValue = static_cast<float>(stats.dwCachedFileCount);
			return true;

		default:
			break;
		}
	} else if (pNode->dwCounterID == PERF_COUNTER_OVERLAP_COUNT) {
		uint32_t dwTotal = 0;
		uint32_t dwActive = 0;
		uint32_t dwAvailable = 0;
		ServerFramework_GetOverlapCounters(&dwTotal, &dwActive, &dwAvailable);
		pNode->fValue = static_cast<float>(dwActive);
		return true;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0094FC40]
 * ServerFramework_GetOverlapCounters
 * Native implementation @ 0x0094FC40 (32 bytes)
 */
void ServerFramework_GetOverlapCounters(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable) {
	if (pTotal != nullptr) {
		*pTotal = g_dwTotalOverlapContexts;
	}
	if (pActive != nullptr) {
		*pActive = g_dwActiveOverlapContexts;
	}
	if (pAvailable != nullptr) {
		*pAvailable = g_dwAvailableOverlapContexts;
	}
}

} // namespace ServerFramework
