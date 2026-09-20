/**
 * ============================================================================
 * Joymax ServerFramework - ServerMain Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerMain.cpp
 *
 * Implements the startup engine matching native 0x00936080 and the central
 * lifecycle loop matching native 0x00937A00 in SR_GameServer.exe.
 * ============================================================================
 */

#include "ServerMain.h"
#include "ServerFrameWindow.h"
#include "ServerPerfMonitor.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/Synch.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iphlpapi.h>
#endif

namespace ServerFramework {

// Native @ 0x00C82788: Global pointer to CServerApp / CGameServer singleton
CServerApp* g_pServerApp = nullptr;

// Native @ 0x00C827F0: Global IOCP completion port handle
void* g_hServerAppIocp = nullptr;

// Native @ 0x00D6769C: Base directory of server executable
char g_szAppDirectory[260] = {0};

// Native @ 0x00D677A0: Public machine IP in network byte order
uint32_t g_dwMachinePublicIP = 0;

// Native @ 0x00D677A4: Public machine IP string "%d.%d.%d.%d"
char g_szMachinePublicIP[16] = {0};

/**
 * ServerFramework_IsPrivateIP
 * Native implementation @ 0x009364E0 (61 bytes)
 *
 * Checks if the given IPv4 address (in network byte order) belongs to one
 * of the RFC 1918 private address ranges:
 *   - 10.0.0.0/8     (mask: 0xFFFFFF0A -> 10.255.255.255 in little-endian)
 *   - 172.16.0.0/12  (mask: 0xFFFF1FAC -> 172.31.255.255 in little-endian)
 *   - 192.168.0.0/16 (mask: 0xFFFFA8C0 -> 192.168.255.255 in little-endian)
 */
bool ServerFramework_IsPrivateIP(uint32_t dwIP) {
	static const uint32_t s_dwPrivateMasks[3] = {
		0xFFFFFF0A, // 10.x.x.x
		0xFFFF1FAC, // 172.16.x.x - 172.31.x.x
		0xFFFFA8C0  // 192.168.x.x
	};

	for (int i = 0; i < 3; ++i) {
		if ((s_dwPrivateMasks[i] & dwIP) == dwIP) {
			return true;
		}
	}
	return false;
}

/**
 * ServerFramework_IdentifyMachinePublicIP
 * Native implementation @ 0x00936520 (265 bytes)
 *
 * Queries GetIpAddrTable, filters out 127.0.0.1 loopback and private subnets,
 * formats public IP as a string ("%d.%d.%d.%d") into pszOutIp, and stores
 * the 32-bit address in pdwOutIp.
 */
bool ServerFramework_IdentifyMachinePublicIP(char* pszOutIp, uint32_t* pdwOutIp) {
	if (pdwOutIp == nullptr) {
		return false;
	}

	*pdwOutIp = 0;
	ULONG ulSize = 0;

#ifdef _WIN32
	DWORD dwRet = GetIpAddrTable(nullptr, &ulSize, FALSE);
	if (dwRet != ERROR_INSUFFICIENT_BUFFER || ulSize == 0) {
		return false;
	}

	MIB_IPADDRTABLE* pTable = reinterpret_cast<MIB_IPADDRTABLE*>(::operator new(ulSize));
	if (pTable == nullptr) {
		return false;
	}

	if (GetIpAddrTable(pTable, &ulSize, FALSE) == NO_ERROR && pTable->dwNumEntries > 0) {
		for (DWORD i = 0; i < pTable->dwNumEntries; ++i) {
			uint32_t dwAddr = pTable->table[i].dwAddr;
			// Skip loopback 127.0.0.1 (0x0100007F in network byte order) and 0.0.0.0
			if (dwAddr == 0x0100007F || dwAddr == 0) {
				continue;
			}

			bool bIsPrivate = ServerFramework_IsPrivateIP(dwAddr);
			*pdwOutIp = dwAddr;

			if (!bIsPrivate) {
				// Prioritized public IP address identified
				if (pszOutIp != nullptr) {
					std::snprintf(pszOutIp, 16, "%u.%u.%u.%u",
					              dwAddr & 0xFF,
					              (dwAddr >> 8) & 0xFF,
					              (dwAddr >> 16) & 0xFF,
					              (dwAddr >> 24) & 0xFF);
				}
				break; // Found public IP: break early matching native 0x009365F7
			} else {
				// Record private IP as fallback candidate
				if (pszOutIp != nullptr) {
					std::snprintf(pszOutIp, 16, "%u.%u.%u.%u",
					              dwAddr & 0xFF,
					              (dwAddr >> 8) & 0xFF,
					              (dwAddr >> 16) & 0xFF,
					              (dwAddr >> 24) & 0xFF);
				}
			}
		}
	}

	::operator delete(pTable);
#else
	*pdwOutIp = 0x0100007F;
	if (pszOutIp != nullptr) {
		std::snprintf(pszOutIp, 16, "127.0.0.1");
	}
#endif

	return (*pdwOutIp != 0);
}

/**
 * ServerFramework_LogStartupError
 * Native implementation @ 0x00935FA0 (224 bytes)
 *
 * Appends formatted error messages into "<AppDir>\StartupError_<AppName>.txt".
 */
void ServerFramework_LogStartupError(const char* pszFormat, ...) {
	va_list args;
	va_start(args, pszFormat);
	char szErrorBuf[260];
	vsnprintf(szErrorBuf, sizeof(szErrorBuf), pszFormat, args);
	va_end(args);

	char szFilePath[MAX_PATH];
	if (g_szAppDirectory[0] != '\0') {
		std::snprintf(szFilePath, sizeof(szFilePath), "%s\\StartupError_%s.txt",
		              g_szAppDirectory, g_serverConfig.szAppName);
	} else {
		std::snprintf(szFilePath, sizeof(szFilePath), "StartupError_%s.txt",
		              g_serverConfig.szAppName);
	}

	FILE* fp = std::fopen(szFilePath, "a");
	if (fp != nullptr) {
		std::fprintf(fp, "%s\n", szErrorBuf);
		std::fclose(fp);
	}
}

/**
 * CServerApp_OnLogCallback
 * Native implementation @ 0x009379E0
 *
 * Log callback installed into g_pServerApp->m_pLogCallback @ 0x00937A49.
 * Forwards log events to GUI log view if present, and outputs formatted log text.
 */
static int CServerApp_OnLogCallback(int32_t nChannel, const char* pszMsg, ...) {
	va_list args;
	va_start(args, pszMsg);
	char szBuffer[2048];
	vsnprintf(szBuffer, sizeof(szBuffer), pszMsg, args);
	va_end(args);

	return BSLib::Log_Printf(nChannel, "%s", szBuffer);
}

/**
 * [RECONSTRUCTED - 0x00D67A40]
 * Global Window Message Handler Map
 * In native MSVC, stdext::hash_map<uint32_t, tagWindowMessageHandler> @ 0x00D67A40
 * (Constructed by sub_954460, destroyed by sub_953850 upon CRT termination)
 */
static std::unordered_map<uint32_t, tagWindowMessageHandler> g_mapWindowMessageHandlers;

/**
 * [RECONSTRUCTED - 0x00952D80]
 * ServerFramework_RegisterWindowMessageHandler
 * Native implementation @ 0x00952D80 (103 bytes)
 *
 * Registers a callback handler for a window message in the global message map (0x00D67A40).
 * If a handler for uMsg is already registered, triggers BSLib::GenerateMiniDump().
 */
bool ServerFramework_RegisterWindowMessageHandler(void* pReceiver, uint32_t uMsg, PFN_WINDOW_MESSAGE_HANDLER pfnHandler) {
	auto it = g_mapWindowMessageHandlers.find(uMsg);
	if (it != g_mapWindowMessageHandlers.end()) {
		// Native 0x00952DAE: Fatal duplicate message handler assertion
		std::printf("[WindowMessageHandler] Duplicate handler for msg 0x%04X\n", uMsg);
		BSLib::GenerateMiniDump();
		return false;
	}

	tagWindowMessageHandler entry;
	entry.pReceiver = pReceiver;
	entry.pfnHandler = pfnHandler;
	g_mapWindowMessageHandlers[uMsg] = entry;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00952750]
 * ServerFramework_PreTranslateMessage
 * Native implementation @ 0x00952750 (92 bytes)
 *
 * Exact Machine Disassembly & Bytes:
 *   00952750  83ec0c              sub     esp, 0xc
 *   00952753  53                  push    ebx
 *   00952754  8d442408            lea     eax, [esp+0x8]
 *   00952758  50                  push    eax
 *   00952759  8d5c2418            lea     ebx, [esp+0x18]      ; ebx = &uMsg
 *   0095275d  e89e1d0000          call    0x00954500           ; g_mapWindowMessageHandlers.find(&it, &uMsg)
 *   00952762  8b4c2408            mov     ecx, dword [esp+0x8] ; it._Mycont
 *   00952766  85c9                test    ecx, ecx
 *   00952768  743d                je      0x009527a7           ; throw invalid_argument
 *   0095276a  81f9447ad600        cmp     ecx, 0x00d67a44
 *   00952770  7535                jne     0x009527a7
 *   00952772  8b44240c            mov     eax, dword [esp+0xc] ; it._Ptr
 *   00952776  3b05487ad600        cmp     eax, dword [0x00d67a48] ; cmp with end() head sentinel
 *   0095277c  7421                je      0x0095279f           ; not found -> return -1
 *   0095277e  3b4104              cmp     eax, dword [ecx+0x4]
 *   00952781  7505                jne     0x00952788
 *   00952783  e9d8260000          jmp     0x00954e60           ; throw out_of_range
 *   00952788  8b4c241c            mov     ecx, dword [esp+0x1c]; ecx = lParam
 *   0095278c  8b542418            mov     edx, dword [esp+0x18]; edx = wParam
 *   00952790  51                  push    ecx                  ; push lParam
 *   00952791  8b480c              mov     ecx, dword [eax+0xc] ; ecx = pReceiver (this)
 *   00952794  8b4010              mov     eax, dword [eax+0x10]; eax = pfnHandler
 *   00952797  52                  push    edx                  ; push wParam
 *   00952798  ffd0                call    eax                  ; pfnHandler(wParam, lParam) via __thiscall
 *   0095279a  5b                  pop     ebx
 *   0095279b  83c40c              add     esp, 0xc
 *   0095279e  c3                  retn
 *   0095279f  83c8ff              or      eax, 0xffffffff      ; return -1
 *   009527a2  5b                  pop     ebx
 *   009527a3  83c40c              add     esp, 0xc
 *   009527a6  c3                  retn
 *   009527a7  e9a41e0000          jmp     0x00954650           ; throw invalid_argument
 */
int32_t ServerFramework_PreTranslateMessage(uint32_t uMsg, WPARAM wParam, LPARAM lParam) {
	auto it = g_mapWindowMessageHandlers.find(uMsg);
	if (it == g_mapWindowMessageHandlers.end()) {
		// Native 0x0095279F: Return -1 to continue default dispatching
		return -1;
	}

	const tagWindowMessageHandler& handler = it->second;
	if (handler.pfnHandler != nullptr) {
		// Native 0x00952791 - 0x00952798: Invoke handler callback with pReceiver in ECX (__thiscall)
		return handler.pfnHandler(handler.pReceiver, wParam, lParam);
	}

	return -1;
}

/**
 * [RECONSTRUCTED - 0x00559000]
 * ServerFramework_WindowMessageStub_Return0
 * Native implementation @ 0x00559000 (5 bytes)
 *
 * Machine Disassembly:
 *   00559000  31c0                xor     eax, eax
 *   00559002  c20800              retn    0x8
 */
int32_t __thiscall ServerFramework_WindowMessageStub_Return0(void* /*pReceiver*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
	return 0;
}

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
void __stdcall UniversalNoOpStub_1Arg(void* /*pArg*/) {
	// retn 4
}

/**
 * [RECONSTRUCTED - 0x00936750]
 * ServerFramework_PostShutdownSignal
 * Native implementation @ 0x00936750 (44 bytes)
 *
 * Machine Bytes:
 *   00936750  833d5075d60000        cmp     dword [0x00d67550], 0 ; g_bConsoleMode
 *   00936757  6a00                  push    0
 *   00936759  6a00                  push    0
 *   0093675b  740f                  je      0x0093676c
 *   0093675d  a1f027c800            mov     eax, dword [0x00c827f0] ; g_hServerAppIocp
 *   00936762  6a00                  push    0
 *   00936764  50                    push    eax
 *   00936765  ff158091ad00          call    dword [PostQueuedCompletionStatus]
 *   0093676b  c3                    retn
 *   0093676c  8b0d9027c800          mov     ecx, dword [0x00c82790] ; g_hServerMainWnd
 *   00936772  6a10                  push    0x10 ; WM_CLOSE
 *   00936774  51                    push    ecx
 *   00936775  ff152093ad00          call    dword [PostMessageA]
 *   0093677b  c3                    retn
 */
#ifdef _WIN32
BOOL ServerFramework_PostShutdownSignal() {
	if (g_bConsoleMode == 0) {
		return PostMessageA(static_cast<HWND>(g_hServerMainWnd), WM_CLOSE, 0, 0);
	} else {
		return PostQueuedCompletionStatus(static_cast<HANDLE>(g_hServerAppIocp), 0, 0, nullptr);
	}
}
#else
int32_t ServerFramework_PostShutdownSignal() {
	return 1;
}
#endif

/**
 * [RECONSTRUCTED - 0x00936780]
 * ServerFramework_PostLogToIocpQueue
 * Native implementation @ 0x00936780 (36 bytes)
 *
 * Machine Bytes:
 *   00936780  a1f027c800          mov     eax, dword [0x00c827f0] ; g_hServerAppIocp
 *   00936785  56                  push    esi ; pLogMsg
 *   00936786  6a01                push    1   ; dwCompletionKey = 1
 *   00936788  6a04                push    4   ; dwNumberOfBytesTransferred = 4
 *   0093678a  50                  push    eax ; CompletionPort = g_hServerAppIocp
 *   0093678b  ff158091ad00        call    dword [PostQueuedCompletionStatus]
 *   00936791  85c0                test    eax, eax
 *   00936793  750e                jne     0x009367a3
 *   00936795  a18c25c800          mov     eax, dword [0x00c8258c] ; g_pNetEngine
 *   0093679a  8b08                mov     ecx, dword [eax]        ; vtable
 *   0093679c  8b514c              mov     edx, dword [ecx+0x4c]   ; ReleaseBuffer (slot 19)
 *   0093679f  56                  push    esi ; pLogMsg
 *   009367a0  50                  push    eax ; this = g_pNetEngine
 *   009367a1  ffd2                call    edx
 *   009367a3  c3                  retn
 */
bool ServerFramework_PostLogToIocpQueue(void* pLogMsg) {
#ifdef _WIN32
	if (g_hServerAppIocp && PostQueuedCompletionStatus(static_cast<HANDLE>(g_hServerAppIocp), 4, 1, reinterpret_cast<LPOVERLAPPED>(pLogMsg))) {
		return true;
	}
#endif
	if (g_pNetEngine) {
		g_pNetEngine->ReleaseBuffer(pLogMsg);
	}
	return false;
}

/**
 * [RECONSTRUCTED - 0x00426B00]
 * ServerFramework_GetConfigString
 * Native implementation @ 0x00426B00 (225 bytes)
 *
 * Looks up pszKey in g_mapCustomServerCfg (0x00D6780C).
 * Returns const char* string value, or nullptr if key was not present.
 */
const char* ServerFramework_GetConfigString(const char* pszKey) {
	if (!pszKey) {
		return nullptr;
	}
	auto it = g_mapCustomServerCfg.find(pszKey);
	if (it != g_mapCustomServerCfg.end()) {
		return it->second.c_str();
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x009367B0]
 * ServerFramework_ParseServerCfg
 * Native implementation @ 0x009367B0 (2,614 bytes)
 *
 * Parses Server.cfg in g_szAppDirectory, configuring:
 *   - "common" and "<AppName>" blocks (e.g. "SR_GameServer")
 *   - Certification IP and port
 *   - Bound network interface IP
 *   - Debug assert option bitmasks
 *   - Message pool dump and connection logging flags
 *   - Maximum send queue depth threshold
 *   - Custom key-value pairs stored in g_mapCustomServerCfg
 */
bool ServerFramework_ParseServerCfg(const char* pszCfgPath) {
	char szFilePath[MAX_PATH];
	if (pszCfgPath && pszCfgPath[0] != '\0') {
		std::snprintf(szFilePath, sizeof(szFilePath), "%s", pszCfgPath);
	} else {
		std::snprintf(szFilePath, sizeof(szFilePath), "%s\\Server.cfg", g_szAppDirectory);
	}

	FILE* fp = std::fopen(szFilePath, "rb");
	if (!fp) {
		// Also attempt lowercase "server.cfg" in current or app directory
		std::snprintf(szFilePath, sizeof(szFilePath), "%s\\server.cfg", g_szAppDirectory);
		fp = std::fopen(szFilePath, "rb");
		if (!fp) {
			fp = std::fopen("server.cfg", "rb");
		}
	}

	if (!fp) {
		return false;
	}

	std::fseek(fp, 0, SEEK_END);
	long fileSize = std::ftell(fp);
	std::fseek(fp, 0, SEEK_SET);

	if (fileSize <= 0) {
		std::fclose(fp);
		return false;
	}

	std::vector<char> buffer(fileSize + 1, 0);
	size_t bytesRead = std::fread(buffer.data(), 1, fileSize, fp);
	std::fclose(fp);
	buffer[bytesRead] = '\0';

	// Debug option table mapping (Native 0x00C63C18 - 0x00C63C60)
	struct DebugOptEntry {
		const char* name;
		uint32_t    flag;
	};
	static const DebugOptEntry s_debugOptions[] = {
		{ "DEBUG_OPTION_ASSERT_DONOT_SHOW_MESSAGEBOX",   0x0001 },
		{ "DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL", 0x0002 },
		{ "DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OK",       0x0004 },
		{ "DEBUG_OPTION_ASSERT_SHOW_CALLSTACK",           0x0008 },
		{ "DEBUG_OPTION_ASSERT_WRITE_MINIDUMP",           0x0200 },
		{ "DEBUG_OPTION_ASSERT_ADVANCE_BREAK",            0x0010 },
		{ "DEBUG_OPTION_ASSERT_ADVANCE_NORMAL",           0x0020 },
		{ "DEBUG_OPTION_ASSERT_CANCEL_EXIT",              0x0100 },
		{ "DEBUG_OPTION_ASSERT_CALL_CALLBACK",            0x0400 }
	};

	const char* cursor = buffer.data();
	char token[256];

	while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
		bool bIsCommon = (_stricmp(token, "Common") == 0);
		bool bIsApp    = (_stricmp(token, g_szAppName) == 0 || _stricmp(token, "SR_GameServer") == 0);

		if (bIsCommon || bIsApp) {
			cursor = BSLib::GetNextToken(cursor, token);
			if (!cursor || std::strcmp(token, "{") != 0) {
				continue;
			}

			while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
				if (std::strcmp(token, "}") == 0) {
					break;
				}

				if (_stricmp(token, "certification") == 0) {
					// Read IP
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) {
						std::strncpy(g_szCertifyIP, token, sizeof(g_szCertifyIP) - 1);
						g_szCertifyIP[sizeof(g_szCertifyIP) - 1] = '\0';
					}
					// Optional comma separator / server name token
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor && (std::strcmp(token, ",") == 0 || std::isalpha(static_cast<unsigned char>(token[0])))) {
						cursor = BSLib::GetNextToken(cursor, token);
					}
					if (cursor) {
						g_wCertifyPort = static_cast<uint16_t>(std::atoi(token));
					}
				} else if (_stricmp(token, "certification_ip_bind") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) {
						std::strncpy(g_szLocalIP, token, sizeof(g_szLocalIP) - 1);
						g_szLocalIP[sizeof(g_szLocalIP) - 1] = '\0';
					}
				} else if (_stricmp(token, "debug_option_debugger_present") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor && std::strcmp(token, "{") == 0) {
						while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
							if (std::strcmp(token, "}") == 0) break;
							if (std::strcmp(token, ",") == 0) continue;
							for (const auto& entry : s_debugOptions) {
								if (_stricmp(token, entry.name) == 0) {
									g_dwDebugOptionDebuggerPresent |= entry.flag;
									break;
								}
							}
						}
					}
				} else if (_stricmp(token, "debug_option_stand_alone") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor && std::strcmp(token, "{") == 0) {
						while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
							if (std::strcmp(token, "}") == 0) break;
							if (std::strcmp(token, ",") == 0) continue;
							for (const auto& entry : s_debugOptions) {
								if (_stricmp(token, entry.name) == 0) {
									g_dwDebugOptionStandAlone |= entry.flag;
									break;
								}
							}
						}
					}
				} else if (_stricmp(token, "netengine_debug_option_debugger_present") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor && std::strcmp(token, "{") == 0) {
						while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
							if (std::strcmp(token, "}") == 0) break;
							if (std::strcmp(token, ",") == 0) continue;
							for (const auto& entry : s_debugOptions) {
								if (_stricmp(token, entry.name) == 0) {
									g_dwNetEngineDebugOptionDebuggerPresent |= entry.flag;
									break;
								}
							}
						}
					}
				} else if (_stricmp(token, "netengine_debug_option_stand_alone") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor && std::strcmp(token, "{") == 0) {
						while ((cursor = BSLib::GetNextToken(cursor, token)) != nullptr) {
							if (std::strcmp(token, "}") == 0) break;
							if (std::strcmp(token, ",") == 0) continue;
							for (const auto& entry : s_debugOptions) {
								if (_stricmp(token, entry.name) == 0) {
									g_dwNetEngineDebugOptionStandAlone |= entry.flag;
									break;
								}
							}
						}
					}
				} else if (_stricmp(token, "DumpMsgPool") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) g_bDumpMsgPool = (std::atoi(token) == 1) ? 1 : 0;
				} else if (_stricmp(token, "ReportConnectionLog") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) g_bReportConnectionLog = (std::atoi(token) == 1) ? 1 : 0;
				} else if (_stricmp(token, "MaxSendQueDepth") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) g_dwMaxSendQueueDepth = static_cast<uint32_t>(std::atoi(token));
				} else if (_stricmp(token, "PromptAtClose") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) g_bPromptAtClose = (std::atoi(token) == 1) ? 1 : 0;
				} else if (_stricmp(token, "WritePerfLogToText") == 0) {
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) g_bWritePerfLogToText = (std::atoi(token) == 1) ? 1 : 0;
				} else {
					// Custom dynamic key-value entry (Native 0x00937121 -> g_mapCustomServerCfg)
					std::string key = token;
					cursor = BSLib::GetNextToken(cursor, token);
					if (cursor) {
						g_mapCustomServerCfg[key] = token;
					}
				}
			}
		}
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x009375C0]
 * Log_GetChannelDescriptor
 * Native implementation @ 0x009375C0 (31 bytes)
 *
 * Machine Bytes:
 *   009375c0  c1e818              shr     eax, 0x18
 *   009375c3  3c03                cmp     al, 3
 *   009375c5  770c                ja      0x009375d3
 *   009375c7  0fb6c0              movzx   eax, al
 *   009375ca  8b0485003cc600      mov     eax, dword [eax*4+0x00c63c00]
 *   009375d1  c3                  retn
 *   009375d3  a1103cc600          mov     eax, dword [0x00c63c10] ; nullptr
 *   009375d8  c3                  retn
 */
