/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessOverlap Class
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessOverlap.h
 *
 * Implements the network overlapped I/O server process task:
 *   - VTable @ 0x00B4038C (RTTI: .?AVCServerProcessOverlap@ServerFramework@@)
 *   - Base: CServerProcessBase (VTable @ 0x00B40EC4) -> CServiceObject -> CBase
 *   - Runtime Class Descriptor @ 0x00ADD954
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERPROCESSOVERLAP_H_
#define _JMX_SERVERFRAMEWORK_SERVERPROCESSOVERLAP_H_

#include "ServerProcessBase.h"
#include <cstdint>

namespace ServerFramework {

// Native task ID @ 0x00C67738
constexpr uint32_t TASK_ID_PROCESS_OVERLAP = 0x01000001;

class CServerProcessOverlap : public CServerProcessBase {
public:
	CServerProcessOverlap() = default;
	virtual ~CServerProcessOverlap() override = default;

	// slot 0 (+0x00) @ 0x00937330: GetRuntimeClass
	virtual const CRuntimeClass* GetRuntimeClass() const override;

	static void* CreateObject();
	static void DestroyObject(void* pObj);

	// Native runtime descriptor @ 0x00ADD954:
	// +0x00: "CServerProcessOverlap" (0x00B402AC)
	// +0x04: 0x000400B8 (262,328 bytes)
	// +0x08: CServerProcessOverlap_CreateObject (0x00937200)
	// +0x0C: CRuntimeClass_DefaultDestroyObject (0x008364E0)
	static const CRuntimeClass ms_runtimeClass;
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERPROCESSOVERLAP_H_
