/**
 * ============================================================================
 * Joymax PathFindEngine - AI Data Common Structures
 * Original Source: D:\WORK2005\Source\JMX_Library\PathFindEngine\AIDataCommon.h
 *
 * Implements:
 *   - tagNavEdge / tagContainEdge (12 bytes)
 *   - CAIDataCommon:
 *       GetContainEdge @ 0x00555780: m_dwTotalContainEdgeCount >= dwCustomEdgeID
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_PATHFINDENGINE_AIDATACOMMON_H_
#define _JMX_LIBRARY_PATHFINDENGINE_AIDATACOMMON_H_

#include <cstdint>
#include <cstddef>
#include "../BSLib/BSLog.h"

namespace PathFindEngine {

#pragma pack(push, 1)
// 12-byte navigation contain edge structure
struct tagContainEdge {
	uint32_t dwEdgeID;      // +0x00
	uint32_t dwSourceNode;  // +0x04
	uint32_t dwTargetNode;  // +0x08
};
#pragma pack(pop)

class CAIDataCommon {
public:
	CAIDataCommon();
	virtual ~CAIDataCommon() = default;

	/*
	================
	GetContainEdge
	[RECONSTRUCTED - Native 0x00555780]
	Line 252 assert: m_dwTotalContainEdgeCount >= dwCustomEdgeID
	================
	*/
	tagContainEdge* GetContainEdge(uint32_t dwCustomEdgeID);

public:
	uint32_t        m_pad04;                  // +0x04
	uint32_t        m_pad08;                  // +0x08
	uint32_t        m_dwTotalContainEdgeCount;// +0x0C
	uint32_t        m_pad10;                  // +0x10
	tagContainEdge* m_pEdges;                 // +0x14
};

} // namespace PathFindEngine

#endif // _JMX_LIBRARY_PATHFINDENGINE_AIDATACOMMON_H_
