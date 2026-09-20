/**
 * ============================================================================
 * Joymax BSLib - BlackSea Library: Synchronization Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\
 *
 * Implements:
 *   - CSynchObject @ 0x00B4287C, Destructor @ 0x00976490
 *   - CCriticalSectionBS @ 0x00AFA5C0, Destructor @ 0x0056AAB0
 *   - CSingleInstanceSemaphore @ 0x0095C470 / 0x0095C4B0 / 0x0095C4D0
 * ============================================================================
 */

#include "Synch.h"

// ============================================================================
// CSynchObject
// ============================================================================

CSynchObject::CSynchObject(const char* pszName)
	: m_hObject(nullptr)
	, m_strName(pszName ? pszName : "") {
}

CSynchObject::~CSynchObject() {
	// Native @ 0x00976490:
	if (m_hObject) {
#ifdef _WIN32
		CloseHandle(m_hObject);
#endif
		m_hObject = nullptr;
	}
}

bool CSynchObject::Lock(uint32_t /*dwTimeout*/) {
	return true;
}

bool CSynchObject::Unlock() {
	return true;
}

HANDLE CSynchObject::GetHandle() const {
	return m_hObject;
}

const std::string& CSynchObject::GetName() const {
	return m_strName;
}

// ============================================================================
// CCriticalSectionBS
// ============================================================================

CCriticalSectionBS::CCriticalSectionBS(const char* pszName)
	: CSynchObject(pszName) {
	InitializeCriticalSection(&m_cs);
}

CCriticalSectionBS::~CCriticalSectionBS() {
	// Native @ 0x0056AAB0:
	DeleteCriticalSection(&m_cs);
}

bool CCriticalSectionBS::Lock(uint32_t /*dwTimeout*/) {
	EnterCriticalSection(&m_cs);
	return true;
}

bool CCriticalSectionBS::Unlock() {
	LeaveCriticalSection(&m_cs);
	return true;
}

CRITICAL_SECTION* CCriticalSectionBS::GetCriticalSection() {
	return &m_cs;
}

// ============================================================================
// CSingleInstanceSemaphore
// ============================================================================

CSingleInstanceSemaphore::CSingleInstanceSemaphore(const char* pszName)
	: m_hSemaphore(nullptr) {
	Create(pszName);
}

CSingleInstanceSemaphore::~CSingleInstanceSemaphore() {
	Close();
}

/**
 * [RECONSTRUCTED - 0x0095C470]
 * Native implementation @ 0x0095C470 (50 bytes)
 */
bool CSingleInstanceSemaphore::Create(const char* pszName) {
#ifdef _WIN32
	m_hSemaphore = CreateSemaphoreA(nullptr, 0, 1, pszName);
	if (m_hSemaphore != nullptr) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(m_hSemaphore);
			m_hSemaphore = nullptr;
		}
	}
#else
	m_hSemaphore = reinterpret_cast<HANDLE>(1);
#endif
	return m_hSemaphore != nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095C4D0]
 * Native implementation @ 0x0095C4D0 (8 bytes)
 */
bool CSingleInstanceSemaphore::IsOpen() const {
	return m_hSemaphore != nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095C4B0]
 * Native implementation @ 0x0095C4B0 (20 bytes)
 */
void CSingleInstanceSemaphore::Close() {
#ifdef _WIN32
	if (m_hSemaphore != nullptr) {
		CloseHandle(m_hSemaphore);
		m_hSemaphore = nullptr;
	}
#else
	m_hSemaphore = nullptr;
#endif
}
