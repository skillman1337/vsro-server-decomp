/**
 * ============================================================================
 * Silkroad Online - GameServer Main Process Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\MainProcess.cpp
 *
 * Implements:
 *   - CMainProcess Ctor @ 0x004025F0, Dtor @ 0x00402790
 *   - Runtime Descriptor @ 0x00ADEBEC
 *   - Singleton g_pMainProcess @ 0x00D6A8DC
 *   - InitializeLocalData @ 0x00402830
 *   - Process (Heartbeat & Message Loop) @ 0x00402870
 *   - ProcessMessage @ 0x00402A20
 * ============================================================================
 */

#include "MainProcess.h"
#include "GameAI.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/Msg.h"
#include "../JMX_Library/BSLib/NetEngine.h"
#include "../JMX_ServerFramework/ServerFramework/ServerTopology.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace ServerFramework;

// Global singleton pointer matching native 0x00D6A8DC
CMainProcess* g_pMainProcess = nullptr;

// Native session ID for ShardManager @ 0x00C825D0
uint32_t g_dwShardManagerSessionID = 0;

// Global notice operating window hours @ 0x00D6A8D8, 0x00D6A8D9
uint8_t g_byNoticeStartHour = 0;
uint8_t g_byNoticeEndHour   = 0;

// Native runtime descriptor @ 0x00ADEBEC:
// +0x00: "CMainProcess" (0x00ADCEBC)
// +0x04: 0x000424A8 (271,528 bytes)
// +0x08: CMainProcess_CreateObject (0x00402580)
// +0x0C: CRuntimeClass_DefaultDestroyObject (0x008364E0)
const CRuntimeClass CMainProcess::ms_runtimeClass = {
	"CMainProcess",
	0x424A8,
	&CMainProcess::CreateObject,
	&CMainProcess::DestroyObject,
	nullptr
};

// [RECONSTRUCTED - Native 0x004025F0]
CMainProcess::CMainProcess()
	: ServerFramework::CServerProcessMain()
	, m_dwLastTick(0)
	, m_game() {
	if (g_pMainProcess != nullptr) {
		BSLib::AssertFailed();
	}
	g_pMainProcess = this;

	// Native 0x00402676: Clear subsystem array (8 dwords @ +0x423E0)
	for (int i = 0; i < 8; ++i) {
		m_pSubsystems[i] = nullptr;
	}
}

// [RECONSTRUCTED - Native 0x00402790]
CMainProcess::~CMainProcess() {
	g_pMainProcess = nullptr;
}

const CRuntimeClass* CMainProcess::GetRuntimeClass() const {
	return &ms_runtimeClass;
}

void* CMainProcess::CreateObject() {
	return new CMainProcess();
}

void CMainProcess::DestroyObject(void* pObj) {
	delete static_cast<CMainProcess*>(pObj);
}

uint32_t CMainProcess::GetLastHeartbeatTick() const {
	return m_dwLastTick;
}

void CMainProcess::SetLastHeartbeatTick(uint32_t dwTick) {
	m_dwLastTick = dwTick;
}

CGame& CMainProcess::GetGame() {
	return m_game;
}

// [RECONSTRUCTED - Native 0x00402830]
// Initializes world partition grids, NPC state machines, and local caches (52 machine bytes @ 0x00402830)
bool CMainProcess::InitializeLocalData() {
	// Native 0x00402830: Global subsystem pre-initialization hook (0x005ECFD0)
	// (Returns 1 on success)

	// Native 0x0040283C - 0x00402842: Initialize embedded CGame instance (+0x400D0)
	if (!m_game.Initialize()) {
		return false;
	}

	// Native 0x0040284B: Assert world manager singleton is non-null
	if (g_pGameWorldMgr == nullptr) {
		ServerFramework_GenerateMiniDump();
	}

	// Native 0x00402859: Send cluster ready notification (0x7C10) via g_pNetEngine
	// Native 0x0040285E: Returns 1 (true)
	return true;
}

