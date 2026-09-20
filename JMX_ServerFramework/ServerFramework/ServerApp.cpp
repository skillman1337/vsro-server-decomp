/**
 * ============================================================================
 * Joymax ServerFramework - CServerApp Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerApp.cpp
 *
 * Implements the lifecycle of ServerFramework::CServerApp:
 *   - Native Ctor @ 0x009355B0
 *   - Native Dtor @ 0x00935670 & Scalar Dtor @ 0x00935650
 *   - Native InitModule @ 0x009356F0
 *   - Native StartServerTasks @ 0x009358E0
 *   - Native Initialize @ 0x00935A00
 *   - Native SetServerState @ 0x00935D70
 * ============================================================================
 */

#include "ServerApp.h"
#include "ServerMain.h"
#include "ServerTopology.h"
#include "ServerProcessMain.h"
#include "ServerProcessOverlap.h"
#include "ServerPerfMonitor.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/FtpManager.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#else
#include <chrono>
#include <thread>
inline uint32_t GetTickCount() {
	using namespace std::chrono;
	return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
inline void Sleep(uint32_t ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif

namespace ServerFramework {

/**
 * [RECONSTRUCTED - 0x009354E0]
 * ServerFramework_WriteFatalLogFile
 * Native implementation @ 0x009354E0 (205 bytes)
 * Calling convention: __regparm (esi = pszMessage)
 *
 * Appends a timestamped fatal log entry to:
 *   "%s\%04d-%02d-%02d_FatalLog.txt"
 * Format:
 *   "%04d-%02d-%02d\t%02d:%02d:%02d\t[%s]\t%s\n"
 */
int ServerFramework_WriteFatalLogFile(const char* pszMessage) {
	if (pszMessage == nullptr) {
		return 0;
	}

	char szFilePath[260];
#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime(&st);
	std::snprintf(szFilePath, sizeof(szFilePath), "%s\\%04d-%02d-%02d_FatalLog.txt",
		g_szAppDirectory, st.wYear, st.wMonth, st.wDay);

	FILE* fp = std::fopen(szFilePath, "a");
	if (fp != nullptr) {
		std::fprintf(fp, "%04d-%02d-%02d\t%02d:%02d:%02d\t[%s]\t%s\n",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond,
			g_szAppName,
			pszMessage);
		std::fclose(fp);
	}
#else
	time_t t = time(nullptr);
	tm* ptm = localtime(&t);
	std::snprintf(szFilePath, sizeof(szFilePath), "%s/%04d-%02d-%02d_FatalLog.txt",
		g_szAppDirectory, ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);

	FILE* fp = std::fopen(szFilePath, "a");
	if (fp != nullptr) {
		std::fprintf(fp, "%04d-%02d-%02d\t%02d:%02d:%02d\t[%s]\t%s\n",
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
			g_szAppName,
			pszMessage);
		std::fclose(fp);
	}
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00964B60]
 * ServerFramework_GenerateMiniDump
 * Native implementation @ 0x00964B60
 */
int ServerFramework_GenerateMiniDump() {
	return ::BSLib::GenerateMiniDump();
}

/**
 * CServerApp::CServerApp
 * Native Constructor @ 0x009355B0
 */
CServerApp::CServerApp()
	: m_pLogCallback(nullptr)
	, m_strAppName()
	, m_dwFlags(0)
	, m_netConfig()
	, m_nServerState(SERVER_STATE_READY)
	, m_netEngine() {
	// Native Ctor @ 0x009355B0:
	// Compiler sets vptr = 0x00B3FE2C, constructs m_strAppName, m_netConfig,
	// initializes m_nServerState = 1 (SERVER_STATE_READY @ 0x00935624), and constructs m_netEngine.
}

/**
 * CServerApp::~CServerApp
 * Native Destructor @ 0x00935670 & Scalar Dtor @ 0x00935650
 *
 * Automatically calls ~CNetEngine() @ 0x0096B520 and inlines ~std::string() in reverse order.
 */
CServerApp::~CServerApp() {
}

/**
 * CServerApp::InitModule
 * Native implementation @ 0x009356F0 (slot 1 @ +0x04)
 *
 * 009356f0  push    esi
 * 009356f1  push    0xb3fc50 ; "initialize module"
 * 009356f6  push    0
 * 009356f8  mov     esi, ecx
 * 009356fa  call    Log_Printf ; Log_Printf(0, "initialize module")
 * 009356ff  mov     eax, [esi]
 * 00935701  mov     edx, [eax+0x4c] ; slot 19 @ +0x4C (SetServerState)
 * 00935704  add     esp, 8
 * 00935707  push    2 ; SERVER_STATE_INITIALIZING
 * 00935709  mov     ecx, esi
 * 0093570b  call    edx ; this->SetServerState(SERVER_STATE_INITIALIZING)
 * 0093570d  mov     al, 1
 * 0093570f  pop     esi
 * 00935710  ret
 */
bool CServerApp::InitModule() {
	BSLib::Log_Printf(0, "initialize module");
	SetServerState(SERVER_STATE_INITIALIZING);
	return true;
}

/**
 * [FOLDED DEFAULT HOOK - 0x00455EB0]
 * CServerApp::InitInstance
 * Native implementation: slot 2 @ 0x00455EB0 (ServerFramework_DefaultTrueStub)
 *
 * Machine Bytes: b0 01 c3 (mov al, 1; retn)
 * MSVC /OPT:ICF COMDAT-folded virtual hook returning true.
 * Default virtual implementation returns true.
 * Overridden by CGameServer::InitInstance @ 0x00401130 to allocate CDisplayWindow.
 */
bool CServerApp::InitInstance() {
	return true;
}

/**
 * [RECONSTRUCTED - 0x00935720]
 * CServerApp_NetEngineMessageCallback
 * Native implementation @ 0x00935720 (26 bytes)
 *
 * Callback passed to m_netConfig.dwTraceCallback @ 0x00935808.
 * Logs message occurrence count to channel 0x02000001.
 */
int CServerApp_NetEngineMessageCallback(uint32_t dwMsgID, int32_t nCount) {
	return BSLib::Log_Printf(0x2000001, "[MsgID: 0x%X >> Count: %d]", dwMsgID, nCount);
}

/**
 * CServerApp::ConfigureNetwork
 * Native implementation @ 0x00935740 (slot 3 @ +0x0C)
 *
 * 00935740  push    esi
 * 00935741  mov     esi, ecx
 * 00935743  lea     eax, [esi+0x28]
 * 00935746  call    CNetConfig_ConfigureServerDefaults ; 0x00935dd0
 * 0093574b  mov     eax, 0x1
 * 00935750  mov     dword [esi+0x38], 0x0
 * 00935757  mov     dword [esi+0x3c], eax
 * 0093575a  mov     dword [esi+0x40], eax
 * 0093575d  pop     esi
 * 0093575e  retn
 */
bool CServerApp::ConfigureNetwork() {
	CNetConfig_ConfigureServerDefaults(&m_netConfig);
	m_netConfig.dwReserved1 = 0;
	m_netConfig.dwMaxConnections = 1;
	m_netConfig.dwMaxPacketSize = 1;
	return true;
}

/**
 * [FOLDED DEFAULT HOOK - 0x0066B100]
 * CServerApp::PostConfigureNetwork
 * Native implementation: slot 4 @ 0x0066B100 (Trace_RuntimeClassStub)
 *
 * Machine Byte: c3 (retn)
 * Default virtual hook called between network config parameter setup and
 * CNetEngine initialization. Empty in CServerApp and CGameServer.
 */
void CServerApp::PostConfigureNetwork() {
	// Native stub @ 0x0066B100 (retn)
}

/**
 * CServerApp::PreInitialize
 * Native implementation @ 0x00935760 (slot 5 @ +0x14)
 * Size: 384 bytes (0x00935760 - 0x009358DF)
 *
 * 1. Binds g_pNetEngine = &this->m_netEngine (+0x140)
 * 2. Polymorphic dispatch: this->ConfigureNetwork() (slot 3 @ +0x0C)
 * 3. Populates m_netConfig daemon parameters:
 *      m_netConfig.dwMainTaskID_1 = g_dwTaskID_ProcessMain (0x01000000)
 *      m_netConfig.dwMainTaskID_2 = g_dwTaskID_ProcessMain (0x01000000)
 *      m_netConfig.dwFlag18 = 1
 *      m_netConfig.dwFlag9 = 1
 *      m_netConfig.dwMaxSendQueueDepth = g_dwMaxSendQueueDepth
 *      m_netConfig.keepAlive.dwReserved1 = 5
 *      m_netConfig.keepAlive.dwPingInterval = 1000
 *      m_netConfig.keepAlive.dwTimeout = 10000
 *      m_netConfig.keepAlive.dwMaxRetries = 1000
 *      m_netConfig.dwDebugOptionDebuggerPresent = g_dwDebugOptionDebuggerPresent
 *      m_netConfig.dwDebugOptionStandAlone = g_dwDebugOptionStandAlone
 *      m_netConfig.dwLogCallback = (uint32_t)&BSLib::Log_Printf
 *      strcpy_s(m_netConfig.szAppName, sizeof(m_netConfig.szAppName), g_szAppName)
 * 4. Polymorphic dispatch: this->PostConfigureNetwork() (slot 4 @ +0x10)
 * 5. Sets m_netConfig.dwTraceCallback = (uint32_t)&CServerApp_NetEngineMessageCallback (0x00935720)
 * 6. Invokes g_pNetEngine->Initialize(m_netConfig) (IBSNet slot 3 @ +0x0C)
 * 7. Computes CPU affinity array:
 *      affinityConfig[0] = 0
 *      affinityConfig[1] = (g_dwNumberOfProcessors > 1) ? 1 : (g_dwNumberOfProcessors - 1)
 *      affinityConfig[2] = (g_dwNumberOfProcessors > 1) ? 1 : (g_dwNumberOfProcessors - 1)
 * 8. Activates 3 core network tasks via g_pNetEngine->ActivateTask (IBSNet slot 10 @ +0x28):
 *      - TASK_ID_NET_PROACTOR_1 (0x10000001, param 1)
 *      - TASK_ID_NET_PROACTOR_2 (0x10000002, param 0)
 *      - TASK_ID_NET_MAINTAINER (0x10000003, param 1)
 */
bool CServerApp::PreInitialize() {
	g_pNetEngine = &m_netEngine;

	// 00935777: call dword [edx+0xc] -> this->ConfigureNetwork()
	ConfigureNetwork();

	// 00935779 - 009357eb: Setup m_netConfig daemon parameters
	m_netConfig.dwMainTaskID_1 = TASK_ID_PROCESS_MAIN; // 0x01000000
	m_netConfig.dwMainTaskID_2 = TASK_ID_PROCESS_MAIN; // 0x01000000
	m_netConfig.dwFlag18 = 1;
	m_netConfig.dwFlag9 = 1;
	m_netConfig.dwMaxSendQueueDepth = g_dwMaxSendQueueDepth;

	m_netConfig.keepAlive.dwReserved1 = 5;
	m_netConfig.keepAlive.dwPingInterval = 1000;
	m_netConfig.keepAlive.dwTimeout = 10000;
	m_netConfig.keepAlive.dwMaxRetries = 1000;

	m_netConfig.dwDebugOptionDebuggerPresent = g_dwDebugOptionDebuggerPresent;
	m_netConfig.dwDebugOptionStandAlone = g_dwDebugOptionStandAlone;

	m_netConfig.dwLogCallback = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&BSLib::Log_Printf));

#ifdef _WIN32
	strcpy_s(m_netConfig.szAppName, sizeof(m_netConfig.szAppName), m_strAppName.empty() ? "SR_GameServer" : m_strAppName.c_str());
#else
	strncpy(m_netConfig.szAppName, m_strAppName.empty() ? "SR_GameServer" : m_strAppName.c_str(), sizeof(m_netConfig.szAppName) - 1);
#endif

	// 00935801: call dword [edx+0x10] -> this->PostConfigureNetwork()
	PostConfigureNetwork();

	// 00935808: mov dword [esi+0x48], 0x935720 -> CServerApp_NetEngineMessageCallback
	m_netConfig.dwTraceCallback = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&CServerApp_NetEngineMessageCallback));

	// 00935811 - 00935827: g_pNetEngine->Initialize(m_netConfig)
	if (!g_pNetEngine->Initialize(m_netConfig)) {
		return false;
	}

	// 00935835 - 00935862: Affinity configuration
	int32_t affinityConfig[3];
	affinityConfig[0] = 0;
	affinityConfig[1] = (g_dwNumberOfProcessors > 1) ? 1 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);
	affinityConfig[2] = (g_dwNumberOfProcessors > 1) ? 1 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);

	// 00935885: ActivateTask(TASK_ID_NET_PROACTOR_1, 1, 0, 0, 0, affinityConfig)
	if (!g_pNetEngine->ActivateTask(BSLib::TASK_ID_NET_PROACTOR_1, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}

	// 009358aa: ActivateTask(TASK_ID_NET_PROACTOR_2, 0, 0, 0, 0, affinityConfig)
	if (!g_pNetEngine->ActivateTask(BSLib::TASK_ID_NET_PROACTOR_2, 0, 0, 0, 0, affinityConfig)) {
		return false;
	}

	// 009358d3: ActivateTask(TASK_ID_NET_MAINTAINER, 1, 0, 0, 0, affinityConfig)
	if (!g_pNetEngine->ActivateTask(BSLib::TASK_ID_NET_MAINTAINER, 1, 0, 0, 0, affinityConfig)) {
		return false;
	}

	return true;
}

