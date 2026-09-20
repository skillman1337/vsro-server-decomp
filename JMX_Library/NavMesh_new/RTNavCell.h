/**
 * ============================================================================
 * Joymax NavMesh - Navigation Cells
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavCell.h
 *
 * Implements:
 *   - _ICell         .?AV_ICell@@          (RTTI 0x00C8137C)
 *   - CRTNavCell     .?AVCRTNavCell@@      (RTTI 0x00C81388), vftable 0x00B43750
 *   - CRTNavCellQuad .?AVCRTNavCellQuad@@  (RTTI 0x00C813A4), vftable 0x00B4365C, size 0x78
 *   - CRTNavCellTri  .?AVCRTNavCellTri@@   (RTTI 0x00C81600), vftable 0x00B43ED8, size 0x8C
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVCELL_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVCELL_H_

#include <cstdint>
#include <vector>
#include "NavMath.h"
#include "RTNavMesh.h"

namespace NavMesh {

class CNavArchive;
class CRTNavEdge;
class CRTNavMeshObj;
class CRTNavMeshTerrain;
struct SNavMeshInst;
struct SNavObjEdge;

/**
 * _ICell / CRTNavCell (vftable 0x00B43750)
 * CORRECTION (Claude): the slot names follow the native bodies. Slot 1 is the walkable slope factor (1.0 on the base),
 * slot 2 is true for terrain cells and false for object triangles (it was CanSpawnNPC), and slot 7 returns the cell kind.
 */
class _ICell {
public:
	virtual uint32_t GetIndex() const = 0;                                // vftable[0]
	virtual float    GetSlopeFactor() = 0;                                // vftable[1]
	virtual bool     IsTerrainCell() const = 0;                           // vftable[2]
	virtual uint32_t GetTileFlag(const SNavVec3* pPos) = 0;               // vftable[3]
	virtual const SNavVec3* GetNormal(const tagNavPos* pPos) = 0;         // vftable[4]
	virtual intptr_t GetOwnerKey(const tagNavPos* pPos) = 0;              // vftable[5]
	virtual intptr_t GetDungeonBlock(const tagNavPos* pPos) = 0;          // vftable[6]
	virtual uint8_t  GetCellType() const = 0;                             // vftable[7]: 0 base, 1 triangle, 2 quad
	virtual ~_ICell() {}                                                  // vftable[8]
};

class CRTNavCell : public _ICell {
public:
	CRTNavCell() : m_pMesh(nullptr), m_nIndex(0) {}
	virtual ~CRTNavCell() override {}                                     // vftable[8] 0x009A73C0

	// [RECONSTRUCTED - 0x00404050] mov eax, [ecx+8]
	virtual uint32_t GetIndex() const override { return static_cast<uint32_t>(m_nIndex); }
	// [RECONSTRUCTED - 0x009A7330] fld1
	virtual float GetSlopeFactor() override { return 1.0f; }
	// [RECONSTRUCTED - 0x00455EB0] mov al, 1
	virtual bool IsTerrainCell() const override { return true; }
	// [RECONSTRUCTED - 0x00537ED0] xor eax, eax / retn 4
	virtual uint32_t GetTileFlag(const SNavVec3* /*pPos*/) override { return 0; }
	// [RECONSTRUCTED - 0x00537ED0]
	virtual const SNavVec3* GetNormal(const tagNavPos* /*pPos*/) override { return nullptr; }
	// [RECONSTRUCTED - 0x009C1450] or eax, -1 / retn 4
	virtual intptr_t GetOwnerKey(const tagNavPos* /*pPos*/) override { return -1; }
	// [RECONSTRUCTED - 0x009C1450]
	virtual intptr_t GetDungeonBlock(const tagNavPos* /*pPos*/) override { return -1; }
	// [RECONSTRUCTED - 0x004C5D50] xor al, al
	virtual uint8_t GetCellType() const override { return 0; }

public:
	CRTNavMesh* m_pMesh;  // +0x04: owning mesh (terrain 0x0099F5D5, object 0x009C01DC)
	int32_t     m_nIndex; // +0x08: index in the owner's cell vector
};

