/**
 * ============================================================================
 * Joymax BSLib - Network Configuration Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\NetConfig.cpp
 *
 * Implements:
 *   - Native CNetConfig_Initialize @ 0x00935E20
 *   - Native CKeepAliveConfig_Initialize @ 0x00935EC0
 *   - Native sub_935dd0 (CNetConfig_ConfigureServerDefaults) @ 0x00935DD0
 * ============================================================================
 */

#include "NetConfig.h"
#include <cstring>

/**
 * [STUB - 0x0066B100]
 * Trace_RuntimeClassStub
 * Native implementation @ 0x0066B100 (1 byte: c3 retn)
 */
void Trace_RuntimeClassStub(const char* /*pszMsg*/) {
}

/**
 * [RECONSTRUCTED - 0x00935EC0]
 * CKeepAliveConfig_Initialize
 * Native implementation @ 0x00935EC0 (57 bytes)
 */
void CKeepAliveConfig_Initialize(tagKeepAliveConfig* pKeepAlive) {
	if (!pKeepAlive) {
		return;
	}

	// 00935ec6 - 00935ee3: Loop 3 times writing { -1, 0 } starting at offset +0x08
	pKeepAlive->dwTimeout    = 0xFFFFFFFF;
	pKeepAlive->dwMaxLatency = 0;
	pKeepAlive->dwRetryCount = 0xFFFFFFFF;
	pKeepAlive->dwReserved2  = 0;
	pKeepAlive->dwMaxRetries = 0xFFFFFFFF;
	pKeepAlive->dwReserved3  = 0;

	// 00935ee5: mov dword [edi], 0xea60
	pKeepAlive->dwTimeout = 60000;

	// 00935eeb: mov dword [eax], 0x3e8
	pKeepAlive->dwPingInterval = 1000;

	// 00935ef1: mov dword [eax+0xc], 0xea60
	pKeepAlive->dwMaxLatency = 60000;

	pKeepAlive->dwReserved1 = 0;
}

tagKeepAliveConfig::tagKeepAliveConfig() {
	CKeepAliveConfig_Initialize(this);
}

/**
 * [RECONSTRUCTED - 0x00935E20]
 * CNetConfig_Initialize
 * Native implementation @ 0x00935E20 (148 bytes)
 */
tagNetConfig* CNetConfig_Initialize(tagNetConfig* pConfig) {
	if (!pConfig) {
		return nullptr;
	}

	// 00935e20 - 00935e23: Initialize keepalive settings at +0x24
	CKeepAliveConfig_Initialize(&pConfig->keepAlive);

	// 00935e2f - 00935e3e: Set 0s
	pConfig->dwFlag3        = 0;
	pConfig->dwFlag4        = 0;
	pConfig->dwMainTaskID_1 = 0;
	pConfig->dwFlag12       = 0;
	pConfig->dwFlag13       = 0;
	pConfig->dwMainTaskID_2 = 0;

	// 00935e44 - 00935e5f: Set 1s
	pConfig->dwFlag1  = 1;
	pConfig->dwFlag2  = 1;
	pConfig->dwFlag5  = 1;
	pConfig->dwFlag6  = 1;
	pConfig->dwFlag9  = 1;
	pConfig->dwFlag10 = 1;
	pConfig->dwFlag11 = 1;
	pConfig->dwFlag14 = 1;
	pConfig->dwFlag15 = 1;
	pConfig->dwFlag18 = 1;

	// 00935e68 - 00935e6b: Set -1 (0xFFFFFFFF)
	pConfig->dwMaxSendQueueDepth = 0xFFFFFFFF;
	pConfig->dwFlag16            = 0xFFFFFFFF;

	// 00935e77 - 00935e80: Set 0s
	pConfig->dwIP          = 0;
	pConfig->dwPort        = 0;
	pConfig->dwReserved1   = 0;
	pConfig->dwLogCallback = 0;

	// 00935e8a - 00935ea6: Protocol & Limits
	pConfig->byPadding          = 0;
	pConfig->byProtocol         = 2;
	pConfig->byMode             = 2;
	pConfig->dwKeepAliveTimeout = 60; // 0x3C
	pConfig->dwMaxConnections   = 0xFFFFFFFF;
	pConfig->dwMaxPacketSize    = 0xFFFFFFFF;
	pConfig->dwTraceCallback    = 0x0066B100; // Trace_RuntimeClassStub
	pConfig->bEnable            = 1;

	pConfig->dwDebugOptionDebuggerPresent = 0x0112;
	pConfig->dwDebugOptionStandAlone      = 0x0201;

	// 00935e83 - 00935ea8: Zero out 128 bytes of application name buffer at +0x94
	std::memset(pConfig->szAppName, 0, sizeof(pConfig->szAppName));

	return pConfig;
}

tagNetConfig::tagNetConfig() {
	CNetConfig_Initialize(this);
}

/**
 * [RECONSTRUCTED - 0x00935DD0]
 * CNetConfig_ConfigureServerDefaults
 * Native implementation @ 0x00935DD0 (sub_935dd0, 79 bytes)
 */
void CNetConfig_ConfigureServerDefaults(tagNetConfig* pConfig) {
	if (!pConfig) {
		return;
	}

	pConfig->dwMaxSendQueueDepth          = 0xFFFFFFFF;
	pConfig->dwFlag16                     = 0xFFFFFFFF;
	pConfig->dwIP                         = 3;
	pConfig->dwPort                       = 10;
	pConfig->dwKeepAliveTimeout           = 60;
	pConfig->dwReserved1                  = 0;
	pConfig->dwMaxConnections             = 1;
	pConfig->dwMaxPacketSize              = 1;
	pConfig->dwDebugOptionDebuggerPresent = 0x0112;
	pConfig->dwDebugOptionStandAlone      = 0x0201;
	pConfig->dwFlag18                     = 1;
	pConfig->dwFlag9                      = 0;
}
