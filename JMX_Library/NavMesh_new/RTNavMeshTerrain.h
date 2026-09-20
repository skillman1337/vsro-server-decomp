/**
 * ============================================================================
 * Joymax NavMesh - Terrain Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshTerrain.cpp
 *
 * Implements:
 *   - CRTNavMeshTerrain  .?AVCRTNavMeshTerrain@@ (RTTI 0x00C8133C), vftable 0x00B4368C, size 0x1B418
 *       constructor 0x0099EE40, destructor 0x0099F080, pool CInstPool<CRTNavMeshTerrain,10> 0x00B43AC4
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHTERRAIN_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHTERRAIN_H_

#include <cstdint>
#include <vector>
#include "NavMath.h"
#include "RTNavMesh.h"
#include "RTNavCell.h"
#include "RTNavEdge.h"

namespace NavMesh {

class CMapLoader;
class CNavArchive;
struct SNavMeshInst;

/**
 * Terrain tile (8 bytes, 96 x 96 at +0x58)
 */
struct SNavTile {
	uint32_t dwCellIndex; // +0x00: low word indexes the quad cell vector (0x0099FF62)
	uint32_t dwFlag;      // +0x04: bit 0 blocked; after loading bits 16-31 hold the tile2d.ifo flag (0x0099F7BE)
};

constexpr int32_t kNavTileCount = 96;
constexpr int32_t kNavHeightCount = 97;
constexpr int32_t kNavSurfaceCount = 6;

class CRTNavMeshTerrain : public CRTNavMesh {
public:
	// [RECONSTRUCTED - 0x0099EE40]
	CRTNavMeshTerrain();
	// [RECONSTRUCTED - 0x0099F080]
	virtual ~CRTNavMeshTerrain() override;

	// [RECONSTRUCTED - 0x009A1150] vftable[1]: bilinear height from the 97 x 97 map, raised to the water / ice
	// surface of the 6 x 6 block when it is higher. Always returns 1.
	virtual int32_t FindHeight(SNavVec3* pPos) override;
	// [RECONSTRUCTED - 0x009A0710] vftable[2]
	virtual int32_t Move(int32_t nActorMode, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) override;
	// [RECONSTRUCTED - 0x0099FD90] vftable[3]
	virtual void FindNavCell(tagNavPos* pPos) override;
	// [PARTIAL - 0x009A1BF0] vftable[4]: registers the event zones of the placed objects with the region manager
	virtual int32_t RegisterEventZones() override;

	// [RECONSTRUCTED - 0x0099F370] (eax = archive) nvm body after the header
	bool Load(CNavArchive* pArchive, CMapLoader* pLoader);

	// [RECONSTRUCTED - 0x0099F130] (eax = this) bKeepData = 1 only clears the tile normal cache
	void Clear(int32_t bKeepData);

	// [RECONSTRUCTED - 0x0099F280] (eax = this, ecx = which) 1 links the internal edges, 0 the global edges
	void LinkEdges(int32_t nWhich);

	// [RECONSTRUCTED - 0x0099F320] (eax = this) drops global edges from the cell direction lists
	void PurgeGlobalEdges();

	// [RECONSTRUCTED - 0x0099F820] (eax = this, edi = position) tile flag word
	uint32_t GetTileFlag(const SNavVec3* pPos) const;

	// [RECONSTRUCTED - 0x0099F860] (edi = this) normal of the tile triangle under the position
	const SNavVec3* GetTileNormal(const tagNavPos* pPos);

	// [RECONSTRUCTED - 0x0099FF00] (esi = position, ebx = this) quad cell under the position; sets its height
	CRTNavCellQuad* GetCellAt(SNavVec3* pPos);

	// [RECONSTRUCTED - 0x009A7B20] (eax = cell vector, edi = index)
	CRTNavCellQuad* GetCell(uint32_t dwIndex);

	// [RECONSTRUCTED - 0x0099FBB0] (eax = this, edi = key) placed object with the same region / uid key
	SNavMeshInst* FindObjectByKey(uint32_t dwKey) const;

	// [RECONSTRUCTED - 0x009A0030] (eax = source, ecx = instance) move through one placed object
	int32_t MoveInObject(tagNavPos* pSource, SNavMeshInst* pInst, int32_t nActorMode, int32_t bProbe, int32_t nFlag,
		tagNavPos* pDest, SNavMoveContext* pContext);

	// [RECONSTRUCTED - 0x009A05A0] (esi = source, edi = destination) 1 when an object of the source cell reports a
	// result for the move
	int32_t CheckObjectsOnPath(tagNavPos* pSource, tagNavPos* pDest, int32_t nFlag, SNavMeshInst* pSkipInst);

public:
	std::vector<SNavMeshInst*>      m_vecObjects;       // +0x14
	std::vector<CRTNavEdgeInternal> m_vecInternalEdges; // +0x24 (0x38 each)
	std::vector<CRTNavEdgeGlobal>   m_vecGlobalEdges;   // +0x34 (0x3C each)
	SNavMeshInst*                   m_pSkipInst;        // +0x44: object ignored by the current move (0x009A10A3)
	std::vector<CRTNavCellQuad>     m_vecCells;         // +0x48 (0x78 each)
	SNavTile                        m_aTiles[kNavTileCount * kNavTileCount];       // +0x58
	uint32_t                        m_dwOpenCellCount;  // +0x12058
	float                           m_afHeights[kNavHeightCount * kNavHeightCount]; // +0x1205C
	uint8_t                         m_abySurfaceType[kNavSurfaceCount * kNavSurfaceCount];   // +0x1B360: bit 1 water / ice
	float                           m_afSurfaceHeight[kNavSurfaceCount * kNavSurfaceCount];  // +0x1B384
	uint8_t*                        m_pNormalCache;     // +0x1B414: 0x2400 built flags + 2 normals per tile (0x0099F893)
};

// [RECONSTRUCTED - 0x0099FC30] (esi = instance, edi = region position) object cell; the position takes its height
CRTNavCellTri* NavObj_FindCell(SNavMeshInst* pInst, SNavVec3* pPos);

// [RECONSTRUCTED - 0x0099FCF0] (eax = instance, ecx = region position) 1 when the object mesh contains the point,
// -1 for dungeon resources
int32_t NavObj_IsInside(SNavMeshInst* pInst, const SNavVec3* pPos);

// [RECONSTRUCTED - 0x0099FFB0] (ecx = position) wraps an outdoor position into the neighbouring region;
// returns 1 when the region changed
int32_t NavPos_WrapOutdoor(tagNavPos* pPos);

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHTERRAIN_H_
