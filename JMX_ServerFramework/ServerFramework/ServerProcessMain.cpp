/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessMain Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessMain.cpp
 *
 * Implements:
 *   - CServerProcessMain Methods (VTable @ 0x00B40E5C)
 *   - OnCertificationResponse @ 0x00949CF0
 *   - CheckServerReadyState @ 0x0094A510
 *   - Runtime Class Descriptor @ 0x00ADD940
 * ============================================================================
 */

#include "ServerProcessMain.h"
#include "../../JMX_Library/BSLib/BSLog.h"

namespace ServerFramework {

// Native runtime descriptor @ 0x00ADD940:
// +0x00: "CServerProcessMain" (0x00B40B74)
// +0x04: 0x000400C8 (262,344 bytes)
// +0x08: CServerProcessMain_CreateObject (0x00948C60)
// +0x0C: CRuntimeClass_DefaultDestroyObject (0x008364E0)
const CRuntimeClass CServerProcessMain::ms_runtimeClass = {
	"CServerProcessMain",
	0x400C8,
	&CServerProcessMain::CreateObject,
	&CServerProcessMain::DestroyObject,
	nullptr
};

const CRuntimeClass* CServerProcessMain::GetRuntimeClass() const {
	return &ms_runtimeClass;
}

void* CServerProcessMain::CreateObject() {
	return new CServerProcessMain();
}

void CServerProcessMain::DestroyObject(void* pObj) {
	delete static_cast<CServerProcessMain*>(pObj);
}

// [RECONSTRUCTED - 0x00949CB0]
// slot 22 (+0x58) @ 0x00949CB0: SendCertificationResponse
// Native implementation @ 0x00949CB0 (64 bytes)
int32_t CServerProcessMain::SendCertificationResponse(CMassiveMsg* pMsg, uint32_t dwSessionID) {
	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	pPacket->SetOpcode(0xA003);
	ServerFramework_EncodeCertificationTopology(pMsg, pPacket, this);
	int32_t nRet = pPacket->Send(dwSessionID, 0);
	pPacket->Release();
	return nRet;
}

// [RECONSTRUCTED - 0x00949CF0]
// slot 23 (+0x5C) @ 0x00949CF0: OnCertificationResponse
int32_t CServerProcessMain::OnCertificationResponse(CMassiveMsg* pMsg, void* pContext, void* pSession) {
	(void)pContext;
	(void)pSession;
	if (!pMsg) {
		ServerFramework_PostShutdownSignal();
		return -1;
	}

	uint8_t byResult = 0;
	pMsg->ReadBytes(&byResult, 1);

	if (byResult == 2) {
		// Certification rejected
		BSLib::Log_Printf(0, "Server certification failed: result=%d", byResult);
		ServerFramework_PostShutdownSignal();
		return -1;
	}

	// Decode full cluster topology from certification response (0x0093CCC0)
	ServerFramework_DecodeCertificationTopology(pMsg);
	BSLib::Log_Printf(0, "successfully server certificated");

	return 0;
}

// [RECONSTRUCTED - 0x0094A510]
// slot 45 (+0xB4) @ 0x0094A510: CheckServerReadyState
uint8_t CServerProcessMain::CheckServerReadyState(uint8_t nAction) {
	if (nAction == 1) {
		if (g_pLocalServerInfo && g_pLocalServerInfo->nState == SERVER_STATE_RUNNING) {
			uint32_t dwTotalLinks = 0;
			uint32_t dwActiveLinks = 0;
			ServerFramework_CountServerLinks(g_pLocalServerInfo->wServerID, &dwTotalLinks, &dwActiveLinks);
			if (dwActiveLinks == dwTotalLinks) {
				return 0; // All cluster links active & connected
			}
		}
		return 3;
	} else if (nAction == 2) {
		if (g_pLocalServerInfo && g_pLocalServerInfo->nState == 5) {
			return 0;
		}
		return 3;
	}
	return 3;
}

// [RECONSTRUCTED - 0x00948E70]
// slot 6 (+0x18) @ 0x00948E70: Registers framework and inter-server message handlers
// Registers 21 cluster and topology message handlers mapping to virtual method thunks
int32_t CServerProcessMain::ProcessMessage() {
	// Native 0x00948E73: Call base class CServerProcessBase::ProcessMessage
	CServerProcessBase::ProcessMessage();

	// Native 0x00948EB5: Opcode 0xA003 (Certification Response) -> OnCertificationResponse (slot 23 @ +0x5C)
	RegisterMsgHandler(0xA003, reinterpret_cast<PFN_MSGHANDLER>(&CServerProcessMain::OnCertificationResponse));

	return 0;
}

} // namespace ServerFramework