/**
 * CRTNavCellQuad (vftable 0x00B4365C, 0x78) - axis-aligned terrain cell
 */
class CRTNavCellQuad : public CRTNavCell {
public:
	// [RECONSTRUCTED - 0x009A2650] constructor, 0x009C0FD0 destructor
	CRTNavCellQuad();
	virtual ~CRTNavCellQuad() override;

	// [RECONSTRUCTED - 0x009C1410] -> CRTNavMeshTerrain::GetTileFlag 0x0099F820
	virtual uint32_t GetTileFlag(const SNavVec3* pPos) override;
	// [RECONSTRUCTED - 0x009C1430] -> CRTNavMeshTerrain::GetTileNormal 0x0099F860
	virtual const SNavVec3* GetNormal(const tagNavPos* pPos) override;
	// [RECONSTRUCTED - 0x009A7320] mov al, 2
	virtual uint8_t GetCellType() const override { return 2; }

	// [RECONSTRUCTED - 0x009C10E0] vftable[9]
	// Returns 2 with the neighbour cell, the edge and the crossing point when the segment leaves through an edge,
	// 1 when every edge leaves it inside this cell, 0 otherwise.
	virtual int32_t FindCrossEdge(const SNavLine2* pSegment, CRTNavCell** ppOutCell, CRTNavEdge** ppOutEdge, SNavVec2* pOutPoint);

	// [RECONSTRUCTED - 0x009C1210] vftable[10]
	virtual bool Load(CNavArchive* pArchive);

	// [RECONSTRUCTED - 0x009C1050] moves the point 0.2 towards the centre (onto it when closer)
	void NudgeToCenter(SNavVec2* pPoint) const;

	// [RECONSTRUCTED - 0x009C1460 / 0x009C14B0] both segment ends inside the bounds (inclusive)
	bool IsSegmentInside(const SNavLine2* pSegment) const;

	// [RECONSTRUCTED - 0x009C1350] removes edges whose IsInternal() is false before global edges are relinked
	void PurgeGlobalEdges();

	void AddEdge(uint8_t byDirection, CRTNavEdge* pEdge);

	CRTNavMeshTerrain* GetTerrain() const;

public:
	std::vector<SNavMeshInst*> m_vecObjects;   // +0x0C: object instances overlapping the cell
	int32_t                    m_nEdgeCount;   // +0x1C: edges linked into the direction lists (0x009C0AA1)
	float                      m_fCenterX;     // +0x20
	float                      m_fCenterZ;     // +0x24
	float                      m_fMinX;        // +0x28
	float                      m_fMinZ;        // +0x2C
	float                      m_fMaxX;        // +0x30
	float                      m_fMaxZ;        // +0x34
	std::vector<CRTNavEdge*>   m_avecEdges[4]; // +0x38: by direction (array constructor 0x009DD3D7, 4 x 0x10)
};

/**
 * Triangle geometry cached at CRTNavCellTri +0x34 (0x40)
 */
struct SNavTriData {
	SNavVec3  v0;     // +0x00
	SNavVec3  v1;     // +0x0C
	SNavVec3  v2;     // +0x18
	SNavVec3  normal; // +0x24
	SNavPlane plane;  // +0x30
};

/**
 * Object mesh vertex (0x14)
 */
struct SNavObjVertex {
	SNavVec3 pos; // +0x00
	SNavVec2 dir; // +0x0C: unit direction from the loader's 256-entry table (0x009A9BA0)
};

/**
 * CRTNavCellTri (vftable 0x00B43ED8, 0x8C) - object mesh triangle
 */
class CRTNavCellTri : public CRTNavCell {
public:
	// [RECONSTRUCTED - 0x009BDD20 / 0x009BF6F0] constructors, 0x009C0180 destructor
	CRTNavCellTri();
	virtual ~CRTNavCellTri() override {}