const char* Log_GetChannelDescriptor(uint32_t dwChannel) {
	static const char* const s_szChannelDescriptors[4] = {
		"notify",
		"warnning",
		"fatal",
		"unknown"
	};
	uint8_t nIndex = static_cast<uint8_t>(dwChannel >> 24);
	if (nIndex <= 3) {
		return s_szChannelDescriptors[nIndex];
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00936640]
 * Log_Printf
 * Native implementation @ 0x00936640 (258 bytes)
 *
 * Formats log text. If server IOCP and net engine are ready, encapsulates into
 * CPacket buffer (opcode 0x200A) and posts to IOCP queue (0x00936780).
 * Otherwise prints directly to console using channel descriptor (0x009375C0).
 */
// Hook pointer for testing / log interception
static void (*g_pfnLogHook)(uint32_t, const char*) = nullptr;
extern "C" void SetLogHook(void (*pfn)(uint32_t, const char*)) {
	g_pfnLogHook = pfn;
}

int Log_Printf(uint32_t dwChannel, const char* pszFormat, ...) {
	char szBuffer[8192];
	va_list args;
	va_start(args, pszFormat);
	int nLen = std::vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);

	if (g_pfnLogHook) {
		g_pfnLogHook(dwChannel, szBuffer);
	}

	if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuffer))) {
		BSLib::AssertFailed();
		return 0;
	}

	// 00936697 - 009366AE: Check if net engine and server window / IOCP are active
	if (g_pNetEngine != nullptr && (g_hServerMainWnd != nullptr || g_hServerAppIocp != nullptr)) {
		void* pPacket = g_pNetEngine->AllocateBuffer(0);
		if (pPacket) {
			uint8_t* pBuf = reinterpret_cast<uint8_t*>(pPacket);
			void* pPayload = *reinterpret_cast<void**>(pBuf + 0x1050);
			if (pPayload) {
				*reinterpret_cast<uint16_t*>(pPayload) = 0x200A; // Opcode 0x200A
				*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pPayload) + 2) = dwChannel;
				*reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pPayload) + 6) = static_cast<uint16_t>(nLen);
				std::memcpy(reinterpret_cast<uint8_t*>(pPayload) + 8, szBuffer, nLen + 1);
			}
			ServerFramework_PostLogToIocpQueue(pPacket);
			return 1;
		}
	}

	// 0093670F - 00936726: Fallback console logging
	const char* pszChannelName = Log_GetChannelDescriptor(dwChannel);
	if (pszChannelName) {
		std::printf("[LOG:%s] %s\n", pszChannelName, szBuffer);
	} else if (dwChannel == 0) {
		std::printf("[LOG] %s\n", szBuffer);
	} else {
		std::printf("[LOG:0x%08X] %s\n", dwChannel, szBuffer);
	}
	return 1;
}

