/**
 * ============================================================================
 * Joymax EngineCommon - Memory Allocator Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\EngineCommon\IMemory.cpp
 * ============================================================================
 */

#include "IMemory.h"
#include <new>

namespace EngineCommon {

CDefaultMemory::~CDefaultMemory() = default;

void* CDefaultMemory::Allocate(size_t nBytes) {
	return ::operator new(nBytes);
}

void CDefaultMemory::Free(void* pBlock) {
	::operator delete(pBlock);
}

void* CDefaultMemory::Reallocate(void* pBlock, size_t nNewBytes) {
	void* pNew = Allocate(nNewBytes);
	if (pBlock && pNew) {
		Free(pBlock);
	}
	return pNew;
}

} // namespace EngineCommon