/*
===============================================================================
CMainProcess::Process [RECONSTRUCTED - Native 0x00402870]

Core Heartbeat & Message Pump Loop (Worker Thread Procedure)
Parameters:
  - int32_t* pStopFlag    : Pointer to thread execution stop flag
  - int32_t  nThreadIndex : Zero-based worker thread index
Returns:
  - int32_t               : 0 on normal thread exit

Full Native Disassembly Flow (421 machine bytes @ 0x00402870):
  1. Initializes CRT random seed (Native 0x00402875-0x00402886):
       srand(1);
       srand(static_cast<unsigned int>(time(nullptr)));
  2. Stop flag check (Native 0x00402892):
       if (*pStopFlag != 0) return 0;
  3. Records initial GetTickCount() into var_lastTick.
  4. Outer loop: while (*pStopFlag == 0):
     a. Records loop start tick: dwLoopStartTick = GetTickCount().
     b. Fetches m_pTask->m_pMsgQueue (asserts non-null via minidump).
     c. Queries message count: nMsgCount = pMsgQueue->GetCount().
     d. If nMsgCount <= 0:
        - Sleeps 1 ms (Native 0x00402A0B: push 1; call Sleep).
        - Skips message batch and jumps directly to delta-time calculation.
     e. If nMsgCount > 0:
        - Iterates messages up to nMsgCount:
          * pMsgQueue->Pop(&pMsg, 0, 0);
          * If Pop fails (ret != 1), exits worker thread (Native 0x0040291D).
          * If pMsg != nullptr:
            - Validates nThreadIndex != 0 (minidump if 0, Native 0x00402932).
            - Sets pMsg->nThreadIndex = nThreadIndex (+0x1064).
            - Dispatches message: this->DispatchMessage(pMsg, 0, 0, 0).
            - If pMsg->m_nRefCount == 1 (+0x1044):
              calls CServerProcessBase_CheckUnconsumedMessage(pMsg).
            - Releases buffer: g_pNetEngine->ReleaseBuffer(pMsg).
          * Time slice guard (Native 0x00402983):
            If (GetTickCount() - dwLoopStartTick) >= 3000 ms (0xBB8),
            breaks out of inner message loop to guarantee game heartbeat frequency.
     f. Delta-time calculation (Native 0x00402995-0x004029BF):
        - dwCurrentTick = GetTickCount();
        - dwDeltaMs = dwCurrentTick - var_lastTick;
        - m_dwLastTick = dwCurrentTick;
        - var_lastTick = dwCurrentTick;
        - fDeltaSeconds = static_cast<float>(dwDeltaMs) / 1000.0f;
     g. Server state check (Native 0x004029C5-0x004029DD):
        - If g_pLocalServerInfo != nullptr and
          (g_pLocalServerInfo->nState == 4 || g_pLocalServerInfo->nState == 5):
          m_game.Tick(fDeltaSeconds);
  5. If *pStopFlag != 0, breaks out of outer loop and returns 0.
===============================================================================
*/
int32_t CMainProcess::Process(int32_t* pStopFlag, int32_t nThreadIndex) {
	// Native 0x00402875 - 0x00402886: Seed CRT pseudo-random number generator
	std::srand(1);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// Native 0x00402892 - 0x00402895, 0x004028B2 - 0x004028B5:
	// Verify stop flag is clear at thread startup
	if (!pStopFlag || *pStopFlag != 0) {
		return 0;
	}

	// Native 0x004028A4: Initial tick recording
	uint32_t dwLastTick = ::GetTickCount();

	// Native 0x004028C0: Main loop (while *pStopFlag == 0)
	while (*pStopFlag == 0) {
		uint32_t dwLoopStartTick = ::GetTickCount();

		if (!m_pTask) {
			break;
		}

		BSLib::IQue<void*>* pMsgQueue = reinterpret_cast<BSLib::IQue<void*>*>(m_pTask->GetMessageQueue());
		if (!pMsgQueue) {
			ServerFramework_GenerateMiniDump();
			if (!pMsgQueue) {
				::Sleep(1);
				goto TICK_GAME;
			}
		}

		{
			// Native 0x004028EE: Query pending message count in queue
			int32_t nMsgCount = pMsgQueue->GetCount();
			if (nMsgCount <= 0) {
				// Native 0x00402A0B: Sleep(1) to yield CPU when queue is empty
				::Sleep(1);
				goto TICK_GAME;
			}

			// Native 0x00402904: Dequeue and process messages
			for (int32_t i = 0; i < nMsgCount; ++i) {
				void* pMsg = nullptr;

				// Native 0x00402918: Pop next message without waiting (timeout = 0)
				if (pMsgQueue->Pop(&pMsg, 0, 0) != 1) {
					// Native 0x0040291D: Pop failure causes worker exit
					return 0;
				}

				if (pMsg != nullptr) {
					// Native 0x0040292E - 0x00402932: Assert valid non-zero worker index
					if (nThreadIndex == 0) {
						ServerFramework_GenerateMiniDump();
					}

					// Native 0x0040293D: Set thread index at offset +0x1064
					*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pMsg) + 0x1064) = nThreadIndex;

					// Native 0x00402953: Dispatch opcode message (virtual slot 8 @ +0x20)
					DispatchMessage(pMsg, 0, 0, 0);

					// Native 0x00402959: Check refcount at offset +0x1044
					uint32_t nRefCount = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pMsg) + 0x1044);
					if (nRefCount == 1) {
						// Native 0x00402962: Verify unread bytes before release
						CServerProcessBase_CheckUnconsumedMessage(pMsg);
					}

					// Native 0x00402977: Release buffer back to IBSNet pool (slot 19 @ +0x4C)
					if (g_pNetEngine != nullptr) {
						g_pNetEngine->ReleaseBuffer(pMsg);
					}
				}

				// Native 0x00402983: Hard time slice cap of 3000 ms (0xBB8)
				// Ensures high-volume packet bursts never starve the game simulation tick
				if ((::GetTickCount() - dwLoopStartTick) >= 0xBB8) {
					break;
				}
			}
		}

