/**
 * ============================================================================
 * Silkroad Online - GameServer Class Definition
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameServer.h
 *
 * Implements the concrete CGameServer daemon derived from ServerFramework::CServerApp:
 *   - Native VTable @ 0x00ADCE6C, RTTI: .?AVCGameServer@@
 *   - Native Ctor @ 0x00401040, Dtor @ 0x004010A0, Scalar Dtor @ 0x004010F0
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAMESERVER_H_
#define _SR_GAMESERVER_GAMESERVER_H_

#include "../JMX_ServerFramework/ServerFramework/ServerApp.h"

/**
 * CGameServer
 * Native Class: .?AVCGameServer@@
 * Native VTable @ 0x00ADCE6C
 */
class CGameServer : public ServerFramework::CServerApp {
public:
	CGameServer();
	virtual ~CGameServer() override;

	// Authentic virtual method overrides matching VTable @ 0x00ADCE6C:
	virtual bool InitInstance() override;                           // slot 2  (+0x08) @ 0x00401130
	virtual bool Initialize() override;                             // slot 7  (+0x1C) @ 0x00401710
	virtual bool OnServerReady() override;                          // slot 11 (+0x2C) @ 0x00401B70
	virtual bool Cleanup() override;                                // slot 12 (+0x30) @ 0x00401CD0
	virtual void DumpGObjMemoryStats() override;                    // slot 15 (+0x3C) @ 0x00401000
	virtual void ReloadQuestScripts() override;                     // slot 16 (+0x40) @ 0x00401020

	// Watchdog / Hang monitor thread management matching native 0x004016A0 / 0x00401390:
	static bool StartWatchdogThread();
	static unsigned long __stdcall WatchdogThreadProc(void* pParam);
};

// Global singleton instance (Native 0x00CC3880)
extern CGameServer g_gameServer;

// Global thread control matching native 0x00C825C0 / 0x00C66934
extern void* g_hWatchdogThread;
extern volatile uint32_t g_bWatchdogRunning;

// Forward declarations & Global Accessors matching native 0x00401D10 - 0x00401D50
class CShardDB;
class CMainProcess;
class CReferenceData;

CShardDB* GetShardDB();       // Native 0x00401D50
CMainProcess* GetMainProcess(); // Native 0x00401D30
CReferenceData* GetRefData(); // Native 0x00404CA0

#endif // _SR_GAMESERVER_GAMESERVER_H_
