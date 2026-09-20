/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessBase Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessBase.cpp
 *
 * Implements:
 *   - COverlapJob Methods (VTable @ 0x00B41174)
 *   - COverlapJob_Local Methods (VTable @ 0x00B41188)
 *   - COverlapJobManager Methods (VTable @ 0x00B411D0)
 *   - CServerProcessBase Methods (VTable @ 0x00B40FFC):
 *       - Ctor @ 0x0094C840, Dtor @ 0x0094C940 / 0x0094C8D0
 *       - GetRuntimeClass @ 0x0094C830
 *       - Initialize @ 0x0094C9C0
 *       - Process @ 0x0094CA50
 *       - OnMessage @ 0x0094CBA0
 *       - ProcessMessage @ 0x0094CC40
 *       - RegisterMsgHandler @ 0x0094CBE0
 *       - DispatchMessage @ 0x0094CBF0
 *       - OnUnhandledMsg @ 0x0094CC90
 *       - PostHandshakeTimer @ 0x0094CE30
 *       - OnMsg_ProcessTimers @ 0x0094CE60
 *       - OnMsg_RoutedMessageReq @ 0x0094CF20
 *       - OnMsg_RoutedMessageAck @ 0x0094D240
 *       - OnMsg_OverlapJobAck @ 0x0094D660
 *       - ForwardRoutedMessageAck @ 0x0094DA60
 *       - ForwardRoutedMessage @ 0x0094D990
 *       - OnIdle @ 0x009371F0
 *       - BroadcastToSessions @ 0x0094DC30
 *       - StartOverlapJob @ 0x0094DB80
 *   - Timer Subsystem Helpers (0x0094CD50 - 0x0094CDE0)
 *   - Cluster Routing Helpers (0x0094D6F0 - 0x0094D860)
 *   - Message Diagnostic Routines (0x009375E0, 0x00937850)
 * ============================================================================
 */

#include "ServerProcessBase.h"
#include "ServerTopology.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ServerFramework {

// ============================================================================
// COverlapJob Implementation (VTable @ 0x00B41174)
// ============================================================================

// [RECONSTRUCTED - Native 0x0094FC70]
COverlapJob::COverlapJob()
	: m_dwJobID(0)
	, m_dwParam(0)
	, m_byState(0)
	, m_bActive(0)
	, m_wPadding(0)
	, m_dwStartTick(0)
	, m_dwTimeoutMs(0xFFFFFFFF)
	, m_dwJobType(0)
	, m_wSourceServerID(0)
	, m_wDestServerID(0)
	, m_dwOpcode(0)
	, m_wSubOpcode(0)
	, m_wPad26(0)
	, m_dwContext(0) {
}

COverlapJob::~COverlapJob() {
	Release();
}

// [RECONSTRUCTED - Native 0x0094FCA0]
uint32_t COverlapJob::Initialize(int32_t nParam) {
	m_dwParam = static_cast<uint32_t>(nParam);
	m_byState = 0;
#ifdef _WIN32
	m_dwStartTick = ::GetTickCount();
#else
	m_dwStartTick = 0;
#endif
	m_dwTimeoutMs = 0xFFFFFFFF;
	return m_dwStartTick;
}

// [RECONSTRUCTED - Native 0x0094FCF0]
void COverlapJob::Release() {
	m_dwJobID = 0;
	m_bActive = 0;
}

// [RECONSTRUCTED - Native 0x008245C0]
void COverlapJob::OnTimeout() {
	m_byState = 3; // Timeout state
}

// ============================================================================
// COverlapJob_Local Implementation (VTable @ 0x00B41188)
// ============================================================================

// [RECONSTRUCTED - Native 0x009511D0]
COverlapJob_Local::COverlapJob_Local()
	: COverlapJob() {
}

COverlapJob_Local::~COverlapJob_Local() {
}

// [RECONSTRUCTED - Native 0x0094FDB0]
void COverlapJob_Local::OnExecute() {
	Release();
	delete this;
}

