/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessMain Class
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessMain.h
 *
 * Implements the primary server process task:
 *   - VTable @ 0x00B40E5C (RTTI: .?AVCServerProcessMain@ServerFramework@@)
 *   - Base: CServerProcessBase (VTable @ 0x00B40EC4) -> CServiceObject -> CBase
 *   - Runtime Class Descriptor @ 0x00ADD940
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERPROCESSMAIN_H_
#define _JMX_SERVERFRAMEWORK_SERVERPROCESSMAIN_H_

#include "../../JMX_Library/BSLib/BSObj.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "ServerConfig.h"
#include "ServerTopology.h"
#include "ServerMain.h"
#include "ServerProcessBase.h"
#include <cstdint>

namespace ServerFramework {

// Native task ID @ 0x00C67734
constexpr uint32_t TASK_ID_PROCESS_MAIN = 0x01000000;

class CServerProcessMain : public CServerProcessBase {
public:
	CServerProcessMain() = default;
	virtual ~CServerProcessMain() override = default;

	// slot 0 (+0x00) @ 0x00948CC0: GetRuntimeClass
	virtual const CRuntimeClass* GetRuntimeClass() const override;

	// [RECONSTRUCTED - 0x00949CB0]
	// slot 22 (+0x58) @ 0x00949CB0: SendCertificationResponse
	virtual int32_t SendCertificationResponse(CMassiveMsg* pMsg, uint32_t dwSessionID = 0);

	// [RECONSTRUCTED - 0x00949CF0]
	// slot 23 (+0x5C) @ 0x00949CF0: OnCertificationResponse
	virtual int32_t OnCertificationResponse(CMassiveMsg* pMsg, void* pContext = nullptr, void* pSession = nullptr);

	// [RECONSTRUCTED - 0x0094A510]
	// slot 45 (+0xB4) @ 0x0094A510: CheckServerReadyState
	virtual uint8_t CheckServerReadyState(uint8_t nAction);

	// [RECONSTRUCTED - 0x00948E70]
	// slot 6 (+0x18) @ 0x00948E70: Registers framework and inter-server message handlers
	virtual int32_t ProcessMessage() override;

	static void* CreateObject();
	static void DestroyObject(void* pObj);

	// Native runtime descriptor @ 0x00ADD940
	static const CRuntimeClass ms_runtimeClass;
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERPROCESSMAIN_H_