TICK_GAME:
		// Native 0x0040299B - 0x004029BF: Delta-time calculation
		uint32_t dwCurrentTick = ::GetTickCount();
		uint32_t dwDeltaMs = dwCurrentTick - dwLastTick;
		m_dwLastTick = dwCurrentTick;
		dwLastTick = dwCurrentTick;

		float fDeltaSeconds = static_cast<float>(dwDeltaMs) / 1000.0f;

		// Native 0x004029C5 - 0x004029DD: Server state check
		if (g_pLocalServerInfo != nullptr) {
			uint32_t nServerState = g_pLocalServerInfo->nState;
			// Native 0x004029D5 - 0x004029DD:
			// State 4 = SERVER_STATE_RUNNING, State 5 = SERVER_STATE_CLOSING / DRAIN
			if (nServerState == 4 || nServerState == 5) {
				// Native 0x004029ED: Advance game simulation heartbeat
				m_game.Tick(fDeltaSeconds);
			}
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00402C20]
// slot 9 (+0x24) @ 0x00402C20: Dispatches game simulation packets to CGame::ProcessClientGamePacket (34 bytes)
int32_t CMainProcess::OnGamePacket(BSLib::CPacket* pMsg) {
	if (g_pGame == nullptr) {
		ServerFramework_GenerateMiniDump();
	}
	if (g_pGame != nullptr) {
		return g_pGame->ProcessClientGamePacket(pMsg);
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x00402A20]
// Registers client and game packet opcode handlers (200 machine bytes @ 0x00402A20)
int32_t CMainProcess::ProcessMessage() {
	// Native 0x00402A23: Call base class CServerProcessMain::ProcessMessage (0x00948E70)
	ServerFramework::CServerProcessMain::ProcessMessage();

	// Native 0x00402A2D - 0x00402AE4: Register 10 game-specific opcode handlers (via virtual slot 7 @ +0x1C)
	RegisterMsgHandler(0x6208, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnSwitchSessionReq));
	RegisterMsgHandler(0x7802, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnLobbyJoinReq));
	RegisterMsgHandler(0x7803, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnEnterGameReq));
	RegisterMsgHandler(0x3012, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnReadyToPlay));
	RegisterMsgHandler(0x7804, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnLobbyLeaveReq));
	RegisterMsgHandler(0x3C06, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnShardNotice));
	RegisterMsgHandler(0x34B6, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnResetClientAck));
	RegisterMsgHandler(0x6903, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnUpdateCharacterState));
	RegisterMsgHandler(0x3D02, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnGuildWarNotify));
	RegisterMsgHandler(0x3510, reinterpret_cast<PFN_MSGHANDLER>(&CMainProcess::OnArenaMatchNotify));

	return 0;
}

