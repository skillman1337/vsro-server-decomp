/**
 * ============================================================================
 * Joymax NavMesh - Navigation Mesh Base Class
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMesh.h
 *
 * Implements:
 *   - CRTNavMesh  .?AVCRTNavMesh@@ (RTTI 0x00C81254), vftable 0x00B43F04, constructor body 0x009C09D0
 *   - tagNavPos   navigation-resolved position (0x18 bytes on x86)
 *   - SNavMoveContext  query context embedded in CRegionManagerBody at +0x10C (0x48 bytes)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESH_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESH_H_

#include <cstdint>
#include "NavMath.h"

namespace NavMesh {

class CRTNavCell;
class CRTNavMeshTerrain;
struct SNavMeshInst;

/**
 * tagNavPos (x86 0x18)
 * CORRECTION (Claude): +0x04 is the object instance (SNavMeshInst, pool 0x009AC880) the position lies on, not a
 * CRTNavMeshObj: CRTNavMeshTerrain::FindNavCell stores the instance (0x0099FE86) and the movement code reads its
 * matrices (+0x20 / +0x60) and resource (+0xB4). Renamed from pNavMeshObj; both pointers are typed now.
 */
struct tagNavPos {
	CRTNavCell*   pNavCell     = nullptr; // +0x00: resolved cell (CRTNavCellQuad on terrain, CRTNavCellTri on an object)
	SNavMeshInst* pNavMeshInst = nullptr; // +0x04: object instance when the cell belongs to an object mesh
	uint16_t      wRegionID    = 0;       // +0x08: region ID (bit 15 = dungeon)
	uint16_t      wPad0A       = 0;       // +0x0A
	float         fPosX        = 0.0f;    // +0x0C: region-local X (0 .. 1920)
	float         fPosY        = 0.0f;    // +0x10
	float         fPosZ        = 0.0f;    // +0x14: region-local Z (0 .. 1920)

	SNavVec3*       Pos()       { return reinterpret_cast<SNavVec3*>(&fPosX); }
	const SNavVec3* Pos() const { return reinterpret_cast<const SNavVec3*>(&fPosX); }
};

/**
 * SNavMoveContext (0x48) - CRegionManagerBody +0x10C .. +0x153
 * Written by CRegionManagerBody::QueryMovement (0x0098B384 .. 0x0098B3BE) and copied whole by 0x009A10E0.
 */
struct SNavMoveContext {
	uint32_t  dwLinkEdgeIndex = 0;       // +0x00: object global edge index a move left through (0x009B55AA)
	int32_t   nStepsLeft      = 0;       // +0x04: cell crossings still allowed (0x7FFFFFFF when unlimited)
	uint8_t   pad08[0x0C]     = {};      // +0x08
	tagNavPos srcCopy;                   // +0x14: last query source (QueryMovement 0x0098B65B, +0x154 bit 0)
	tagNavPos dstCopy;                   // +0x2C: last query destination
	void*     pLastObjEdge    = nullptr; // +0x44: last object edge crossed (0x009B5403)
};

/**
 * CRTNavMesh (vftable 0x00B43F04)
 * Common base of the terrain (type 1), object (type 2) and dungeon (type 3) meshes.
 */
class CRTNavMesh {
public:
	// [RECONSTRUCTED - 0x009C09D0] parent 0, region 0xFFFF, type 0
	CRTNavMesh();

	// vftable[0] 0x009C09F0 (scalar deleting destructor, body 0x009C0A10)
	virtual ~CRTNavMesh();

	// vftable[1]: terrain 0x009A1150 FindHeight, object 0x009B4A90 IsOnMesh, dungeon 0x00537ED0 (returns 0)
	virtual int32_t FindHeight(SNavVec3* pPos) = 0;

	// vftable[2]: terrain 0x009A0710, object 0x009B5A80, dungeon 0x00999CF0
	// Moves from pSource towards pDest. Result bits:
	//   0x00000001 blocked, 0x00000002 left through an object link edge, 0x00000004 entered an object cell,
	//   0x00000008 crossed a railing edge, 0x00000010 left the object, 0x00000020 step budget exhausted,
	//   0x00000080 siege-gate edge, 0x00000100 on a siege object, 0x10000000 failed, 0x20000000 no object cell.
	virtual int32_t Move(int32_t nActorMode, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) = 0;

	// vftable[3]: terrain 0x0099FD90, object 0x009BF500 (no-op), dungeon 0x00999AA0
	virtual void FindNavCell(tagNavPos* pPos) = 0;

	// vftable[4]: terrain 0x009A1BF0 RegisterEventZones, object / dungeon 0x005ECFD0 (returns 1)
	virtual int32_t RegisterEventZones() = 0;

	uint16_t GetRegionID() const { return m_wRegionID; }
	int32_t  GetMeshType() const { return m_nMeshType; }

public:
	CRTNavMeshTerrain* m_pParentTerrain; // +0x04: object resource -> terrain that loaded it last (0x0099F444); 0 on terrain
	uint16_t           m_wRegionID;      // +0x08: 0xFFFF until loaded (0x009C09DB)
	uint16_t           m_wPad0A;         // +0x0A
	int32_t            m_nMeshType;      // +0x0C: 1 terrain, 2 object, 3 dungeon
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESH_H_
