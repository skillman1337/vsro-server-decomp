/**
 * ============================================================================
 * Joymax NavMesh - Object Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshObj.cpp
 *
 * Implements:
 *   - SNavMeshInst::SNavMeshInst          @ 0x009ACE50
 *   - CRTNavMeshObj::CRTNavMeshObj        @ 0x009B2E10
 *   - CRTNavMeshObj::FindHeight           @ 0x009B4A90 (vftable[1])
 *   - CRTNavMeshObj::Move                 @ 0x009B5A80 (vftable[2])
 *   - CRTNavMeshObj::Load                 @ 0x009B36A0
 *   - CRTNavMeshObj::LoadGrid             @ 0x009B3270
 *   - CRTNavMeshObj::FindCell             @ 0x009B4B00
 *   - CRTNavMeshObj::IsInside             @ 0x009B4C50
 *   - CRTNavMeshObj::TriggerEventZone     @ 0x009B4CD0
 *   - CRTNavMeshObj::CrossGrid            @ 0x009B4D30
 *   - CRTNavMeshObj::WalkCells            @ 0x009B5380
 *   - CRTNavMeshObj::FindLinkEntryCell    @ 0x009B5800
 *   - CRTNavMeshObj::FindGlobalEdgeIndex  @ 0x009B31E0
 * ============================================================================
 */

#include "RTNavMeshObj.h"
#include "MapLoader.h"
#include "NavArchive.h"
#include "RegionManagerBody.h"
#include "../BSLib/BSLog.h"

#include <cmath>
#include <cstring>

