/**
 * ============================================================================
 * Joymax NavMesh - Dungeon Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshDungeon.cpp
 *
 * Implements:
 *   - CRTNavMeshDungeon::Move         [PARTIAL - 0x00999CF0]
 *   - CRTNavMeshDungeon::FindNavCell  [PARTIAL - 0x00999AA0]
 * ============================================================================
 */

#include "RTNavMeshDungeon.h"

namespace NavMesh {

CRTNavMeshDungeon::CRTNavMeshDungeon()
	: m_pObjectInfo(nullptr) {
	m_nMeshType = 3;
}

CRTNavMeshDungeon::~CRTNavMeshDungeon() {
}

/*
================
CRTNavMeshDungeon::Move
[PARTIAL - 0x00999CF0]
Dungeon blocks are not loaded: the move fails.
================
*/
int32_t CRTNavMeshDungeon::Move(int32_t /*nActorMode*/, int32_t /*nFlag*/, tagNavPos* /*pSource*/, tagNavPos* pDest,
	SNavMoveContext* /*pContext*/) {
	pDest->pNavCell = nullptr;
	return 0x10000000;
}

/*
================
CRTNavMeshDungeon::FindNavCell
[PARTIAL - 0x00999AA0]
No dungeon cells exist in the port: the position gets none.
================
*/
void CRTNavMeshDungeon::FindNavCell(tagNavPos* pPos) {
	pPos->pNavCell = nullptr;
	pPos->pNavMeshInst = nullptr;
}

} // namespace NavMesh
