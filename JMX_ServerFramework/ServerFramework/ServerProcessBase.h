/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessBase Class
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessBase.h
 *
 * Implements base server process message processing task:
 *   - VTable @ 0x00B40FFC (RTTI: .?AVCServerProcessBase@ServerFramework@@)
 *   - Base: CServiceObject -> CBase
 *   - Runtime Class Descriptor @ 0x00ADD914 (size 0x400B8 = 262,328 bytes)
 *   - Direct Opcode Dispatch Table @ offset +0x4C (size 0x3FFFC = 65,535 entries)
 *   - Windows Timer Queue & Periodic Timer Map @ offset +0x40048 - +0x4005B
 *   - Asynchronous Overlap Job Manager @ offset +0x4005C - +0x400AF
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERPROCESSBASE_H_
#define _JMX_SERVERFRAMEWORK_SERVERPROCESSBASE_H_

#include "../../JMX_Library/BSLib/BSObj.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/NetEngine.h"
#include "../../JMX_Library/BSLib/Packet.h"
#include "../../JMX_Library/BSLib/Synch.h"
#include "ServerConfig.h"
#include <cstdint>
#include <map>
#include <list>

// Forward declarations
class CMsg;
class CMassiveMsg;

namespace ServerFramework {

class CServerProcessBase;

// Function pointer signature for opcode message handlers
// Native 0x0094CBF0 dispatch calls: (*pfn)(pProcess, pMsg, dwParam1, dwParam2, 0)
typedef int32_t (*PFN_MSGHANDLER)(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

// Timer callback method signature
typedef bool (CServerProcessBase::*PFN_TIMERCALLBACK)(uint8_t byTimerID);

/**
 * tagTimerEntry
 * Periodic timer descriptor registered in m_mapTimers @ +0x40050
 * Native size: 0x10 bytes (16 bytes inside std::map node)
 */
struct tagTimerEntry {
	uint8_t           byTimerID;    // +0x00: 8-bit timer identifier
	uint8_t           byPadding[3]; // +0x01: Alignment padding
	uint32_t          dwLastTick;   // +0x04: Last fired tick count from GetTickCount()
	uint32_t          dwIntervalMs; // +0x08: Interval in milliseconds (e.g. 5000ms = 0x1388)
	PFN_TIMERCALLBACK pfnCallback;  // +0x0C: Member function callback pointer (returns false to auto-unregister)
};

/**
 * COverlapJob
 * Base asynchronous inter-server routed job tracking descriptor
 * Native VTable @ 0x00B41174 (5 virtual slots)
 * Native size: 0x2C bytes (44 bytes)
 */
class COverlapJob {
public:
	// [RECONSTRUCTED - Native 0x0094FC70]
	COverlapJob();
	virtual ~COverlapJob();

	// [RECONSTRUCTED - Native 0x0094FCA0]
	// slot 1 (+0x04): Initialize
	virtual uint32_t Initialize(int32_t nParam);

	// [RECONSTRUCTED - Native 0x0094FCF0]
	// slot 2 (+0x08): Release
	virtual void Release();

	// slot 3 (+0x0C): OnExecute (purecall @ 0x009DD3AD)
	virtual void OnExecute() = 0;

	// [RECONSTRUCTED - Native 0x008245C0]
	// slot 4 (+0x10): OnTimeout
	virtual void OnTimeout();

public:
	uint32_t m_dwJobID;         // +0x04: Unique overlapped transaction identifier
	uint32_t m_dwParam;         // +0x08: Context parameter
	uint8_t  m_byState;         // +0x0C: Execution state
	uint8_t  m_bActive;         // +0x0D: Active flag
	uint16_t m_wPadding;        // +0x0E: Alignment
	uint32_t m_dwStartTick;     // +0x10: Creation timestamp from GetTickCount()
	uint32_t m_dwTimeoutMs;     // +0x14: Timeout deadline in milliseconds
	uint32_t m_dwJobType;       // +0x18: Job category / type
	uint16_t m_wSourceServerID; // +0x1C: Originating server node ID
	uint16_t m_wDestServerID;   // +0x1E: Destination server node ID
	uint32_t m_dwOpcode;        // +0x20: Inner routed protocol opcode
	uint16_t m_wSubOpcode;      // +0x24: Protocol sub-command
	uint16_t m_wPad26;          // +0x26: Alignment
	uint32_t m_dwContext;       // +0x28: Transaction tracking context
};

/**
 * COverlapJob_Local
 * Derived local overlap job descriptor for inter-server proxy communication
 * Native VTable @ 0x00B41188 (6 virtual slots)
 */
class COverlapJob_Local : public COverlapJob {
public:
	// [RECONSTRUCTED - Native 0x009511D0]
	COverlapJob_Local();
	virtual ~COverlapJob_Local() override;

	// [RECONSTRUCTED - Native 0x0094FDB0]
	// slot 3 (+0x0C): OnExecute
	virtual void OnExecute() override;

	// [RECONSTRUCTED - Native 0x0094FDD0]
	// slot 5 (+0x14): Setup
	virtual int32_t Setup(int32_t nParam, int16_t wSrcServer, int16_t wDestServer, int16_t wSubOpcode, int32_t nOpcode, int32_t nContext);

	// [RECONSTRUCTED - Native 0x0094FD90]
	static COverlapJob_Local* Allocate();
};

/**
 * COverlapJobManager
 * Manages active pending overlapped transactions across cluster
 * Native VTable @ 0x00B411D0 (1 virtual slot = destructor)
 * Native size: 0x50 bytes (80 bytes)
 */
class COverlapJobManager {
public:
	// [RECONSTRUCTED - Native 0x0094FE20]
	COverlapJobManager();

	// [RECONSTRUCTED - Native 0x0094FE90 / 0x0094FF00]
	virtual ~COverlapJobManager();

	// [RECONSTRUCTED - Native 0x009500C0]
	bool RegisterJob(COverlapJob* pJob);

	// [RECONSTRUCTED - Native 0x009501C0]
	bool UnregisterJob(COverlapJob* pJob);

	// [RECONSTRUCTED - Native 0x009502C0]
	COverlapJob* PopJob(uint32_t dwJobID);

	// [RECONSTRUCTED - Native 0x0094FF70]
	void Clear();

public:
	BSLib::CCriticalSectionBS       m_cs;      // +0x04: Thread synchronization lock (64 bytes)
	std::map<uint32_t, COverlapJob*> m_mapJobs; // +0x44: Active pending jobs indexed by JobID (12 bytes)
};

/**
 * CServerProcessBase
 * Base server process task driver and network dispatcher
 * Native VTable @ 0x00B40FFC (20 virtual slots = 80 bytes)
 * Memory Layout (Exact 262,328 bytes / 0x400B8):
 *   +0x00 - +0x4B:     CServiceObject base (76 bytes)
 *   +0x4C - +0x40047:  PFN_MSGHANDLER m_pfnMsgHandlers[65535] (262,140 bytes)
 *   +0x40048:          HANDLE m_hTimerQueueTimer (4 bytes)
 *   +0x4004C:          uint32_t m_dwTimerCount (4 bytes)
 *   +0x40050 - +0x4005B: std::map<uint8_t, tagTimerEntry> m_mapTimers (12 bytes)
 *   +0x4005C:          uint32_t m_dwOverlapJobCount (4 bytes)
 *   +0x40060 - +0x400AF: COverlapJobManager m_overlapJobManager (80 bytes = 0x50)
 *   +0x400B0:          void* m_pDefaultSession (4 bytes)
 *   +0x400B4:          int32_t m_bDrainQueue (4 bytes)
 */
class CServerProcessBase : public CServiceObject {
public:
	// [RECONSTRUCTED - Native 0x0094C840]
	CServerProcessBase();

	// [RECONSTRUCTED - Native 0x0094C8D0 / 0x0094C940]
	// slot 1 (+0x04): Destructor
	virtual ~CServerProcessBase() override;

	// [RECONSTRUCTED - Native 0x0094C830]
	// slot 0 (+0x00): GetRuntimeClass
	virtual const CRuntimeClass* GetRuntimeClass() const override;

	// [RECONSTRUCTED - Native 0x0094C9C0]
	// slot 2 (+0x08): Initialize
	virtual bool Initialize(BSLib::CTask* pTask, uint32_t dwParam) override;

	// [RECONSTRUCTED - Native 0x0094CA50]
	// slot 3 (+0x0C): Process
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) override;

	// [RECONSTRUCTED - Native 0x0094CBA0]
	// slot 4 (+0x10): OnMessage (drains pending timers and clears overlap jobs)
	virtual void OnMessage() override;

	// [RECONSTRUCTED - Native 0x00404050]
	// slot 5 (+0x14): GetGameID (inherited dummy return 0)
	virtual uint32_t GetGameID();

	// [RECONSTRUCTED - Native 0x0094CC40]
	// slot 6 (+0x18): ProcessMessage (registers default handlers for 0x2007, 0x6008, 0xA008, 0xA009)
	virtual int32_t ProcessMessage();

	// [RECONSTRUCTED - Native 0x0094CBE0]
	// slot 7 (+0x1C): RegisterMsgHandler
	virtual uint32_t RegisterMsgHandler(uint16_t wMsgID, PFN_MSGHANDLER pfnHandler);

	// [RECONSTRUCTED - Native 0x0094CBF0]
	// slot 8 (+0x20): DispatchMessage
	virtual int32_t DispatchMessage(void* pMsg, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwParam3 = 0);

	// [RECONSTRUCTED - Native 0x0094CC90]
	// slot 9 (+0x24): OnUnhandledMsg
	virtual int32_t OnUnhandledMsg(void* pMsg, void* pMassiveMsg = nullptr);

	// [RECONSTRUCTED - Native 0x0094CE30]
	// slot 10 (+0x28): PostHandshakeTimer (allocates buffer, sets opcode 0x2007, pushes to queue)
	virtual int32_t PostHandshakeTimer();

	// [RECONSTRUCTED - Native 0x0094CE60]
	// slot 11 (+0x2C): OnMsg_ProcessTimers (opcode 0x2007 handler; dispatches periodic timer callbacks)
	virtual int32_t OnMsg_ProcessTimers(void* pMsg, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwReserved = 0);

	// [RECONSTRUCTED - Native 0x0094CF20]
	// slot 12 (+0x30): OnMsg_RoutedMessageReq (opcode 0x6008 handler; forwards packet across cluster)
	virtual int32_t OnMsg_RoutedMessageReq(void* pMsg, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwReserved = 0);

	// [RECONSTRUCTED - Native 0x0094D240]
	// slot 13 (+0x34): OnMsg_RoutedMessageAck (opcode 0xA008 handler; handles response / overlap completion)
	virtual int32_t OnMsg_RoutedMessageAck(void* pMsg, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwReserved = 0);

	// [RECONSTRUCTED - Native 0x0094D660]
	// slot 14 (+0x38): OnMsg_OverlapJobAck (opcode 0xA009 handler; handles local overlap job completion)
	virtual int32_t OnMsg_OverlapJobAck(void* pMsg, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwReserved = 0);

	// [RECONSTRUCTED - Native 0x0094DA60]
	// slot 15 (+0x3C): ForwardRoutedMessageAck
	virtual int32_t ForwardRoutedMessageAck(uint32_t wLength, void* pMsg, uint32_t dwTargetSession);

	// [RECONSTRUCTED - Native 0x0094D990]
	// slot 16 (+0x40): ForwardRoutedMessage
	virtual int32_t ForwardRoutedMessage(void* pMsg, void* pBuffer, uint32_t dwTargetSession);

	// [RECONSTRUCTED - Native 0x009371F0]
	// slot 17 (+0x44): OnIdle (no-op)
	virtual void OnIdle();

	// [RECONSTRUCTED - Native 0x0094DC30]
	// slot 18 (+0x48): BroadcastToSessions
	virtual int32_t BroadcastToSessions(int32_t pMsg, std::list<uint32_t>* pSessionList);

	// [RECONSTRUCTED - Native 0x0094DB80]
	// slot 19 (+0x4C): StartOverlapJob
	virtual int32_t StartOverlapJob(COverlapJob* pJob, void* pTargetSession = nullptr);

	// Timer subsystem helper methods
	// [RECONSTRUCTED - Native 0x0094CD60]
	bool CreateTimerQueueTimer();

	// [RECONSTRUCTED - Native 0x0094CDE0]
	void DeleteTimers();

	// [RECONSTRUCTED - Native 0x0094CD90]
	bool RegisterDefaultTimer();

	// [RECONSTRUCTED - Native 0x0094DD30]
	bool RegisterTimer(uint8_t byTimerID, uint32_t dwIntervalMs, PFN_TIMERCALLBACK pfnCallback);

	// Cluster proxy forwarding helpers
	// [RECONSTRUCTED - Native 0x0094D6F0]
	bool RouteMsgAcrossCluster(void* pMsg, uint32_t dwTargetServerID, uint32_t dwParam1, uint32_t dwParam2, int32_t* pResult);

	// [RECONSTRUCTED - Native 0x0094D860]
	bool RouteMassiveMsgAcrossCluster(void* pMsg, uint32_t dwTargetServerID, uint32_t dwParam1, uint32_t dwParam2, int32_t* pResult);

	// Static opcode handler forwarding thunks (Native 0x0094E6A0 - 0x0094E6D0)
	// Formerly mislabeled in reverse engineering as CServerFrameWindow GUI thunks!
	static int32_t thunk_DefaultMsgHandler(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);
	static int32_t thunk_OnMsg_ProcessTimers(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);
	static int32_t thunk_OnMsg_RoutedMessageReq(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);
	static int32_t thunk_OnMsg_RoutedMessageAck(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);
	static int32_t thunk_OnMsg_OverlapJobAck(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// Windows Timer Queue Callback procedure (Native 0x0094CD50)
	static void CALLBACK TimerQueueCallback(void* lpParameter, uint8_t TimerOrWaitFired);

	// Native runtime descriptor @ 0x00ADD914
	static const CRuntimeClass ms_runtimeClass;

	// [RECONSTRUCTED - Native 0x0094C7D0]
	static void* CreateObject();
	static void DestroyObject(void* pObj);

public:
	// Native offset +0x4C: Direct function pointer lookup table for opcodes 0x0000 - 0xFFFE
	// Size: 65,535 pointers * 4 = 262,140 (0x3FFFC) bytes
	PFN_MSGHANDLER m_pfnMsgHandlers[65535];

	// Native offset +0x40048: Windows timer queue timer handle
	HANDLE m_hTimerQueueTimer; // +0x40048

	// Native offset +0x4004C: Active periodic timer counter / connection count
	uint32_t m_dwTimerCount; // +0x4004C

	// Native offset +0x40050: Registered periodic timers map (12 bytes)
	std::map<uint8_t, tagTimerEntry> m_mapTimers; // +0x40050

	// Native offset +0x4005C: Active overlap job counter
	uint32_t m_dwOverlapJobCount; // +0x4005C

	// Native offset +0x40060: Asynchronous overlap job manager (80 bytes = 0x50)
	COverlapJobManager m_overlapJobManager; // +0x40060

	// Native offset +0x400B0: Default session descriptor
	void* m_pDefaultSession; // +0x400B0

	// Native offset +0x400B4: Queue drain flag checked during shutdown
	int32_t m_bDrainQueue; // +0x400B4
};

// [RECONSTRUCTED - Native 0x00937850]
// Verifies all payload bytes of a message were read before releasing buffer
void CServerProcessBase_CheckUnconsumedMessage(void* pMsg);

// [RECONSTRUCTED - Native 0x009375E0]
// Formats diagnostic string for an unconsumed message
const char* CServerProcessBase_FormatUnconsumedMessage(char* pszOut, void* pMsg);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERPROCESSBASE_H_
