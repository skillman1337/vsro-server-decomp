/**
 * ============================================================================
 * Joymax ServerFramework - CServerProcessOverlap Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerProcessOverlap.cpp
 *
 * Implements:
 *   - CServerProcessOverlap Methods (VTable @ 0x00B4038C)
 *   - Runtime Class Descriptor @ 0x00ADD954
 * ============================================================================
 */

#include "ServerProcessOverlap.h"

namespace ServerFramework {

// Native runtime descriptor @ 0x00ADD954:
// +0x00: "CServerProcessOverlap" (0x00B402AC)
// +0x04: 0x000400B8 (262,328 bytes)
// +0x08: CServerProcessOverlap_CreateObject (0x00937200)
// +0x0C: CRuntimeClass_DefaultDestroyObject (0x008364E0)
const CRuntimeClass CServerProcessOverlap::ms_runtimeClass = {
	"CServerProcessOverlap",
	0x400B8,
	&CServerProcessOverlap::CreateObject,
	&CServerProcessOverlap::DestroyObject,
	nullptr
};

const CRuntimeClass* CServerProcessOverlap::GetRuntimeClass() const {
	return &ms_runtimeClass;
}

void* CServerProcessOverlap::CreateObject() {
	return new CServerProcessOverlap();
}

void CServerProcessOverlap::DestroyObject(void* pObj) {
	delete static_cast<CServerProcessOverlap*>(pObj);
}

} // namespace ServerFramework