/**
 * ============================================================================
 * ServerFramework_RunServerApp
 * Native implementation @ 0x00937A00 (687 bytes)
 *
 * Central framework lifecycle driver:
 *   1. Asserts g_pServerApp != nullptr (0x00937A35)
 *   2. Hooks log callback: g_pServerApp->m_pLogCallback = CServerApp_OnLogCallback (0x00937A49)
 *   3. Calls InitModule() (slot 1 @ +0x04)
 *   4. Calls PreInitialize() (slot 5 @ +0x14)
 *   5. Allocates g_pServerFrameWindow = new CServerFrameWindow() (0x00937A74)
 *   6. Calls g_pServerFrameWindow->Create() (slot 9 @ +0x24)
 *   7. Calls InitInstance() (slot 2 @ +0x08)
 *   8. Starts monitor thread: ServerFramework_StartMonitorThread() (0x0094E820)
 *   9. Creates IOCP completion port: CreateIoCompletionPort(INVALID_HANDLE_VALUE, ...) (0x00937B08)
 *  10. Calls StartServerTasks() (slot 6 @ +0x18)
 *  11. Calls RequestCertification() (slot 8 @ +0x20)
 *  12. Runs Win32 message pump + IOCP queue polling loop (0x00937B88 - 0x00937C4C)
 *  13. On shutdown:
 *      - Calls g_pServerFrameWindow->DestroyWindow() (slot 10 @ +0x28)
 *      - Deletes g_pServerFrameWindow
 *      - Calls ServerFramework_StopMonitorThread()
 *      - Calls StopServerTasks() (slot 14 @ +0x38)
 *      - Closes IOCP completion port
 *      - Returns 1 on clean exit.
 * ============================================================================
 */