/**
 * CServerApp::StartServerTasks
 * Native implementation @ 0x009358E0 (slot 6 @ +0x18)
 */
bool CServerApp::StartServerTasks() {
	if (!g_pNetEngine) {
		std::fprintf(stderr, "FATAL: g_pNetEngine is null in StartServerTasks\n");
		return false;
	}

	// 1. Setup processor affinity:
	// If CPUs > 1, bind to CPU 1. If CPUs <= 1, bind to (CPUs - 1).
	int32_t affinityConfig[3];
	affinityConfig[0] = 0;
	affinityConfig[1] = (g_dwNumberOfProcessors > 1) ? 1 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);
	affinityConfig[2] = (g_dwNumberOfProcessors > 1) ? 1 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);

	// 2. Register CServerProcessOverlap task (Native 0x00935946)
	if (!g_pNetEngine->RegisterTask(TASK_ID_PROCESS_OVERLAP, 0, &CServerProcessOverlap::ms_runtimeClass, 0)) {
		BSLib::ShowErrorMessage("It has been failed to register task");
		return false;
	}

	// 3. Activate CServerProcessOverlap task (Native 0x0093597B)
	if (!g_pNetEngine->ActivateTask(TASK_ID_PROCESS_OVERLAP, 1, 0, 0, 0, affinityConfig)) {
		BSLib::ShowErrorMessage("It has been failed to activate task");
		return false;
	}

	// 4. Virtual dispatch to slot 7 (+0x1C): this->Initialize()
	// (Polymorphic: for CServerApp registers CServerProcessMain; for CGameServer sets up World/Shard)
	if (!Initialize()) {
		BSLib::ShowErrorMessage("It has been failed to initialize server task main");
		return false;
	}

	// 5. Initialize Server FTP Manager (Native 0x009359C5)
	if (!BSLib::InitializeFtpManager(m_strAppName.c_str())) {
		BSLib::ShowErrorMessage("It has been failed to initialize server ftp manager");
		return false;
	}

	BSLib::Log_Printf(0, "successfully server task registered");
	return true;
}

