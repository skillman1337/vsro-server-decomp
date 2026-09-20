/**
 * ============================================================================
 * Joymax NavMesh - Object Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshObj.cpp
 *
 * Implements:
 *   - CRTNavMeshObj  .?AVCRTNavMeshObj@@ (RTTI 0x00C81624), vftable 0x00B43EBC, size 0x9C
 *       constructor 0x009B2E10, destructor 0x009B2F20, pool CInstPool<CRTNavMeshObj,1000> 0x00B43ACC
 *   - SNavMeshInst   placed object instance (pool CMemPool<SNavMeshInst,1000> 0x00C814C8, constructor 0x009ACE50)
 *
 * CORRECTION (Claude): replaces the earlier "dynamic collision object" stub (Initialize / TestCollision), which has
 * no counterpart in the binary. An object resource is the collision mesh of one object.ifo entry; instances placed
 * by the nvm object list share it.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHOBJ_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHOBJ_H_

#include <cstdint>
#include <string>
#include <vector>
#include "NavMath.h"
#include "RTNavMesh.h"
#include "RTNavCell.h"
#include "RTNavEdge.h"

namespace NavMesh {

class CMapLoader;
class CNavArchive;
struct SNavEventZone;
struct SObjectInfo;

/**
 * nvm object list entry (0x1E bytes read by 0x0099F3C9; copied into SNavMeshInst +0x02)
 */
#pragma pack(push, 1)
struct SNvmObjectEntry {
	uint32_t dwObjectID;   // +0x00: object.ifo index
	SNavVec3 vPos;         // +0x04
	uint16_t wType;        // +0x10: 0xFFFF static, 0 skinned
	float    fYaw;         // +0x12
	uint16_t wUID;         // +0x16: unique within the owning region
	uint16_t wShort0;      // +0x18
	uint8_t  bIsLarge;     // +0x1A
	uint8_t  bIsStructure; // +0x1B
	uint16_t wRegionID;    // +0x1C: region that owns the object
};
#pragma pack(pop)

/**
 * Link between objects placed in one region (6 bytes, SNavMeshInst +0xA0)
 */
#pragma pack(push, 1)
struct SNvmObjectLink {
	uint16_t wLinkObject;   // +0x00: index in the terrain object list, 0xFFFF none
	uint16_t wLinkEdge;     // +0x02: global edge index on the linked object
	uint16_t wEdge;         // +0x04: global edge index on this object
};
#pragma pack(pop)

/**
 * SNavMeshInst - object placed on a terrain region
 */
struct SNavMeshInst {
	// [RECONSTRUCTED - 0x009ACE50] identity matrices, empty link vector
	SNavMeshInst();

	uint16_t                    wPad00;          // +0x00
	SNvmObjectEntry             entry;           // +0x02: copy of the nvm entry (0x009AA3FD)
	SNavMatrix                  matLocalToWorld; // +0x20: RotationY(yaw) with the entry position as translation
	SNavMatrix                  matWorldToLocal; // +0x60: inverse of +0x20 (0x0099EAD0)
	std::vector<SNvmObjectLink> vecLinks;        // +0xA0
	CRTNavMeshTerrain*          pTerrain;        // +0xB0: terrain whose object list holds the instance (0x0099F407)
	CRTNavMesh*                 pResource;       // +0xB4: CRTNavMeshObj (type 2) or dungeon (type 3)
	void*                       pDungeonLink;    // +0xB8: dungeon entrance (type 3 resources only)

	CRTNavMeshObj* GetObjResource() const;
	// entry key used to match the same object across regions (0x0099FBB0): region << 16 | uid
	uint32_t GetKey() const { return (static_cast<uint32_t>(entry.wRegionID) << 16) | entry.wUID; }
};

/**
 * Grid reference to a global edge (6 bytes, packed)
 */
#pragma pack(push, 1)
struct SNavObjEdgeRef {
	SNavObjEdge* pEdge;  // +0x00
	uint16_t     wIndex; // +0x04: index in the global edge vector (0x009B33EA)
};
#pragma pack(pop)

class CRTNavMeshObj : public CRTNavMesh {
public:
	// [RECONSTRUCTED - 0x009B2E10] type 2
	CRTNavMeshObj();
	virtual ~CRTNavMeshObj() override;

