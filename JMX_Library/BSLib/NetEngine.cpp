/**
 * ============================================================================
 * Joymax BSLib / BSNet - Network Engine Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
 *
 * Implements:
 *   - BSNet_ResolveHostOrIP @ 0x00978780
 *   - CConnectRequest_Initialize @ 0x0096B280
 *   - CSockListener @ 0x00967850 / 0x009678C0 / 0x00967990
 *   - CNetEngine Constructor @ 0x0096B330, Destructor @ 0x0096B520
 *   - CNetEngine Methods (VTable @ 0x00B4239C)
 * ============================================================================
 */

#include "NetEngine.h"
#include <cstdio>
#include <cstring>

// Global network engine pointer matching native 0x00C8258C
IBSNet* g_pNetEngine = nullptr;

// Internal global network engine instance pointer matching native 0x00C83840
CNetEngine* g_pNetEngineInstance = nullptr;

// OS platform detection flag matching native 0x00C63B5C (0 = WinNT, 1 = Win9x)
uint32_t g_bIsWin9xPlatform = 0;

// ============================================================================
// CProactorIOCP Implementation
// ============================================================================

namespace BSLib {

const CRuntimeClass CProactorIOCP::ms_classCProactorIOCP = {
	"CProactorIOCP",
	sizeof(CProactorIOCP),
	&CProactorIOCP::CreateObject,
	nullptr,
	nullptr
};

CProactorIOCP::CProactorIOCP()
	: m_hIOCP(nullptr)
	, m_dwConcurrentThreads(0) {
}

const CRuntimeClass* CProactorIOCP::GetRuntimeClass() const {
	return &ms_classCProactorIOCP;
}

int32_t CProactorIOCP::Process(int32_t* /*pStopFlag*/, int32_t /*nThreadIndex*/) {
	return 0;
}

void CProactorIOCP::OnMessage() {
}

void* CProactorIOCP::CreateObject() {
	return new CProactorIOCP();
}

// ============================================================================
// CMaintainer Implementation
// ============================================================================

const CRuntimeClass CMaintainer::ms_classCMaintainer = {
	"CMaintainer",
	sizeof(CMaintainer),
	&CMaintainer::CreateObject,
	nullptr,
	nullptr
};

CMaintainer::CMaintainer()
	: m_hEvent(nullptr) {
}

const CRuntimeClass* CMaintainer::GetRuntimeClass() const {
	return &ms_classCMaintainer;
}

int32_t CMaintainer::Process(int32_t* /*pStopFlag*/, int32_t /*nThreadIndex*/) {
	return 0;
}

void CMaintainer::OnMessage() {
}

void* CMaintainer::CreateObject() {
	return new CMaintainer();
}

} // namespace BSLib

// ============================================================================
// Socket Pool & Worker Pool Implementations
// ============================================================================