/**
 * CServerApp::Initialize
 * Native implementation @ 0x00935A00 (slot 7 @ +0x1C)
 */
bool CServerApp::Initialize() {
	int32_t affinityConfig[3];
	affinityConfig[0] = 0;
	affinityConfig[1] = (g_dwNumberOfProcessors > 0) ? 0 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);
	affinityConfig[2] = (g_dwNumberOfProcessors > 0) ? 0 : (static_cast<int32_t>(g_dwNumberOfProcessors) - 1);

	// Register primary server process task (Native 0x00935A48)
	if (!g_pNetEngine || !g_pNetEngine->RegisterTask(TASK_ID_PROCESS_MAIN, 0, &CServerProcessMain::ms_runtimeClass, 0)) {
		BSLib::ShowErrorMessage("It has been failed to register task CServerProcessMain");
		return false;
	}

	// Activate primary server process task (Native 0x00935A7F)
	if (!g_pNetEngine->ActivateTask(TASK_ID_PROCESS_MAIN, 1, 0, 0, 0, affinityConfig)) {
		BSLib::ShowErrorMessage("It has been failed to activate task CServerProcessMain");
		return false;
	}

	BSLib::Log_Printf(0, "CServerApp::Initialize() successfully registered and activated CServerProcessMain");
	return true;
}