	// [RECONSTRUCTED - 0x009B4A90] vftable[1]: 1 when any triangle contains the point (XZ); sets its height
	virtual int32_t FindHeight(SNavVec3* pPos) override;
	// [RECONSTRUCTED - 0x009B5A80] vftable[2]: positions are in object-local space
	virtual int32_t Move(int32_t nActorMode, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) override;
	// [RECONSTRUCTED - 0x009BF500] vftable[3]: no-op
	virtual void FindNavCell(tagNavPos* /*pPos*/) override {}
	// [RECONSTRUCTED - 0x005ECFD0] vftable[4]
	virtual int32_t RegisterEventZones() override { return 1; }

	// [RECONSTRUCTED - 0x009B36A0] (ebx = archive) collision mesh section of a bms
	bool Load(CNavArchive* pArchive, CMapLoader* pLoader, uint32_t dwStructOption);

	// [RECONSTRUCTED - 0x009B4B00] (eax = local position) triangle containing the point whose plane height is the
	// closest to the given height; the position takes that height
	CRTNavCellTri* FindCell(SNavVec3* pLocalPos);

	// [RECONSTRUCTED - 0x009B4C50] 1 when any triangle contains the point
	int32_t IsInside(const SNavVec3* pLocalPos);

	// [RECONSTRUCTED - 0x009B4CD0] (ecx = this, esi = cell index, edi = previous index, bl = mode) event zone trigger
	int32_t TriggerEventZone(int32_t nCellIndex, int32_t nPrevIndex, uint8_t byMode, uint8_t byEvent);

	// [RECONSTRUCTED - 0x009B4D30] nearest blocking / entry edge on the segment from pSource to pDestLocal through the grid
	CRTNavCell* CrossGrid(SNavVec3* pDestLocal, CRTNavCell* pCurrentCell, SNavVec3* pSource, int32_t* pResult, int32_t nFlag);

	// [RECONSTRUCTED - 0x009B5380] walks the triangles along the segment
	CRTNavCellTri* WalkCells(CRTNavCellTri* pCell, SNavVec2* pStart, SNavVec2* pEnd, int32_t* pResult, int32_t nFlag,
		SNavMoveContext* pContext, SNavObjEdge* pPrevEdge);

	// [RECONSTRUCTED - 0x009B5800] (eax = this, ecx = edge index, ebx = position) cell entered through a link edge
	CRTNavCellTri* FindLinkEntryCell(uint32_t dwEdgeIndex, SNavVec3* pPos);

	// [RECONSTRUCTED - 0x009B31E0] index of a global edge, 0xFFFF when absent
	uint16_t FindGlobalEdgeIndex(const SNavObjEdge* pEdge) const;

	// [RECONSTRUCTED - 0x009B3270] (archive, +0x54 grid header, +0x24 global edges)
	bool LoadGrid(CNavArchive* pArchive);

public:
	const SObjectInfo*            m_pObjectInfo;     // +0x10: object.ifo entry (0x009AA3E5)
	std::vector<SNavObjVertex>    m_vecVertices;     // +0x14
	std::vector<SNavObjEdge>      m_avecEdges[2];    // +0x24 global (outline / link) edges, +0x34 internal edges
	std::vector<CRTNavCellTri>    m_vecCells;        // +0x44
	float                         m_fGridOriginX;    // +0x54
	float                         m_fGridOriginZ;    // +0x58
	int32_t                       m_nGridWidth;      // +0x5C
	int32_t                       m_nGridHeight;     // +0x60
	std::vector<std::vector<SNavObjEdgeRef>> m_vecGrid; // +0x64: 100 x 100 tiles of global edges
	std::vector<std::string>      m_vecEventNames;   // +0x74
	std::vector<SNavEventZone*>   m_vecEventZones;   // +0x84
	uint8_t                       m_bEventOnly;      // +0x94: a single "event" zone on a two-cell mesh (0x009B4145)
	uint32_t                      m_dwStructOption;  // +0x98: bms flags (bit 0 edge events, bit 1 cell events, bit 2 names, bit 3 siege)
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHOBJ_H_