// [RECONSTRUCTED - Native 0x0094FDD0]
int32_t COverlapJob_Local::Setup(int32_t nParam, int16_t wSrcServer, int16_t wDestServer, int16_t wSubOpcode, int32_t nOpcode, int32_t nContext) {
	Initialize(nParam);
	m_wSourceServerID = static_cast<uint16_t>(wSrcServer);
	m_wDestServerID = static_cast<uint16_t>(wDestServer);
	m_wSubOpcode = static_cast<uint16_t>(wSubOpcode);
	m_dwOpcode = static_cast<uint32_t>(nOpcode);
	m_dwContext = static_cast<uint32_t>(nContext);
	m_bActive = 1;
	m_dwJobType = 1;
	return 1;
}

// [RECONSTRUCTED - Native 0x0094FD90]
COverlapJob_Local* COverlapJob_Local::Allocate() {
	static uint32_t s_dwNextJobID = 1;
	auto* pJob = new COverlapJob_Local();
	pJob->m_dwJobID = s_dwNextJobID++;
	return pJob;
}

// ============================================================================
// COverlapJobManager Implementation (VTable @ 0x00B411D0)
// ============================================================================

// [RECONSTRUCTED - Native 0x0094FE20]
COverlapJobManager::COverlapJobManager()
	: m_cs("COverlapJobManager::CS") {
}

// [RECONSTRUCTED - Native 0x0094FE90 / 0x0094FF00]
COverlapJobManager::~COverlapJobManager() {
	Clear();
}

// [RECONSTRUCTED - Native 0x009500C0]
bool COverlapJobManager::RegisterJob(COverlapJob* pJob) {
	if (!pJob || pJob->m_dwJobID == 0) {
		return false;
	}

	m_cs.Lock();
	m_mapJobs[pJob->m_dwJobID] = pJob;
	m_cs.Unlock();
	return true;
}

// [RECONSTRUCTED - Native 0x009501C0]
bool COverlapJobManager::UnregisterJob(COverlapJob* pJob) {
	if (!pJob) {
		return false;
	}

	m_cs.Lock();
	auto it = m_mapJobs.find(pJob->m_dwJobID);
	if (it != m_mapJobs.end()) {
		m_mapJobs.erase(it);
		pJob->Release();
		m_cs.Unlock();
		return true;
	}
	m_cs.Unlock();
	return false;
}

// [RECONSTRUCTED - Native 0x009502C0]
COverlapJob* COverlapJobManager::PopJob(uint32_t dwJobID) {
	m_cs.Lock();
	auto it = m_mapJobs.find(dwJobID);
	if (it != m_mapJobs.end()) {
		COverlapJob* pJob = it->second;
		m_mapJobs.erase(it);
		m_cs.Unlock();
		return pJob;
	}
	m_cs.Unlock();
	return nullptr;
}

// [RECONSTRUCTED - Native 0x0094FF70]
void COverlapJobManager::Clear() {
	m_cs.Lock();
	for (auto& pair : m_mapJobs) {
		COverlapJob* pJob = pair.second;
		if (pJob) {
			pJob->OnTimeout();
			pJob->Release();
		}
	}
	m_mapJobs.clear();
	m_cs.Unlock();
}

// ============================================================================
// CServerProcessBase Implementation (VTable @ 0x00B40FFC)
// ============================================================================

// Native runtime class descriptor @ 0x00ADD914
const CRuntimeClass CServerProcessBase::ms_runtimeClass = {
	"CServerProcessBase",
	0x400B8, // 262,328 bytes
	&CServerProcessBase::CreateObject,
	&CServerProcessBase::DestroyObject,
	&CServiceObject::ms_classCServiceObject
};

// [RECONSTRUCTED - Native 0x0094C7D0]
void* CServerProcessBase::CreateObject() {
	return new CServerProcessBase();
}

void CServerProcessBase::DestroyObject(void* pObj) {
	delete static_cast<CServerProcessBase*>(pObj);
}

