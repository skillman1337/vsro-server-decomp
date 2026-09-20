/**
 * ============================================================================
 * Joymax NavMesh - Navigation Edges
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavEdge.cpp
 *
 * Implements:
 *   - NavEdge             .?AUNavEdge@@             vftable 0x00B436A8 (serialisable edge record)
 *   - NavEdgeGlobal       .?AUNavEdgeGlobal@@       vftable 0x00B436BC
 *   - CRTNavEdge          .?AVCRTNavEdge@@          vftable 0x00B436D0 (pure)
 *   - CRTNavEdgeInternal  .?AVCRTNavEdgeInternal@@  vftable 0x00B436F8, size 0x38
 *   - CRTNavEdgeGlobal    .?AVCRTNavEdgeGlobal@@    vftable 0x00B43724, size 0x3C
 *   - SNavObjEdge         object mesh edge (0x3C, no vftable; 0x009C0050 / 0x009C0070)
 *
 * CORRECTION (Claude): replaces the earlier weighted graph edge (ConnectCells / GetCost), which has no counterpart
 * in the binary.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVEDGE_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVEDGE_H_

#include <cstdint>
#include "NavMath.h"

namespace NavMesh {

class CNavArchive;
class CRTNavCell;
class CRTNavCellTri;
class CRTNavMeshObj;
class CRTNavMeshTerrain;
struct SNavObjVertex;

/**
 * NavEdge (vftable 0x00B436A8) - embedded in CRTNavEdge at +0x10
 */
struct NavEdge {
	virtual int32_t IsGlobal() const { return 0; }   // vftable[0] 0x00559C70
	virtual int32_t IsInternal() const { return 1; } // vftable[1] 0x005ECFD0
	// vftable[2] 0x009A7430 is the writer used by the tools; the server never saves navmesh data.
	// [RECONSTRUCTED - 0x009A75E0] vftable[3]
	virtual bool    Load(CNavArchive* pArchive);
	virtual ~NavEdge() {}

	SNavLine2 line = {};         // +0x04 (edge +0x14)
	SNavVec2  normal = {};       // +0x14 (edge +0x24): derived on first use by 0x009A7750
	uint8_t   bNormalValid = 0;  // +0x18 (edge +0x2C)
	uint8_t   flag = 0;          // +0x20 (edge +0x30): bits 0x03 block the edge (vftable[4])
	uint8_t   assocDir[2] = {};  // +0x21 (edge +0x31): direction list index in cell A / B
	uint16_t  assocCell[2] = {}; // +0x24 (edge +0x34): cell index A / B
};

/**
 * NavEdgeGlobal (vftable 0x00B436BC)
 */
struct NavEdgeGlobal : public NavEdge {
	virtual int32_t IsGlobal() const override { return 1; }   // 0x005ECFD0
	virtual int32_t IsInternal() const override { return 0; } // 0x00559C70
	// [RECONSTRUCTED - 0x009A7580] vftable[3]
	virtual bool    Load(CNavArchive* pArchive) override;

	uint16_t assocRgn[2] = {}; // +0x28 (edge +0x38): region of cell A / B
};

/**
 * CRTNavEdge (vftable 0x00B436D0)
 * The shared slots dispatch to the same bodies in both derived tables, so they are plain members here.
 */
class CRTNavEdge {
public:
	CRTNavEdge() : m_pMesh(nullptr), m_pCellA(nullptr), m_pCellB(nullptr) {}
	virtual ~CRTNavEdge() {} // vftable[9]: 0x009A78A0 / 0x009A7830

	// [RECONSTRUCTED - 0x009A76C0] vftable[0]
	uint8_t GetFlag() const { return Data().flag; }
	// [RECONSTRUCTED - 0x009A7850] vftable[1]
	SNavLine2* GetLine() { return &Data().line; }
	// [RECONSTRUCTED - 0x009A7860] vftable[2]
	void SetFlag(uint8_t byFlag) { Data().flag = byFlag; }
	// [RECONSTRUCTED - 0x009A7870 -> 0x009A76D0] vftable[3]: 2 in front (dot > tol), 1 behind (dot < -tol), 0 on the line
	int32_t SideTest(const SNavVec2* pPoint, float fTolerance);
	// [RECONSTRUCTED - 0x009A7810] vftable[4]
	int32_t IsBlocked() const { return ((Data().flag & 3) != 0) ? 1 : 0; }
	// [RECONSTRUCTED - 0x009A7820] vftable[5] -> m_sData vftable[0]
	int32_t IsGlobal() const { return Data().IsGlobal(); }
	// [RECONSTRUCTED - 0x009A7890] vftable[6] -> m_sData vftable[1]
	int32_t IsInternal() const { return Data().IsInternal(); }
	// vftable[7]: 0x009C0A40 internal / 0x009C0B10 global
	virtual void Link() = 0;
	// [RECONSTRUCTED - 0x009C0BD0] vftable[8] -> m_sData vftable[3]
	bool Load(CNavArchive* pArchive) { return Data().Load(pArchive); }

