/**
 * ============================================================================
 * Joymax BSLib - Network Configuration Structures
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\NetConfig.h
 *
 * Reverse-Engineered from:
 *   - Native CNetConfig_Initialize @ 0x00935E20
 *   - Native CKeepAliveConfig_Initialize @ 0x00935EC0
 *   - Native sub_935dd0 (CNetConfig_ConfigureServerDefaults) @ 0x00935DD0
 *   - Native Trace_RuntimeClassStub @ 0x0066B100
 *   - Used in CServerApp @ +0x28 and CNetEngine @ +0x08
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_NETCONFIG_H_
#define _JMX_LIBRARY_BSLIB_NETCONFIG_H_

#include <cstdint>
#include <cstring>
#include <cstddef>

#pragma pack(push, 1)

/**
 * [STUB] Trace callback stub matching native Trace_RuntimeClassStub @ 0x0066B100
 *
 * 0066b100  c3  retn
 */
void Trace_RuntimeClassStub(const char* pszMsg);

/**
 * tagKeepAliveConfig
 * Native 32-byte (0x20) keepalive configuration block initialized by 0x00935EC0.
 *
 * Binary Layout:
 *   +0x00: dwPingInterval (uint32_t, 1000 ms)
 *   +0x04: dwReserved1    (uint32_t, 0)
 *   +0x08: dwTimeout       (uint32_t, 60000 ms)
 *   +0x0C: dwMaxLatency   (uint32_t, 60000 ms)
 *   +0x10: dwRetryCount   (uint32_t, 0xFFFFFFFF)
 *   +0x14: dwReserved2    (uint32_t, 0)
 *   +0x18: dwMaxRetries   (uint32_t, 0xFFFFFFFF)
 *   +0x1C: dwReserved3    (uint32_t, 0)
 */
struct tagKeepAliveConfig {
	uint32_t dwPingInterval;            // +0x00: 1000 ms (0x3E8)
	uint32_t dwReserved1;               // +0x04: 0
	uint32_t dwTimeout;                 // +0x08: 60000 ms (0xEA60)
	uint32_t dwMaxLatency;              // +0x0C: 60000 ms (0xEA60)
	uint32_t dwRetryCount;              // +0x10: -1 (0xFFFFFFFF)
	uint32_t dwReserved2;               // +0x14: 0
	uint32_t dwMaxRetries;              // +0x18: -1 (0xFFFFFFFF)
	uint32_t dwReserved3;               // +0x1C: 0

	tagKeepAliveConfig();
};
static_assert(sizeof(tagKeepAliveConfig) == 0x20, "tagKeepAliveConfig size must be exactly 32 bytes (0x20)");
static_assert(offsetof(tagKeepAliveConfig, dwPingInterval) == 0x00, "dwPingInterval offset mismatch");
static_assert(offsetof(tagKeepAliveConfig, dwTimeout)      == 0x08, "dwTimeout offset mismatch");
static_assert(offsetof(tagKeepAliveConfig, dwMaxLatency)   == 0x0C, "dwMaxLatency offset mismatch");
static_assert(offsetof(tagKeepAliveConfig, dwRetryCount)   == 0x10, "dwRetryCount offset mismatch");
static_assert(offsetof(tagKeepAliveConfig, dwMaxRetries)   == 0x18, "dwMaxRetries offset mismatch");

/**
 * CKeepAliveConfig_Initialize
 * Native implementation @ 0x00935EC0 (55 bytes) in NetConfig.cpp
 */
void CKeepAliveConfig_Initialize(tagKeepAliveConfig* pKeepAlive);

/**
 * tagNetConfig
 * Native 276-byte (0x114) network engine configuration block initialized by 0x00935E20 / 0x00935DD0.
 */
