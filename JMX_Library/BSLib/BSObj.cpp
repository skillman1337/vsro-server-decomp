/**
 * ============================================================================
 * Joymax BSLib - Base Service Object Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\
 *
 * Implements:
 *   - CServiceObject::Initialize @ 0x009859C0
 *   - CServiceObject runtime class @ 0x00B427E4
 * ============================================================================
 */

#include "BSObj.h"

void* CRuntimeClass::CreateObject() const {
	return pfnCreateObject ? pfnCreateObject() : nullptr;
}

void CRuntimeClass::DestroyObject(void* pObj) const {
	if (pfnDestroyObject && pObj) {
		pfnDestroyObject(pObj);
	}
}

const CRuntimeClass CServiceObject::ms_classCServiceObject = {
	"CServiceObject",
	sizeof(CServiceObject),
	nullptr,
	nullptr,
	nullptr
};

CServiceObject::CServiceObject()
	: m_pTask(nullptr)
	, m_dwParam(0) {
}

const CRuntimeClass* CServiceObject::GetRuntimeClass() const {
	return &ms_classCServiceObject;
}

/**
 * [RECONSTRUCTED - 0x009859C0]
 * Slot 2 @ +0x08: Initialize(CTask* pTask, uint32_t dwParam)
 *
 * Machine trace:
 *   mov dword ptr [ecx+8], edx
 *   mov dword ptr [ecx+0Ch], eax
 *   mov al, 1
 *   ret 8
 */
bool CServiceObject::Initialize(BSLib::CTask* pTask, uint32_t dwParam) {
	m_pTask = pTask;
	m_dwParam = dwParam;
	return true;
}

BSLib::CTask* CServiceObject::GetTask() const {
	return m_pTask;
}

uint32_t CServiceObject::GetParam() const {
	return m_dwParam;
}

int32_t CServiceDefaultObject::Process(int32_t* /*pStopFlag*/, int32_t /*nThreadIndex*/) {
	return 0;
}

void CServiceDefaultObject::OnMessage() {
}
