/**
 * ============================================================================
 * Joymax BSLib - Server FTP Manager Subsystem Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\FtpManager.cpp
 *
 * Implements:
 *   - CServerFtpManager_Initialize @ 0x0095BB40
 *   - BSLib_StopFtpManager         @ 0x0095BB90
 *   - Native globals:
 *       g_pFtpNetEngine       @ 0x00D67B80
 *       g_dwFtpCommandPort    @ 0x00D67B84
 *       g_strFtpServerAppName @ 0x00C67760
 * ============================================================================
 */

#include "FtpManager.h"

namespace BSLib {

// Native globals (0x00D67B80, 0x00D67B84, 0x00C67760)
IBSNet*     g_pFtpNetEngine = nullptr;
uint32_t    g_dwFtpCommandPort = 0;
std::string g_strFtpServerAppName;

/**
 * CServerFtpManager_Initialize
 * Native implementation @ 0x0095BB40
 */
bool InitializeFtpManager(const char* pszAppName) {
	if (!g_pNetEngine) {
		return false;
	}

	g_pFtpNetEngine = g_pNetEngine;
	g_dwFtpCommandPort = 0x11;
	g_strFtpServerAppName = pszAppName ? pszAppName : "";

	return true;
}

/**
 * CServerFtpManager_Stop / BSLib_StopFtpManager
 * Native implementation @ 0x0095BB90
 *
 * Clears FTP connection list and resets g_dwFtpCommandPort to 0.
 */
bool StopFtpManager() {
	g_pFtpNetEngine = nullptr;
	g_dwFtpCommandPort = 0;
	g_strFtpServerAppName.clear();
	return true;
}

} // namespace BSLib