// [RECONSTRUCTED - Native 0x0094C840]
CServerProcessBase::CServerProcessBase()
	: CServiceObject()
	, m_hTimerQueueTimer(nullptr)
	, m_dwTimerCount(0)
	, m_dwOverlapJobCount(0)
	, m_pDefaultSession(nullptr)
	, m_bDrainQueue(0) {
	for (int i = 0; i < 65535; ++i) {
		m_pfnMsgHandlers[i] = &CServerProcessBase::thunk_DefaultMsgHandler;
	}
}

// [RECONSTRUCTED - Native 0x0094C8D0 / 0x0094C940]
CServerProcessBase::~CServerProcessBase() {
	DeleteTimers();
	m_overlapJobManager.Clear();
}

// [RECONSTRUCTED - Native 0x0094C830]
const CRuntimeClass* CServerProcessBase::GetRuntimeClass() const {
	return &ms_runtimeClass;
}

// [RECONSTRUCTED - Native 0x0094C9C0]
bool CServerProcessBase::Initialize(BSLib::CTask* pTask, uint32_t dwParam) {
	m_pTask = pTask;
	m_dwParam = dwParam;

	// Populate all 65,535 handler slots with the default procedure (0x0094C9E0)
	for (int i = 0; i < 65535; ++i) {
		m_pfnMsgHandlers[i] = &CServerProcessBase::thunk_DefaultMsgHandler;
	}

	// Register specific opcode handlers via virtual ProcessMessage()
	ProcessMessage();

	// If timer count or periodic requirements are configured, start Windows timer
	if (m_dwTimerCount != 0) {
		CreateTimerQueueTimer();
	}

	// If overlap jobs are configured, register default maintenance timer
	if (m_dwOverlapJobCount != 0) {
		RegisterDefaultTimer();
	}

	return true;
}

// [RECONSTRUCTED - Native 0x0094CA50]
int32_t CServerProcessBase::Process(int32_t* pStopFlag, int32_t nThreadIndex) {
	if (!pStopFlag || *pStopFlag != 0) {
		return 0;
	}

	while (*pStopFlag == 0 || m_bDrainQueue == 1) {
		void* pMsg = nullptr;

		if (!m_pTask) {
			break;
		}

		BSLib::IQue<void*>* pMsgQueue = reinterpret_cast<BSLib::IQue<void*>*>(m_pTask->GetMessageQueue());
		if (!pMsgQueue) {
			break;
		}

		// Pop next message with infinite wait (-1 / 0xFFFFFFFF) (Native 0x0094CAC0)
		if (pMsgQueue->Pop(&pMsg, 0xFFFFFFFF, 0) == 0) {
			break;
		}

		if (pMsg) {
			if (nThreadIndex == 0) {
				BSLib::GenerateMiniDump();
			}

			// Store thread index at offset +0x1064 (Native 0x0094CB03)
			*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pMsg) + 0x1064) = nThreadIndex;

			// Dispatch message via virtual slot 8 (+0x20) (Native 0x0094CB17)
			DispatchMessage(pMsg, 0, 0, 0);

			// Validate consumption (Native 0x0094CB1C)
			CServerProcessBase_CheckUnconsumedMessage(pMsg);

			// Release packet buffer via g_pNetEngine (Native 0x0094CB30)
			if (g_pNetEngine) {
				g_pNetEngine->ReleaseBuffer(pMsg);
			}
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x0094CBA0]
void CServerProcessBase::OnMessage() {
	if (m_dwTimerCount != 0) {
		DeleteTimers();
	}

	if (m_dwOverlapJobCount != 0) {
		m_overlapJobManager.Clear();
		m_dwOverlapJobCount = 0;
	}
}