namespace NavMesh {

namespace {

// 0x00B46068: distance a blocked point is pushed off an object edge along the vertex direction, stored as the
// double 0.009999999776482582 (float 0.01f widened)
constexpr double kNavEdgePushOut = 0.009999999776482582;
// 0x00B45AF0 (double 100.0): object grid tile size
constexpr double kNavObjGridTile = 100.0;

const char* const kRTNavMeshObjFile = "D:\\WORK2005\\Source\\JMX_Library\\NavMesh_new\\RTNavMeshObj.cpp";

// [RECONSTRUCTED - 0x009B5277 .. 0x009B52E0 / 0x009B566F .. 0x009B56D8] the vertex of the edge nearer to the point
// supplies the push-out direction (vertex B when it is not farther than vertex A)
void PushOffEdge(const SNavObjEdge* pEdge, float fX, float fZ, float* pOutX, float* pOutZ) {
	const SNavObjVertex* pA = pEdge->GetVertex(0);
	const double fADX = static_cast<float>(pA->pos.x - fX);
	const double fADZ = static_cast<float>(pA->pos.z - fZ);
	const float fDistA = static_cast<float>(fADX * fADX + fADZ * fADZ);
	const SNavObjVertex* pB = pEdge->GetVertex(1);
	const double fBDX = static_cast<float>(pB->pos.x - fX);
	const double fBDZ = static_cast<float>(pB->pos.z - fZ);
	const float fDistB = static_cast<float>(fBDZ * fBDZ + fBDX * fBDX);

	// The offset is stored as a float before it is added to the crossing point (0x009B528E / 0x009B529D, 0x009B52E0)
	const SNavObjVertex* pNearest = (fDistB <= fDistA) ? pB : pA;
	const float fOffsetX = static_cast<float>(pNearest->dir.x * kNavEdgePushOut);
	const float fOffsetZ = static_cast<float>(pNearest->dir.z * kNavEdgePushOut);
	*pOutX = fX + fOffsetX;
	*pOutZ = fZ + fOffsetZ;
}

// [RECONSTRUCTED - 0x0098ADD0 caller 0x009B4321 .. 0x009B439D] a single event zone named exactly "event" / "EVENT"
bool IsEventOnlyName(const std::string& strName) {
	return strName == "event" || strName == "EVENT";
}

} // namespace

// ============================================================================
// SNavMeshInst
// ============================================================================

SNavMeshInst::SNavMeshInst()
	: wPad00(0)
	, entry{}
	, pTerrain(nullptr)
	, pResource(nullptr)
	, pDungeonLink(nullptr) {
	std::memset(matLocalToWorld.m, 0, sizeof(matLocalToWorld.m));
	matLocalToWorld.m[0] = matLocalToWorld.m[5] = matLocalToWorld.m[10] = matLocalToWorld.m[15] = 1.0f;
	std::memset(matWorldToLocal.m, 0, sizeof(matWorldToLocal.m));
	matWorldToLocal.m[0] = matWorldToLocal.m[5] = matWorldToLocal.m[10] = matWorldToLocal.m[15] = 1.0f;
}

CRTNavMeshObj* SNavMeshInst::GetObjResource() const {
	return static_cast<CRTNavMeshObj*>(pResource);
}

// ============================================================================
// CRTNavMeshObj
// ============================================================================

CRTNavMeshObj::CRTNavMeshObj()
	: m_pObjectInfo(nullptr)
	, m_fGridOriginX(0.0f)
	, m_fGridOriginZ(0.0f)
	, m_nGridWidth(0)
	, m_nGridHeight(0)
	, m_bEventOnly(0)
	, m_dwStructOption(0) {
	m_nMeshType = 2;
}

CRTNavMeshObj::~CRTNavMeshObj() {
}

/*
================
CRTNavMeshObj::FindHeight
[RECONSTRUCTED - 0x009B4A90]
================
*/
int32_t CRTNavMeshObj::FindHeight(SNavVec3* pPos) {
	for (size_t i = 0; i < m_vecCells.size(); ++i) {
		if (m_vecCells[i].IsInside(pPos)) {
			return 1;
		}
	}
	return 0;
}

/*
================
CRTNavMeshObj::Load
[RECONSTRUCTED - 0x009B36A0]
Vertices, triangles, the global and internal edge groups, event zone names (bit 2) and the edge grid.
================
*/
bool CRTNavMeshObj::Load(CNavArchive* pArchive, CMapLoader* pLoader, uint32_t dwStructOption) {
	m_dwStructOption = dwStructOption;

	const uint32_t dwVertexCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecVertices.resize(dwVertexCount);
	for (uint32_t i = 0; i < dwVertexCount; ++i) {
		pArchive->Read(&m_vecVertices[i].pos, sizeof(SNavVec3));
		const uint8_t byDir = pArchive->ReadValue<uint8_t>();
		m_vecVertices[i].dir = pLoader->GetDirection(byDir);
	}

	const uint32_t dwCellCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecCells.resize(dwCellCount);
	for (uint32_t i = 0; i < dwCellCount; ++i) {
		const uint16_t wV0 = pArchive->ReadValue<uint16_t>();
		const uint16_t wV1 = pArchive->ReadValue<uint16_t>();
		const uint16_t wV2 = pArchive->ReadValue<uint16_t>();
		const uint16_t wFlag = pArchive->ReadValue<uint16_t>();
		if (wV0 >= dwVertexCount || wV1 >= dwVertexCount || wV2 >= dwVertexCount) {
			return false; // 0x009B39A4: vector subscript out of range
		}
		m_vecCells[i].Init(this, static_cast<int32_t>(i), &m_vecVertices[wV0], &m_vecVertices[wV1], &m_vecVertices[wV2], wFlag);
		if ((dwStructOption & 2) != 0) {
			m_vecCells[i].m_byEventZone = pArchive->ReadValue<uint8_t>();
		}
	}

	for (int32_t nGroup = 0; nGroup < 2; ++nGroup) {
		const uint32_t dwEdgeCount = pArchive->ReadValue<uint32_t>();
		if (pArchive->IsFailed()) {
			return false;
		}
		std::vector<SNavObjEdge>& vecEdges = m_avecEdges[nGroup];
		vecEdges.assign(dwEdgeCount, SNavObjEdge());
		for (uint32_t i = 0; i < dwEdgeCount; ++i) {
			const uint16_t wVertexA = pArchive->ReadValue<uint16_t>();
			const uint16_t wVertexB = pArchive->ReadValue<uint16_t>();
			const uint16_t wCellA = pArchive->ReadValue<uint16_t>();
			const uint16_t wCellB = pArchive->ReadValue<uint16_t>();
			const uint8_t  byFlag = pArchive->ReadValue<uint8_t>();
			if (wCellA >= dwCellCount || (wCellB != 0xFFFF && wCellB >= dwCellCount) ||
				wVertexA >= dwVertexCount || wVertexB >= dwVertexCount) {
				return false; // 0x009B3D37 / 0x009B39A4
			}

			SNavObjEdge& edge = vecEdges[i];
			CRTNavCellTri* pCellA = &m_vecCells[wCellA];
			pCellA->AddEdge(&edge);
			CRTNavCellTri* pCellB = nullptr;
			if (wCellB != 0xFFFF) {
				pCellB = &m_vecCells[wCellB];
				pCellB->AddEdge(&edge);
			}
			edge.Init(this, &m_vecVertices[wVertexA], &m_vecVertices[wVertexB], pCellA, pCellB, byFlag);
			if ((dwStructOption & 1) != 0) {
				edge.byEventZone = pArchive->ReadValue<uint8_t>();
			}
		}
	}

	if ((dwStructOption & 4) != 0) {
		const uint32_t dwEventCount = pArchive->ReadValue<uint32_t>();
		if (pArchive->IsFailed()) {
			return false;
		}
		// 0x009B3F8A: the names are kept unless the region manager has user data (+0xC4); the port has none.
		m_vecEventZones.assign(dwEventCount, nullptr);
		m_vecEventNames.assign(dwEventCount, std::string());
		for (uint32_t i = 0; i < dwEventCount; ++i) {
			pArchive->ReadString(m_vecEventNames[i]);
			m_vecEventZones[i] = g_pRegionManagerBody->RegisterEventZone(m_vecEventNames[i], nullptr);
		}
		if (dwEventCount == 1 && dwCellCount == 2 && IsEventOnlyName(m_vecEventNames[0])) {
			m_bEventOnly = 1;
		}
	}

	return LoadGrid(pArchive) && !pArchive->IsFailed();
}

/*
================
CRTNavMeshObj::LoadGrid
[RECONSTRUCTED - 0x009B3270]
================
*/
bool CRTNavMeshObj::LoadGrid(CNavArchive* pArchive) {
	m_fGridOriginX = pArchive->ReadValue<float>();
	m_fGridOriginZ = pArchive->ReadValue<float>();
	m_nGridWidth = pArchive->ReadValue<int32_t>();
	m_nGridHeight = pArchive->ReadValue<int32_t>();
	const uint32_t dwTileCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}

