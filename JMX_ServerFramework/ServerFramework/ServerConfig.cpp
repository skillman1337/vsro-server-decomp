/**
 * ============================================================================
 * Joymax ServerFramework - Server Configuration Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerConfig.cpp
 *
 * Implements globals:
 *   - Global Server Framework State & Certification Configuration (0x00D677B4 - 0x00D67808)
 *   - Custom server config map (0x00D6780C)
 *   - Local server node & runtime state
 * ============================================================================
 */

#include "ServerConfig.h"

namespace ServerFramework {

// Global Server Framework State & Certification Configuration (Native 0x00D677B4 - 0x00D67808)
uint32_t g_dwNumberOfProcessors                  = 1;            // 0x00D677B4
char     g_szCertifyIP[16]                       = "127.0.0.1";  // 0x00D677B8
char     g_szLocalIP[16]                         = "127.0.0.1";  // 0x00D677C8
uint16_t g_wCertifyPort                          = 32000;        // 0x00D677D8
uint32_t g_dwDebugOptionDebuggerPresent          = 0x0112;       // 0x00D677E4
uint32_t g_dwDebugOptionStandAlone               = 0x0201;       // 0x00D677E8
uint32_t g_dwNetEngineDebugOptionDebuggerPresent = 0x0112;       // 0x00D677EC
uint32_t g_dwNetEngineDebugOptionStandAlone      = 0x0201;       // 0x00D677F0
uint32_t g_bDumpMsgPool                          = 0;            // 0x00D677F4
uint32_t g_bReportConnectionLog                  = 0;            // 0x00D677F8
uint32_t g_dwMaxSendQueueDepth                   = 0xFFFFFFFF;   // 0x00D677FC
uint32_t g_bPromptAtClose                        = 0;            // 0x00D67800
uint32_t g_bDirectCertification                  = 0;            // 0x00D67804
uint32_t g_bWritePerfLogToText                   = 0;            // 0x00D67808

// Dynamic Custom Server Configuration Key-Value Map (Native 0x00D6780C)
std::map<std::string, std::string> g_mapCustomServerCfg;

// Default DB Configuration
tagDbConfig g_defaultDbConfig;

// Global local server node / info descriptor pointer matching native 0x00C8279C
CServerNode  g_defaultServerNode;
CServerNode* g_pLocalServerNode              = &g_defaultServerNode;
CServerNode* g_pLocalServerInfo              = &g_defaultServerNode;

// Global state observer pointer matching native 0x00C827C4
IServerStateObserver* g_pServerStateObserver = nullptr;

ServerConfig g_serverConfig;
bool g_bServerRunning                        = true;

// Server Framework Operation Mode (Native 0x00D67550): 0 = GUI Frame Window, 1 = Headless Service/Console
uint32_t g_bConsoleMode                      = 0;

// Command Line Buffer (Native 0x00D67554)
char g_szCmdLine[260]                        = "";

// Application Name Buffer (Native 0x00D67558)
char g_szAppName[64]                         = "SR_GameServer";

// Resource Library Handle (Native 0x00C8278C)
void* g_hServerFrameworkRes                  = nullptr;

} // namespace ServerFramework
