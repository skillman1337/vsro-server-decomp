/**
 * ============================================================================
 * Joymax BSLib - Server FTP Manager Subsystem
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\FtpManager.h
 *
 * Implements native CServerFtpManager_Initialize @ 0x0095BB40:
 *   - Binds network engine pointer to FTP manager
 *   - Sets default command port / task ID (0x11)
 *   - Stores application module name
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_FTPMANAGER_H_
#define _JMX_LIBRARY_BSLIB_FTPMANAGER_H_

#include "NetEngine.h"
#include <string>

namespace BSLib {

// Native globals (0x00D67B80, 0x00D67B84, 0x00C67760)
extern IBSNet*     g_pFtpNetEngine;
extern uint32_t    g_dwFtpCommandPort;
extern std::string g_strFtpServerAppName;

/**
 * CServerFtpManager_Initialize
 * Native implementation @ 0x0095BB40
 */
bool InitializeFtpManager(const char* pszAppName);

/**
 * CServerFtpManager_Stop / BSLib_StopFtpManager
 * Native implementation @ 0x0095BB90
 */
bool StopFtpManager();

} // namespace BSLib

#endif // _JMX_LIBRARY_BSLIB_FTPMANAGER_H_