	const std::vector<SNavObjEdge>& vecGlobal = m_avecEdges[0];
	m_vecGrid.assign(dwTileCount, std::vector<SNavObjEdgeRef>());
	for (uint32_t nTile = 0; nTile < dwTileCount; ++nTile) {
		const uint32_t dwRefCount = pArchive->ReadValue<uint32_t>();
		if (pArchive->IsFailed()) {
			return false;
		}
		std::vector<SNavObjEdgeRef>& vecRefs = m_vecGrid[nTile];
		vecRefs.resize(dwRefCount);
		for (uint32_t i = 0; i < dwRefCount; ++i) {
			const uint16_t wIndex = pArchive->ReadValue<uint16_t>();
			vecRefs[i].wIndex = wIndex;
			if (wIndex >= vecGlobal.size()) {
				return false; // 0x009B3461
			}
			vecRefs[i].pEdge = const_cast<SNavObjEdge*>(&vecGlobal[wIndex]);
		}
	}
	return true;
}

/*
================
CRTNavMeshObj::FindCell
[RECONSTRUCTED - 0x009B4B00]
The candidate height starts at -1.0 (0x00B45D80) and is replaced by the first hit, then by closer ones.
The position takes the height of the chosen triangle.
The native copy of the best point is uninitialised when nothing matched; the port starts it at the input.
================
*/
CRTNavCellTri* CRTNavMeshObj::FindCell(SNavVec3* pLocalPos) {
	float fBestDiff = -1.0f;
	SNavVec3 probe = *pLocalPos;
	SNavVec3 best = *pLocalPos;
	CRTNavCellTri* pBest = nullptr;

	for (size_t i = 0; i < m_vecCells.size(); ++i) {
		CRTNavCellTri& cell = m_vecCells[i];
		if (!cell.IsInside(&probe)) {
			continue;
		}
		const float fDiff = std::fabs(probe.y - pLocalPos->y);
		if (fDiff < fBestDiff || 0.0f > fBestDiff) {
			fBestDiff = fDiff;
			pBest = &cell;
			best = probe;
		}
	}

	*pLocalPos = best;
	if (pBest != nullptr && (pBest->m_byEventZone & 0xC0) != 0) {
		TriggerEventZone(pBest->m_nIndex, -1, static_cast<uint8_t>(pBest->m_byEventZone >> 6),
			static_cast<uint8_t>(pBest->m_byEventZone & 0x3F));
	}
	return pBest;
}

