/**
 * ============================================================================
 * Silkroad Online - ServerFramework Performance Monitoring Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\JMX_ServerFramework\ServerFramework\ServerPerfMonitor.h
 *
 * Implements performance counter structures and registration routines:
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

#ifndef _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERPERFMONITOR_H_
#define _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERPERFMONITOR_H_

#include "../../JMX_Library/BSLib/Synch.h"
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>

namespace ServerFramework {

class CServerMoniterView;

/**
 * Built-in Performance Counter Identifiers (Native 0x0094EBF0 switch)
 */
enum PerfCounterID : uint32_t {
	PERF_COUNTER_MSG_COUNT         = 0x2010001, // "MsgCount"
	PERF_COUNTER_SESSION_COUNT     = 0x2010002, // "SessionCount"
	PERF_COUNTER_SOCK_TCP_COUNT    = 0x2010003, // "SockTCPCount"
	PERF_COUNTER_CLASS_LINK_COUNT  = 0x2010004, // "ClassLinkCount"
	PERF_COUNTER_ACCEPT_COUNT      = 0x2010005, // "AcceptCount"
	PERF_COUNTER_USED_FILE_COUNT   = 0x2010006, // "UsedFileCount"
	PERF_COUNTER_CACHED_FILE_COUNT = 0x2010007, // "CachedFileCount"
	PERF_COUNTER_MASSIVE_MSG_COUNT = 0x3010001, // "MassiveMsgCount"
	PERF_COUNTER_OVERLAP_COUNT     = 0x3010002, // "OverlapCount"
};

#pragma pack(push, 1)
/**
 * tagPerfCounterNode
 * Native allocation size: 28 bytes (0x1C @ 0x0094EC64 / 0x0094EA64)
 */
struct tagPerfCounterNode {
	uint32_t    dwSampleCapacity = 100;                      // +0x00: Sample buffer capacity (0x64 in 0x0094EB59)
	uint32_t    dwReserved1 = 0;                             // +0x04
	float       fValue      = 0.0f;                          // +0x08: Current metric value
	uint32_t    dwSampleCount = 0;                           // +0x0C: Total samples taken
	uint32_t    dwCounterID = 0;                             // +0x10: Counter ID
	uint32_t    dwReserved3 = 0;                             // +0x14
	bool        (*pfnUpdate)(tagPerfCounterNode*) = nullptr; // +0x18: Update callback function
};

/**
 * tagNetEngineStats
 * Native buffer size: 120 bytes (0x78 @ 0x0094EE0F / 0x0096C310)
 */
struct tagNetEngineStats {
	uint32_t    dwMsgIn;                                     // +0x00
	uint32_t    dwMsgCount;                                  // +0x04: Current message queue count
	uint32_t    dwMsgOut;                                    // +0x08
	uint32_t    dwReserved0C[3];                             // +0x0C - +0x14
	uint32_t    dwSockTCPCount;                              // +0x18: TCP socket count
	uint32_t    dwReserved1C[4];                             // +0x1C - +0x28
	uint32_t    dwSessionCount;                              // +0x2C: Active session count
	uint32_t    dwReserved30[3];                             // +0x30 - +0x38
	uint32_t    dwCachedFileCount;                           // +0x3C: Cached file count
	uint32_t    dwUsedFileCount;                             // +0x40: Used file count
	uint32_t    dwReserved44[4];                             // +0x44 - +0x50
	uint32_t    dwAcceptCount;                               // +0x54: Accepted connection count
	uint32_t    dwReserved58[4];                             // +0x58 - +0x64
	uint32_t    dwClassLinkCount;                            // +0x68: Class link count
	uint32_t    dwReserved6C[3];                             // +0x6C - +0x74
};
#pragma pack(pop)

#if defined(_WIN64) || defined(__x86_64__) || defined(_M_X64)
static_assert(sizeof(tagPerfCounterNode) == 0x20, "tagPerfCounterNode must be 32 bytes on 64-bit architectures");
#else
static_assert(sizeof(tagPerfCounterNode) == 0x1C, "tagPerfCounterNode must be exactly 28 bytes (0x1C) on 32-bit x86");
#endif
static_assert(sizeof(tagNetEngineStats) == 0x78, "tagNetEngineStats must be exactly 120 bytes (0x78)");

// Native globals
extern CCriticalSectionBS                      g_csPerfCounterMap;             // 0x00D67970
extern std::map<uint32_t, tagPerfCounterNode*> g_mapPerfCounters;              // 0x00D67964
extern uint32_t                                g_dwMsgCountOfDumpTrigger;      // 0x00C827D0
extern uint32_t                                g_dwMsgDumpTriggerElapseTick;   // 0x00C827D4
extern CServerMoniterView*                     g_pPerfMonitorWnd;              // 0x00C827D8
extern void*                                   g_hPerfMonitorThread;           // 0x00C827DC
extern void*                                   g_hPerfMonitorEvent;            // 0x00C827E0
extern uint32_t                                g_dwLastMsgDumpTick;            // 0x00D6AA24

extern uint32_t                                g_dwTotalOverlapContexts;       // 0x00D67954
extern uint32_t                                g_dwActiveOverlapContexts;      // 0x00D67958
extern uint32_t                                g_dwAvailableOverlapContexts;   // 0x00D6795C

/**
 * [RECONSTRUCTED - 0x0094E7D0]
 * ServerFramework_InitPerfDumpConfig
 * Loads "MsgCountOfDumpTrigger" and "MsgDumpTriggerElapseTick" config values.
 */
void ServerFramework_InitPerfDumpConfig();

/**
 * [RECONSTRUCTED - 0x0094E820]
 * ServerFramework_StartPerfMonitorThread
 * Spawns the background performance and UI status update thread.
 */
void* ServerFramework_StartPerfMonitorThread();

/**
 * [RECONSTRUCTED - 0x0094E870]
 * ServerFramework_StopMonitorThread
 * Signals shutdown event, waits for monitor thread termination, closes handles,
 * and frees all registered performance counter nodes.
 */
void ServerFramework_StopMonitorThread();

/**
 * [RECONSTRUCTED - 0x0094E960]
 * ServerFramework_PerfMonitorThreadProc
 * Background thread procedure waking up every 1000ms to invoke each counter callback.
 */
#ifdef _WIN32
DWORD WINAPI ServerFramework_PerfMonitorThreadProc(void* lpParam);
#else
uint32_t ServerFramework_PerfMonitorThreadProc(void* lpParam);
#endif

/**
 * [RECONSTRUCTED - 0x0094EAB0]
 * RegisterCustomPerformanceCounter
 * Registers a user-defined performance counter node with the subsystem.
 */
bool RegisterCustomPerformanceCounter(uint32_t dwCounterID, const char* pszName, tagPerfCounterNode* pNode);

/**
 * [RECONSTRUCTED - 0x0094EBF0]
 * RegisterBuiltinPerformanceCounter
 * Allocates and registers a built-in counter (MsgCount, SessionCount, etc.).
 */
bool RegisterBuiltinPerformanceCounter(uint32_t dwCounterID);

/**
 * [RECONSTRUCTED - 0x0094EDB0]
 * BuiltinPerformanceCounterCallback
 * Samples current metric from CNetEngine or IOCP overlap pools.
 */
bool BuiltinPerformanceCounterCallback(tagPerfCounterNode* pNode);

/**
 * [RECONSTRUCTED - 0x0094FC40]
 * ServerFramework_GetOverlapCounters
 * Retrieves active IOCP overlap pool counters.
 */
void ServerFramework_GetOverlapCounters(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERPERFMONITOR_H_
