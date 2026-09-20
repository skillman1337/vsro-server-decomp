/**
 * ============================================================================
 * Joymax BSLib - Thread Manager
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\thread\threadmanager.h
 *
 * Implements:
 *   - CThreadManager @ 0x00B42F50 / 0x00985F60
 *     Native VTable @ 0x00B42F50, Struct Size: 84 bytes (0x54)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_THREAD_THREADMANAGER_H_
#define _JMX_LIBRARY_BSLIB_THREAD_THREADMANAGER_H_

#include <cstdint>
#include <vector>
#include "../Synch.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace BSLib {

class CThreadManager {
public:
	CThreadManager();
	virtual ~CThreadManager();

	// [RECONSTRUCTED - Native 0x00985F60]
	bool CreateWorkerThreads(const int32_t* pAffinityConfig, void* pTaskObject, uint32_t dwThreadCount, std::vector<HANDLE>& handles, void* pTask, uint32_t dwParam);

	// [RECONSTRUCTED - Native 0x00986320 / 0x00986460]
	void StopAllThreads();

	// [RECONSTRUCTED - Native 0x00986440]
	void PauseThreads();

	// [RECONSTRUCTED - Native 0x00986450]
	void StopThreads();

	uint32_t GetWorkerCount() const;

private:
	uint32_t            m_dwThreadCount;    // +0x04
	void*               m_pHead;            // +0x08
	void*               m_pTail;            // +0x0C
	void*               m_pQue;             // +0x10
	CCriticalSectionBS  m_csThreads;        // +0x14: 60 bytes (0x3C)
};

} // namespace BSLib

#endif // _JMX_LIBRARY_BSLIB_THREAD_THREADMANAGER_H_