// [RECONSTRUCTED - Native 0x00404050]
uint32_t CServerProcessBase::GetGameID() {
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CC40]
int32_t CServerProcessBase::ProcessMessage() {
	RegisterMsgHandler(0x2007, &CServerProcessBase::thunk_OnMsg_ProcessTimers);
	RegisterMsgHandler(0xA008, &CServerProcessBase::thunk_OnMsg_RoutedMessageAck);
	RegisterMsgHandler(0x6008, &CServerProcessBase::thunk_OnMsg_RoutedMessageReq);
	RegisterMsgHandler(0xA009, &CServerProcessBase::thunk_OnMsg_OverlapJobAck);
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CBE0]
uint32_t CServerProcessBase::RegisterMsgHandler(uint16_t wMsgID, PFN_MSGHANDLER pfnHandler) {
	if (wMsgID < 65535) {
		m_pfnMsgHandlers[wMsgID] = pfnHandler;
	}
	return static_cast<uint32_t>(wMsgID);
}

// [RECONSTRUCTED - Native 0x0094CBF0]
int32_t CServerProcessBase::DispatchMessage(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3) {
	if (!pMsg) {
		return 0;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pMsg);
	uint8_t* pPayload = *reinterpret_cast<uint8_t**>(pByte + 0x1050);
	if (!pPayload) {
		return 0;
	}

	uint16_t wOpcode = *reinterpret_cast<uint16_t*>(pPayload);
	if (wOpcode < 65535) {
		PFN_MSGHANDLER pfnHandler = m_pfnMsgHandlers[wOpcode];
		if (pfnHandler) {
			return pfnHandler(this, pMsg, dwParam1, dwParam2, dwParam3);
		}
	}

	return OnUnhandledMsg(pMsg, nullptr);
}

// [RECONSTRUCTED - Native 0x0094CC90]
int32_t CServerProcessBase::OnUnhandledMsg(void* pMsg, void* /*pMassiveMsg*/) {
	if (pMsg) {
		char szDesc[256] = {};
		CServerProcessBase_FormatUnconsumedMessage(szDesc, pMsg);
		BSLib::Log_Printf(0x1000000, "unhandled msg received %s : %s", GetRuntimeClass()->lpszClassName, szDesc);

		// Advance read pointer to end so unconsumed warning is not duplicated
		uint8_t* pByte = reinterpret_cast<uint8_t*>(pMsg);
		*reinterpret_cast<uint16_t*>(pByte + 0x103C) = *reinterpret_cast<uint16_t*>(pByte + 0x103E);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CE30]
int32_t CServerProcessBase::PostHandshakeTimer() {
	if (!g_pNetEngine || !m_pTask) {
		return 0;
	}

	void* pBuffer = g_pNetEngine->AllocateBuffer(0);
	if (!pBuffer) {
		return 0;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pBuffer);
	uint8_t* pPayload = *reinterpret_cast<uint8_t**>(pByte + 0x1050);
	if (pPayload) {
		*reinterpret_cast<uint16_t*>(pPayload) = 0x2007; // Opcode 0x2007: Periodic Timer Event
	}

	BSLib::IQue<void*>* pMsgQueue = reinterpret_cast<BSLib::IQue<void*>*>(m_pTask->GetMessageQueue());
	if (pMsgQueue) {
		return pMsgQueue->Push(pBuffer);
	}

	g_pNetEngine->ReleaseBuffer(pBuffer);
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CE60]
int32_t CServerProcessBase::OnMsg_ProcessTimers(void* /*pMsg*/, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, uint32_t /*dwReserved*/) {
#ifdef _WIN32
	uint32_t dwNow = ::GetTickCount();
#else
	uint32_t dwNow = 0;
#endif

	auto it = m_mapTimers.begin();
	while (it != m_mapTimers.end()) {
		tagTimerEntry& entry = it->second;
		if (dwNow - entry.dwLastTick >= entry.dwIntervalMs) {
			entry.dwLastTick = dwNow;
			if (entry.pfnCallback) {
				bool bKeepRunning = (this->*(entry.pfnCallback))(entry.byTimerID);
				if (!bKeepRunning) {
					it = m_mapTimers.erase(it);
					continue;
				}
			}
		}
		++it;
	}

	return 1;
}

// [RECONSTRUCTED - Native 0x0094CF20]
int32_t CServerProcessBase::OnMsg_RoutedMessageReq(void* pMsg, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, uint32_t /*dwReserved*/) {
	if (!pMsg) {
		return 0;
	}

	BSLib::CPacket packet(pMsg);
	uint32_t dwLength = 0;
	uint16_t wSequence = 0;
	uint16_t wDestServerID = 0;

	packet.Read(&dwLength, sizeof(dwLength));
	packet.Read(&wSequence, sizeof(wSequence));
	packet.Read(&wDestServerID, sizeof(wDestServerID));

	// If this packet is targeted directly to local server, dispatch inner payload
	if (g_pLocalServerInfo && wDestServerID == g_pLocalServerInfo->wServerID) {
		return DispatchMessage(pMsg, 0, 0, 0);
	}

	// Forward packet across cluster to target server
	int32_t nResult = 0;
	if (!RouteMsgAcrossCluster(pMsg, wDestServerID, dwLength, wSequence, &nResult)) {
		// Routing failed, respond with error ack 0xA008
		if (g_pNetEngine) {
			void* pAckBuffer = g_pNetEngine->AllocateBuffer(0);
			if (pAckBuffer) {
				uint8_t* pAckByte = reinterpret_cast<uint8_t*>(pAckBuffer);
				uint8_t* pAckPayload = *reinterpret_cast<uint8_t**>(pAckByte + 0x1050);
				if (pAckPayload) {
					*reinterpret_cast<uint16_t*>(pAckPayload) = 0xA008;
				}
				BSLib::CPacket ackPkt(pAckBuffer);
				uint8_t byStatus = 2; // Error
				ackPkt.Write(&byStatus, 1);
				ackPkt.Write(&dwLength, 4);
				ackPkt.Write(&wSequence, 2);

				uint8_t* pOrigByte = reinterpret_cast<uint8_t*>(pMsg);
				uint32_t dwSessionID = *reinterpret_cast<uint32_t*>(pOrigByte + 0x1060);
				g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwSessionID)), pAckBuffer);
				g_pNetEngine->ReleaseBuffer(pAckBuffer);
			}
		}
	}

	return 1;
}

