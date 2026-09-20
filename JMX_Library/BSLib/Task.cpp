/**
 * ============================================================================
 * Joymax BSLib - Task and Thread Management Subsystem Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\thread\threadmanager.cpp
 *
 * Implements:
 *   - CThreadManager @ 0x00B42F50 / 0x00985F60
 *   - CTask @ 0x00B42E60 / 0x00983DC0 / 0x00983E50 / 0x00983E70
 *   - CTaskManager @ 0x00B42D3C / 0x00981940 / 0x009819B0
 *   - CSystem @ 0x00B42D44 / 0x00981BB0 / 0x00981CB0
 *   - Global g_pSystem @ 0x00D6CA2C
 * ============================================================================
 */

#include "Task.h"
#include "BSObj.h"

namespace BSLib {

// Global system singleton pointer matching native 0x00D6CA2C
CSystem* g_pSystem = nullptr;

CSystem* GetSystem() {
	static CSystem s_system;
	if (!g_pSystem) {
		g_pSystem = &s_system;
	}
	return g_pSystem;
}

// ============================================================================
// CSockSystem
// ============================================================================

CSockSystem::CSockSystem()
	: m_dwReserved(0) {
}

// ============================================================================
// CTask
// ============================================================================

CTask::CTask()
	: m_pMsgQueue(nullptr)
	, m_dwThreadCount(0)
	, m_bActive(0)
	, m_pThreadManager(nullptr)
	, m_pThis(this)
	, m_pTaskObject(nullptr)
	, m_dwParam1(0)
	, m_dwParam2(0xFFFFFFFF)
	, m_dwParam3(0)
	, m_csTask("CTask::m_csTask")
	, m_dwWorkerCount(0) {
}

CTask::~CTask() {
	DeactivateWorkerThreads();
	if (m_pTaskObject) {
		delete m_pTaskObject;
		m_pTaskObject = nullptr;
	}
}

bool CTask::Initialize(CThreadManager* pThreadManager, const CRuntimeClass* pRuntimeClass, uint32_t dwParam) {
	m_pThreadManager = pThreadManager;
	m_pThis = this;

	if (!pRuntimeClass) {
		return false;
	}

	m_pTaskObject = reinterpret_cast<CServiceObject*>(pRuntimeClass->CreateObject());
	if (!m_pTaskObject) {
		return false;
	}

	return m_pTaskObject->Initialize(this, dwParam);
}

bool CTask::Initialize(CThreadManager* pThreadManager, const CRuntimeClass* pRuntimeClass, uint32_t /*dwTaskID*/, uint32_t dwParam) {
	return Initialize(pThreadManager, pRuntimeClass, dwParam);
}

bool CTask::PrepareActivation() {
	m_csTask.Lock();
	bool bReady = (m_pTaskObject != nullptr);
	m_csTask.Unlock();
	return bReady;
}

bool CTask::ActivateWorkerThreads(uint32_t dwThreadCount, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3, const int32_t* pAffinityConfig) {
	m_csTask.Lock();
	if (m_bActive) {
		m_csTask.Unlock();
		return true;
	}

	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;
	m_dwParam3 = dwParam3;
	m_dwWorkerCount = dwThreadCount;

	if (m_pThreadManager) {
		std::vector<HANDLE> handles;
		m_pThreadManager->CreateWorkerThreads(pAffinityConfig, m_pTaskObject, dwThreadCount, handles, this, dwParam3);
	}

	m_bActive = 1;
	m_csTask.Unlock();
	return true;
}

bool CTask::DeactivateWorkerThreads() {
	m_csTask.Lock();
	if (!m_bActive || !m_pThreadManager) {
		m_csTask.Unlock();
		return false;
	}

	m_pThreadManager->StopThreads();
	m_bActive = 0;
	m_csTask.Unlock();
	return true;
}

bool CTask::PauseWorkerThreads() {
	m_csTask.Lock();
	if (!m_bActive || !m_pThreadManager) {
		m_csTask.Unlock();
		return false;
	}

	m_pThreadManager->PauseThreads();
	m_csTask.Unlock();
	return true;
}

CServiceObject* CTask::GetTaskObject() const {
	return m_pTaskObject;
}

uint32_t CTask::GetWorkerCount() const {
	return m_dwWorkerCount;
}

bool CTask::IsActive() const {
	return m_bActive != 0;
}

void* CTask::GetMessageQueue() const {
	return m_pMsgQueue;
}

// ============================================================================
// CTaskManager
// ============================================================================

CTaskManager::CTaskManager() = default;

CTaskManager::~CTaskManager() {
	Clear();
}

void CTaskManager::Clear() {
	for (auto& pair : m_mapTasks) {
		if (pair.second) {
			delete pair.second;
		}
	}
	m_mapTasks.clear();
}

void CTaskManager::StopAllTasks() {
	m_csTasks.Lock();
	for (auto& pair : m_mapTasks) {
		CTask* pTask = pair.second;
		if (pTask) {
			pTask->DeactivateWorkerThreads();
		}
	}
	m_csTasks.Unlock();
}

CTask* CTaskManager::FindTaskEntry(uint32_t dwTaskID) {
	m_csTasks.Lock();
	auto it = m_mapTasks.find(dwTaskID);
	CTask* pTask = (it != m_mapTasks.end()) ? it->second : nullptr;
	m_csTasks.Unlock();
	return pTask;
}

bool CTaskManager::RegisterTaskEntry(uint32_t dwTaskID, CTask* pTask) {
	if (!pTask) {
		return false;
	}
	m_csTasks.Lock();
	auto it = m_mapTasks.find(dwTaskID);
	if (it != m_mapTasks.end()) {
		m_csTasks.Unlock();
		return false;
	}
	m_mapTasks[dwTaskID] = pTask;
	m_csTasks.Unlock();
	return true;
}

bool CTaskManager::UnregisterTaskEntry(uint32_t dwTaskID) {
	m_csTasks.Lock();
	auto it = m_mapTasks.find(dwTaskID);
	if (it != m_mapTasks.end()) {
		if (it->second) {
			delete it->second;
		}
		m_mapTasks.erase(it);
		m_csTasks.Unlock();
		return true;
	}
	m_csTasks.Unlock();
	return false;
}

// ============================================================================
// CSystem
// ============================================================================

CSystem::CSystem()
	: m_pSockSystem(new CSockSystem())
	, m_pTaskManager(new CTaskManager())
	, m_pThreadManager(new CThreadManager()) {
}

CSystem::~CSystem() {
	if (m_pTaskManager) {
		m_pTaskManager->StopAllTasks();
	}
	delete m_pThreadManager;
	m_pThreadManager = nullptr;

	delete m_pTaskManager;
	m_pTaskManager = nullptr;

	delete m_pSockSystem;
	m_pSockSystem = nullptr;
}

CSockSystem* CSystem::GetSockSystem() const {
	return m_pSockSystem;
}

CTaskManager* CSystem::GetTaskManager() const {
	return m_pTaskManager;
}

CThreadManager* CSystem::GetThreadManager() const {
	return m_pThreadManager;
}

} // namespace BSLib