struct tagNetConfig {
	uint8_t             bEnable;                        // +0x00: Enable flag (1)
	uint8_t             byMode;                         // +0x01: Mode (2)
	uint8_t             byProtocol;                     // +0x02: Protocol (2)
	uint8_t             byPadding;                      // +0x03: Alignment byte (0)
	uint32_t            dwIP;                           // +0x04: Listen IP (0)
	uint32_t            dwPort;                         // +0x08: Listen Port (0)
	uint32_t            dwKeepAliveTimeout;             // +0x0C: 60 sec (0x3C)
	uint32_t            dwReserved1;                    // +0x10: 0
	uint32_t            dwMaxConnections;               // +0x14: Unlimited (-1)
	uint32_t            dwMaxPacketSize;                // +0x18: Unlimited (-1)
	uint32_t            dwLogCallback;                  // +0x1C: Log callback (Log_Printf @ 0x00936640)
	uint32_t            dwTraceCallback;                // +0x20: Trace callback (CServerApp_NetEngineMessageCallback @ 0x00935720)

	tagKeepAliveConfig  keepAlive;                      // +0x24: Nested 32-byte keepalive settings (size 0x20, up to 0x44)

	uint32_t            dwFlag1;                        // +0x44: 1
	uint32_t            dwFlag2;                        // +0x48: 1
	uint32_t            dwFlag3;                        // +0x4C: 0
	uint32_t            dwFlag4;                        // +0x50: 0
	uint32_t            dwFlag5;                        // +0x54: 1
	uint32_t            dwFlag6;                        // +0x58: 1
	uint32_t            dwMaxSendQueueDepth;            // +0x5C: Max send queue depth (g_dwMaxSendQueueDepth @ 0x00D677FC)
	uint32_t            dwMainTaskID_1;                 // +0x60: Main task ID (g_dwTaskID_ProcessMain @ 0x00C67734)
	uint32_t            dwFlag9;                        // +0x64: 1
	uint32_t            dwFlag10;                       // +0x68: 1
	uint32_t            dwFlag11;                       // +0x6C: 1
	uint32_t            dwFlag12;                       // +0x70: 0
	uint32_t            dwFlag13;                       // +0x74: 0
	uint32_t            dwFlag14;                       // +0x78: 1
	uint32_t            dwFlag15;                       // +0x7C: 1
	uint32_t            dwFlag16;                       // +0x80: -1
	uint32_t            dwMainTaskID_2;                 // +0x84: Main task ID (g_dwTaskID_ProcessMain @ 0x00C67734)
	uint32_t            dwFlag18;                       // +0x88: 1

	uint32_t            dwDebugOptionDebuggerPresent;    // +0x8C: Debugger present assert flags (0x0112 via sub_935dd0)
	uint32_t            dwDebugOptionStandAlone;        // +0x90: Standalone assert flags (0x0201 via sub_935dd0)

	char                szAppName[128];                 // +0x94: Application module name buffer (128 bytes, up to 0x114)

	tagNetConfig();
};
static_assert(sizeof(tagNetConfig) == 0x114, "tagNetConfig size must be exactly 276 bytes (0x114)");
static_assert(offsetof(tagNetConfig, bEnable)                      == 0x00, "bEnable offset mismatch");
static_assert(offsetof(tagNetConfig, dwIP)                         == 0x04, "dwIP offset mismatch");
static_assert(offsetof(tagNetConfig, dwPort)                       == 0x08, "dwPort offset mismatch");
static_assert(offsetof(tagNetConfig, dwKeepAliveTimeout)           == 0x0C, "dwKeepAliveTimeout offset mismatch");
static_assert(offsetof(tagNetConfig, dwLogCallback)                == 0x1C, "dwLogCallback offset mismatch");
static_assert(offsetof(tagNetConfig, dwTraceCallback)              == 0x20, "dwTraceCallback offset mismatch");
static_assert(offsetof(tagNetConfig, keepAlive)                    == 0x24, "keepAlive offset mismatch");
static_assert(offsetof(tagNetConfig, dwFlag1)                      == 0x44, "dwFlag1 offset mismatch");
static_assert(offsetof(tagNetConfig, dwMaxSendQueueDepth)          == 0x5C, "dwMaxSendQueueDepth offset mismatch");
static_assert(offsetof(tagNetConfig, dwMainTaskID_1)               == 0x60, "dwMainTaskID_1 offset mismatch");
static_assert(offsetof(tagNetConfig, dwFlag9)                      == 0x64, "dwFlag9 offset mismatch");
static_assert(offsetof(tagNetConfig, dwFlag16)                     == 0x80, "dwFlag16 offset mismatch");
static_assert(offsetof(tagNetConfig, dwMainTaskID_2)               == 0x84, "dwMainTaskID_2 offset mismatch");
static_assert(offsetof(tagNetConfig, dwFlag18)                     == 0x88, "dwFlag18 offset mismatch");
static_assert(offsetof(tagNetConfig, dwDebugOptionDebuggerPresent) == 0x8C, "dwDebugOptionDebuggerPresent offset mismatch");
static_assert(offsetof(tagNetConfig, dwDebugOptionStandAlone)     == 0x90, "dwDebugOptionStandAlone offset mismatch");
static_assert(offsetof(tagNetConfig, szAppName)                    == 0x94, "szAppName offset mismatch");