// [RECONSTRUCTED - Native 0x0094D240]
int32_t CServerProcessBase::OnMsg_RoutedMessageAck(void* pMsg, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, uint32_t /*dwReserved*/) {
	if (!pMsg) {
		return 0;
	}

	BSLib::CPacket packet(pMsg);
	uint8_t  byStatus = 0;
	uint32_t dwJobID = 0;
	uint16_t wSubCode = 0;

	packet.Read(&byStatus, 1);
	packet.Read(&dwJobID, 4);
	packet.Read(&wSubCode, 2);

	COverlapJob* pJob = m_overlapJobManager.PopJob(dwJobID);
	if (pJob) {
		if (g_pLocalServerInfo && pJob->m_wDestServerID == g_pLocalServerInfo->wServerID) {
			if (byStatus == 1) {
				DispatchMessage(pMsg, dwJobID, pJob->m_dwParam, 0);
			}
			pJob->Release();
			return 1;
		}

		// Forward routed ack back along inter-server path
		ForwardRoutedMessageAck(dwJobID, pMsg, pJob->m_wSourceServerID);
		pJob->Release();
		return 1;
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x0094D660]
int32_t CServerProcessBase::OnMsg_OverlapJobAck(void* pMsg, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, uint32_t /*dwReserved*/) {
	if (!pMsg) {
		return 0;
	}

	BSLib::CPacket packet(pMsg);
	uint32_t dwJobID = 0;
	packet.Read(&dwJobID, sizeof(dwJobID));

	COverlapJob* pJob = m_overlapJobManager.PopJob(dwJobID);
	if (!pJob) {
		BSLib::Log_Printf(0x2000000, "overlapjob local ack : pOverlapJob is NULL\n");
		return 0;
	}

	if (pJob->m_bActive == 0 && pJob->m_byState == 3) {
		pJob->OnTimeout();
	} else {
		pJob->OnExecute();
	}

	pJob->Release();
	return 1;
}

// [RECONSTRUCTED - Native 0x0094DA60]
int32_t CServerProcessBase::ForwardRoutedMessageAck(uint32_t wLength, void* pMsg, uint32_t dwTargetSession) {
	if (!g_pNetEngine || !pMsg) {
		return 0;
	}

	void* pBuffer = g_pNetEngine->AllocateBuffer(0);
	if (!pBuffer) {
		return 0;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pBuffer);
	uint8_t* pPayload = *reinterpret_cast<uint8_t**>(pByte + 0x1050);
	if (pPayload) {
		*reinterpret_cast<uint16_t*>(pPayload) = 0xA008;
	}

	BSLib::CPacket outPkt(pBuffer);
	uint8_t byStatus = 1;
	outPkt.Write(&byStatus, 1);
	outPkt.Write(&wLength, 4);

	g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwTargetSession)), pBuffer);
	g_pNetEngine->ReleaseBuffer(pBuffer);
	return 1;
}