/*
================
CRTNavMeshObj::IsInside
[RECONSTRUCTED - 0x009B4C50]
================
*/
int32_t CRTNavMeshObj::IsInside(const SNavVec3* pLocalPos) {
	SNavVec3 probe = *pLocalPos;
	int32_t nResult = 0;
	for (size_t i = 0; i < m_vecCells.size(); ++i) {
		if (m_vecCells[i].IsInside(&probe)) {
			nResult = 1;
		}
	}
	return nResult;
}

/*
================
CRTNavMeshObj::TriggerEventZone
[RECONSTRUCTED - 0x009B4CD0]
Mode bit 0 fires when the previous index is lower than the cell index (entering), bit 1 when it is higher (leaving).
================
*/
int32_t CRTNavMeshObj::TriggerEventZone(int32_t nCellIndex, int32_t nPrevIndex, uint8_t byMode, uint8_t byEvent) {
	if (byEvent >= m_vecEventZones.size()) {
		return 0; // 0x009B4CEE: vector subscript out of range
	}
	SNavEventZone* pZone = m_vecEventZones[byEvent];
	if ((byMode & 1) != 0 && nPrevIndex < nCellIndex) {
		return g_pRegionManagerBody->TriggerEventZone(pZone, 1);
	}
	if ((byMode & 2) != 0 && nPrevIndex > nCellIndex) {
		return g_pRegionManagerBody->TriggerEventZone(pZone, 0);
	}
	return 0;
}

