/**
 * ============================================================================
 * Joymax ServerFramework - Server Configuration & Assert Options
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\
 *
 * Reverse-Engineered from:
 *   - Native 0x00C63C18 / 0x00D677E4 (Server_Main configuration block)
 *   - Native 0x00D677B4 (g_dwNumberOfProcessors)
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERCONFIG_H_
#define _JMX_SERVERFRAMEWORK_SERVERCONFIG_H_

#include <cstdint>
#include <cstdio>
#include <string>
#include <map>
#include "../../JMX_Library/BSLib/BSLog.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HWND;
#endif

namespace ServerFramework {

enum DebugOptionAssertFlags : uint32_t {
	DEBUG_OPTION_ASSERT_DONOT_SHOW_MESSAGEBOX       = 0x0001,
	DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL     = 0x0002,
	DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OK           = 0x0004,
	DEBUG_OPTION_ASSERT_SHOW_CALLSTACK               = 0x0008,
	DEBUG_OPTION_ASSERT_ADVANCE_BREAK                = 0x0010,
	DEBUG_OPTION_ASSERT_ADVANCE_NORMAL               = 0x0020,
	DEBUG_OPTION_ASSERT_CANCEL_EXIT                  = 0x0100,
	DEBUG_OPTION_ASSERT_WRITE_MINIDUMP               = 0x0200,
	DEBUG_OPTION_ASSERT_CALL_CALLBACK                = 0x0400,

	// Native defaults from Server_Main @ 0x009362D9 - 0x009362E3
	DEBUG_OPTION_ASSERT_DEFAULT_DEBUGGER             = 0x0112, // CANCEL_EXIT | ADVANCE_BREAK | SHOW_MESSAGEBOX_OKCANCEL
	DEBUG_OPTION_ASSERT_DEFAULT_STANDALONE           = 0x0201  // WRITE_MINIDUMP | DONOT_SHOW_MESSAGEBOX
};

/**
 * Server State Constants
 * Used across ServerFramework and certification subsystems
 */
enum ServerState : uint32_t {
	SERVER_STATE_NONE         = 0,
	SERVER_STATE_READY        = 1, // Set in CServerApp ctor (0x00935624)
	SERVER_STATE_INITIALIZING = 2, // Set in CServerApp::InitModule (0x00935707)
	SERVER_STATE_CERTIFYING   = 3, // Set in CServerApp::RequestCertification (0x00935ABB)
	SERVER_STATE_RUNNING      = 4, // Set after certification completed (0x00935C45)
	SERVER_STATE_STOPPING     = 7,
	SERVER_STATE_STOPPED      = 8, // Set in CServerApp::StopServerTasks (0x00935B78)
};

struct ServerConfig {
	uint32_t    dwDebugOptionDebuggerPresent = DEBUG_OPTION_ASSERT_DEFAULT_DEBUGGER;
	uint32_t    dwDebugOptionStandAlone      = DEBUG_OPTION_ASSERT_DEFAULT_STANDALONE;
	bool        bDumpMsgPool                 = false;
	uint32_t    dwMaxSendQueueDepth          = 0xFFFFFFFF; // Unlimited
	bool        bPromptAtClose               = false;
	bool        bDirectCertification         = false;
	bool        bWritePerfLogToText          = false;

	char        szAppName[64]                = "SR_GameServer";
	char        szCmdLine[260]               = "";
	char        szCertifyIP[16]              = "127.0.0.1";
	uint16_t    wCertifyPort                 = 32000;
};

// Global Server Framework State & Certification Configuration (Native 0x00D677B4 - 0x00D67808)

extern uint32_t g_dwNumberOfProcessors;                  // 0x00D677B4
extern char     g_szCertifyIP[16];                       // 0x00D677B8
extern char     g_szLocalIP[16];                         // 0x00D677C8
extern uint16_t g_wCertifyPort;                          // 0x00D677D8
extern uint32_t g_dwDebugOptionDebuggerPresent;          // 0x00D677E4
extern uint32_t g_dwDebugOptionStandAlone;               // 0x00D677E8
extern uint32_t g_dwNetEngineDebugOptionDebuggerPresent; // 0x00D677EC
extern uint32_t g_dwNetEngineDebugOptionStandAlone;      // 0x00D677F0
extern uint32_t g_bDumpMsgPool;                          // 0x00D677F4
extern uint32_t g_bReportConnectionLog;                  // 0x00D677F8
extern uint32_t g_dwMaxSendQueueDepth;                   // 0xFFFFFFFF
extern uint32_t g_bPromptAtClose;                        // 0x00D67800
extern uint32_t g_bDirectCertification;                  // 0x00D67804
extern uint32_t g_bWritePerfLogToText;                   // 0x00D67808

// Dynamic Custom Server Configuration Key-Value Map (Native 0x00D6780C)
extern std::map<std::string, std::string> g_mapCustomServerCfg;

/**
 * ServerFramework_ParseServerCfg
 * Native implementation @ 0x009367B0 (2,614 bytes)
 *
 * Locates "%s\Server.cfg" in g_szAppDirectory, parses sections:
 *   - "Common" and "SR_GameServer" (or current app name)
 * Populates certification settings, debug options, and dynamic custom config keys.
 */
bool ServerFramework_ParseServerCfg(const char* pszCfgPath = nullptr);

/**
 * ServerFramework_GetConfigString
 * Native implementation @ 0x00426B00 (225 bytes)
 *
 * Looks up pszKey in g_mapCustomServerCfg (0x00D6780C).
 * Returns const char* string value, or nullptr if key was not present.
 */
const char* ServerFramework_GetConfigString(const char* pszKey);

