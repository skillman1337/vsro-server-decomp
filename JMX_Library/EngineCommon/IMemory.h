/**
 * ============================================================================
 * Joymax EngineCommon - Memory Allocator Interface
 * Original Source: D:\WORK2005\Source\JMX_Library\EngineCommon\IMemory.h
 *
 * Implements the abstract memory allocator interface referenced across
 * NavMesh and PathFindEngine modules.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_ENGINECOMMON_IMEMORY_H_
#define _JMX_LIBRARY_ENGINECOMMON_IMEMORY_H_

#include <cstddef>
#include <cstdint>

namespace EngineCommon {

class IMemory {
public:
	virtual ~IMemory() = default;

	// Pure virtual memory management interface
	virtual void* Allocate(size_t nBytes) = 0;
	virtual void  Free(void* pBlock) = 0;
	virtual void* Reallocate(void* pBlock, size_t nNewBytes) = 0;
};

// Default CRT memory allocator wrapper
class CDefaultMemory : public IMemory {
public:
	virtual ~CDefaultMemory() override;

	virtual void* Allocate(size_t nBytes) override;
	virtual void  Free(void* pBlock) override;
	virtual void* Reallocate(void* pBlock, size_t nNewBytes) override;
};

} // namespace EngineCommon

#endif // _JMX_LIBRARY_ENGINECOMMON_IMEMORY_H_
