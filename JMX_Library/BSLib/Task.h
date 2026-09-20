/**
 * ============================================================================
 * Joymax BSLib - Task and Thread Management Subsystem
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\thread\threadmanager.cpp
 *
 * Implements:
 *   - CTask: Worker task instance with message queue and thread binding (VTable @ 0x00B42E60)
 *   - CTaskManager: Registry of active server tasks (VTable @ 0x00B42D3C)
 *   - CSystem: Global system service manager (VTable @ 0x00B42D44)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_TASK_H_
#define _JMX_LIBRARY_BSLIB_TASK_H_

#include "Synch.h"
#include "thread/threadmanager.h"
#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdio>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

struct CRuntimeClass;
class CServiceObject;

namespace BSLib {

template <typename T>
class IQue {
public:
	virtual ~IQue() = default;
	virtual int32_t Push(const T& item) = 0;
	virtual int32_t GetCount() = 0;
	virtual int32_t Clear() = 0;
	virtual int32_t Destroy() = 0;
	virtual int32_t Pop(T* pOut, uint32_t dwTimeoutMs = 0, uint32_t dwReserved = 0) = 0;
};

class CTaskManager;
class CSockSystem;
class CTask;

/**
 * CSockSystem
 * Native VTable @ 0x00B42908, Size: 8 bytes (0x08)
 */
class CSockSystem {
public:
	CSockSystem();
	virtual ~CSockSystem() = default;

private:
	uint32_t m_dwReserved; // +0x04
};

/**
 * CTask
 * Native VTable @ 0x00B42E60, Destructor @ 0x00983E50
 * Native Size: 108 bytes (0x6C)
 */
class CTask {
public:
	// Native constructor @ 0x00983DC0
	CTask();

	// Native destructor @ 0x00983E50
	virtual ~CTask();

	// [RECONSTRUCTED - 0x00983E70]
	bool Initialize(CThreadManager* pThreadManager, const CRuntimeClass* pRuntimeClass, uint32_t dwParam);
	bool Initialize(CThreadManager* pThreadManager, const CRuntimeClass* pRuntimeClass, uint32_t dwTaskID, uint32_t dwParam);

	// [RECONSTRUCTED - 0x0096A9A0]
	bool PrepareActivation();

	// [RECONSTRUCTED - 0x009843A0]
	bool ActivateWorkerThreads(uint32_t dwThreadCount, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3, const int32_t* pAffinityConfig);

	// [RECONSTRUCTED - 0x009842A0]
	bool DeactivateWorkerThreads();

	// [RECONSTRUCTED - 0x009841A0]
	bool PauseWorkerThreads();

	CServiceObject* GetTaskObject() const;
	uint32_t        GetWorkerCount() const;
	bool            IsActive() const;
	void*           GetMessageQueue() const;

protected:
	// Exact struct layout matching native binary bytes:
	void*               m_pMsgQueue;        // +0x04: IQue<CMsg*>*
	uint32_t            m_dwThreadCount;    // +0x08: Worker thread count
	std::list<HANDLE>   m_listThreads;      // +0x0C: Thread handles (12 bytes)
	uint32_t            m_bActive;          // +0x10: 1 = Active, 0 = Inactive
	CThreadManager*     m_pThreadManager;   // +0x14: Pointer to CThreadManager
	CTask*              m_pThis;            // +0x18: Self pointer
	CServiceObject*     m_pTaskObject;      // +0x1C: Created by CRuntimeClass
	uint32_t            m_dwParam1;         // +0x20
	uint32_t            m_dwParam2;         // +0x24
	uint32_t            m_dwParam3;         // +0x28
	CCriticalSectionBS  m_csTask;           // +0x2C: Critical section (60 bytes = 0x3C)
	uint32_t            m_dwWorkerCount;    // +0x68: Total worker count
};

/**
 * CTaskManager
 * Native VTable @ 0x00B42D3C
 * Constructor @ 0x00981940, Destructor @ 0x009819B0
 * Scalar Deleting Destructor @ 0x00981B40
 * Native Size: 76 bytes (0x4C)
 */
class CTaskManager {
public:
	CTaskManager();
	virtual ~CTaskManager();

	void Clear();
	void StopAllTasks();
	CTask* FindTaskEntry(uint32_t dwTaskID);
	bool RegisterTaskEntry(uint32_t dwTaskID, CTask* pTask);
	bool UnregisterTaskEntry(uint32_t dwTaskID);

private:
	std::map<uint32_t, CTask*>  m_mapTasks; // +0x04: 12 bytes
	CCriticalSectionBS          m_csTasks;  // +0x10: 60 bytes (0x3C)
};

/**
 * CSystem
 * Native VTable @ 0x00B42D44, Constructor @ 0x00981BB0, Destructor @ 0x00981CB0
 * Scalar Deleting Destructor @ 0x00981C90
 * Native Size: 16 bytes (0x10)
 * Global pointer @ 0x00D6CA2C, Global instance @ 0x00D67C70
 */
class CSystem {
public:
	CSystem();
	virtual ~CSystem();

	CSockSystem*    GetSockSystem() const;
	CTaskManager*   GetTaskManager() const;
	CThreadManager* GetThreadManager() const;

private:
	CSockSystem*    m_pSockSystem;      // +0x04
	CTaskManager*   m_pTaskManager;     // +0x08
	CThreadManager* m_pThreadManager;   // +0x0C
};

// Global system singleton pointer matching native 0x00D6CA2C
extern CSystem* g_pSystem;

CSystem* GetSystem();

} // namespace BSLib

#endif // _JMX_LIBRARY_BSLIB_TASK_H_
