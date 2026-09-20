/**
 * ============================================================================
 * Joymax BSLib - Thread Manager Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\thread\threadmanager.cpp
 *
 * Implements:
 *   - CThreadManager methods:
 *       CreateWorkerThreads @ 0x00985F60
 *       StopAllThreads      @ 0x00986320 / 0x00986460
 *       PauseThreads        @ 0x00986440
 *       StopThreads         @ 0x00986450
 * ============================================================================
 */

#include "threadmanager.h"
#include "../BSLog.h"

namespace BSLib {

CThreadManager::CThreadManager()
	: m_dwThreadCount(0)
	, m_pHead(nullptr)
	, m_pTail(nullptr)
	, m_pQue(nullptr)
	, m_csThreads("ThreadManager::Threads")
{
}

CThreadManager::~CThreadManager() {
	StopAllThreads();
}

/*
================
CThreadManager::CreateWorkerThreads
[RECONSTRUCTED - Native 0x00985F60]
================
*/
bool CThreadManager::CreateWorkerThreads(const int32_t* pAffinityConfig, void* pTaskObject, uint32_t dwThreadCount, std::vector<HANDLE>& handles, void* pTask, uint32_t dwParam) {
	(void)pAffinityConfig;
	(void)pTaskObject;
	(void)dwParam;
	(void)pTask;
	(void)handles;
	m_dwThreadCount += dwThreadCount;
	return true;
}

/*
================
CThreadManager::StopAllThreads
[RECONSTRUCTED - Native 0x00986320 / 0x00986460]
================
*/
void CThreadManager::StopAllThreads() {
	m_dwThreadCount = 0;
}

/*
================
CThreadManager::PauseThreads
[RECONSTRUCTED - Native 0x00986440]
================
*/
void CThreadManager::PauseThreads() {
}

/*
================
CThreadManager::StopThreads
[RECONSTRUCTED - Native 0x00986450]
================
*/
void CThreadManager::StopThreads() {
	m_dwThreadCount = 0;
}

uint32_t CThreadManager::GetWorkerCount() const {
	return m_dwThreadCount;
}

} // namespace BSLib