/**
 * ============================================================================
 * CServerApp::RequestCertification
 * Native implementation @ 0x00935AA0 (slot 8 @ +0x20)
 *
 * [RECONSTRUCTED]:
 * Initiates connection to Certification Server.
 *
 * Assembly trace:
 *   00935aa0  push    esi
 *   00935aa1  push    0xb3fdc4        ; "request server certification"
 *   00935aa6  push    0x0             ; Log level / category 0
 *   00935aa8  mov     esi, ecx        ; esi = this
 *   00935aaa  call    Log_Printf      ; 0x00936640
 *   00935aaf  mov     eax, dword [esi]; this->vptr
 *   00935ab1  mov     edx, dword [eax+0x4c] ; slot 19 @ +0x4c (SetServerState)
 *   00935ab4  add     esp, 0x8
 *   00935ab7  push    0x3             ; SERVER_STATE_CERTIFYING (3)
 *   00935ab9  mov     ecx, esi        ; this
 *   00935abb  call    edx             ; this->SetServerState(SERVER_STATE_CERTIFYING)
 *   00935abd  cmp     dword [0xd67804], 0x0 ; g_bDirectCertification
 *   00935ac4  movzx   edx, word [0xd677d8]  ; g_wCertifyPort
 *   00935acb  mov     eax, dword [0xc8258c] ; g_pNetEngine
 *   00935ad0  mov     ecx, dword [eax]      ; g_pNetEngine->vptr
 *   00935ad2  pop     esi
 *   00935ad3  push    0x0             ; dwParam2 = 0
 *   00935ad5  push    0x0             ; dwParam1 = 0
 *   00935ad7  push    0xd677c8        ; g_szLocalIP
 *   00935adc  push    edx             ; g_wCertifyPort
 *   00935add  push    0xd677b8        ; g_szCertifyIP
 *   00935ae2  push    eax             ; this (g_pNetEngine)
 *   00935ae3  je      0x935aed        ; if (!g_bDirectCertification) goto standard Connect
 *   00935ae5  mov     eax, dword [ecx+0x70] ; slot 28 @ +0x70 (ConnectDirect)
 *   00935ae8  call    eax             ; g_pNetEngine->ConnectDirect(...)
 *   00935aea  mov     al, 0x1         ; return true
 *   00935aec  retn
 *   00935aed  mov     eax, dword [ecx+0x14] ; slot 5 @ +0x14 (Connect)
 *   00935af0  call    eax             ; g_pNetEngine->Connect(...)
 *   00935af2  test    eax, eax
 *   00935af4  jne     0x935aea        ; if (!= 0) return true
 *   00935af6  xor     al, al          ; return false
 *   00935af8  retn
 * ============================================================================
 */