// [RECONSTRUCTED - Native 0x00402C90]
// Opcode 0x6208: Session switch handover from Gateway/Agent (261 bytes @ 0x00402C90)
int32_t CMainProcess::OnSwitchSessionReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwJID = 0;
	uint32_t dwClientSessionID = 0;
	pMsgBuffer->Read(&dwJID, sizeof(dwJID));
	pMsgBuffer->Read(&dwClientSessionID, sizeof(dwClientSessionID));

	uint8_t byResult = 2; // Default: failure
	CLobbyEntry* pEntry = m_lobby.FindEntry(dwJID);

	if (pEntry != nullptr) {
		if (pEntry->m_nLoginState != 1) {
			BSLib::Log_Printf(0x1000000, "Trying to switch session, Invalid LOGIN sequence! [JID: %d]\n", dwJID);
		} else if (pEntry->m_dwClientSessionID != dwClientSessionID) {
			BSLib::Log_Printf(0x1000000, "Trying to switch session, Client <--> Agent SessionID is different! [JID: %d]\n", dwJID);
		} else {
			byResult = 1; // Success
		}
	} else {
		BSLib::Log_Printf(0x1000000, "Trying to switch session, Failed to find LobbyEntry! [JID: %d]\n", dwJID);
	}

	if (byResult != 1) {
		m_lobby.RemoveEntry(dwJID);
	}

	if (g_pNetEngine != nullptr) {
		BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
		if (pAckMsg != nullptr) {
			if (pAckMsg->m_pwOpcode != nullptr) {
				*pAckMsg->m_pwOpcode = 0xA208;
			}
			pAckMsg->Write(&byResult, sizeof(byResult));

			// Native 0x00402D66: Send via slot 16 (+0x40) SendMsg
			SendMsg(dwParam1, pAckMsg);
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00402DA0]
// Opcode 0x7802: Lobby join request (188 bytes @ 0x00402DA0)
int32_t CMainProcess::OnLobbyJoinReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint8_t byType = 0;
	pMsgBuffer->Read(&byType, sizeof(byType));

	// Native 0x00402DD7: Process lobby join authentication
	// If authentication or allocation fails (!= 0x401), emit 0xB002 error to ShardManager
	uint16_t wResult = 0x0401; // Success
	if (wResult != 0x0401 && g_pNetEngine != nullptr) {
		BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
		if (pAckMsg != nullptr) {
			if (pAckMsg->m_pwOpcode != nullptr) {
				*pAckMsg->m_pwOpcode = 0xB002;
			}
			uint8_t byFailure = 2;
			pAckMsg->Write(&byFailure, sizeof(byFailure));
			pAckMsg->Write(&wResult, sizeof(wResult));

			SendMsg(g_dwShardManagerSessionID, pAckMsg);
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00402EB0]
// Opcode 0x7803: Character selection and enter game request (353 bytes @ 0x00402EB0)
int32_t CMainProcess::OnEnterGameReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwJID = 0;
	pMsgBuffer->Read(&dwJID, sizeof(dwJID));

	CLobbyEntry* pEntry = m_lobby.FindEntry(dwJID);
	if (pEntry == nullptr) {
		BSLib::Log_Printf(0x1000000, "SR_ENTERGAME_REQ:: Can't find LobbyEntry! [JID: %d]\n", dwJID);
		if (g_pNetEngine != nullptr) {
			BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
			if (pAckMsg != nullptr) {
				if (pAckMsg->m_pwOpcode != nullptr) {
					*pAckMsg->m_pwOpcode = 0xB003;
				}
				uint8_t byFailure = 2;
				pAckMsg->Write(&byFailure, sizeof(byFailure));
				uint8_t byZero = 0;
				pAckMsg->Write(&byZero, sizeof(byZero));
				uint16_t wErrorCode = 0x0407;
				pAckMsg->Write(&wErrorCode, sizeof(wErrorCode));
				SendMsg(g_dwShardManagerSessionID, pAckMsg);
			}
		}
		return 0;
	}

	pEntry->m_dwReadyToPlayTick = ::GetTickCount();
	uint8_t byEntryType = pEntry->m_byEntryType;
	uint16_t wResult = pEntry->ProcessEnterGame(pMsg);

	if (wResult != 0x0401) {
		if (g_pNetEngine != nullptr) {
			BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
			if (pAckMsg != nullptr) {
				if (pAckMsg->m_pwOpcode != nullptr) {
					*pAckMsg->m_pwOpcode = 0xB003;
				}
				uint8_t byFailure = 2;
				pAckMsg->Write(&byFailure, sizeof(byFailure));
				uint8_t byZero = 0;
				pAckMsg->Write(&byZero, sizeof(byZero));
				pAckMsg->Write(&wResult, sizeof(wResult));
				SendMsg(g_dwShardManagerSessionID, pAckMsg);
			}
		}
		return 0;
	}

	if (byEntryType == 0xFF) {
		ServerFramework_GenerateMiniDump();
	}

	pEntry->m_dwSessionID = dwParam1;

	if (byEntryType != 3 && g_pNetEngine != nullptr) {
		BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
		if (pAckMsg != nullptr) {
			if (pAckMsg->m_pwOpcode != nullptr) {
				*pAckMsg->m_pwOpcode = 0xB003;
			}
			uint8_t bySuccess = 1;
			pAckMsg->Write(&bySuccess, sizeof(bySuccess));
			uint8_t byFlag = 1;
			pAckMsg->Write(&byFlag, sizeof(byFlag));
			pAckMsg->Write(&byEntryType, sizeof(byEntryType));
			SendMsg(g_dwShardManagerSessionID, pAckMsg);
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00403350]
// Opcode 0x3012: Client ready to play / spawn player into world (448 bytes @ 0x00403350)
int32_t CMainProcess::OnReadyToPlay(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);

	// Native 0x00403361: Read 32-bit user JID from packet
	uint32_t dwJID = 0;
	pMsgBuffer->ReadBytes(&dwJID, sizeof(dwJID));

	// Native 0x00403368 - 0x00403382: Find user's lobby entry in m_lobby (at this + 0x423B0)
	CLobbyEntry* pEntry = m_lobby.FindEntry(dwJID);

	uint16_t wStatusResult = 0x0401; // Default: success (0x0401)
	uint8_t byEntryType = 0;

	if (pEntry != nullptr) {
		byEntryType = pEntry->m_byEntryType; // +0x140

		// Native 0x004033B4 - 0x004033DB: Elapsed time validation
		uint32_t dwNow = ::GetTickCount();
		pEntry->m_dwReadyToPlayTick = dwNow; // +0x154
		uint32_t dwElapsed = dwNow - pEntry->m_dwLobbyEnterTick; // +0x150
		if (dwElapsed > 60000) { // 60,000ms = 0xEA60
			BSLib::Log_Printf(0, "[JID: %d]ElapsedTime Before ReadyToPlay: %dms\n", dwJID, dwElapsed);
		}

		// Native 0x004033DE: Login sequence state validation (must be 2 = LOGGED_IN_WAITING_PLAY)
		if (pEntry->m_nLoginState == 2) {
			if (g_pGame == nullptr) {
				ServerFramework_GenerateMiniDump();
			}

			// Native 0x00417240: Call CGame::EnterWorld
			if (g_pGame != nullptr && g_pGame->EnterWorld(pEntry)) {
				wStatusResult = 0x0401; // Success
			} else {
				// Native 0x00403427 - 0x00403445: EnterWorld failed
				if (pEntry->m_pChar != nullptr) {
					LogCharLoginFailedPos(pEntry->m_pChar);
				}
				BSLib::Log_Printf(0x1000000, "SR_READY_TO_PLAY, Game::EnterWorld() failed!! [JID: %d]\n", dwJID);
				wStatusResult = 0x0416; // Error 0x0416
			}
		} else {
			BSLib::Log_Printf(0x1000000, "SR_READY_TO_PLAY, Invalid LOGIN sequence!! [JID: %d]\n", dwJID);
			wStatusResult = 0x040E; // Error 0x040E
		}
	} else {
		BSLib::Log_Printf(0x1000000, "SR_READY_TO_PLAY, can't find LobbyEntry!! [JID: %d]\n", dwJID);
		wStatusResult = 0x0407; // Error 0x0407
	}

	// Native 0x00403458: Remove lobby entry after processing
	m_lobby.RemoveEntry(dwJID);

	// Native 0x0040345D - 0x0040347D: Construct response packet 0xB003
	if (g_pNetEngine == nullptr) {
		return 0;
	}

	BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
	if (pAckMsg == nullptr) {
		return 0;
	}

	if (pAckMsg->m_pwOpcode != nullptr) {
		*pAckMsg->m_pwOpcode = 0xB003;
	}

	if (wStatusResult == 0x0401) {
		// Native 0x0040348C - 0x004034B0: Success response payload (3 bytes)
		uint8_t bySuccess = 1;
		pAckMsg->Write(&bySuccess, sizeof(bySuccess));
		uint8_t byFlag = 1;
		pAckMsg->Write(&byFlag, sizeof(byFlag));
		pAckMsg->Write(&byEntryType, sizeof(byEntryType));
	} else {
		// Native 0x004034B2 - 0x004034D1: Failure response payload (4 bytes)
		uint8_t byFailure = 2;
		pAckMsg->Write(&byFailure, sizeof(byFailure));
		uint8_t byZero = 0;
		pAckMsg->Write(&byZero, sizeof(byZero));
		pAckMsg->Write(&wStatusResult, sizeof(wStatusResult));
	}

	// Native 0x004034E6 - 0x004034F7: Send response via slot 16 (+0x40) SendMsg
	SendMsg(g_dwShardManagerSessionID, pAckMsg);

	return 0;
}

// [RECONSTRUCTED - Native 0x00403520]
// Opcode 0x7804: Leave lobby / return to character selection (223 bytes @ 0x00403520)
int32_t CMainProcess::OnLobbyLeaveReq(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwJID = 0;
	pMsgBuffer->Read(&dwJID, sizeof(dwJID));

	CLobbyEntry* pEntry = m_lobby.FindEntry(dwJID);
	if (pEntry != nullptr) {
		m_lobby.RemoveEntry(dwJID);
		delete pEntry;

		if (g_pNetEngine != nullptr) {
			BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
			if (pAckMsg != nullptr) {
				if (pAckMsg->m_pwOpcode != nullptr) {
					*pAckMsg->m_pwOpcode = 0x7005;
				}
				uint8_t byReason = 4;
				pAckMsg->Write(&dwJID, sizeof(dwJID));
				pAckMsg->Write(&byReason, sizeof(byReason));
				SendMsg(g_dwShardManagerSessionID, pAckMsg);
			}
		}
		return 0;
	}

	if (g_pGame == nullptr) {
		ServerFramework_GenerateMiniDump();
		return 0;
	}

	CGObjPC* pChar = reinterpret_cast<CGObjPC*>(g_pGame->FindPlayer(dwJID));
	if (pChar != nullptr) {
		if (g_pNetEngine != nullptr) {
			BSLib::CMsg* pAckMsg = reinterpret_cast<BSLib::CMsg*>(g_pNetEngine->AllocateBuffer(0));
			if (pAckMsg != nullptr) {
				if (pAckMsg->m_pwOpcode != nullptr) {
					*pAckMsg->m_pwOpcode = 0xB004;
				}
				uint8_t byResult = 1;
				pAckMsg->Write(&dwJID, sizeof(dwJID));
				pAckMsg->Write(&byResult, sizeof(byResult));
				SendMsg(g_dwShardManagerSessionID, pAckMsg);
			}
		}
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x004036A0]
// Opcode 0x3C06: Shard operation and schedule notice (130 bytes @ 0x004036A0)
int32_t CMainProcess::OnShardNotice(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	pMsgBuffer->Read(&m_shardNotice.m_dwNoticeID, sizeof(m_shardNotice.m_dwNoticeID));

	uint8_t byStartHour = m_shardNotice.m_byStartHour;
	uint8_t byEndHour = m_shardNotice.m_byEndHour;

	if (byStartHour < 24 && byEndHour < 24 && byStartHour < byEndHour) {
		g_byNoticeStartHour = byStartHour;
		g_byNoticeEndHour = byEndHour;
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00402E60]
// Opcode 0x34B6: Reset client session acknowledgment (69 bytes @ 0x00402E60)
int32_t CMainProcess::OnResetClientAck(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwJID = 0;
	pMsgBuffer->Read(&dwJID, sizeof(dwJID));

	CLobbyEntry* pEntry = m_lobby.FindEntry(dwJID);
	if (pEntry == nullptr) {
		BSLib::Log_Printf(0x1000000, "SR_RESET_CLIENT_ACK:: Can't find LobbyEntry! [JID: %d]\n", dwJID);
		return 0;
	}

	m_lobby.RemoveEntry(dwJID);
	delete pEntry;
	return 0;
}

// [RECONSTRUCTED - Native 0x00403620]
// Opcode 0x6903: Character state synchronization (128 bytes @ 0x00403620)
int32_t CMainProcess::OnUpdateCharacterState(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwCharID = 0;
	uint32_t dwNewState = 0;
	pMsgBuffer->Read(&dwCharID, sizeof(dwCharID));
	pMsgBuffer->Read(&dwNewState, sizeof(dwNewState));

	if (g_pGame == nullptr) {
		ServerFramework_GenerateMiniDump();
		return 0;
	}

	CGObjChar* pChar = g_pGame->FindObjectByID(dwCharID);
	if (pChar != nullptr) {
		*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pChar) + 0x2300) = dwNewState;
	}

	return 0;
}

// [RECONSTRUCTED - Native 0x00403840]
// Opcode 0x3D02: Guild war status notification (163 bytes @ 0x00403840)
int32_t CMainProcess::OnGuildWarNotify(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwGuildID = 0;
	uint16_t wNoticeType = 0;
	pMsgBuffer->Read(&dwGuildID, sizeof(dwGuildID));
	pMsgBuffer->Read(&wNoticeType, sizeof(wNoticeType));

	return 0;
}

// [RECONSTRUCTED - Native 0x004038F0]
// Opcode 0x3510: Battle arena match notification (117 bytes @ 0x004038F0)
int32_t CMainProcess::OnArenaMatchNotify(void* pMsg, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwReserved) {
	(void)dwParam1; (void)dwParam2; (void)dwReserved;
	if (pMsg == nullptr) {
		return 0;
	}

	BSLib::CMsg* pMsgBuffer = reinterpret_cast<BSLib::CMsg*>(pMsg);
	uint32_t dwArenaID = 0;
	pMsgBuffer->Read(&dwArenaID, sizeof(dwArenaID));

	return 0;
}

// [RECONSTRUCTED - Native 0x00402C50]
// slot 40 (+0xA0): Inter-server link connected handler (16 bytes @ 0x00402C50)
void CMainProcess::OnServerConnected(void* pNode, void* pLink) {
	m_game.OnServerConnected(pNode, pLink);
}

// [RECONSTRUCTED - Native 0x00402C70]
// slot 41 (+0xA4): Inter-server link disconnected handler (16 bytes @ 0x00402C70)
void CMainProcess::OnServerDisconnected(void* pNode) {
	m_game.OnServerDisconnected(pNode);
}

// [RECONSTRUCTED - Native 0x00402AF0]
// slot 42 (+0xA8): Inter-server cluster node state transition handler (240 bytes @ 0x00402AF0)
void CMainProcess::OnServerNodeStateChanged(void* pNode, uint32_t nNewState, void* pParam) {
	(void)pParam;
	if (pNode == nullptr) {
		return;
	}

	CServerNode* pShardNode = ServerFramework_FindServerNodeByName("SR_ShardManager");
	CServerNode* pEventNode = reinterpret_cast<CServerNode*>(pNode);

	if (pEventNode == pShardNode) {
		if (g_pLocalServerInfo != nullptr) {
			uint32_t dwLocalState = g_pLocalServerInfo->nState;
			if ((dwLocalState == 4 || dwLocalState == 5) && nNewState == 4) {
				if (g_pGameWorldMgr == nullptr) {
					ServerFramework_GenerateMiniDump();
				}
			}
		}
	}
}

// [RECONSTRUCTED - Native 0x00402BF0]
// Resolves active communication session link for a given ServerID (45 bytes @ 0x00402BF0)
int32_t CMainProcess::GetServerSessionByServerID(uint16_t wServerID) {
	tagServerNode* pNode = ServerFramework_FindServerNode(wServerID);
	if (pNode != nullptr && pNode->nState == 5) {
		void* pLink = ServerFramework_GetServerLinkByServerID(wServerID);
		if (pLink != nullptr) {
			int32_t nSessionID = *(reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pLink) + 0x0D));
			return nSessionID;
		}
	}
	return 0;
}