/*
================
CRTNavMeshObj::CrossGrid
[RECONSTRUCTED - 0x009B4D30]
Searches the grid tiles covered by the segment for the global edge crossed nearest to the source.
Blocked edge (flag 0x01) while entering (nFlag 1): the source is pushed off the edge and the current cell kept.
Otherwise the source moves just inside cell A of the edge, which is returned.
================
*/
CRTNavCell* CRTNavMeshObj::CrossGrid(SNavVec3* pDestLocal, CRTNavCell* pCurrentCell, SNavVec3* pSource, int32_t* pResult, int32_t nFlag) {
	int32_t nX0 = static_cast<int32_t>((static_cast<double>(pSource->x) - m_fGridOriginX) / kNavObjGridTile);
	int32_t nZ0 = static_cast<int32_t>((static_cast<double>(pSource->z) - m_fGridOriginZ) / kNavObjGridTile);
	int32_t nX1 = static_cast<int32_t>((static_cast<double>(pDestLocal->x) - m_fGridOriginX) / kNavObjGridTile);
	int32_t nZ1 = static_cast<int32_t>((static_cast<double>(pDestLocal->z) - m_fGridOriginZ) / kNavObjGridTile);

	CLAMP(nX0, 0, m_nGridWidth - 1);  // RTNavMeshObj.cpp line 0x2D0
	CLAMP(nZ0, 0, m_nGridHeight - 1); // line 0x2D1
	CLAMP(nX1, 0, m_nGridWidth - 1);  // line 0x2D3
	CLAMP(nZ1, 0, m_nGridHeight - 1); // line 0x2D4

	if (nX0 > nX1) {
		std::swap(nX0, nX1);
	}
	if (nZ0 > nZ1) {
		std::swap(nZ0, nZ1);
	}

	const SNavLine2 segment = { pSource->x, pSource->z, pDestLocal->x, pDestLocal->z };
	float fBestDist = 0.0f;
	SNavObjEdge* pBestEdge = nullptr;
	SNavVec2 bestPoint = { 0.0f, 0.0f };

	for (int32_t nZ = nZ0; nZ <= nZ1; ++nZ) {
		for (int32_t nX = nX0; nX <= nX1; ++nX) {
			if (nX < 0 || nZ < 0 || nX >= m_nGridWidth || nZ >= m_nGridHeight) {
				continue;
			}
			const size_t nTile = static_cast<size_t>(m_nGridWidth * nZ + nX);
			if (nTile >= m_vecGrid.size()) {
				continue; // 0x009B50F9
			}
			const std::vector<SNavObjEdgeRef>& vecRefs = m_vecGrid[nTile];
			for (size_t i = 0; i < vecRefs.size(); ++i) {
				SNavObjEdge* pEdge = vecRefs[i].pEdge;
				if ((pEdge->flag & 0x10) != 0) {
					continue;
				}
				SNavVec2 hit;
				if (Line2_Intersect(pEdge->GetLine(), &segment, &hit) != 2) {
					continue;
				}
				const double fDX = static_cast<float>(pSource->x - hit.x);
				const double fDZ = static_cast<float>(pSource->z - hit.z);
				const float fDist = static_cast<float>(fDX * fDX + fDZ * fDZ);
				if (pBestEdge == nullptr || fBestDist > fDist) {
					fBestDist = fDist;
					pBestEdge = pEdge;
					bestPoint = hit;
				}
			}
		}
	}

	if (pBestEdge == nullptr) {
		return pCurrentCell;
	}

	CRTNavCellTri* pCellA = pBestEdge->GetCell(0);
	if ((pBestEdge->byEventZone & 0xC0) != 0) {
		const int32_t nEvent = TriggerEventZone(pCellA->m_nIndex, -1, static_cast<uint8_t>(pBestEdge->byEventZone >> 6),
			static_cast<uint8_t>(pBestEdge->byEventZone & 0x3F));
		if ((nEvent & 0x10000001) != 0) {
			*pResult |= nEvent;
			return nullptr;
		}
	}

	const uint8_t byEdgeFlag = pBestEdge->flag;
	if ((byEdgeFlag & 0x80) != 0) {
		*pResult |= 0x80;
		if ((m_dwStructOption & 8) != 0) {
			*pResult |= 0x100;
		}
		SNavVec3& gate = g_pRegionManagerBody->m_vSiegeGatePoint;
		gate.x = bestPoint.x;
		gate.z = bestPoint.z;
		pCellA->ApplyHeight(&gate);
	}
	if ((byEdgeFlag & 0x20) != 0) {
		*pResult |= 8;
	}

	if ((byEdgeFlag & 1) != 0 && nFlag == 1) {
		PushOffEdge(pBestEdge, bestPoint.x, bestPoint.z, &pSource->x, &pSource->z);
		*pResult |= 1;
		return pCurrentCell;
	}

	if ((pCellA->m_byEventZone & 0xC0) != 0) {
		const int32_t nEvent = TriggerEventZone(pCellA->m_nIndex, -1, static_cast<uint8_t>(pCellA->m_byEventZone >> 6),
			static_cast<uint8_t>(pCellA->m_byEventZone & 0x3F));
		if ((nEvent & 0x10000001) != 0) {
			*pResult |= nEvent;
			return nullptr;
		}
	}

	*pResult |= 4;
	pCellA->NudgeToCenter(&bestPoint);
	pSource->x = bestPoint.x;
	pSource->z = bestPoint.z;
	return pCellA;
}