bool CServerApp::RequestCertification() {
	// Native 0x00935AA1 - 0x00935AAA: Log certification request
	BSLib::Log_Printf(0, "request server certification");

	// Native 0x00935AAF - 0x00935ABB:
	// Virtual call to slot 19 (+0x4C): this->SetServerState(SERVER_STATE_CERTIFYING)
	SetServerState(SERVER_STATE_CERTIFYING);

	// Native 0x00935ABD - 0x00935AF8: Connect via network engine
	if (!g_pNetEngine) {
		return false;
	}

	if (g_bDirectCertification != 0) {
		// Native 0x00935AE5: IBSNet::ConnectDirect (slot 28 @ +0x70)
		g_pNetEngine->ConnectDirect(g_szCertifyIP, g_wCertifyPort, g_szLocalIP, 0, 0);
		return true;
	}

	// Native 0x00935AED: IBSNet::Connect (slot 5 @ +0x14)
	return g_pNetEngine->Connect(g_szCertifyIP, g_wCertifyPort, g_szLocalIP, 0, 0);
}

/**
 * [FOLDED DEFAULT HOOK - 0x00455EB0]
 * CServerApp::OnPreServerReady
 * Native implementation @ 0x00455EB0 (slot 9 @ +0x24) (ServerFramework_DefaultTrueStub)
 *
 * Machine Bytes: b0 01 c3 (mov al, 1; retn)
 * MSVC /OPT:ICF COMDAT-folded virtual hook returning true.
 * Dispatched by CServerApp_ProcessIocpPacket @ 0x00935C2E on certification reply.
 * Inherited by CGameServer.
 */
bool CServerApp::OnPreServerReady() {
	return true;
}

/**
 * CServerApp::CreateListener
 * Native implementation @ 0x00935B00 (slot 10 @ +0x28)
 *
 * [RECONSTRUCTED]:
 * Dispatched on certification completion to bind and listen on the server's
 * configured port.
 *
 * Assembly trace:
 *   00935b00  cmp     dword [0xc8258c], 0x0   ; if (!g_pNetEngine)
 *   00935b07  jne     0x935b0e
 *   00935b09  call    BSLib::AssertFailed     ; 0x00964B60
 *   00935b0e  cmp     dword [0xc8279c], 0x0   ; if (!g_pLocalServerNode)
 *   00935b15  jne     0x935b1c
 *   00935b17  call    BSLib::AssertFailed     ; 0x00964B60
 *   00935b1c  mov     edx, dword [0xc8279c]   ; edx = g_pLocalServerNode
 *   00935b22  movzx   edx, word [edx+0xe]     ; edx = (uint16_t)g_pLocalServerNode->wPort
 *   00935b26  mov     eax, dword [0xc8258c]   ; eax = g_pNetEngine
 *   00935b2b  mov     ecx, dword [eax]        ; ecx = g_pNetEngine->vptr
 *   00935b2d  push    edx                     ; wPort
 *   00935b2e  push    0x0                     ; pszIP = nullptr
 *   00935b30  push    eax                     ; this = g_pNetEngine
 *   00935b31  mov     eax, dword [ecx+0x1c]   ; slot 7 @ +0x1C (CreateListener)
 *   00935b34  call    eax
 *   00935b36  test    eax, eax
 *   00935b38  jne     0x935b5a                ; return true
 *   00935b3a  mov     ecx, dword [0xc8279c]
 *   00935b40  movzx   edx, word [ecx+0xe]
 *   00935b44  push    edx
 *   00935b45  push    0xb3fde4                ; "cannot create listener : port (%d)"
 *   00935b4a  push    0x2000000               ; LOG_CAT_NET
 *   00935b4f  call    Log_Printf              ; 0x00936640
 *   00935b54  add     esp, 0xc
 *   00935b57  xor     al, al                  ; return false
 *   00935b59  retn
 *   00935b5a  mov     al, 0x1                 ; return true
 *   00935b5c  retn
 */
bool CServerApp::CreateListener() {
	if (!g_pNetEngine) {
		BSLib::AssertFailed();
	}
	if (!g_pLocalServerNode) {
		BSLib::AssertFailed();
	}

	uint16_t wPort = g_pLocalServerNode ? g_pLocalServerNode->wListenPort : 15884;
	if (g_pNetEngine && g_pNetEngine->CreateListener(nullptr, wPort)) {
		BSLib::Log_Printf(0, "successfully created listener on port %u", wPort);
		return true;
	}

	BSLib::Log_Printf(0x2000000, "cannot create listener : port (%d)", wPort);
	return false;
}