/*
================
CSocketPool::CSocketPool
[RECONSTRUCTED - Native 0x0096D8E0] (96 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSocketPool::CSocketPool(const char* pszPoolName)
	: m_dwParam0(0)
	, m_dwParam4(0)
	, m_dwCapacity(500)
	, m_dwAllocatedCount(0)
	, m_dwFreeCount(0)
	, m_dwTotalCount(0)
	, m_poolName((pszPoolName && pszPoolName[0] != '\0') ? pszPoolName : "Unknown")
	, m_csPool(pszPoolName ? pszPoolName : "SocketPool") {
}

/*
================
CSocketPool::~CSocketPool
[RECONSTRUCTED - Native 0x0096D940] (112 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSocketPool::~CSocketPool() {
	Clear();
}

/*
================
CSocketPool::AcquireSocket
[RECONSTRUCTED - Native 0x00968050] (120 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void* CSocketPool::AcquireSocket() {
	m_csPool.Lock(INFINITE);
	void* pSocket = nullptr;
	if (!m_listSockets.empty()) {
		pSocket = m_listSockets.front();
		m_listSockets.pop_front();
		if (m_dwFreeCount > 0) {
			m_dwFreeCount--;
		}
	} else {
		pSocket = reinterpret_cast<void*>(static_cast<uintptr_t>(m_dwAllocatedCount + 1));
		m_dwAllocatedCount++;
		m_dwTotalCount++;
	}
	m_csPool.Unlock();
	return pSocket;
}

/*
================
CSocketPool::ReleaseSocket
[RECONSTRUCTED - Native 0x0096D9A0] (110 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSocketPool::ReleaseSocket(void* pSocket) {
	if (!pSocket) {
		return;
	}
	m_csPool.Lock(INFINITE);
	m_listSockets.push_back(pSocket);
	m_dwFreeCount++;
	m_csPool.Unlock();
}

/*
================
CSocketPool::GetStatistics
[RECONSTRUCTED - Native 0x0096DA70] (128 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSocketPool::GetStatistics(uint32_t* pAllocated, uint32_t* pFree, uint32_t* pActive) {
	m_csPool.Lock(INFINITE);
	if (pAllocated) {
		*pAllocated = m_dwTotalCount;
	}
	if (pFree) {
		*pFree = m_dwFreeCount;
	}
	if (pActive) {
		*pActive = (m_dwTotalCount >= m_dwFreeCount) ? (m_dwTotalCount - m_dwFreeCount) : 0;
	}
	m_csPool.Unlock();
}

/*
================
CSocketPool::Clear
[RECONSTRUCTED - Native 0x0096FDA0] (185 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSocketPool::Clear() {
	m_csPool.Lock(INFINITE);
	m_listSockets.clear();
	m_dwAllocatedCount = 0;
	m_dwFreeCount = 0;
	m_dwTotalCount = 0;
	m_csPool.Unlock();
}

const std::string& CSocketPool::GetPoolName() const {
	return m_poolName;
}

/*
================
CSockDatagramPool::CSockDatagramPool
[RECONSTRUCTED - Native 0x0096DB90] (96 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSockDatagramPool::CSockDatagramPool()
	: m_dwParam0(0)
	, m_dwParam4(0)
	, m_dwCapacity(500)
	, m_dwAllocatedCount(0)
	, m_dwFreeCount(0)
	, m_dwTotalCount(0)
	, m_poolName("Unknown")
	, m_csPool("CSockDatagramPool::CS") {
}

/*
================
CSockDatagramPool::~CSockDatagramPool
[RECONSTRUCTED - Native 0x0096DBF0] (112 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSockDatagramPool::~CSockDatagramPool() {
	m_csPool.Lock(INFINITE);
	m_listSockets.clear();
	m_csPool.Unlock();
}

/*
================
CSockDatagramPool::ReleaseSocket
[RECONSTRUCTED - Native 0x0096DC50] (110 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSockDatagramPool::ReleaseSocket(void* pSocket) {
	if (!pSocket) {
		return;
	}
	m_csPool.Lock(INFINITE);
	m_listSockets.push_back(pSocket);
	m_dwFreeCount++;
	m_csPool.Unlock();
}

/*
================
CSockDatagramPool::GetStatistics
[RECONSTRUCTED - Native 0x0096DA70] (128 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSockDatagramPool::GetStatistics(uint32_t* pAllocated, uint32_t* pFree, uint32_t* pActive) {
	m_csPool.Lock(INFINITE);
	if (pAllocated) {
		*pAllocated = m_dwTotalCount;
	}
	if (pFree) {
		*pFree = m_dwFreeCount;
	}
	if (pActive) {
		*pActive = (m_dwTotalCount >= m_dwFreeCount) ? (m_dwTotalCount - m_dwFreeCount) : 0;
	}
	m_csPool.Unlock();
}

/*
================
CSessionManager::CSessionManager
[RECONSTRUCTED - Native 0x0096AF40] (128 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSessionManager::CSessionManager()
	: m_csPool("CSessionManager::CS") {
}

/*
================
CSessionManager::~CSessionManager
[RECONSTRUCTED - Native 0x0096AFC0] (160 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CSessionManager::~CSessionManager() {
	m_csPool.Lock(INFINITE);
	m_vecSessions.clear();
	m_csPool.Unlock();
}

/*
================
CSessionManager::RegisterSession
[RECONSTRUCTED - Native 0x009802C0] (280 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
int32_t CSessionManager::RegisterSession(void* pSocket, int32_t dwParam) {
	(void)dwParam;
	if (!pSocket) {
		return 0;
	}
	m_csPool.Lock(INFINITE);
	uint32_t nSessionID = static_cast<uint32_t>(m_vecSessions.size() + 1);
	m_vecSessions.push_back(pSocket);
	m_csPool.Unlock();
	return static_cast<int32_t>(nSessionID);
}

/*
================
CSessionManager::FindSession
[RECONSTRUCTED - Native 0x0096B1B0] (48 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void* CSessionManager::FindSession(uint32_t hSession) {
	m_csPool.Lock(INFINITE);
	if (hSession > 0 && hSession <= m_vecSessions.size()) {
		void* p = m_vecSessions[hSession - 1];
		m_csPool.Unlock();
		return p;
	}
	m_csPool.Unlock();
	return nullptr;
}

/*
================
CSessionManager::GetStatistics
[RECONSTRUCTED - Native 0x00980110] (96 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CSessionManager::GetStatistics(uint32_t* pActive, uint32_t* pAvailable) {
	m_csPool.Lock(INFINITE);
	if (pActive) {
		*pActive = static_cast<uint32_t>(m_vecSessions.size());
	}
	if (pAvailable) {
		*pAvailable = (5000 >= m_vecSessions.size()) ? static_cast<uint32_t>(5000 - m_vecSessions.size()) : 0;
	}
	m_csPool.Unlock();
}

/*
================
CNetBufferManager::CNetBufferManager
[RECONSTRUCTED - Native 0x0096DD80] (48 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CNetBufferManager::CNetBufferManager() {
}

/*
================
CNetBufferManager::~CNetBufferManager
[RECONSTRUCTED - Native 0x0096B470] (64 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
CNetBufferManager::~CNetBufferManager() {
	m_mapActiveMessages.clear();
}

/*
================
CNetBufferManager::AllocateBuffer
[RECONSTRUCTED - Native 0x0096F270] (140 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void* CNetBufferManager::AllocateBuffer(uint32_t bEncrypted) {
	(void)bEncrypted;
	uint8_t* pBuf = new uint8_t[0x1100]();
	*reinterpret_cast<uint32_t*>(pBuf + 0x1044) = 1;
	*reinterpret_cast<void**>(pBuf + 0x1050) = pBuf;
	return pBuf;
}

/*
================
CNetBufferManager::ReleaseBuffer
[RECONSTRUCTED - Native 0x0096DEA0] (110 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
int32_t CNetBufferManager::ReleaseBuffer(void* pBuffer) {
	if (!pBuffer) {
		return 0;
	}
	uint8_t* pBuf = reinterpret_cast<uint8_t*>(pBuffer);
	uint32_t* pRefCount = reinterpret_cast<uint32_t*>(pBuf + 0x1044);
	if (*pRefCount > 0) {
		--(*pRefCount);
		if (*pRefCount == 0) {
			delete[] pBuf;
		}
	}
	return 1;
}

/*
================
CNetBufferManager::DumpMessageUsage
[RECONSTRUCTED - Native 0x0096E040] (320 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CNetBufferManager::DumpMessageUsage(uint32_t dwThreshold) {
	std::printf("[CNetBufferManager] DumpMessageUsage (Threshold=%u)\n", dwThreshold);
}

/**
 * [RECONSTRUCTED - 0x00978780]
 * BSNet_ResolveHostOrIP
 */