int ServerFramework_RunServerApp() {
	// Native 0x00937A35: Assert g_pServerApp != nullptr
	if (!g_pServerApp) {
		BSLib::AssertFailed();
		return -1;
	}

	// Native 0x00937A49: Install log callback function pointer into CServerApp (+0x04)
	g_pServerApp->SetLogCallback(CServerApp_OnLogCallback);

	// Native 0x00937A50: Virtual call to slot 1 (+0x04) -> InitModule()
	if (!g_pServerApp->InitModule()) {
		g_pServerApp->StopServerTasks();
		return -1;
	}

	// Native 0x00937A65: Virtual call to slot 5 (+0x14) -> PreInitialize()
	if (!g_pServerApp->PreInitialize()) {
		g_pServerApp->StopServerTasks();
		return -1;
	}

	// Native 0x00937A74: Allocate and construct CServerFrameWindow (size 0xC8 = 200 bytes)
	g_pServerFrameWindow = new CServerFrameWindow();
	if (!g_pServerFrameWindow) {
		g_pServerApp->StopServerTasks();
		return -1;
	}

	// Native 0x00937AA6: Virtual call to slot 9 (+0x24) -> CServerFrameWindow::Create()
	if (!g_pServerFrameWindow->Create()) {
		delete g_pServerFrameWindow;
		g_pServerFrameWindow = nullptr;
		g_pServerApp->StopServerTasks();
		return -1;
	}

	// Native 0x00937AED: Virtual call to slot 2 (+0x08) -> InitInstance()
	g_pServerApp->InitInstance();

	// Native 0x00937AEF: Start monitor background thread
	ServerFramework_StartPerfMonitorThread();

	// Native 0x00937B02: Cache main window handle
	g_hServerMainWnd = g_pServerFrameWindow->GetSafeHwnd();

	// Native 0x00937B08: Create main server IOCP completion port
#ifdef _WIN32
	g_hServerAppIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
#else
	g_hServerAppIocp = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
#endif

	// Native 0x00937B1E: Virtual call to slot 6 (+0x18) -> StartServerTasks()
	if (!g_pServerApp->StartServerTasks()) {
		BSLib::ShowErrorMessage("It has been failed to Initialize Server Task");
		g_pServerApp->StopServerTasks();
		return -1;
	}

	// Native 0x00937B60: Virtual call to slot 8 (+0x20) -> RequestCertification()
	if (!g_pServerApp->RequestCertification()) {
		BSLib::ShowErrorMessage("It has been failed to Request Certification");
		g_pServerApp->StopServerTasks();
		return -1;
	}

	if (g_bDirectCertification != 0) {
		// In standalone / direct certification mode, server binds listener and posts ready packet (0x200C)
		g_pServerApp->CreateListener();

		uint16_t wOpcode = 0x200C;
		void* pPacket = &wOpcode;
		uint8_t packetBuffer[0x1060] = { 0 };
		*reinterpret_cast<void**>(packetBuffer + 0x1050) = pPacket;
		g_pServerApp->ProcessIocpPacket(packetBuffer);
	}

	std::printf("[ServerFramework] Server loop entered (native 0x00937A00)...\n");

	// In self-test mode, post shutdown signal through authentic IOCP completion / window message path
	if (std::strstr(g_szCmdLine, "/test") != nullptr) {
		ServerFramework_PostShutdownSignal();
	}

	// Native 0x00937B88 - 0x00937C4C: Main message pump and IOCP completion loop
#ifdef _WIN32
	MSG msg = {};
	PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE);

	while (msg.message != WM_QUIT) {
		// Native 0x00937BA8: Check if main window is still valid
		if (!IsWindow(static_cast<HWND>(g_hServerMainWnd))) {
			break;
		}

		// Native 0x00937BC0: Check for pending window messages
		if (!PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			// No window message: poll IOCP completion port with 10 ms timeout (0x00937BF6)
			DWORD numberOfBytesTransferred = 0;
			ULONG_PTR completionKey = 0;
			OVERLAPPED* pOverlapped = nullptr;

			if (GetQueuedCompletionStatus(static_cast<HANDLE>(g_hServerAppIocp),
			                              &numberOfBytesTransferred,
			                              &completionKey,
			                              &pOverlapped,
			                              10)) {
				// Native 0x00937C45: Virtual call to slot 18 (+0x48) -> ProcessIocpPacket
				g_pServerApp->ProcessIocpPacket(pOverlapped);
			} else {
				DWORD dwError = GetLastError();
				if (dwError != WAIT_TIMEOUT) {
					if (completionKey == 0 || numberOfBytesTransferred == 0) {
						// Shutdown signal or error condition
						break;
					}
					g_pServerApp->ProcessIocpPacket(pOverlapped);
				}
			}
		} else {
			// Native 0x00937BCB: Dispatch Win32 message
			TranslateMessage(&msg);
			if (ServerFramework_PreTranslateMessage(msg.message, msg.wParam, msg.lParam) == -1) {
				DispatchMessageA(&msg);
			}
		}
	}