/**
 * [FOLDED DEFAULT HOOK - 0x00455EB0]
 * CServerApp::OnServerReady
 * Native implementation @ 0x00455EB0 (slot 11 @ +0x2C) (ServerFramework_DefaultTrueStub)
 *
 * Machine Bytes: b0 01 c3 (mov al, 1; retn)
 * MSVC /OPT:ICF COMDAT-folded virtual hook returning true.
 * Dispatched by CServerApp_ProcessIocpPacket @ 0x00935C3C on certification response.
 * Overridden by CGameServer::OnServerReady @ 0x00401B70 to initialize databases and world pools.
 */
bool CServerApp::OnServerReady() {
	return true;
}

/**
 * [FOLDED DEFAULT HOOK - 0x00455EB0]
 * CServerApp::Cleanup
 * Native implementation @ 0x00455EB0 (slot 12 @ +0x30) (ServerFramework_DefaultTrueStub)
 *
 * Machine Bytes: b0 01 c3 (mov al, 1; retn)
 * MSVC /OPT:ICF COMDAT-folded virtual hook returning true.
 * Dispatched by CServerApp_StopServerTasks @ 0x00935BB7.
 * Overridden by CGameServer::Cleanup @ 0x00401CD0 to release mutex and disconnect databases.
 */
bool CServerApp::Cleanup() {
	return true;
}

/**
 * ============================================================================
 * CServerApp::OnCertificationComplete
 * Native implementation @ 0x00935DA0 (slot 13 @ +0x34)
 *
 * [RECONSTRUCTED]:
 * Machine Bytes:
 *   00935da0  55                push    ebp
 *   00935da1  8bec              mov     ebp, esp
 *   00935da3  83e4f8            and     esp, 0xfffffff8
 *   00935da6  6801000102        push    0x2010001 ; PERF_COUNTER_MSG_COUNT
 *   00935dab  e8408e0100        call    ServerFramework_RegisterBuiltinPerformanceCounter
 *   00935db0  83c404            add     esp, 0x4
 *   00935db3  6802000103        push    0x3010002 ; PERF_COUNTER_OVERLAP_COUNT
 *   00935db8  e8338e0100        call    ServerFramework_RegisterBuiltinPerformanceCounter
 *   00935dbd  83c404            add     esp, 0x4
 *   00935dc0  b001              mov     al, 0x1
 *   00935dc2  8be5              mov     esp, ebp
 *   00935dc4  5d                pop     ebp
 *   00935dc5  c3                retn
 *
 * Registers the two framework-level built-in performance monitors:
 *   1. PERF_COUNTER_MSG_COUNT (0x02010001) -> "MsgCount"
 *   2. PERF_COUNTER_OVERLAP_COUNT (0x03010002) -> "OverlapCount"
 * ============================================================================
 */
bool CServerApp::OnCertificationComplete() {
	RegisterBuiltinPerformanceCounter(PERF_COUNTER_MSG_COUNT);
	RegisterBuiltinPerformanceCounter(PERF_COUNTER_OVERLAP_COUNT);
	return true;
}

/**
 * ============================================================================
 * CServerApp::StopServerTasks
 * Native implementation @ 0x00935B60 (slot 14 @ +0x38)
 *
 * [RECONSTRUCTED]:
 * Gracefully terminates all background server processes, threads, and network:
 *
 * Assembly trace:
 *   00935b60  push    ebx
 *   00935b61  push    esi
 *   00935b62  push    edi
 *   00935b63  mov     edi, [0x00AD90E4]    ; GetTickCount
 *   00935b69  mov     esi, ecx             ; esi = this
 *   00935b6b  call    edi                  ; GetTickCount()
 *   00935b6d  mov     ebx, eax             ; ebx = dwStartTime
 *   00935b6f  mov     eax, [esi]           ; this->vptr
 *   00935b71  mov     edx, [eax+0x4c]      ; slot 19 @ +0x4c (SetServerState)
 *   00935b74  push    8                    ; SERVER_STATE_STOPPED = 8
 *   00935b76  mov     ecx, esi             ; this
 *   00935b78  call    edx                  ; this->SetServerState(SERVER_STATE_STOPPED)
 *   00935b7a  call    0x0095BB90           ; BSLib_StopFtpManager()
 *   00935b7f  call    edi                  ; GetTickCount()
 *   00935b81  sub     eax, ebx             ; dwElapsed = current - dwStartTime
 *   00935b83  cmp     eax, 0x12c           ; 300 ms
 *   00935b88  jae     0x00935B95           ; if >= 300 ms skip sleep
 *   00935b8a  push    0x12c                ; 300 ms
 *   00935b8f  call    [0x00AD90F4]         ; Sleep(300)
 *   00935b95  mov     eax, [0x00C8258C]    ; g_pNetEngine
 *   00935b9a  test    eax, eax
 *   00935b9c  je      0x00935BB0
 *   00935b9e  mov     ecx, [eax]           ; g_pNetEngine->vptr
 *   00935ba0  mov     edx, [ecx+0x8]       ; slot 2 @ +0x8 (IBSNet::Stop)
 *   00935ba3  push    eax                  ; this (g_pNetEngine)
 *   00935ba4  call    edx                  ; g_pNetEngine->Stop()
 *   00935ba6  mov     dword [0x00C8258C], 0; g_pNetEngine = nullptr
 *   00935bb0  mov     eax, [esi]           ; this->vptr
 *   00935bb2  mov     edx, [eax+0x30]      ; slot 12 @ +0x30 (this->Cleanup)
 *   00935bb5  mov     ecx, esi             ; this
 *   00935bb7  call    edx                  ; this->Cleanup()
 *   00935bb9  call    0x009396B0           ; ServerFramework_CleanupServerRouting()
 *   00935bbe  call    0x00957BF0           ; ServerFramework_CleanupServerNodes()
 *   00935bc3  pop     edi
 *   00935bc4  pop     esi
 *   00935bc5  mov     al, 1
 *   00935bc7  pop     ebx
 *   00935bc8  retn
 * ============================================================================
 */
