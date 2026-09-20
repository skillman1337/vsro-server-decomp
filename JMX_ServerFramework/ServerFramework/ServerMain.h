/**
 * ============================================================================
 * Joymax ServerFramework - ServerMain Header
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerMain.h
 *
 * Declares the central server startup routine matching native 0x00936080 and
 * the lifecycle engine matching native 0x00937A00.
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERMAIN_H_
#define _JMX_SERVERFRAMEWORK_SERVERMAIN_H_

#include "ServerApp.h"
#include "../../JMX_Library/BSLib/Synch.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HINSTANCE;
typedef char* LPSTR;
#endif

namespace ServerFramework {

// Global server application pointer (Native 0x00C82788)
extern CServerApp* g_pServerApp;

// Global IOCP Completion Port Handle (Native 0x00C827F0)
extern void* g_hServerAppIocp;

// Global Machine Public IP & Directory (Native 0x00D6769C, 0x00D677A0, 0x00D677A4)
extern char     g_szAppDirectory[260];   // 0x00D6769C: Base directory of server executable
extern uint32_t g_dwMachinePublicIP;     // 0x00D677A0: Public IP in network byte order
extern char     g_szMachinePublicIP[16]; // 0x00D677A4: Public IP formatted as "%d.%d.%d.%d"

using ::CSingleInstanceSemaphore;

/**
 * ServerFramework_IsPrivateIP
 * Native implementation @ 0x009364E0 (61 bytes)
 *
 * Checks if the given IPv4 address belongs to an RFC 1918 private subnet:
 *   - 10.0.0.0/8     (mask 0xFFFFFF0A)
 *   - 172.16.0.0/12  (mask 0xFFFF1FAC)
 *   - 192.168.0.0/16 (mask 0xFFFFA8C0)
 * Returns true if private, false if public.
 */
bool ServerFramework_IsPrivateIP(uint32_t dwIP);

/**
 * ServerFramework_IdentifyMachinePublicIP
 * Native implementation @ 0x00936520 (265 bytes)
 *
 * Queries GetIpAddrTable, filters out 127.0.0.1 loopback and private subnets,
 * formats public IP as a string ("%d.%d.%d.%d") into pszOutIp, and stores
 * the 32-bit address in pdwOutIp.
 */
bool ServerFramework_IdentifyMachinePublicIP(char* pszOutIp, uint32_t* pdwOutIp);

/**
 * ServerFramework_LogStartupError
 * Native implementation @ 0x00935FA0 (224 bytes)
 *
 * Appends formatted error messages into "<AppDir>\StartupError_<AppName>.txt".
 */
void ServerFramework_LogStartupError(const char* pszFormat, ...);

/**
 * ServerFramework_PostShutdownSignal
 * Native implementation @ 0x00936750 (44 bytes)
 *
 * Posts termination signal via PostQueuedCompletionStatus or WM_CLOSE.
 */
#ifdef _WIN32
BOOL ServerFramework_PostShutdownSignal();
#else
int32_t ServerFramework_PostShutdownSignal();
#endif

/**
 * ServerFramework_PostLogToIocpQueue
 * Native implementation @ 0x00936780 (36 bytes)
 *
 * Enqueues formatted log packet buffer into server IOCP completion queue:
 *   PostQueuedCompletionStatus(g_hServerAppIocp, 4, 1, (LPOVERLAPPED)pLogMsg)
 * On failure, reclaims buffer via g_pNetEngine->ReleaseBuffer(pLogMsg).
 */
bool ServerFramework_PostLogToIocpQueue(void* pLogMsg);

/**
 * Log_GetChannelDescriptor
 * Native implementation @ 0x009375C0 (31 bytes)
 *
 * Returns channel name string ("notify", "warnning", "fatal", "unknown")
 * based on channel category index (dwChannel >> 24).
 */
const char* Log_GetChannelDescriptor(uint32_t dwChannel);

/**
 * Log_Printf
 * Native implementation @ 0x00936640 (258 bytes)
 *
 * Central formatted logging function for ServerFramework:
 *   - If net engine and server IOCP / window are active:
 *       1. Allocates buffer via g_pNetEngine->AllocateBuffer(0)
 *       2. Sets packet opcode 0x200A
 *       3. Encodes channel ID and formatted message into packet buffer
 *       4. Posts to IOCP queue via ServerFramework_PostLogToIocpQueue
 *   - Otherwise:
 *       Resolves channel name via Log_GetChannelDescriptor and prints to console.
 */