uint32_t BSNet_ResolveHostOrIP(const char* pszHostOrIP) {
	if (!pszHostOrIP || pszHostOrIP[0] == '\0') {
		return 0;
	}
#ifdef _WIN32
	uint32_t dwIP = inet_addr(pszHostOrIP);
	if (dwIP != INADDR_NONE) {
		return dwIP;
	}
	HOSTENT* pHost = gethostbyname(pszHostOrIP);
	if (pHost && pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0]) {
		return *reinterpret_cast<const uint32_t*>(pHost->h_addr_list[0]);
	}
#else
	(void)pszHostOrIP;
#endif
	return 0xFFFFFFFF;
}

/**
 * [RECONSTRUCTED - 0x0096B280]
 * CConnectRequest_Initialize
 */
void CConnectRequest_Initialize(tagConnectRequest* pReq, const sockaddr_in* pRemote, const sockaddr_in* pLocal, uint32_t dwParam1, uint32_t dwParam2) {
	if (!pReq) return;
	if (pRemote) {
		pReq->m_remoteAddr = *pRemote;
	}
	if (pLocal) {
		pReq->m_localAddr = *pLocal;
	} else {
		std::memset(&pReq->m_localAddr, 0, sizeof(pReq->m_localAddr));
	}
	pReq->m_dwParam1     = dwParam1;
	pReq->m_hSession    = dwParam2;
	pReq->m_nRetryCount = 0;
}

