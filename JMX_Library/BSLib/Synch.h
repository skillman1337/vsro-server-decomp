/**
 * ============================================================================
 * Joymax BSLib - BlackSea Library: Synchronization
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\
 *
 * Implements Win32 synchronization objects matching native vtables:
 *   - CSynchObject @ 0x00B4287C (RTTI: .?AVCSynchObject@@)
 *   - CCriticalSectionBS @ 0x00AFA5C0 (RTTI: .?AVCCriticalSectionBS@@)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_SYNCH_H_
#define _JMX_LIBRARY_BSLIB_SYNCH_H_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
typedef void* HANDLE;
typedef struct { pthread_mutex_t mutex; } CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_init(&cs->mutex, nullptr); }
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(&cs->mutex); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_lock(&cs->mutex); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_unlock(&cs->mutex); }
inline void CloseHandle(HANDLE) {}
#endif

#include <cstdint>
#include <string>

/**
 * CSynchObject
 * Native VTable @ 0x00B4287C, Destructor @ 0x00976490
 */
class CSynchObject {
public:
	CSynchObject(const char* pszName = nullptr);
	virtual ~CSynchObject();

	virtual bool Lock(uint32_t dwTimeout = 0xFFFFFFFF);
	virtual bool Unlock();

	HANDLE GetHandle() const;
	const std::string& GetName() const;

protected:
	HANDLE      m_hObject;  // +0x04: Kernel object handle (if applicable)
	std::string m_strName;  // +0x08: Sync object name
};

/**
 * CCriticalSectionBS
 * Native VTable @ 0x00AFA5C0, Destructor @ 0x0056AAB0
 */
class CCriticalSectionBS : public CSynchObject {
public:
	CCriticalSectionBS(const char* pszName = nullptr);
	virtual ~CCriticalSectionBS() override;

	virtual bool Lock(uint32_t dwTimeout = 0xFFFFFFFF) override;
	virtual bool Unlock() override;

	CRITICAL_SECTION* GetCriticalSection();

private:
	CRITICAL_SECTION m_cs;  // +0x24: Native Win32 CRITICAL_SECTION
};

/**
 * [RECONSTRUCTED - 0x0095C470, 0x0095C4B0, 0x0095C4D0]
 * CSingleInstanceSemaphore
 * Native implementations:
 *   - Create @ 0x0095C470 (50 bytes)
 *   - Close  @ 0x0095C4B0 (20 bytes)
 *   - IsOpen @ 0x0095C4D0 (8 bytes)
 *
 * Single-instance kernel object guard for Joymax server processes.
 */
class CSingleInstanceSemaphore {
public:
	HANDLE m_hSemaphore = nullptr; // 4-byte struct matching native offset [esi]

	CSingleInstanceSemaphore() = default;
	explicit CSingleInstanceSemaphore(const char* pszName);
	~CSingleInstanceSemaphore();

	bool Create(const char* pszName);
	bool IsOpen() const;
	void Close();
};

namespace BSLib {
	using ::CSynchObject;
	using ::CCriticalSectionBS;
	using ::CSingleInstanceSemaphore;
}

#endif // _JMX_LIBRARY_BSLIB_SYNCH_H_