bool CServerApp::StopServerTasks() {
	// Native 0x00935B6B: Record shutdown start timestamp
	uint32_t dwStartTime = GetTickCount();

	// Native 0x00935B6F - 0x00935B78:
	// Virtual call to slot 19 (+0x4C): this->SetServerState(SERVER_STATE_STOPPED)
	SetServerState(SERVER_STATE_STOPPED);

	// Native 0x00935B7A: Teardown FTP manager subsystem
	BSLib::StopFtpManager();

	// Native 0x00935B7F - 0x00935B8F:
	// Drain wait: ensure at least 300 ms for queued network completion packets
	uint32_t dwElapsed = GetTickCount() - dwStartTime;
	if (dwElapsed < 300) {
		Sleep(300);
	}

	// Native 0x00935B95 - 0x00935BA6:
	// Virtual call to IBSNet slot 2 (+0x8): g_pNetEngine->Stop()
	if (g_pNetEngine != nullptr) {
		g_pNetEngine->Stop();
		g_pNetEngine = nullptr;
	}

	// Native 0x00935BB0 - 0x00935BB7:
	// Virtual call to slot 12 (+0x30): this->Cleanup()
	Cleanup();

	// Native 0x00935BB9: Clean up all cluster routing tables, lists, and topology maps
	ServerFramework_CleanupServerRouting();

	// Native 0x00935BBE: CleanupServerNodes @ 0x00957BF0
	ServerFramework_CleanupServerNodes();

	return true;
}

// [NATIVE - 0x0066B100]
// Default base implementation for slot 15 (+0x3C)
// Overridden by CGameServer @ 0x00401000 to dump memory pool statistics
void CServerApp::DumpGObjMemoryStats() {
}

// [NATIVE - 0x0066B100]
// Default base implementation for slot 16 (+0x40)
// Overridden by CGameServer @ 0x00401020 to reload quest scripts
void CServerApp::ReloadQuestScripts() {
}

/**
 * CServerApp::OnIdle
 * Native implementation @ 0x009371F0 (slot 17 @ +0x44)
 */
void CServerApp::OnIdle() {
	// Virtual hook called during idle cycles
}

/**
 * [RECONSTRUCTED - 0x00935BD0]
 * CServerApp::ProcessIocpPacket
 * Native implementation @ 0x00935BD0 (slot 18 @ +0x48) (416 bytes)
 *
 * Processes internal IOCP packets dispatched from network / framework threads:
 *   - 0x200A: Log messages from worker threads
 *             Decodes channel ID and string message from stream buffer:
 *             If channel == 0x02000001 or 0x03000000 -> writes to FatalLog.txt via ServerFramework_WriteFatalLogFile
 *             If channel != 0x03000000 -> forwards to m_pLogCallback(nChannel, strMsg.c_str())
 *   - 0x200B: Shutdown / error signal -> ServerFramework_PostShutdownSignal() (0x00936750)
 *   - 0x200C: Server ready packet -> OnPreServerReady, ServerFramework_CacheLocalServerType,
 *             OnServerReady, OnCertificationComplete, Log_Printf
 *   - Always forwards buffer to g_pNetEngine->ReleaseBuffer(pOverlapped) (slot 19 @ +0x4C)
 */