int Log_Printf(uint32_t dwChannel, const char* pszFormat, ...);

/**
 * Window Message Handler callback convention
 * Native calling convention: __thiscall (ECX = pReceiver, stack has wParam, lParam, callee pops 8 bytes)
 */
typedef int32_t (__thiscall *PFN_WINDOW_MESSAGE_HANDLER)(void* pReceiver, WPARAM wParam, LPARAM lParam);

struct tagWindowMessageHandler {
	void* pReceiver;
	PFN_WINDOW_MESSAGE_HANDLER pfnHandler;
};

/**
 * ServerFramework_RegisterWindowMessageHandler
 * Native implementation @ 0x00952D80 (103 bytes)
 *
 * Registers a handler callback for a specific window message (e.g. WM_KEYDOWN, WM_MOUSEHWHEEL).
 * If a handler for uMsg already exists, triggers ServerFramework_GenerateMiniDump()
 * (native assert for duplicate message handler).
 */
bool ServerFramework_RegisterWindowMessageHandler(void* pReceiver, uint32_t uMsg, PFN_WINDOW_MESSAGE_HANDLER pfnHandler);

/**
 * ServerFramework_PreTranslateMessage
 * Native implementation @ 0x00952750 (92 bytes)
 *
 * Checks the registered window message map (g_mapWindowMessageHandlers @ 0x00D67A40).
 * If a handler is registered for uMsg:
 *   Invokes pReceiver->pfnHandler(wParam, lParam) via __thiscall.
 * Returns the handler return value, or -1 (0xFFFFFFFF) if no handler was registered.
 */
int32_t ServerFramework_PreTranslateMessage(uint32_t uMsg, WPARAM wParam, LPARAM lParam);

/**
 * ServerFramework_WindowMessageStub_Return0
 * Native implementation @ 0x00559000 (5 bytes)
 *
 * Generic stub handler: xor eax, eax / retn 8. Returns 0.
 */
int32_t __thiscall ServerFramework_WindowMessageStub_Return0(void* pReceiver, WPARAM wParam, LPARAM lParam);

/**
 * [RECONSTRUCTED - 0x009BF500]
 * UniversalNoOpStub_1Arg
 * Native implementation @ 0x009BF500 (3 bytes)
 *
 * Machine Disassembly:
 *   009bf500  c20400              retn    0x4
 *
 * Universal 1-argument dummy stub returning void.
 * Reused via MSVC Identical COMDAT Folding (ICF) across:
 *   - 456 vtable default/empty virtual methods (CMainProcess, CCmdSource, CDBRecord, AQ_Base)
 *   - Network packet dispatch table default no-op handlers (g_aPacketDispatchTable)
 *   - GUI display layer no-op render callbacks (CDisplayWindow::RenderItem)
 */
void __stdcall UniversalNoOpStub_1Arg(void* pArg);

/**
 * ServerFramework_RunServerApp
 * Native implementation @ 0x00937A00

 *
 * Central lifecycle driver and message pump for ServerFramework:
 *   1. Calls InitModule() (slot 1 @ +0x04)
 *   2. Sets log callback: m_pLogCallback = CServerApp_OnLogCallback
 *   3. Calls PreInitialize() (slot 5 @ +0x14)
 *   4. Allocates CServerFrameWindow and calls Create() (slot 9 @ +0x24)
 *   5. Calls InitInstance() (slot 2 @ +0x08)
 *   6. Starts monitor thread: ServerFramework_StartMonitorThread()
 *   7. Creates IOCP: CreateIoCompletionPort()
 *   8. Calls StartServerTasks() (slot 6 @ +0x18)
 *   9. Calls RequestCertification() (slot 8 @ +0x20)
 *  10. Runs message loop and IOCP queue polling (10ms timeout)
 *  11. On exit: DestroyWindow(), deletes frame window, stops monitor thread,
 *      calls StopServerTasks() (slot 14 @ +0x38), closes IOCP.
 */
int ServerFramework_RunServerApp();

/**
 * Server_Main
 * Native implementation @ 0x00936080
 */
int Server_Main(HINSTANCE hInstance, const char* lpCmdLine);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERMAIN_H_