// [RECONSTRUCTED - Native 0x0094D990]
int32_t CServerProcessBase::ForwardRoutedMessage(void* pMsg, void* /*pBuffer*/, uint32_t dwTargetSession) {
	if (!g_pNetEngine || !pMsg) {
		return 0;
	}

	void* pNewBuffer = g_pNetEngine->AllocateBuffer(0);
	if (!pNewBuffer) {
		return 0;
	}

	uint8_t* pNewByte = reinterpret_cast<uint8_t*>(pNewBuffer);
	uint8_t* pNewPayload = *reinterpret_cast<uint8_t**>(pNewByte + 0x1050);
	if (pNewPayload) {
		*reinterpret_cast<uint16_t*>(pNewPayload) = 0xA008;
	}

	g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwTargetSession)), pNewBuffer);
	g_pNetEngine->ReleaseBuffer(pNewBuffer);
	return 1;
}

// [RECONSTRUCTED - Native 0x009371F0]
void CServerProcessBase::OnIdle() {
}

// [RECONSTRUCTED - Native 0x0094DC30]
int32_t CServerProcessBase::BroadcastToSessions(int32_t pMsg, std::list<uint32_t>* pSessionList) {
	if (!g_pNetEngine || !pSessionList || !pMsg) {
		return 0;
	}

	void* pBuffer = reinterpret_cast<void*>(static_cast<uintptr_t>(pMsg));
	for (uint32_t dwSessionID : *pSessionList) {
		g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwSessionID)), pBuffer);
	}

	g_pNetEngine->ReleaseBuffer(pBuffer);
	return 1;
}

// [RECONSTRUCTED - Native 0x0094DB80]
int32_t CServerProcessBase::StartOverlapJob(COverlapJob* pJob, void* pTargetSession) {
	if (!pJob || !g_pNetEngine) {
		return 0;
	}

	void* pBuffer = g_pNetEngine->AllocateBuffer(0);
	if (!pBuffer) {
		return 0;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pBuffer);
	uint8_t* pPayload = *reinterpret_cast<uint8_t**>(pByte + 0x1050);
	if (pPayload) {
		*reinterpret_cast<uint16_t*>(pPayload) = 0x6009; // Opcode 0x6009: Start Overlap Job
	}

	BSLib::CPacket pkt(pBuffer);
	pkt.Write(&pJob->m_dwJobID, sizeof(pJob->m_dwJobID));

	m_overlapJobManager.RegisterJob(pJob);

	void* pSession = pTargetSession ? pTargetSession : m_pDefaultSession;
	if (pSession) {
		uint32_t dwSessionID = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pSession) + 0x04);
		g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwSessionID)), pBuffer);
		g_pNetEngine->ReleaseBuffer(pBuffer);
		return 1;
	}

	m_overlapJobManager.UnregisterJob(pJob);
	g_pNetEngine->ReleaseBuffer(pBuffer);
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CD60]
bool CServerProcessBase::CreateTimerQueueTimer() {
#ifdef _WIN32
	if (m_hTimerQueueTimer != nullptr) {
		return true;
	}

	BOOL bRes = ::CreateTimerQueueTimer(
		&m_hTimerQueueTimer,
		nullptr,
		reinterpret_cast<WAITORTIMERCALLBACK>(&CServerProcessBase::TimerQueueCallback),
		this,
		1000,
		1000,
		WT_EXECUTEDEFAULT
	);
	return bRes != FALSE;
#else
	return true;
#endif
}