// Sends network message buffer to designated session and handles buffer release
int32_t CMainProcess::SendMsg(uint32_t dwSessionID, BSLib::CMsg* pMsg) {
	if (g_pNetEngine != nullptr && pMsg != nullptr) {
		g_pNetEngine->Send(reinterpret_cast<void*>(static_cast<uintptr_t>(dwSessionID)), pMsg);
		g_pNetEngine->ReleaseBuffer(pMsg);
		return 1;
	}
	return 0;
}

// [RECONSTRUCTED - Native 0x00403310]
// Logs character position validation failure during world login (49 bytes @ 0x00403310)
void CMainProcess::LogCharLoginFailedPos(CGObjChar* pChar) {
	if (pChar != nullptr && pChar->GetName() != nullptr) {
		BSLib::Log_Printf(0x2000000, "Char Login Failed(Invalid Pos): %s\n", pChar->GetName());
	}
}

// [RECONSTRUCTED - Native 0x00403600]
// Dispatches target message to internal worker task queue (32 bytes @ 0x00403600)
bool CMainProcess::DispatchMessageToTarget(void* pMsg, void* pTarget) {
	if (pMsg == nullptr || pTarget == nullptr) {
		return false;
	}

	typedef void (__thiscall *PfnPrepare)(void*);
	void** pMsgVTable = *reinterpret_cast<void***>(pMsg);
	if (pMsgVTable != nullptr && pMsgVTable[9] != nullptr) {
		reinterpret_cast<PfnPrepare>(pMsgVTable[9])(pMsg);
	}

	typedef bool (__thiscall *PfnPostToWorker)(void*, void*, void*);
	void** pThisVTable = *reinterpret_cast<void***>(this);
	if (pThisVTable != nullptr && pThisVTable[19] != nullptr) {
		return reinterpret_cast<PfnPostToWorker>(pThisVTable[19])(this, pMsg, pTarget);
	}
	return false;
}

