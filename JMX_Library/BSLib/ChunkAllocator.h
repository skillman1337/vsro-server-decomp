/**
 * ============================================================================
 * Joymax BSLib - Single-threaded chunk allocator
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\ChunkAllocator.h
 *
 * CChunkAllocatorST<T> ("CChunkAllocatorST::AllocItemBlock() Failed", 0x00ADF268)
 * Static instances reconstructed so far:
 *   - sAutoCommand                @ 0x00CCF0B0 (ctor 0x004AF630, Alloc 0x004AF7B0, Free 0x004AF810,
 *                                               AllocItemBlock 0x004AFCA0)
 *   - Skill::sSkillPreEngageData  @ 0x00CE2B18 (ctor 0x005AC160, Alloc 0x005AC2E0, Free 0x005AC340,
 *                                               AllocItemBlock 0x005B1C20)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_CHUNKALLOCATOR_H_
#define _JMX_LIBRARY_BSLIB_CHUNKALLOCATOR_H_

#include <cstdint>
#include <deque>
#include <list>
#include "BSLog.h"

/**
 * [RECONSTRUCTED - template, see instances above]
 * Items are constructed a block at a time (500 per block, set by every static constructor)
 * and recycled through a free queue. The optional initialiser at +0x00 runs on every Alloc;
 * both instances above leave it null.
 */
template <typename T>
class CChunkAllocatorST {
public:
	typedef void (*PFN_ITEM_INIT)(T* pItem, uintptr_t dwArg);

	// [RECONSTRUCTED - e.g. 0x005AC160]
	CChunkAllocatorST()
		: m_pfnInit(nullptr)
		, m_nItemsPerBlock(500)
		, m_dwMaxFree(0xFFFFFFFF)
		, m_dwInitArg(0)
		, m_nAllocated(0) {
	}

	~CChunkAllocatorST() {
		for (typename std::list<T*>::iterator it = m_listBlocks.begin(); it != m_listBlocks.end(); ++it) {
			delete[] *it;
		}
	}

	// [RECONSTRUCTED - e.g. 0x005AC2E0 / 0x005AEB60]
	T* Alloc() {
		T* pItem = PopFree();
		if (pItem == nullptr) {
			AllocItemBlock();
			pItem = PopFree();
		}
		++m_nAllocated;
		return pItem;
	}

	// [RECONSTRUCTED - e.g. 0x005AC340]
	void Free(T* pItem) {
		if (pItem == nullptr) {
			ASSERT(false);
			return;
		}
		if (m_dwMaxFree == 0xFFFFFFFF || m_queFree.size() < m_dwMaxFree) {
			m_queFree.push_back(pItem);
		}
		--m_nAllocated;
	}

	uint32_t GetAllocatedCount() const { return m_nAllocated; }

private:
	T* PopFree() {
		if (m_queFree.empty()) {
			return nullptr;
		}
		T* pItem = m_queFree.front();
		m_queFree.pop_front();
		if (m_pfnInit != nullptr) {
			m_pfnInit(pItem, m_dwInitArg);
		}
		return pItem;
	}

	// [RECONSTRUCTED - e.g. 0x005B1C20] Only grows while the free queue is empty.
	int32_t AllocItemBlock() {
		if (!m_queFree.empty()) {
			return 0;
		}
		T* pBlock = new T[m_nItemsPerBlock];
		m_listBlocks.push_back(pBlock);
		for (uint32_t i = 0; i < m_nItemsPerBlock; ++i) {
			if (m_dwMaxFree == 0xFFFFFFFF || m_queFree.size() < m_dwMaxFree) {
				m_queFree.push_back(&pBlock[i]);
			}
		}
		return 1;
	}

private:
	PFN_ITEM_INIT   m_pfnInit;        // +0x00
	uint32_t        m_nItemsPerBlock; // +0x08
	uint32_t        m_dwMaxFree;      // +0x10 (CQue max count at +0x0C)
	std::deque<T*>  m_queFree;        // +0x14 (element count at +0x24)
	std::list<T*>   m_listBlocks;     // +0x28
	uintptr_t       m_dwInitArg;      // +0x34
	uint32_t        m_nAllocated;     // +0x38
};

#endif // _JMX_LIBRARY_BSLIB_CHUNKALLOCATOR_H_