// [RECONSTRUCTED - Native 0x0094CDE0]
void CServerProcessBase::DeleteTimers() {
	if (m_mapTimers.empty()) {
		return;
	}

	m_mapTimers.clear();
	m_dwTimerCount = 0;

#ifdef _WIN32
	if (m_hTimerQueueTimer != nullptr) {
		::DeleteTimerQueueTimer(nullptr, m_hTimerQueueTimer, nullptr);
		m_hTimerQueueTimer = nullptr;
	}
#endif
}

// [RECONSTRUCTED - Native 0x0094CD90]
bool CServerProcessBase::RegisterDefaultTimer() {
	return RegisterTimer(1, 5000, nullptr);
}

// [RECONSTRUCTED - Native 0x0094DD30]
bool CServerProcessBase::RegisterTimer(uint8_t byTimerID, uint32_t dwIntervalMs, PFN_TIMERCALLBACK pfnCallback) {
	tagTimerEntry entry = {};
	entry.byTimerID = byTimerID;
#ifdef _WIN32
	entry.dwLastTick = ::GetTickCount();
#else
	entry.dwLastTick = 0;
#endif
	entry.dwIntervalMs = dwIntervalMs;
	entry.pfnCallback = pfnCallback;

	m_mapTimers[byTimerID] = entry;
	m_dwTimerCount = static_cast<uint32_t>(m_mapTimers.size());
	return true;
}

// [RECONSTRUCTED - Native 0x0094D6F0]
bool CServerProcessBase::RouteMsgAcrossCluster(void* pMsg, uint32_t dwTargetServerID, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, int32_t* pResult) {
	if (pResult) {
		*pResult = 0;
	}

	CServerLink* pLink = ServerFramework_GetServerLinkByServerID(static_cast<uint16_t>(dwTargetServerID));
	if (!pLink || pLink->m_dwSessionID == 0) {
		return false;
	}

	COverlapJob_Local* pJob = COverlapJob_Local::Allocate();
	if (!pJob) {
		return false;
	}

	pJob->Setup(0, g_pLocalServerInfo ? g_pLocalServerInfo->wServerID : 0, static_cast<int16_t>(dwTargetServerID), 0, 0x6008, 0);
	m_overlapJobManager.RegisterJob(pJob);

	if (g_pNetEngine) {
		g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(pLink->m_dwSessionID)), pMsg);
		if (pResult) {
			*pResult = 1;
		}
		return true;
	}

	m_overlapJobManager.UnregisterJob(pJob);
	return false;
}

// [RECONSTRUCTED - Native 0x0094D860]
bool CServerProcessBase::RouteMassiveMsgAcrossCluster(void* pMsg, uint32_t dwTargetServerID, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, int32_t* pResult) {
	if (pResult) {
		*pResult = 0;
	}

	CServerLink* pLink = ServerFramework_GetServerLinkByServerID(static_cast<uint16_t>(dwTargetServerID));
	if (!pLink || pLink->m_dwSessionID == 0) {
		BSLib::Log_Printf(0x2000000, "FindServerCordToRouteServer Failed !!! (%d -> %d)\n",
			g_pLocalServerInfo ? g_pLocalServerInfo->wServerID : 0, dwTargetServerID);
		return false;
	}

	COverlapJob_Local* pJob = COverlapJob_Local::Allocate();
	if (!pJob) {
		return false;
	}

	pJob->Setup(0, g_pLocalServerInfo ? g_pLocalServerInfo->wServerID : 0, static_cast<int16_t>(dwTargetServerID), 0, 0x6008, 0);
	m_overlapJobManager.RegisterJob(pJob);

	if (g_pNetEngine) {
		g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(pLink->m_dwSessionID)), pMsg);
		if (pResult) {
			*pResult = 1;
		}
		return true;
	}

	m_overlapJobManager.UnregisterJob(pJob);
	return false;
}

