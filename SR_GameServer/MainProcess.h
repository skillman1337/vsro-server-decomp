/**
 * ============================================================================
 * Silkroad Online - GameServer Main Process
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\MainProcess.h
 *
 * Implements CMainProcess derived from ServerFramework::CServerProcessMain:
 *   - Native VTable @ 0x00ADEFFC (RTTI: .?AVCMainProcess@@)
 *   - Native Ctor @ 0x004025F0, Dtor @ 0x00402790
 *   - Native Runtime Descriptor @ 0x00ADEBEC (size 0x424A8 = 271,528 bytes)
 *   - Native singleton pointer g_pMainProcess @ 0x00D6A8DC
 *   - Native Process (Heartbeat Loop) @ 0x00402870
 *   - Native ProcessMessage @ 0x00402A20
 *   - Native InitializeLocalData @ 0x00402830
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_MAINPROCESS_H_
#define _SR_GAMESERVER_MAINPROCESS_H_

#include "../JMX_ServerFramework/ServerFramework/ServerProcessMain.h"
#include "Game.h"
#include "Lobby.h"
#include "GameWorldMgr.h"
#include <cstdint>
#include <string>

class CMainProcess;

// Global singleton pointer matching native 0x00D6A8DC
extern CMainProcess* g_pMainProcess;

// [RECONSTRUCTED - Native 0x00404E70]
// Embedded shard announcement data matching native size 0xA8 (168 bytes)
struct tagShardNotice {
	uint32_t    m_dwNoticeID;        // +0x00
	uint8_t     m_pad04[4];          // +0x04
	std::string m_strNotice;         // +0x08
	uint8_t     m_pad24[0x70];       // +0x24 to +0x94
	uint8_t     m_byStartHour;       // +0x94
	uint8_t     m_byEndHour;         // +0x95
	uint8_t     m_pad96[0x12];       // +0x96 to +0xA8
};

class CMainProcess : public ServerFramework::CServerProcessMain {
public:
	// [RECONSTRUCTED - Native 0x004025F0]
	CMainProcess();

	// [RECONSTRUCTED - Native 0x00402790]
	virtual ~CMainProcess() override;

	// slot 0 (+0x00) @ 0x004025E0: GetRuntimeClass
	virtual const CRuntimeClass* GetRuntimeClass() const override;

	// [RECONSTRUCTED - Native 0x00402870]
	// slot 3 (+0x0C): Core Heartbeat and Message Loop
	// Dequeues C2S packets with 3000ms timeslice guard, drives CGame::Tick at 30Hz
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) override;

	// [RECONSTRUCTED - Native 0x00402A20]
	// slot 6 (+0x18): Registers client packet handlers (0x3012, 0x6208, 0x7802, etc.)
	virtual int32_t ProcessMessage() override;

	// [RECONSTRUCTED - Native 0x00402830]
	// Initializes world partition grids, NPC state machines, and local caches
	virtual bool InitializeLocalData();

	// [RECONSTRUCTED - Native 0x00402C20]
	// slot 9 (+0x24): Dispatches game simulation packets to CGame::ProcessClientGamePacket
	virtual int32_t OnGamePacket(BSLib::CPacket* pMsg);

	// [RECONSTRUCTED - Native 0x00402C90]
	// Opcode 0x6208: Session switch handover from Gateway/Agent
	int32_t OnSwitchSessionReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00402DA0]
	// Opcode 0x7802: Lobby join request
	int32_t OnLobbyJoinReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00402EB0]
	// Opcode 0x7803: Character selection and enter game request
	int32_t OnEnterGameReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00403350]
	// Opcode 0x3012: Client ready to play / spawn player into world
	int32_t OnReadyToPlay(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00403520]
	// Opcode 0x7804: Leave lobby / return to character selection
	int32_t OnLobbyLeaveReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x004036A0]
	// Opcode 0x3C06: Shard operation and schedule notice
	int32_t OnShardNotice(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00402E60]
	// Opcode 0x34B6: Reset client session acknowledgment
	int32_t OnResetClientAck(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00403620]
	// Opcode 0x6903: Character state synchronization
	int32_t OnUpdateCharacterState(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00403840]
	// Opcode 0x3D02: Guild war status notification
	int32_t OnGuildWarNotify(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x004038F0]
	// Opcode 0x3510: Battle arena match notification
	int32_t OnArenaMatchNotify(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved);

	// [RECONSTRUCTED - Native 0x00402C50]
	// slot 40 (+0xA0): Inter-server link connected handler
	virtual void OnServerConnected(void* pNode, void* pLink);

	// [RECONSTRUCTED - Native 0x00402C70]
	// slot 41 (+0xA4): Inter-server link disconnected handler
	virtual void OnServerDisconnected(void* pNode);

	// [RECONSTRUCTED - Native 0x00402AF0]
	// slot 42 (+0xA8): Inter-server cluster node state transition handler
	virtual void OnServerNodeStateChanged(void* pNode, uint32_t nNewState, void* pParam);

	// [RECONSTRUCTED - Native 0x00402BF0]
	// Resolves active communication session link for a given ServerID
	int32_t GetServerSessionByServerID(uint16_t wServerID);

	// Sends network message buffer to designated session and handles buffer release
	int32_t SendMsg(uint32_t dwSessionID, BSLib::CMsg* pMsg);

	// [RECONSTRUCTED - Native 0x00403310]
	// Logs character position validation failure during world login
	void LogCharLoginFailedPos(CGObjChar* pChar);

	// [RECONSTRUCTED - Native 0x00403600]
	// Dispatches target message to internal worker task queue
	bool DispatchMessageToTarget(void* pMsg, void* pTarget);

	// [RECONSTRUCTED - Native 0x00403730]
	// Validates world partition bounds and relocates player if coordinates are invalid
	void ValidateAndRelocateInvalidGameWorld(CGObjChar* pChar, int32_t nEntryType);

	// [RECONSTRUCTED - Native 0x004E7350]
	// Submits async learned skill/mastery change to ShardDB overlap task
	bool SubmitLearnedChangeDBQuery(int32_t nKind, uint32_t dwCharID, uint32_t dwParam1, uint32_t dwParam2);

	uint32_t GetLastHeartbeatTick() const;
	void SetLastHeartbeatTick(uint32_t dwTick);

	CGame& GetGame();

	static void* CreateObject();
	static void DestroyObject(void* pObj);

	static const CRuntimeClass ms_runtimeClass;

public:
	// Native offset +0x400CC: Last tick record for delta-time calculation
	uint32_t m_dwLastTick;

	// Native offset +0x400D0: Embedded CGame world simulation instance
	CGame m_game;

	// Native offset +0x423B0: Embedded CLobby session manager
	CLobby m_lobby;

	// Native offset +0x423E0: 8 task/subsystem context pointers (MainProcess + 7 Overlap workers)
	union {
		void* m_pSubsystems[8];
		void* m_pTasks[8];
	};

	// Native offset +0x42400: Embedded Shard notice data (size 0xA8, total size 0x424A8)
	tagShardNotice m_shardNotice;
};

#endif // _SR_GAMESERVER_MAINPROCESS_H_