	// [RECONSTRUCTED - 0x009C0A30] the cell on the other side of pCell
	CRTNavCell* GetOtherCell(const CRTNavCell* pCell) const { return (m_pCellA == pCell) ? m_pCellB : m_pCellA; }

	virtual NavEdge&       Data() = 0;
	virtual const NavEdge& Data() const = 0;

public:
	CRTNavMeshTerrain* m_pMesh;  // +0x04: 0x009C0A20
	CRTNavCell*        m_pCellA; // +0x08
	CRTNavCell*        m_pCellB; // +0x0C
};

class CRTNavEdgeInternal : public CRTNavEdge {
public:
	// [RECONSTRUCTED - 0x009C0A40] vftable[7]
	virtual void Link() override;
	virtual NavEdge&       Data() override { return m_sData; }
	virtual const NavEdge& Data() const override { return m_sData; }

	NavEdge m_sData; // +0x10
};

class CRTNavEdgeGlobal : public CRTNavEdge {
public:
	// [RECONSTRUCTED - 0x009C0B10] vftable[7]
	virtual void Link() override;
	virtual NavEdge&       Data() override { return m_sData; }
	virtual const NavEdge& Data() const override { return m_sData; }

	NavEdgeGlobal m_sData; // +0x10
};

/**
 * SNavObjEdge (0x3C) - object mesh edge
 * Flag bits by use: 0x01 / 0x02 block one side (0x009B5768 / 0x009B5789), 0x04 internal edge (0x009B5570),
 * 0x08 link to another object (0x009B557A), 0x10 skipped by the grid search (0x009B4FF9), 0x12 blocks leaving
 * (0x009B55B3), 0x20 railing (0x009B51D8), 0x80 siege gate (0x009B518D).
 */
struct SNavObjEdge {
	uint8_t        flag = 0;          // +0x00
	SNavObjVertex* pVertexA = nullptr; // +0x04
	SNavObjVertex* pVertexB = nullptr; // +0x08
	CRTNavCellTri* pCellA = nullptr;   // +0x0C
	CRTNavCellTri* pCellB = nullptr;   // +0x10
	CRTNavMeshObj* pMesh = nullptr;    // +0x14
	uint8_t        bLineValid = 0;     // +0x18
	SNavLine2      line = {};          // +0x1C: vertex A (x, z) to vertex B (x, z)
	uint8_t        byUnk34 = 0;        // +0x34: cleared whenever the line cache is rebuilt (0x009C00E4)
	uint8_t        byEventZone = 0;    // +0x38: bits 0-5 event index, bits 6-7 trigger mode

	// [RECONSTRUCTED - 0x009C0070] (eax = this, ecx = mesh, edx = vertex A)
	void Init(CRTNavMeshObj* pOwner, SNavObjVertex* pA, SNavObjVertex* pB, CRTNavCellTri* pCellOfA, CRTNavCellTri* pCellOfB, uint8_t byFlag);
	// [RECONSTRUCTED - 0x009C00C0]
	SNavLine2* GetLine();
	// [RECONSTRUCTED - 0x009C0110]
	SNavObjVertex* GetVertex(int32_t nIndex) const { return (nIndex == 0) ? pVertexA : ((nIndex == 1) ? pVertexB : nullptr); }
	// [RECONSTRUCTED - 0x009C0130]
	CRTNavCellTri* GetCell(int32_t nIndex) const { return (nIndex == 0) ? pCellA : ((nIndex == 1) ? pCellB : nullptr); }
	// [RECONSTRUCTED - 0x009C04B0]
	CRTNavCellTri* GetOtherCell(const CRTNavCellTri* pCell) const { return (pCellA == pCell) ? pCellB : pCellA; }
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVEDGE_H_