int32_t CServerApp::ProcessIocpPacket(void* pOverlapped) {
	if (!pOverlapped) {
		return 0;
	}

	// Native 0x00935C02 - 0x00935C0F: Read packet header opcode
	// edi = pOverlapped; eax = *(edi + 0x1050); opcode = *(uint16_t*)eax;
	void* pPacket = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pOverlapped) + 0x1050);
	uint16_t wOpcode = pPacket ? *reinterpret_cast<uint16_t*>(pPacket) : 0;

	if (wOpcode == 0x200A) {
		// Native 0x00935C6D - 0x00935D2E: Log message packet from worker thread
		if (m_pLogCallback == nullptr) {
			BSLib::AssertFailed();
		} else {
			uint32_t nChannel = 0;
			std::string strMsg;

			// Stream buffer payload decoding (native sub_5216c0 / sub_403f60 / sub_404b60):
			// Offset +0x00: uint16_t wOpcode (0x200A)
			// Offset +0x02: uint32_t nChannel
			// Offset +0x06: uint16_t wStrLen
			// Offset +0x08: payload string characters
			uint8_t* pData = reinterpret_cast<uint8_t*>(pPacket);
			if (pData != nullptr) {
				nChannel = *reinterpret_cast<uint32_t*>(pData + 2);
				uint16_t wStrLen = *reinterpret_cast<uint16_t*>(pData + 6);
				if (wStrLen > 0 && wStrLen < 8192) {
					strMsg.assign(reinterpret_cast<const char*>(pData + 8), wStrLen);
				} else if (pData[6] != '\0') {
					strMsg.assign(reinterpret_cast<const char*>(pData + 6));
				}
			}

			// Native 0x00935CBD - 0x00935CE7:
			// Channel 0x02000001 (system fatal) or 0x03000000 (direct fatal file):
			if (nChannel == 0x02000001 || nChannel == 0x03000000) {
				ServerFramework_WriteFatalLogFile(strMsg.c_str());
			}

			// Channel 0x03000000 skips the UI log callback; all other channels forward:
			if (nChannel != 0x03000000) {
				m_pLogCallback(nChannel, strMsg.c_str());
			}
		}
	} else if (wOpcode == 0x200B) {
		// Native 0x00935C63: Shutdown / termination signal
		ServerFramework_PostShutdownSignal();
	} else if (wOpcode == 0x200C) {
		// Native 0x00935C29 - 0x00935C5B: Server certification ready sequence
		OnPreServerReady();
		ServerFramework_CacheLocalServerType();
		OnServerReady();
		OnCertificationComplete();
		BSLib::Log_Printf(0x2000001, "%s is initialized successfully",
			m_strAppName.empty() ? g_serverConfig.szAppName : m_strAppName.c_str());
	}

	// Native 0x00935D33 - 0x00935D4D:
	// Always forwards pOverlapped to g_pNetEngine->ReleaseBuffer(pOverlapped) (slot 19 @ +0x4C)
	if (g_pNetEngine == nullptr) {
		BSLib::AssertFailed();
		return 0;
	}
	return g_pNetEngine->ReleaseBuffer(pOverlapped);
}

/**
 * CServerApp::SetServerState
 * Native implementation @ 0x00935D70 (slot 19 @ +0x4C)
 *
 * 00935d70  mov     eax, [0x00C8279C]         ; g_pLocalServerInfo
 * 00935d75  test    eax, eax
 * 00935d77  jne     0x00935D86
 * 00935d79  mov     eax, [esp+4]              ; nNewState
 * 00935d7d  mov     [ecx+0x13c], eax          ; this->m_nServerState = nNewState
 * 00935d83  retn    4
 * 00935d86  movzx   ecx, word [eax]           ; wServerID = g_pLocalServerInfo->wServerID
 * 00935d89  mov     eax, [esp+4]              ; nNewState
 * 00935d8d  push    0                         ; dwParam4 = 0
 * 00935d8f  push    ecx                       ; wServerID
 * 00935d90  call    0x0093B650                ; ServerFramework_NotifyServerStateChange (ServerTopology.cpp)
 * 00935d95  add     esp, 8
 * 00935d98  retn    4
 */
uint32_t CServerApp::SetServerState(uint32_t nNewState) {
	if (!g_pLocalServerInfo) {
		m_nServerState = nNewState;
		return nNewState;
	}

	uint16_t wServerID = g_pLocalServerInfo->wServerID;
	m_nServerState = nNewState;
	ServerFramework_NotifyServerStateChange(nNewState, wServerID, nullptr);
	return nNewState;
}

void CServerApp::SetLogCallback(PFN_LOG_CALLBACK pCallback) {
	m_pLogCallback = pCallback;
}

CServerApp::PFN_LOG_CALLBACK CServerApp::GetLogCallback() const {
	return m_pLogCallback;
}

const std::string& CServerApp::GetAppName() const {
	return m_strAppName;
}

uint32_t CServerApp::GetServerState() const {
	return m_nServerState;
}

bool CServerApp::IsActive() const {
	return m_nServerState != SERVER_STATE_STOPPED && m_nServerState != SERVER_STATE_NONE;
}

const CNetConfig& CServerApp::GetNetConfig() const {
	return m_netConfig;
}

} // namespace ServerFramework