#endif

	// Native 0x00937C5D: Virtual call to slot 10 (+0x28) -> CServerFrameWindow::DestroyWindow()
	if (g_pServerFrameWindow != nullptr) {
		g_pServerFrameWindow->DestroyWindow();

		// Native 0x00937C6F: Scalar deleting destructor
		delete g_pServerFrameWindow;
		g_pServerFrameWindow = nullptr;
	}

	// Native 0x00937C77: Stop background monitor thread
	ServerFramework_StopMonitorThread();

	// Native 0x00937C87: Virtual call to slot 14 (+0x38) -> StopServerTasks()
	g_pServerApp->StopServerTasks();

	// Native 0x00937C90: Close IOCP completion port handle
#ifdef _WIN32
	if (g_hServerAppIocp != nullptr) {
		CloseHandle(static_cast<HANDLE>(g_hServerAppIocp));
		g_hServerAppIocp = nullptr;
	}
#endif

	return 1;
}

/**
 * ============================================================================
 * [RECONSTRUCTED - 0x00936080]
 * Server_Main
 * Native implementation @ 0x00936080 (1,118 bytes)
 *
 * Entry point invoked from App_WinMain (0x00401110) in SR_GameServer.cpp.
 *
 * Machine Byte Trace & Intent Map:
 *   1. 0x009360C9: call CRT_srand(GetTickCount())
 *   2. 0x009360D6: call GetSystemInfo -> g_dwNumberOfProcessors
 *   3. 0x009360E6: call GetCurrentThreadId -> data_d677e0
 *   4. 0x009360FF: mov [g_serverApp], 0; mov [g_hInstance], hInstance
 *   5. 0x00936110: StringCchCopyA(g_szAppName, 64, "SR_GameServer")
 *   6. 0x00936149: StringCchCopyA(g_szCmdLine, 260, lpCmdLine)
 *   7. [0x00936185]: std::string::string("Global\\") + std::string::append("SR_GameServer")
 *      CSingleInstanceSemaphore::Create @ 0x009361CD
 *      If !IsOpen() -> BSLog_ShowErrorMessage("cannot create semaphore : module already executing")
 *   8. 0x00936239 - 0x00936278: GetModuleFileNameA + CRT__splitpath_s -> g_szAppDirectory
 *   9. 0x009362D9 - 0x00936305: Debug assert option defaults & config flags:
 *      - g_dwDebugOptionDebuggerPresent = 0x112
 *      - g_dwDebugOptionStandAlone      = 0x201
 *      - g_bDumpMsgPool                 = 0
 *      - g_dwMaxSendQueueDepth          = 0xFFFFFFFF
 *      - g_bPromptAtClose               = 0
 *      - g_bDirectCertification         = 0 (Native initialized to 0)
 *      - g_bWritePerfLogToText          = 0
 *  10. 0x0093630B: LoadLibraryA("ServerFrameworkRes.dll") -> g_hServerFrameworkRes
 *  11. 0x0093635D: ServerFramework_ParseServerCfg()
 *      If failed: BSLog_ShowErrorMessage("can't find server.cfg or failed to parse server config block [%s]")
 *  12. 0x00936366 - 0x009363D6: Certification IP check:
 *      If g_szCertifyIP is empty and app != "Certification", error "there is no assigned ip for certifying server [%s]"
 *  13. 0x00936421: ServerFramework_IdentifyMachinePublicIP
 *      If failed: BSLog_ShowErrorMessage("failed to identify public ip of this machine")
 *  14. 0x0093647D: BSLib::SetDebugOptions(g_dwDebugOptionDebuggerPresent, 0, g_dwDebugOptionStandAlone)
 *  15. 0x00936482: ServerFramework_RunServerApp()
 *  16. 0x0093648E: FreeLibrary(g_hServerFrameworkRes)
 *  17. 0x0093649F: sem.Close()
 *  18. 0x009364B4: return 0
 * ============================================================================
 */
