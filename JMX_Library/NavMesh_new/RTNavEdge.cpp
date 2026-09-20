/**
 * ============================================================================
 * Joymax NavMesh - Navigation Edges
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavEdge.cpp
 *
 * Implements:
 *   - NavEdge::Load                 @ 0x009A75E0
 *   - NavEdgeGlobal::Load           @ 0x009A7580
 *   - CRTNavEdge::SideTest          @ 0x009A7870 -> 0x009A76D0 (normal 0x009A7750)
 *   - CRTNavEdgeInternal::Link      @ 0x009C0A40
 *   - CRTNavEdgeGlobal::Link        @ 0x009C0B10
 *   - SNavObjEdge::Init             @ 0x009C0070
 *   - SNavObjEdge::GetLine          @ 0x009C00C0
 * ============================================================================
 */

#include "RTNavEdge.h"
#include "NavArchive.h"
#include "RTNavCell.h"
#include "RTNavMeshObj.h"
#include "RTNavMeshTerrain.h"
#include "RegionManagerBody.h"
#include "../BSLib/BSLog.h"

namespace NavMesh {

/*
================
NavEdge::Load
[RECONSTRUCTED - 0x009A75E0]
================
*/
bool NavEdge::Load(CNavArchive* pArchive) {
	pArchive->Read(&line, sizeof(line));
	flag = pArchive->ReadValue<uint8_t>();
	assocDir[0] = pArchive->ReadValue<uint8_t>();
	assocDir[1] = pArchive->ReadValue<uint8_t>();
	assocCell[0] = pArchive->ReadValue<uint16_t>();
	assocCell[1] = pArchive->ReadValue<uint16_t>();
	return !pArchive->IsFailed();
}

/*
================
NavEdgeGlobal::Load
[RECONSTRUCTED - 0x009A7580]
================
*/
bool NavEdgeGlobal::Load(CNavArchive* pArchive) {
	NavEdge::Load(pArchive);
	assocRgn[0] = pArchive->ReadValue<uint16_t>();
	assocRgn[1] = pArchive->ReadValue<uint16_t>();
	return !pArchive->IsFailed();
}

/*
================
CRTNavEdge::SideTest
[RECONSTRUCTED - 0x009A7870 -> 0x009A76D0]
The normal is (dz, -dx) of the normalised edge direction, built on first use (0x009A7750).
================
*/
int32_t CRTNavEdge::SideTest(const SNavVec2* pPoint, float fTolerance) {
	NavEdge& data = Data();
	if (data.bNormalValid == 0) {
		SNavVec2 dir;
		dir.x = static_cast<float>(static_cast<double>(data.line.x1) - data.line.x0);
		dir.z = static_cast<float>(static_cast<double>(data.line.z1) - data.line.z0);
		Vec2_Normalize(&dir);
		data.normal.x = dir.z;
		data.normal.z = -dir.x;
		data.bNormalValid = 1;
	}

	// Offsets and the dot product are stored as floats (0x009A76E7 / 0x009A76F1 / 0x009A7705); the products are
	// evaluated in double between those stores.
	const float fOffsetX = static_cast<float>(static_cast<double>(pPoint->x) - data.line.x0);
	const float fOffsetZ = static_cast<float>(static_cast<double>(pPoint->z) - data.line.z0);
	const float fDot = static_cast<float>(static_cast<double>(data.normal.z) * fOffsetZ + static_cast<double>(data.normal.x) * fOffsetX);
	if (fTolerance < fDot) {
		return 2;
	}
	if (-fTolerance > fDot) {
		return 1;
	}
	return 0;
}

/*
================
CRTNavEdgeInternal::Link
[RECONSTRUCTED - 0x009C0A40]
Cell B is only resolved for passable edges.
================
*/
void CRTNavEdgeInternal::Link() {
	CRTNavCellQuad* pCellA = m_pMesh->GetCell(m_sData.assocCell[0]);
	m_pCellA = pCellA;
	if (pCellA != nullptr) {
		pCellA->AddEdge(m_sData.assocDir[0], this);
	}

	if ((m_sData.flag & 3) == 0) {
		CRTNavCellQuad* pCellB = m_pMesh->GetCell(m_sData.assocCell[1]);
		m_pCellB = pCellB;
		if (pCellB != nullptr) {
			pCellB->AddEdge(m_sData.assocDir[1], this);
		}
	}
}

/*
================
CRTNavEdgeGlobal::Link
[RECONSTRUCTED - 0x009C0B10]
Only cell A (in this region) keeps the edge; the neighbour region's own global edge links the other side.
================
*/
void CRTNavEdgeGlobal::Link() {
	if (m_pMesh->GetRegionID() != m_sData.assocRgn[0]) {
		BSLib::AssertReport(0x46, "D:\\WORK2005\\Source\\JMX_Library\\NavMesh_new\\RTNavEdge.cpp",
			"pRgnA->GetRegionID() == m_sData.AssocRgn[0]");
	}

	CRTNavCellQuad* pCellA = m_pMesh->GetCell(m_sData.assocCell[0]);
	m_pCellA = pCellA;
	pCellA->AddEdge(m_sData.assocDir[0], this);

	m_pCellB = nullptr;
	if ((m_sData.flag & 3) == 0) {
		CRTNavMesh* pOther = g_pRegionManager->FindNavMesh(m_sData.assocRgn[1]);
		if (pOther != nullptr) {
			// 0x009C0F90 indexes the cell vector of the neighbour as a terrain mesh
			m_pCellB = static_cast<CRTNavMeshTerrain*>(pOther)->GetCell(m_sData.assocCell[1]);
		}
	}
}

/*
================
SNavObjEdge::Init
[RECONSTRUCTED - 0x009C0070]
================
*/
void SNavObjEdge::Init(CRTNavMeshObj* pOwner, SNavObjVertex* pA, SNavObjVertex* pB, CRTNavCellTri* pCellOfA,
	CRTNavCellTri* pCellOfB, uint8_t byFlag) {
	pMesh = pOwner;
	pVertexA = pA;
	pVertexB = pB;
	pCellA = pCellOfA;
	pCellB = pCellOfB;
	flag = byFlag;
	bLineValid = 0;
	byEventZone = 0;
}

/*
================
SNavObjEdge::GetLine
[RECONSTRUCTED - 0x009C00C0]
================
*/
SNavLine2* SNavObjEdge::GetLine() {
	if (bLineValid == 0) {
		const float fAZ = pVertexA->pos.z;
		const float fBX = pVertexB->pos.x;
		const float fBZ = pVertexB->pos.z;
		byUnk34 = 0;
		line.x0 = pVertexA->pos.x;
		line.z0 = fAZ;
		line.x1 = fBX;
		line.z1 = fBZ;
		bLineValid = 1;
	}
	return &line;
}

} // namespace NavMesh
