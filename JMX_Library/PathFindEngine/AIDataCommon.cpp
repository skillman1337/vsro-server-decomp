/**
 * ============================================================================
 * Joymax PathFindEngine - AI Data Common Structures Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\PathFindEngine\AIDataCommon.cpp
 *
 * Implements:
 *   - CAIDataCommon::GetContainEdge @ 0x00555780
 * ============================================================================
 */

#include "AIDataCommon.h"

namespace PathFindEngine {

CAIDataCommon::CAIDataCommon()
	: m_dwTotalContainEdgeCount(0)
	, m_pEdges(nullptr)
{
}

/*
================
GetContainEdge
[RECONSTRUCTED - Native 0x00555780]
Line 252 assert: m_dwTotalContainEdgeCount >= dwCustomEdgeID
================
*/
tagContainEdge* CAIDataCommon::GetContainEdge(uint32_t dwCustomEdgeID) {
	if (m_dwTotalContainEdgeCount >= dwCustomEdgeID && m_pEdges != nullptr) {
		return &m_pEdges[dwCustomEdgeID];
	}
	BSLib::Log_Printf(0x2000000, "Assertion failed in AIDataCommon.h: m_dwTotalContainEdgeCount >= dwCustomEdgeID");
	return nullptr;
}

} // namespace PathFindEngine