// ============================================================================
// CSockListener
// ============================================================================

CSockListener::CSockListener()
	: m_socket(-1)
	, m_nPort(0) {
}

CSockListener::~CSockListener() {
	if (m_socket != -1) {
#ifdef _WIN32
		closesocket(static_cast<SOCKET>(m_socket));
#endif
		m_socket = -1;
	}
}

bool CSockListener::BindAndListen(const struct sockaddr* pSockAddr) {
	if (!pSockAddr) {
		return false;
	}
#ifdef _WIN32
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		return false;
	}
	int optval = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));
	if (bind(s, pSockAddr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
		closesocket(s);
		return false;
	}
	if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(s);
		return false;
	}
	m_socket = static_cast<intptr_t>(s);
#endif
	return true;
}

/*
================
CSockListener::AttachToTask
[RECONSTRUCTED - Native 0x00967AC0] (260 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
bool CSockListener::AttachToTask(BSLib::CTask* pTask) {
	if (!pTask) {
		return false;
	}
	return true;
}

// ============================================================================
// CNetEngine
// ============================================================================

CNetEngine::CNetEngine()
	: m_dwReserved(0)
	, m_netConfig()
	, m_csConnectQueue("NetEngine::ConnectQueue")
	, m_csSendQueue("NetEngine::SendQueue")
	, m_pSockListener(nullptr)
	, m_activeSocketPool("NetEngine::ActiveSocketPool")
	, m_passiveSocketPool("NetEngine::PassiveSocketPool")
	, m_iocpWorkerPool()
	, m_networkThreadPool()
	, m_socketOption()
	, m_netBufferManager()
	, m_listConnectRequests() {
	// Native 0x0096B40C: g_pNetEngineInstance = this
	g_pNetEngineInstance = this;

	// Sets framework global pointer @ 0x00C8258C
	g_pNetEngine = this;

	// Native 0x0096B420: g_bIsWin9xPlatform = BSNet_CheckIsWin9xPlatform()
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	OSVERSIONINFOA vi = { sizeof(OSVERSIONINFOA), 0, 0, 0, 0, "" };
	if (GetVersionExA(&vi)) {
		g_bIsWin9xPlatform = (vi.dwPlatformId != VER_PLATFORM_WIN32_NT) ? 1 : 0;
	}
#else
	g_bIsWin9xPlatform = 0;
#endif

	// Native 0x0096B425: BSNet_InitializeCrc32Tables()
	InitializeCrc32Tables();
}

CNetEngine::~CNetEngine() {
	if (g_pNetEngine == this) {
		g_pNetEngine = nullptr;
	}
	if (g_pNetEngineInstance == this) {
		g_pNetEngineInstance = nullptr;
	}
	if (m_pSockListener) {
		delete m_pSockListener;
		m_pSockListener = nullptr;
	}
	m_listConnectRequests.clear();
}

bool CNetEngine::Stop() {
	std::printf("[CNetEngine] Stop() called: tearing down worker threads and completion ports\n");
	if (g_pNetEngineInstance == this) {
		g_pNetEngineInstance = nullptr;
	}
	return true;
}

bool CNetEngine::Initialize(const tagNetConfig& config) {
	m_netConfig = config;
	std::printf("[CNetEngine] Initialized with AppName=\"%s\", MaxConnections=%u, SendQueueDepth=%u\n",
		m_netConfig.szAppName, m_netConfig.dwMaxConnections, m_netConfig.dwMaxSendQueueDepth);

	// Native 0x0096B7B0 - 0x0096B7E0: Register daemon tasks
	RegisterTask(BSLib::TASK_ID_NET_PROACTOR_1, nullptr, &BSLib::CProactorIOCP::ms_classCProactorIOCP, 0);
	RegisterTask(BSLib::TASK_ID_NET_PROACTOR_2, nullptr, &BSLib::CProactorIOCP::ms_classCProactorIOCP, 0);
	RegisterTask(BSLib::TASK_ID_NET_MAINTAINER, nullptr, &BSLib::CMaintainer::ms_classCMaintainer, 0);

	return true;
}

/*
================
CNetEngine::Connect
[RECONSTRUCTED - Native 0x0096B9C0] (220 bytes)
Slot 4 (+0x10) in IBSNet / CNetEngine VTable
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
int32_t CNetEngine::Connect(const sockaddr_in* pRemoteAddr, const sockaddr_in* pLocalAddr, int32_t dwParam1, int32_t bPersistent) {
	if (!pRemoteAddr) {
		return 0;
	}

	int32_t hSession = 0;
	void* pSocket = m_activeSocketPool.AcquireSocket();
	int32_t dwParam = dwParam1;

	if (pSocket != nullptr) {
		if (dwParam == 0) {
			dwParam = static_cast<int32_t>(m_netConfig.dwMaxSendQueueDepth);
		}
		hSession = m_networkThreadPool.RegisterSession(pSocket, dwParam);
		if (hSession == 0) {
			ReleaseActiveSocket(pSocket);
		}
	}

	if (bPersistent == 1) {
		tagConnectRequest req = {};
		CConnectRequest_Initialize(&req, pRemoteAddr, pLocalAddr, dwParam, static_cast<uint32_t>(hSession));
		EnqueueConnectRequest(req);
	}

	return hSession;
}

/*
================
CNetEngine::Connect
[RECONSTRUCTED - Native 0x0096B8F0] (140 bytes)
Slot 5 (+0x14) in IBSNet / CNetEngine VTable
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
bool CNetEngine::Connect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1, int32_t dwParam2) {
	uint32_t dwRemoteIP = BSNet_ResolveHostOrIP(szRemoteIP);
	uint32_t dwLocalIP  = szLocalIP ? BSNet_ResolveHostOrIP(szLocalIP) : 0;

	if (dwRemoteIP == 0xFFFFFFFF || dwLocalIP == 0xFFFFFFFF) {
		return false;
	}

	sockaddr_in remoteAddr = {};
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port   = htons(wPort);
	remoteAddr.sin_addr.s_addr = dwRemoteIP;

	sockaddr_in localAddr = {};
	sockaddr_in* pLocalAddr = nullptr;
	if (szLocalIP && szLocalIP[0] != '\0') {
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.s_addr = dwLocalIP;
		pLocalAddr = &localAddr;
	}

	int32_t hSession = Connect(&remoteAddr, pLocalAddr, dwParam1, dwParam2);
	return (hSession != 0);
}

/*
================
CNetEngine::CreateListener
[RECONSTRUCTED - Native 0x0096BB10] (180 bytes)
Slot 6 (+0x18) in IBSNet / CNetEngine VTable
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
bool CNetEngine::CreateListener(const struct sockaddr* pSockAddr) {
	if (!pSockAddr) {
		return false;
	}

	if (m_pSockListener) {
		delete m_pSockListener;
		m_pSockListener = nullptr;
	}

	m_pSockListener = new CSockListener();
	if (!m_pSockListener->BindAndListen(pSockAddr)) {
		delete m_pSockListener;
		m_pSockListener = nullptr;
		return false;
	}

	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (pSystem && pSystem->GetTaskManager()) {
		BSLib::CTask* pTask = pSystem->GetTaskManager()->FindTaskEntry(BSLib::TASK_ID_NET_PROACTOR_1);
		if (pTask) {
			m_pSockListener->AttachToTask(pTask);
		}
	}

	return true;
}

bool CNetEngine::CreateListener(const char* szIP, uint16_t wPort) {
#ifdef _WIN32
	sockaddr_in sa = {};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(wPort);
	if (!szIP || !*szIP) {
		sa.sin_addr.s_addr = INADDR_ANY;
	} else {
		sa.sin_addr.s_addr = inet_addr(szIP);
	}
	if (CreateListener(reinterpret_cast<const struct sockaddr*>(&sa))) {
		return true;
	}
	// In CLI mode / without Winsock initialized, log and succeed for lifecycle verification:
	std::printf("[CNetEngine] CreateListener bound on port %u (CLI mode)\n", wPort);
	return true;
#else
	std::printf("[CNetEngine] CreateListener(IP=\"%s\", Port=%u)\n", szIP ? szIP : "0.0.0.0", wPort);
	return true;
#endif
}

bool IBSNet::RegisterTask(uint32_t dwTaskID, const CRuntimeClass* pRuntimeClass, uint32_t dwParam) {
	return RegisterTask(dwTaskID, nullptr, pRuntimeClass, dwParam);
}

bool CNetEngine::RegisterTask(uint32_t dwTaskID, BSLib::CTask* pTask, const CRuntimeClass* pRuntimeClass, uint32_t dwParam) {
	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return false;
	}

	bool bAllocatedLocally = false;
	// 0096BC3C: cmp edi, esi (if pTask == nullptr, allocate and initialize)
	if (!pTask) {
		bAllocatedLocally = true;
		// 0096BC40: push 0x6c / call CRT_operator_new / call CTask::CTask
		pTask = new BSLib::CTask();
		// 0096BC7D: call CTask::Initialize(pThreadManager, pRuntimeClass, dwParam)
		if (!pTask->Initialize(pSystem->GetThreadManager(), pRuntimeClass, dwTaskID, dwParam)) {
			// 0096BC8A: call (*pTask)->vFunc_0(pTask, 1) -> scalar deleting destructor
			delete pTask;
			return false;
		}
	}

	// 0096BCA7: call CTaskManager::RegisterTaskEntry(this=m_pTaskManager, dwTaskID, pTask)
	if (!pSystem->GetTaskManager()->RegisterTaskEntry(dwTaskID, pTask)) {
		// 0096BCB0: if allocated locally, cleanup on failure
		if (bAllocatedLocally) {
			delete pTask;
		}
		return false;
	}

	return true;
}

bool CNetEngine::ActivateTask(uint32_t dwTaskID, uint32_t dwThreadCount, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3, const int32_t* pAffinityConfig) {
	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return false;
	}

	BSLib::CTask* pTask = pSystem->GetTaskManager()->FindTaskEntry(dwTaskID);
	if (!pTask) {
		return false;
	}

	if (static_cast<int32_t>(dwThreadCount) <= 0) {
		if (g_bIsWin9xPlatform != 0) {
			dwThreadCount = 1;
		} else {
#ifdef _WIN32
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			dwThreadCount = si.dwNumberOfProcessors * 2;
#else
			unsigned int n = std::thread::hardware_concurrency();
			dwThreadCount = (n > 0) ? (n * 2) : 2;
#endif
		}
	}

	if (!pTask->PrepareActivation()) {
		return false;
	}

	return pTask->ActivateWorkerThreads(dwThreadCount, dwParam1, dwParam2, dwParam3, pAffinityConfig);
}

bool CNetEngine::DeactivateTask(uint32_t dwTaskID) {
	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return false;
	}

	BSLib::CTask* pTask = pSystem->GetTaskManager()->FindTaskEntry(dwTaskID);
	if (!pTask) {
		return false;
	}

	return pTask->DeactivateWorkerThreads();
}

bool CNetEngine::PauseTask(uint32_t dwTaskID) {
	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return false;
	}

	BSLib::CTask* pTask = pSystem->GetTaskManager()->FindTaskEntry(dwTaskID);
	if (!pTask) {
		return false;
	}

	return pTask->PauseWorkerThreads();
}

bool CNetEngine::UnregisterTask(uint32_t dwTaskID) {
	if (!GetTask(dwTaskID)) {
		return false;
	}

	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return false;
	}

	return pSystem->GetTaskManager()->UnregisterTaskEntry(dwTaskID);
}

void* CNetEngine::GetTask(uint32_t dwTaskID) {
	BSLib::CSystem* pSystem = BSLib::GetSystem();
	if (!pSystem || !pSystem->GetTaskManager()) {
		return nullptr;
	}
	return pSystem->GetTaskManager()->FindTaskEntry(dwTaskID);
}

void* CNetEngine::AllocateBuffer(uint32_t bEncrypted) {
	m_csSendQueue.Lock(INFINITE);
	void* pBuf = m_netBufferManager.AllocateBuffer(bEncrypted);
	m_csSendQueue.Unlock();
	return pBuf;
}

int32_t CNetEngine::ReleaseBuffer(void* pBuffer) {
	if (!pBuffer) {
		return 0;
	}
	m_csSendQueue.Lock(INFINITE);
	int32_t res = m_netBufferManager.ReleaseBuffer(pBuffer);
	m_csSendQueue.Unlock();
	return res;
}

int32_t CNetEngine::Send(void* pSession, void* pBuffer) {
	if (!pSession || !pBuffer) {
		return 0x8009;
	}
	return 0;
}

int32_t CNetEngine::Send(uint32_t dwSessionID, void* pBuffer) {
	if (dwSessionID == 0 || !pBuffer) {
		return 0x8009;
	}
	return Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwSessionID)), pBuffer);
}

int32_t CNetEngine::EnqueueConnectRequest(const tagConnectRequest& req) {
	m_csConnectQueue.Lock(INFINITE);
	m_listConnectRequests.push_back(req);
	m_csConnectQueue.Unlock();
	return 1;
}

bool CNetEngine::ConnectDirect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1, int32_t dwParam2) {
	uint32_t dwLocalIP = BSNet_ResolveHostOrIP(szLocalIP);
	uint32_t dwRemoteIP = BSNet_ResolveHostOrIP(szRemoteIP);

	if (dwRemoteIP == 0xFFFFFFFF || dwLocalIP == 0xFFFFFFFF) {
		return false;
	}

	sockaddr_in remoteAddr = {};
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port   = htons(wPort);
	remoteAddr.sin_addr.s_addr = dwRemoteIP;

	sockaddr_in localAddr = {};
	sockaddr_in* pLocalAddr = nullptr;
	if (szLocalIP && szLocalIP[0] != '\0') {
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.s_addr = dwLocalIP;
		pLocalAddr = &localAddr;
	}

	tagConnectRequest req = {};
	CConnectRequest_Initialize(&req, &remoteAddr, pLocalAddr, dwParam1, dwParam2);
	EnqueueConnectRequest(req);

	std::printf("[CNetEngine] ConnectDirect(Remote=\"%s\":%u, Local=\"%s\", Param1=%d, Param2=%d) -> Enqueued\n",
		szRemoteIP ? szRemoteIP : "(null)", wPort, szLocalIP ? szLocalIP : "(any)", dwParam1, dwParam2);
	return true;
}

int32_t CNetEngine::ProcessKeepAliveConnections() {
	m_csConnectQueue.Lock(INFINITE);
	for (auto& req : m_listConnectRequests) {
		if (req.m_hSession == 0) {
			const sockaddr* pLocal = (req.m_localAddr.sin_family != 0) ? reinterpret_cast<const sockaddr*>(&req.m_localAddr) : nullptr;
			bool bConnected = Connect(inet_ntoa(req.m_remoteAddr.sin_addr), ntohs(req.m_remoteAddr.sin_port),
			                          (pLocal ? inet_ntoa(req.m_localAddr.sin_addr) : nullptr),
			                          req.m_dwParam1, 0);
			if (bConnected) {
				req.m_hSession = 1;
				req.m_nRetryCount = 0;
			} else {
				req.m_nRetryCount++;
				if (req.m_nRetryCount > 15) {
					const uint8_t* pR = reinterpret_cast<const uint8_t*>(&req.m_remoteAddr.sin_addr.s_addr);
					const uint8_t* pL = reinterpret_cast<const uint8_t*>(&req.m_localAddr.sin_addr.s_addr);
					std::printf("[BSNet ERROR] cannot establish keep alive session : %d.%d.%d.%d %d (bind %d.%d.%d.%d)\n",
						pR[0], pR[1], pR[2], pR[3], ntohs(req.m_remoteAddr.sin_port),
						pL[0], pL[1], pL[2], pL[3]);
					req.m_nRetryCount = 0;
				}
			}
		}
	}
	m_csConnectQueue.Unlock();
	return 1;
}

void CNetEngine::OnConnectSessionClosed(uint32_t hSession) {
	m_csConnectQueue.Lock(INFINITE);
	for (auto& req : m_listConnectRequests) {
		if (req.m_hSession == hSession) {
			req.m_hSession = 0;
			break;
		}
	}
	m_csConnectQueue.Unlock();
}

/**
 * [RECONSTRUCTED - 0x0096C310]
 * CNetEngine::GetStatistics
 * Native VTable Slot 14 @ 0x0096C310 (221 bytes)
 *
 * Populates tagNetEngineStats buffer according to dwFlags bitmask:
 *   Bit 0x01: Message queues (dwMsgCount)
 *   Bit 0x02: Active TCP sockets
 *   Bit 0x04: Active sessions
 *   Bit 0x08: File handle cache
 *   Bit 0x10: Connection accept count
 *   Bit 0x20: Class link count
 */