/**
 * ServerFramework_WriteFatalLogFile
 * Native implementation @ 0x009354E0 in ServerApp.cpp
 *
 * Writes crash/fatal log entry to %s\%04d-%02d-%02d_FatalLog.txt
 */
int ServerFramework_WriteFatalLogFile(const char* pszMessage = nullptr);

/**
 * ServerFramework_GenerateMiniDump
 * Native implementation @ 0x00964B60
 */
int ServerFramework_GenerateMiniDump();

/**
 * tagServerNode / CServerNode / CServerInfo
 * Native local server descriptor @ 0x00C8279C
 *
 * Struct layout proven from:
 *   - Native 0x00935B22: movzx edx, word [edx+0x0E] (wPort / listening port)
 *   - Native 0x00935D70: movzx eax, word [eax]      (wServerID / node ID)
 *   - Native 0x00949D53: movzx edx, word [eax+0x0C] (wServerType)
 *   - Native 0x00401B8D: esi = *(g_pLocalServerInfo + 0x24) (pDbConfig)
 */
/**
 * tagDbConfig
 * Native database configuration passed via g_pLocalServerInfo->pDbConfig
 * Accessed at:
 *   - Native 0x00401BA9: lea ebx, [esi+0x124] (lpszLogConString @ +0x124)
 *   - Native 0x00401BAF: lea eax, [esi+0x24]  (lpszConString @ +0x24)
 */
struct tagDbConfig {
	uint8_t m_reserved[0x24]  = { 0 };
	char    szShardDB[256]    = "Driver={SQL Server};Server=127.0.0.1;Database=SRO_VT_SHARD;Trusted_Connection=yes;";    // +0x24
	char    szShardLogDB[256] = "Driver={SQL Server};Server=127.0.0.1;Database=SRO_VT_SHARDLOG;Trusted_Connection=yes;"; // +0x124
};

extern tagDbConfig g_defaultDbConfig;

/**
 * tagServerNode / CServerNode / CServerInfo
 * Native local server descriptor @ 0x00C8279C
 *
 * Struct layout proven against machine bytes:
 *   - Native 0x00935D86: movzx ecx, word [eax]        (wServerID @ +0x00)
 *   - Native 0x00935B34: movzx eax, word [edx+0x0e]   (wListenPort @ +0x0E)
 *   - Native 0x0093B679: mov eax, dword [edi+0x10]    (nState @ +0x10)
 *   - Native 0x0093B686: mov dword [ebx], esi         (nState write @ +0x10)
 *   - Native 0x0093B72F: push 4; lea eax, [esi+0x10]  (nState packet write @ +0x10)
 *   - Native 0x00401B8D: esi = *(g_pLocalServerInfo + 0x24) (pDbConfig @ +0x24)
 */
struct tagServerNode {
	uint16_t    wServerID       = 1;            // +0x00: Server instance ID
	uint16_t    wServerType     = 1;            // +0x02: Server type code
	uint16_t    wParentServerID = 0;            // +0x04: Parent server node ID
	uint16_t    wSubServerID    = 0;            // +0x06: Sub server ID
	uint32_t    dwIP            = 0;            // +0x08: Bound IP address
	uint16_t    wPort           = 15884;        // +0x0C: Bound port
	uint16_t    wListenPort     = 15884;        // +0x0E: Local TCP listening port (standard SR_GameServer port)
	uint32_t    nState          = 1;            // +0x10: Runtime server state (SERVER_STATE_READY = 1)
	uint32_t    dwCapacity      = 1000;         // +0x14: Max player capacity
	uint32_t    dwCurrentUsers  = 0;            // +0x18: Current active user count
	uint32_t    dwMaxUsers      = 1000;         // +0x1C: Max peak users
	uint32_t    dwReserved      = 0;            // +0x20: Reserved alignment
	tagDbConfig* pDbConfig      = &g_defaultDbConfig; // +0x24: DB connection / shard config
	char        szName[64]      = "SR_GameServer";    // Server node name string
};
static_assert(sizeof(tagServerNode) >= 0x28, "tagServerNode size must cover at least 0x28 bytes");

typedef tagServerNode CServerNode;
typedef tagServerNode CServerInfo;

// Window message for server state changes posted to CServerFrameWindow (Native 0x0093B6B1)
constexpr uint32_t WM_SERVER_STATE_CHANGED = 0x07EA; // WM_USER + 0x03EA

/**
 * IServerStateObserver
 * State change observer sink registered via ServerFramework_SetStateObserver @ 0x009396A0
 * Dispatched by ServerFramework_NotifyServerStateChange @ 0x0093B69D (slot 1 @ +0x04)
 */
class IServerStateObserver {
public:
	virtual ~IServerStateObserver() = default;
	virtual void OnServerStateChanged(CServerNode* pNode, uint32_t nOldState) = 0;
};

// Global local server node / info descriptor pointer matching native 0x00C8279C
extern CServerNode  g_defaultServerNode;
extern CServerNode* g_pLocalServerNode;
extern CServerNode* g_pLocalServerInfo;

// Global state observer pointer matching native 0x00C827C4
extern IServerStateObserver* g_pServerStateObserver;

// Global server frame window handle
extern HWND g_hWndServerFrame;

extern ServerConfig g_serverConfig;
extern bool g_bServerRunning;

// Server Framework Operation Mode (Native 0x00D67550): 0 = GUI Frame Window, 1 = Headless Service/Console
extern uint32_t g_bConsoleMode;

// Command Line Buffer (Native 0x00D67554)
extern char g_szCmdLine[260];

// Application Name Buffer (Native 0x00D67558)
extern char g_szAppName[64];

// Resource Library Handle (Native 0x00C8278C)
extern void* g_hServerFrameworkRes;

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERCONFIG_H_