// [RECONSTRUCTED - Native 0x0094E6A0]
// Static unhandled opcode dispatch procedure
int32_t CServerProcessBase::thunk_DefaultMsgHandler(void* pProcess, void* pMsg, uint32_t /*dwParam1*/, uint32_t /*dwParam2*/, uint32_t /*dwReserved*/) {
	if (pProcess) {
		return static_cast<CServerProcessBase*>(pProcess)->OnUnhandledMsg(pMsg, nullptr);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x0094E6D0]
int32_t CServerProcessBase::thunk_OnMsg_ProcessTimers(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	if (pProcess) {
		return static_cast<CServerProcessBase*>(pProcess)->OnMsg_ProcessTimers(pMsg, dwParam1, dwParam2, dwReserved);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x00939650]
int32_t CServerProcessBase::thunk_OnMsg_RoutedMessageReq(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	if (pProcess) {
		return static_cast<CServerProcessBase*>(pProcess)->OnMsg_RoutedMessageReq(pMsg, dwParam1, dwParam2, dwReserved);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x0094E6B0]
int32_t CServerProcessBase::thunk_OnMsg_RoutedMessageAck(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	if (pProcess) {
		return static_cast<CServerProcessBase*>(pProcess)->OnMsg_RoutedMessageAck(pMsg, dwParam1, dwParam2, dwReserved);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x0094E6C0]
int32_t CServerProcessBase::thunk_OnMsg_OverlapJobAck(void* pProcess, void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	if (pProcess) {
		return static_cast<CServerProcessBase*>(pProcess)->OnMsg_OverlapJobAck(pMsg, dwParam1, dwParam2, dwReserved);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x0094CD50]
void CALLBACK CServerProcessBase::TimerQueueCallback(void* lpParameter, uint8_t /*TimerOrWaitFired*/) {
	if (lpParameter) {
		static_cast<CServerProcessBase*>(lpParameter)->PostHandshakeTimer();
	}
}

// [RECONSTRUCTED - Native 0x00937850]
void CServerProcessBase_CheckUnconsumedMessage(void* pMsg) {
	if (!pMsg) {
		return;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pMsg);
	uint32_t nRefCount = *reinterpret_cast<uint32_t*>(pByte + 0x1044);
	if (nRefCount == 1) {
		uint16_t nReadBytes = *reinterpret_cast<uint16_t*>(pByte + 0x103C);
		uint16_t nTotalBytes = *reinterpret_cast<uint16_t*>(pByte + 0x103E);

		if (nReadBytes != nTotalBytes) {
			char szDesc[256];
			CServerProcessBase_FormatUnconsumedMessage(szDesc, pMsg);
			BSLib::Log_Printf(0x1000000, "msg is not completely used : %s", szDesc);
			*reinterpret_cast<uint16_t*>(pByte + 0x103C) = nTotalBytes;
		}
	}
}

// [RECONSTRUCTED - Native 0x009375E0]
const char* CServerProcessBase_FormatUnconsumedMessage(char* pszOut, void* pMsg) {
	if (!pszOut) {
		return "";
	}

	if (!pMsg) {
		std::snprintf(pszOut, 256, "NULL message");
		return pszOut;
	}

	uint8_t* pByte = reinterpret_cast<uint8_t*>(pMsg);
	uint8_t* pPayload = *reinterpret_cast<uint8_t**>(pByte + 0x1050);
	uint16_t wOpcode = pPayload ? *reinterpret_cast<uint16_t*>(pPayload) : 0;
	uint16_t nRead = *reinterpret_cast<uint16_t*>(pByte + 0x103C);
	uint16_t nTotal = *reinterpret_cast<uint16_t*>(pByte + 0x103E);

	std::snprintf(pszOut, 256, "[Opcode: 0x%04X, Read: %u / Total: %u]", wOpcode, nRead, nTotal);
	return pszOut;
}

} // namespace ServerFramework