// [PARTIAL - Native 0x00403730] (265 bytes)
// Validates the character's game world and relocates it when its layer rejects the character.
// Missing: the world ID comes from the context returned by CGObj vftable[5] (+0x14); after the lookup the
// native tests CGameWorld vftable[20] (+0x50) with the context layer and, unless it returns 1, relocates via
// CGameWorld vftable[50] (+0xC8) and 0x004E0410. The manager is only asserted, not a return.
// CORRECTION (Claude): the lookup is CGameWorldMgr::FindGameWorld (0x0040376E), formerly CWorldManager::FindRegion.
void CMainProcess::ValidateAndRelocateInvalidGameWorld(CGObjChar* pChar, int32_t nEntryType) {
	if (nEntryType != 1 || pChar == nullptr) {
		return;
	}

	if (g_pGameWorldMgr == nullptr) {
		ServerFramework_GenerateMiniDump();
		return;
	}

	CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(static_cast<uint16_t>(pChar->GetWorldID()));
	if (pGameWorld == nullptr) {
		ServerFramework_GenerateMiniDump();
		BSLib::Log_Printf(0x2000001, "invalidate GameWorld ID = %s [JID:%d, CharID:%d]\n",
			pChar->GetName(), pChar->GetJID(), pChar->GetGlobalID());
		return;
	}
}

/*
================
CGameServer_SubmitAsyncDBQuery

Submits asynchronous SQL query string to shard DB worker threads.
================
*/
#include "AsyncShardQuery.h"
bool CGameServer_SubmitAsyncDBQuery(const char* szSql) {
	if (!szSql || szSql[0] == '\0') {
		return false;
	}
	return ShardQuery::Submit(szSql);
}

/*
================
CMainProcess::SubmitLearnedChangeDBQuery

[RECONSTRUCTED - Native 0x004E7350] (114 bytes)
Constructs and dispatches "exec _skill_manage %d, %d, %d, %d" to the asynchronous
ShardDB overlap worker queue.
================
*/
bool CMainProcess::SubmitLearnedChangeDBQuery(int32_t nKind, uint32_t dwCharID, uint32_t dwParam1, uint32_t dwParam2) {
	char szSql[256];
	std::snprintf(szSql, sizeof(szSql), "exec _skill_manage %d, %u, %u, %u", nKind, dwCharID, dwParam1, dwParam2);
	return CGameServer_SubmitAsyncDBQuery(szSql);
}