	// [RECONSTRUCTED - 0x009C03E0] 1 - acos(normal.y) / 1.57, clamped to [0, 1]
	virtual float GetSlopeFactor() override;
	// [RECONSTRUCTED - 0x004C5D50]
	virtual bool IsTerrainCell() const override { return false; }
	// [RECONSTRUCTED - 0x009C04C0] triangle normal rotated into the region by the instance matrix
	virtual const SNavVec3* GetNormal(const tagNavPos* pPos) override;
	// [RECONSTRUCTED - 0x009C05F0] mov eax, [arg]; returns the first field of the position (its cell)
	virtual intptr_t GetOwnerKey(const tagNavPos* pPos) override;
	// [PARTIAL - 0x009C0600] dungeon block of the position (0x0099A520); dungeons are not ported
	virtual intptr_t GetDungeonBlock(const tagNavPos* pPos) override;
	// [RECONSTRUCTED - 0x00455EB0]
	virtual uint8_t GetCellType() const override { return 1; }
	// [RECONSTRUCTED - 0x009BF530] vftable[9]: writes 0xFFFF
	virtual void GetDungeonBlockID(uint16_t* pOut) { *pOut = 0xFFFF; }

	// [RECONSTRUCTED - 0x009C01D0] (eax = this, esi = v0, edx = v1, ecx = v2)
	void Init(CRTNavMeshObj* pMesh, int32_t nIndex, SNavObjVertex* pV0, SNavObjVertex* pV1, SNavObjVertex* pV2, uint16_t wFlag);

	// [RECONSTRUCTED - 0x009C0300] at most 4 edges (the native fourth slot overlaps the triangle cache at +0x34)
	void AddEdge(SNavObjEdge* pEdge);
	// [RECONSTRUCTED - 0x009C04A0]
	SNavObjEdge* GetEdge(int32_t nIndex) const { return m_apEdge[nIndex]; }

	// [RECONSTRUCTED - 0x009C0380] lazily copies the vertices and derives the normal / plane (0x009BF780)
	const SNavTriData* GetTriData();
	// [RECONSTRUCTED - 0x009C03B0] lazily builds the height plane from the vertices (0x00537E10)
	const SNavPlane* GetHeightPlane();

	// [RECONSTRUCTED - 0x009C0330 -> 0x009C0620] sets pPos->y on the triangle plane; true when inside (XZ)
	bool IsInside(SNavVec3* pPos);
	// [RECONSTRUCTED - 0x009C0350] pPos->y from the height plane (0 when the plane is vertical)
	void ApplyHeight(SNavVec3* pPos);
	// [RECONSTRUCTED - 0x009C0270] moves the point 0.2 towards the centre (onto it when closer)
	void NudgeToCenter(SNavVec2* pPoint) const;
	// [RECONSTRUCTED - 0x009B58D0] -1 outside, 0 on an edge, 1 strictly inside (XZ)
	int32_t ClassifyPoint(const SNavVec2* pPoint);

	CRTNavMeshObj* GetObjMesh() const;

public:
	SNavObjVertex* m_apVertex[3];  // +0x0C
	float          m_fCenterX;     // +0x18
	float          m_fCenterZ;     // +0x1C
	uint16_t       m_wFlag;        // +0x20
	int32_t        m_nEdgeCount;   // +0x24
	SNavObjEdge*   m_apEdge[4];    // +0x28
	SNavTriData    m_TriData;      // +0x34
	SNavPlane      m_HeightPlane;  // +0x74
	float          m_fSlope;       // +0x84
	uint8_t        m_bTriDataValid; // +0x88
	uint8_t        m_bPlaneValid;  // +0x89
	uint8_t        m_bSlopeValid;  // +0x8A
	uint8_t        m_byEventZone;  // +0x8B: bits 0-5 event index, bit 6 enter, bit 7 leave
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVCELL_H_