/*
================
CRTNavMeshObj::WalkCells
[RECONSTRUCTED - 0x009B5380]
Follows the segment from pStart to pEnd across triangles. pEnd receives the stopping point when the walk is blocked
or leaves the mesh. Returns the triangle reached, or 0 when the segment left the object.
================
*/
CRTNavCellTri* CRTNavMeshObj::WalkCells(CRTNavCellTri* pCell, SNavVec2* pStart, SNavVec2* pEnd, int32_t* pResult, int32_t nFlag,
	SNavMoveContext* pContext, SNavObjEdge* pPrevEdge) {
	const SNavLine2 segment = { pStart->x, pStart->z, pEnd->x, pEnd->z };
	SNavVec2 cross = { 0.0f, 0.0f };
	SNavObjEdge* pEdge = nullptr;

	for (int32_t i = 0; i < 3; ++i) {
		SNavObjEdge* pCandidate = pCell->GetEdge(i);
		if (pCandidate == pPrevEdge || pCandidate == nullptr) {
			continue;
		}
		if (Line2_Intersect(pCandidate->GetLine(), &segment, &cross) == 2) {
			pEdge = pCandidate;
			break;
		}
	}
	if (pEdge == nullptr) {
		return pCell;
	}

	pContext->pLastObjEdge = pEdge;
	if (pContext->nStepsLeft == 0) {
		*pResult |= 0x20;
		return pCell;
	}
	--pContext->nStepsLeft;

	CRTNavCellTri* pNext = pEdge->GetOtherCell(pCell);

	if ((pEdge->byEventZone & 0xC0) != 0) {
		const int32_t nNextIndex = (pNext != nullptr) ? pNext->m_nIndex : -1;
		const int32_t nEvent = TriggerEventZone(nNextIndex, pCell->m_nIndex, static_cast<uint8_t>(pEdge->byEventZone >> 6),
			static_cast<uint8_t>(pEdge->byEventZone & 0x3F));
		if ((nEvent & 0x10000001) != 0) {
			pCell->NudgeToCenter(&cross);
			*pEnd = cross;
			*pResult |= nEvent;
			return pCell;
		}
	}

	if ((pCell->m_byEventZone & 0xC0) != 0) {
		if (pNext == nullptr || (pNext->m_byEventZone & 0xC0) == 0 || ((pNext->m_byEventZone ^ pCell->m_byEventZone) & 0x3F) != 0) {
			const int32_t nEvent = TriggerEventZone(-1, pCell->m_nIndex, static_cast<uint8_t>(pCell->m_byEventZone >> 6),
				static_cast<uint8_t>(pCell->m_byEventZone & 0x3F));
			if ((nEvent & 0x10000001) != 0) {
				*pResult |= nEvent;
				return pCell;
			}
		}
	}

	const uint8_t byEdgeFlag = pEdge->flag;
	if ((byEdgeFlag & 0x80) != 0) {
		*pResult |= 0x80;
		if ((m_dwStructOption & 8) != 0) {
			*pResult |= 0x100;
		}
		SNavVec3& gate = g_pRegionManagerBody->m_vSiegeGatePoint;
		gate.x = cross.x;
		gate.z = cross.z;
		pCell->ApplyHeight(&gate);
	}
	if ((byEdgeFlag & 0x20) != 0) {
		*pResult |= 8;
	}

	if ((byEdgeFlag & 4) == 0) {
		// Outline or link edge: the segment leaves this object.
		if ((byEdgeFlag & 8) != 0) {
			*pResult |= 2;
			pContext->dwLinkEdgeIndex = FindGlobalEdgeIndex(pEdge);
		}
		if (nFlag != 0 && (byEdgeFlag & 0x12) != 0) {
			*pResult |= 1;
			pCell->NudgeToCenter(&cross);
			*pEnd = cross;
			return pCell;
		}
		*pResult |= 0x10;
		PushOffEdge(pEdge, cross.x, cross.z, &pEnd->x, &pEnd->z);
		return nullptr;
	}

	// Internal edge.
	if (pNext == nullptr) {
		// The native code dereferences the neighbour of an internal edge without a check (0x009B56F5);
		// a missing neighbour is treated as a blocked edge.
		*pResult |= 1;
		pCell->NudgeToCenter(&cross);
		*pEnd = cross;
		return pCell;
	}

	if ((pNext->m_byEventZone & 0xC0) != 0) {
		if ((pCell->m_byEventZone & 0xC0) == 0 || ((pNext->m_byEventZone ^ pCell->m_byEventZone) & 0x3F) != 0) {
			const int32_t nEvent = TriggerEventZone(pNext->m_nIndex, -1, static_cast<uint8_t>(pNext->m_byEventZone >> 6),
				static_cast<uint8_t>(pNext->m_byEventZone & 0x3F));
			if ((nEvent & 0x10000001) != 0) {
				pCell->NudgeToCenter(&cross);
				*pEnd = cross;
				*pResult |= nEvent;
				return pCell;
			}
		}
	}

	if ((byEdgeFlag & 2) != 0 && pEdge->GetCell(0) == pCell) {
		*pResult |= 1;
	}
	if ((*pResult & 1) == 0 && (byEdgeFlag & 1) != 0 && pEdge->GetCell(1) == pCell) {
		*pResult |= 1;
	}
	if ((*pResult & 1) != 0) {
		pCell->NudgeToCenter(&cross);
		*pEnd = cross;
		return pCell;
	}

	pNext->NudgeToCenter(&cross);
	return WalkCells(pNext, &cross, pEnd, pResult, nFlag, pContext, pEdge);
}

