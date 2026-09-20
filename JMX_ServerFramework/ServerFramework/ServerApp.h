/**
 * ============================================================================
 * Joymax ServerFramework - CServerApp Base Class
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerApp.h
 *
 * Implements the base server application class:
 *   - VTable @ 0x00B3FE2C (RTTI: .?AVCServerApp@ServerFramework@@)
 *   - Native Ctor @ 0x009355B0, Dtor @ 0x00935670, Scalar Dtor @ 0x00935650
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERAPP_H_
#define _JMX_SERVERFRAMEWORK_SERVERAPP_H_

#include "ServerConfig.h"
#include "../../JMX_Library/BSLib/Synch.h"
#include "../../JMX_Library/BSLib/NetEngine.h"
#include "../../JMX_Library/BSLib/NetConfig.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include <string>
#include <cstdint>

namespace ServerFramework {

class CServerApp;

/**
 * CServerApp_NetEngineMessageCallback
 * Native implementation @ 0x00935720 in ServerApp.cpp
 *
 * Callback passed to m_netConfig.dwTraceCallback @ 0x00935808.
 * Logs message occurrence count to channel 0x02000001.
 */
int CServerApp_NetEngineMessageCallback(uint32_t dwMsgID, int32_t nCount);

class CServerApp {
public:
	CServerApp();
	virtual ~CServerApp();

	// Framework Virtual Dispatch Interface (matching exact VTable @ 0x00B3FE2C):

	// [RECONSTRUCTED] slot 1 (+0x04) @ 0x009356F0
	virtual bool InitModule();

	// [STUB in base class / Overridden by CGameServer @ 0x00401130] slot 2 (+0x08) @ 0x00455EB0
	virtual bool InitInstance();

	// [RECONSTRUCTED] slot 3 (+0x0C) @ 0x00935740
	virtual bool ConfigureNetwork();

	// [STUB] slot 4 (+0x10) @ 0x0066B100 (Trace_RuntimeClassStub)
	virtual void PostConfigureNetwork();

	// [RECONSTRUCTED] slot 5 (+0x14) @ 0x00935760
	virtual bool PreInitialize();

	// [RECONSTRUCTED] slot 6 (+0x18) @ 0x009358E0
	virtual bool StartServerTasks();

	// [RECONSTRUCTED] slot 7 (+0x1C) @ 0x00935A00
	virtual bool Initialize();

	// [RECONSTRUCTED] slot 8 (+0x20) @ 0x00935AA0
	virtual bool RequestCertification();

	// [STUB in base class (DefaultTrueStub)] slot 9 (+0x24) @ 0x00455EB0
	virtual bool OnPreServerReady();

	// [RECONSTRUCTED] slot 10 (+0x28) @ 0x00935B00
	virtual bool CreateListener();

	// [STUB in base class / Overridden by CGameServer @ 0x00401B70] slot 11 (+0x2C) @ 0x00455EB0
	virtual bool OnServerReady();

	// [STUB in base class / Overridden by CGameServer @ 0x00401CD0] slot 12 (+0x30) @ 0x00455EB0
	virtual bool Cleanup();

	// [RECONSTRUCTED] slot 13 (+0x34) @ 0x00935DA0
	virtual bool OnCertificationComplete();

	// [RECONSTRUCTED] slot 14 (+0x38) @ 0x00935B60
	virtual bool StopServerTasks();

	// [NATIVE] slot 15 (+0x3C) @ 0x0066B100 (Default stub, overridden by CGameServer @ 0x00401000)
	virtual void DumpGObjMemoryStats();

	// [NATIVE] slot 16 (+0x40) @ 0x0066B100 (Default stub, overridden by CGameServer @ 0x00401020)
	virtual void ReloadQuestScripts();

	// [RECONSTRUCTED] slot 17 (+0x44) @ 0x009371F0
	virtual void OnIdle();

	// [RECONSTRUCTED] slot 18 (+0x48) @ 0x00935BD0
	virtual int32_t ProcessIocpPacket(void* pOverlapped);

	// [RECONSTRUCTED] slot 19 (+0x4C) @ 0x00935D70
	virtual uint32_t SetServerState(uint32_t nNewState);

	typedef int (*PFN_LOG_CALLBACK)(int32_t nChannel, const char* pszMsg, ...);
	void SetLogCallback(PFN_LOG_CALLBACK pCallback);
	PFN_LOG_CALLBACK GetLogCallback() const;

	const std::string& GetAppName() const;
	uint32_t GetServerState() const;
	bool IsActive() const;
	const CNetConfig& GetNetConfig() const;

protected:
	// Exact struct layout proven from 0x009355B0 / 0x00935670 / 0x00937A49:
	// +0x00: vptr (0x00B3FE2C)
	PFN_LOG_CALLBACK m_pLogCallback;    // +0x04: Log callback function pointer (set @ 0x00937A49)
	std::string      m_strAppName;      // +0x08: Module name (std::string default empty)
	uint32_t         m_dwFlags;         // +0x24: Runtime state flags (0)
	CNetConfig       m_netConfig;       // +0x28: BSLib Network Configuration (276 bytes / 0x114)
	uint32_t         m_nServerState;    // +0x13C: Current server state (SERVER_STATE_READY = 1)
	CNetEngine       m_netEngine;       // +0x140: IBSNet CNetEngine instance
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERAPP_H_