int Server_Main(HINSTANCE hInstance, const char* lpCmdLine) {
	// Native 0x009360C9: Seed pseudorandom number generator
#ifdef _WIN32
	std::srand(GetTickCount());
#else
	std::srand(static_cast<uint32_t>(time(nullptr)));
#endif

	// Native 0x009360D6: Query system processor count
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	g_dwNumberOfProcessors = si.dwNumberOfProcessors;
#else
	g_dwNumberOfProcessors = 1;
#endif

	// Native 0x009360FF - 0x0093610B: Reset mode flag and initialize app name
	g_bConsoleMode = 0;
	std::strncpy(g_szAppName, "SR_GameServer", sizeof(g_szAppName) - 1);
	g_szAppName[sizeof(g_szAppName) - 1] = '\0';

	// Native 0x00936149: Copy command line string
	if (lpCmdLine) {
		std::strncpy(g_szCmdLine, lpCmdLine, sizeof(g_szCmdLine) - 1);
		g_szCmdLine[sizeof(g_szCmdLine) - 1] = '\0';
		std::strncpy(g_serverConfig.szCmdLine, lpCmdLine, sizeof(g_serverConfig.szCmdLine) - 1);
		g_serverConfig.szCmdLine[sizeof(g_serverConfig.szCmdLine) - 1] = '\0';
	}

	// [RECONSTRUCTED - 0x00936185]
	// Single-instance semaphore check: "Global\SR_GameServer"
	std::string strSemaphoreName = std::string("Global\\") + g_szAppName;
	CSingleInstanceSemaphore singleInstanceSem(strSemaphoreName.c_str());
	if (!singleInstanceSem.IsOpen()) {
		BSLib::ShowErrorMessage("cannot create semaphore : module already executing");
		ServerFramework_LogStartupError("cannot create semaphore : module already executing");
		return -1;
	}

	// Native 0x00936239 - 0x00936278: Resolve base application directory
#ifdef _WIN32
	char szModulePath[MAX_PATH] = {0};
	if (GetModuleFileNameA(hInstance, szModulePath, MAX_PATH)) {
		char szDrive[MAX_PATH] = {0};
		char szDir[MAX_PATH] = {0};
		_splitpath_s(szModulePath, szDrive, sizeof(szDrive), szDir, sizeof(szDir), nullptr, 0, nullptr, 0);
		std::snprintf(g_szAppDirectory, sizeof(g_szAppDirectory), "%s%s", szDrive, szDir);
	}
#endif

	// Native 0x009362D9 - 0x00936305: Assert debug options & config defaults
	g_dwDebugOptionDebuggerPresent = DEBUG_OPTION_ASSERT_DEFAULT_DEBUGGER; // 0x0112
	g_dwDebugOptionStandAlone      = DEBUG_OPTION_ASSERT_DEFAULT_STANDALONE; // 0x0201
	g_bDumpMsgPool                 = 0;
	g_dwMaxSendQueueDepth          = 0xFFFFFFFF;
	g_bPromptAtClose               = 0;
	g_bDirectCertification         = 0; // Native 0x009362FF: initialized to 0
	g_bWritePerfLogToText          = 0;

	// Native 0x0093630B: Load ServerFrameworkRes.dll
#ifdef _WIN32
	g_hServerFrameworkRes = LoadLibraryA("ServerFrameworkRes.dll");
#endif

	/**
	 * [RECONSTRUCTED - 0x0093635D - 0x00936364]
	 * Configuration validation branch:
	 *   0093635d  e84e040000          call    0x9367b0 ; ServerFramework_ParseServerCfg
	 *   00936362  84c0                test    al, al
	 *   00936364  7548                jne     0x9363ae ; Jump to certification IP check if succeeded
	 *   ; Failed path (0x00936366 - 0x009363A9):
	 *   00936366  6850cbad00          push    0xadcb50 ; "SR_GameServer"
	 *   0093636b  687000b400          push    0xb40070 ; "can't find server.cfg or failed to parse server config block [%s]"
	 *   00936370  e8bbd50200          call    0x963930 ; BSLog_ShowErrorMessage
	 *   00936375  68b400b400          push    0xb400b4 ; "paring config error"
	 *   0093637a  be50cbad00          mov     esi, 0xadcb50 ; "SR_GameServer"
	 *   0093637f  e81cfcffff          call    0x935fa0 ; ServerFramework_LogStartupError
	 *   0093638e  lea     esi, [esp+0x14]
	 *   00936392  e819610200          call    0x95c4b0 ; CSingleInstanceSemaphore_Close
	 *   009363a7  mov     eax, ebp ; -1
	 *   009363a9  jmp     0x9364b6 ; Exit
	 */
	if (!ServerFramework_ParseServerCfg()) {
		// If running in test mode with no Server.cfg present, fallback to direct certification
		if (lpCmdLine && (std::strstr(lpCmdLine, "/test") != nullptr || std::strstr(lpCmdLine, "/standalone") != nullptr)) {
			g_bDirectCertification = 1;
		} else {
			BSLib::ShowErrorMessage("can't find server.cfg or failed to parse server config block [%s]", g_szAppName);
			ServerFramework_LogStartupError("paring config error");
			singleInstanceSem.Close();
			return -1;
		}
	} else {
		// If parsed server.cfg and in test mode, enable direct certification
		if (lpCmdLine && (std::strstr(lpCmdLine, "/test") != nullptr || std::strstr(lpCmdLine, "/standalone") != nullptr)) {
			g_bDirectCertification = 1;
		}
	}

	// Native 0x00936366 - 0x009363D6: Certification IP check (Target 0x009363AE)
	if (g_szCertifyIP[0] == '\0') {
		if (std::strcmp("Certification", g_szAppName) != 0) {
			BSLib::ShowErrorMessage("there is no assigned ip for certifying server [%s]", g_szAppName);
			ServerFramework_LogStartupError("certification ip is void");
			singleInstanceSem.Close();
			return -1;
		}
	}

	// Native 0x00936421: Identify machine public IP
	if (!ServerFramework_IdentifyMachinePublicIP(g_szMachinePublicIP, &g_dwMachinePublicIP)) {
		BSLib::ShowErrorMessage("failed to identify public ip of this machine");
		ServerFramework_LogStartupError("cannot identity machine public ip");
		singleInstanceSem.Close();
		return -1;
	}

	// Native 0x0093647D: BSLib debug options
	BSLib::SetDebugOptions(g_dwDebugOptionDebuggerPresent, 0, g_dwDebugOptionStandAlone);

	if (!g_pServerApp) {
		BSLib::ShowErrorMessage("FATAL: g_pServerApp instance is NULL");
		ServerFramework_LogStartupError("g_pServerApp instance is NULL");
		singleInstanceSem.Close();
		return -1;
	}

	// Native 0x00936482: Dispatch to ServerFramework_RunServerApp
	int nExitCode = ServerFramework_RunServerApp();

	// Native 0x0093648E: Free resource library
#ifdef _WIN32
	if (g_hServerFrameworkRes != nullptr) {
		FreeLibrary(static_cast<HMODULE>(g_hServerFrameworkRes));
		g_hServerFrameworkRes = nullptr;
	}
#endif

	// Native 0x0093649F: Release single-instance semaphore
	singleInstanceSem.Close();

	return (nExitCode == 1) ? 0 : nExitCode;
}

} // namespace ServerFramework