/**
 * CNetConfig_Initialize
 * Native implementation @ 0x00935E20 (147 bytes)
 *
 * 00935e20  lea     eax, [esi+0x24]
 * 00935e23  call    0x935ec0                            ; CKeepAliveConfig_Initialize
 * 00935e28  xor     eax, eax
 * 00935e2a  mov     ecx, 0x1
 * 00935e2f  mov     dword [esi+0x4c], eax
 * 00935e32  mov     dword [esi+0x50], eax
 * 00935e35  mov     dword [esi+0x60], eax
 * 00935e38  mov     dword [esi+0x70], eax
 * 00935e3b  mov     dword [esi+0x74], eax
 * 00935e3e  mov     dword [esi+0x84], eax
 * 00935e44  mov     dword [esi+0x44], ecx
 * 00935e47  mov     dword [esi+0x48], ecx
 * 00935e4a  mov     dword [esi+0x54], ecx
 * 00935e4d  mov     dword [esi+0x58], ecx
 * 00935e50  mov     dword [esi+0x64], ecx
 * 00935e53  mov     dword [esi+0x68], ecx
 * 00935e56  mov     dword [esi+0x6c], ecx
 * 00935e59  mov     dword [esi+0x78], ecx
 * 00935e5c  mov     dword [esi+0x7c], ecx
 * 00935e5f  mov     dword [esi+0x88], ecx
 * 00935e65  or      edx, 0xffffffff
 * 00935e68  mov     dword [esi+0x5c], edx
 * 00935e6b  mov     dword [esi+0x80], edx
 * 00935e71  push    0x80
 * 00935e76  push    eax
 * 00935e77  mov     dword [esi+0x4], eax
 * 00935e7a  mov     dword [esi+0x8], eax
 * 00935e7d  mov     dword [esi+0x10], eax
 * 00935e80  mov     dword [esi+0x1c], eax
 * 00935e83  lea     eax, [esi+0x94]
 * 00935e89  push    eax
 * 00935e8a  mov     byte [esi+0x2], 0x2
 * 00935e8e  mov     byte [esi+0x1], 0x2
 * 00935e92  mov     dword [esi+0xc], 0x3c
 * 00935e99  mov     dword [esi+0x14], edx
 * 00935e9c  mov     dword [esi+0x18], edx
 * 00935e9f  mov     dword [esi+0x20], 0x66b100
 * 00935ea6  mov     byte [esi], cl
 * 00935ea8  call    0x9e3520                            ; CRT_memset(eax, 0, 0x80)
 * 00935ead  add     esp, 0xc
 * 00935eb0  mov     eax, esi
 * 00935eb2  retn
 */
/**
 * [RECONSTRUCTED - 0x00935E20]
 * CNetConfig_Initialize
 * Native implementation @ 0x00935E20 (148 bytes) in NetConfig.cpp
 */
tagNetConfig* CNetConfig_Initialize(tagNetConfig* pConfig);

/**
 * [RECONSTRUCTED - 0x00935DD0]
 * CNetConfig_ConfigureServerDefaults
 * Native implementation @ 0x00935DD0 (sub_935dd0, 79 bytes) in NetConfig.cpp
 */
void CNetConfig_ConfigureServerDefaults(tagNetConfig* pConfig);

#pragma pack(pop)

typedef tagNetConfig CNetConfig;

#endif // _JMX_LIBRARY_BSLIB_NETCONFIG_H_