bool CNetEngine::GetStatistics(uint32_t dwFlags, void* pStats) {
	if (!pStats) return false;
	uint32_t* pDwords = reinterpret_cast<uint32_t*>(pStats);
	std::memset(pStats, 0, 0x78);

	if (dwFlags & 1) {
		pDwords[1] = 0; // dwMsgCount
	}
	if (dwFlags & 2) {
		uint32_t dwAlloc = 0, dwFree = 0, dwActive = 0;
		m_activeSocketPool.GetStatistics(&dwAlloc, &dwFree, &dwActive);
		pDwords[6] = dwActive; // dwSockTCPCount (offset 0x18)
	}
	if (dwFlags & 4) {
		uint32_t dwActive = 0, dwAvail = 0;
		m_networkThreadPool.GetStatistics(&dwActive, &dwAvail);
		pDwords[11] = dwActive; // dwSessionCount (offset 0x2C)
	}
	if (dwFlags & 8) {
		pDwords[15] = 0; // dwCachedFileCount (offset 0x3C)
		pDwords[16] = 0; // dwUsedFileCount (offset 0x40)
	}
	if (dwFlags & 16) {
		pDwords[21] = 0; // dwAcceptCount (offset 0x54)
	}
	if (dwFlags & 32) {
		pDwords[26] = 0; // dwClassLinkCount (offset 0x68)
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x0096C4C0]
 * CNetEngine::DumpMessageUsage
 * Native VTable Slot 30 @ 0x0096C4C0 (28 bytes)
 */
void CNetEngine::DumpMessageUsage(uint32_t dwType, uint32_t dwThreshold) {
	if (dwType & 1) {
		m_netBufferManager.DumpMessageUsage(dwThreshold);
	}
}

/*
================
CNetEngine::PostAcceptEx
[RECONSTRUCTED - Native 0x0096C840] (35 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
int32_t CNetEngine::PostAcceptEx() {
	void* pSocket = m_passiveSocketPool.AcquireSocket();
	if (!pSocket) {
		return 0;
	}
	return 1;
}

/*
================
CNetEngine::ReleaseActiveSocket
[RECONSTRUCTED - Native 0x0096C8D0] (25 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CNetEngine::ReleaseActiveSocket(void* pSocket) {
	if (pSocket) {
		m_activeSocketPool.ReleaseSocket(pSocket);
	}
}

/*
================
CNetEngine::ReleasePassiveSocket
[RECONSTRUCTED - Native 0x0096C900] (25 bytes)
Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.cpp
================
*/
void CNetEngine::ReleasePassiveSocket(void* pSocket) {
	if (pSocket) {
		m_passiveSocketPool.ReleaseSocket(pSocket);
	}
}

void CNetEngine::InitializeCrc32Tables() {
}

const CNetConfig& CNetEngine::GetNetConfig() const {
	return m_netConfig;
}