/*
================
CRTNavMeshObj::FindLinkEntryCell
[RECONSTRUCTED - 0x009B5800]
================
*/
CRTNavCellTri* CRTNavMeshObj::FindLinkEntryCell(uint32_t dwEdgeIndex, SNavVec3* pPos) {
	std::vector<SNavObjEdge>& vecGlobal = m_avecEdges[0];
	if (dwEdgeIndex >= vecGlobal.size()) {
		return nullptr; // 0x009B582A: vector subscript out of range
	}
	CRTNavCellTri* pCell = vecGlobal[dwEdgeIndex].GetOtherCell(nullptr);
	if (pCell == nullptr) {
		return nullptr;
	}

	const SNavLine2 segment = { pCell->m_fCenterX, pCell->m_fCenterZ, pPos->x, pPos->z };
	SNavVec2 cross = { pPos->x, pPos->z };
	for (int32_t i = 0; i < 3; ++i) {
		SNavObjEdge* pEdge = pCell->GetEdge(i);
		if (pEdge == nullptr) {
			continue;
		}
		if (Line2_Intersect(pEdge->GetLine(), &segment, &cross) == 2) {
			pCell->NudgeToCenter(&cross);
			pPos->x = cross.x;
			pPos->z = cross.z;
			break;
		}
	}
	pCell->IsInside(pPos);
	return pCell;
}

/*
================
CRTNavMeshObj::FindGlobalEdgeIndex
[RECONSTRUCTED - 0x009B31E0]
================
*/
uint16_t CRTNavMeshObj::FindGlobalEdgeIndex(const SNavObjEdge* pEdge) const {
	const std::vector<SNavObjEdge>& vecGlobal = m_avecEdges[0];
	for (size_t i = 0; i < vecGlobal.size(); ++i) {
		if (&vecGlobal[i] == pEdge) {
			return static_cast<uint16_t>(i);
		}
	}
	return 0xFFFF;
}

/*
================
CRTNavMeshObj::Move
[RECONSTRUCTED - 0x009B5A80]
Positions are object-local. A source outside this mesh looks for an entry edge through the grid; a source on one of
its triangles walks the triangles.
================
*/
int32_t CRTNavMeshObj::Move(int32_t /*nActorMode*/, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) {
	int32_t nResult = 0;
	CRTNavCell* pSourceCell = pSource->pNavCell;
	pDest->pNavCell = pSourceCell;

	if (pSourceCell->m_pMesh != this) {
		SNavVec3 point = *pSource->Pos();
		CRTNavCell* pCell = CrossGrid(pDest->Pos(), pSourceCell, &point, &nResult, nFlag);
		if ((nResult & 5) == 0) {
			pDest->pNavCell = pCell;
			return nResult;
		}

		pDest->fPosX = point.x;
		pDest->fPosZ = point.z;
		if (pCell == nullptr) {
			pDest->fPosY = point.y;
			pDest->pNavCell = pSource->pNavCell;
			return nResult;
		}
		if (pCell->m_pMesh != this) {
			pDest->pNavCell = pCell;
			return nResult;
		}
		static_cast<CRTNavCellTri*>(pCell)->ApplyHeight(pDest->Pos());
		pDest->pNavCell = pCell;
		return nResult;
	}

	CRTNavCellTri* pCell = static_cast<CRTNavCellTri*>(pSourceCell);
	SNavVec2 start = { pSource->fPosX, pSource->fPosZ };
	SNavVec2 end = { pDest->fPosX, pDest->fPosZ };

	const int32_t nClass = pCell->ClassifyPoint(&start);
	if (nClass == 0) {
		pCell->NudgeToCenter(&start);
	} else if (nClass == -1) {
		const SNavLine2 toCenter = { start.x, start.z, pCell->m_fCenterX, pCell->m_fCenterZ };
		for (int32_t i = 0; i < 3; ++i) {
			SNavObjEdge* pEdge = pCell->GetEdge(i);
			if (pEdge != nullptr && Line2_Intersect(pEdge->GetLine(), &toCenter, &start) == 2) {
				pCell->NudgeToCenter(&start);
				break;
			}
		}
	}

	CRTNavCellTri* pReached = WalkCells(pCell, &start, &end, &nResult, nFlag, pContext, nullptr);
	pDest->fPosX = end.x;
	pDest->fPosZ = end.z;
	if (pReached != nullptr) {
		pReached->ApplyHeight(pDest->Pos());
	}
	pDest->pNavCell = pReached;
	return nResult;
}

} // namespace NavMesh
