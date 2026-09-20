/**
 * ============================================================================
 * Joymax NavMesh - Navigation Cells
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavCell.cpp
 *
 * Implements:
 *   - CRTNavCellQuad::CRTNavCellQuad     @ 0x009A2650
 *   - CRTNavCellQuad::GetTileFlag        @ 0x009C1410
 *   - CRTNavCellQuad::GetNormal          @ 0x009C1430
 *   - CRTNavCellQuad::FindCrossEdge      @ 0x009C10E0
 *   - CRTNavCellQuad::Load               @ 0x009C1210
 *   - CRTNavCellQuad::NudgeToCenter      @ 0x009C1050
 *   - CRTNavCellQuad::IsSegmentInside    @ 0x009C1460 / 0x009C14B0
 *   - CRTNavCellQuad::PurgeGlobalEdges   @ 0x009C1350
 *   - CRTNavCellTri::Init                @ 0x009C01D0
 *   - CRTNavCellTri::AddEdge             @ 0x009C0300
 *   - CRTNavCellTri::GetTriData          @ 0x009C0380 (0x009BF780 / 0x009BF7D0)
 *   - CRTNavCellTri::GetHeightPlane      @ 0x009C03B0
 *   - CRTNavCellTri::GetSlopeFactor      @ 0x009C03E0
 *   - CRTNavCellTri::GetNormal           @ 0x009C04C0
 *   - CRTNavCellTri::IsInside            @ 0x009C0330 / 0x009C0620
 *   - CRTNavCellTri::ApplyHeight         @ 0x009C0350
 *   - CRTNavCellTri::NudgeToCenter       @ 0x009C0270
 *   - CRTNavCellTri::ClassifyPoint       @ 0x009B58D0
 * ============================================================================
 */

#include "RTNavCell.h"
#include "NavArchive.h"
#include "RTNavEdge.h"
#include "RTNavMeshObj.h"
#include "RTNavMeshTerrain.h"

#include <algorithm>
#include <cmath>

namespace NavMesh {

namespace {

// 0x00B45DE8: the step a point takes towards a cell centre, stored as the double 0.19999998807907104 (float 0.2f
// widened), so scaling by it in double or in float gives the same float
constexpr double kNavNudgeStep = 0.19999998807907104;

// [RECONSTRUCTED - 0x009C0270 / 0x009C1050] shared shape of both nudges
void NudgeTowards(float fCenterX, float fCenterZ, SNavVec2* pPoint) {
	SNavVec2 delta;
	delta.x = fCenterX - pPoint->x;
	delta.z = fCenterZ - pPoint->z;
	const double dx = delta.x;
	const double dz = delta.z;
	const float fLengthSq = static_cast<float>(dz * dz + dx * dx);
	const float fLength = static_cast<float>(std::sqrt(static_cast<double>(fLengthSq)));
	if (fLength > kNavNudgeStep) {
		Vec2_Normalize(&delta);
		delta.x = static_cast<float>(delta.x * kNavNudgeStep);
		delta.z = static_cast<float>(kNavNudgeStep * delta.z);
	}
	pPoint->x += delta.x;
	pPoint->z += delta.z;
}

} // namespace

// ============================================================================
// CRTNavCellQuad
// ============================================================================

CRTNavCellQuad::CRTNavCellQuad()
	: m_nEdgeCount(0)
	, m_fCenterX(0.0f)
	, m_fCenterZ(0.0f)
	, m_fMinX(0.0f)
	, m_fMinZ(0.0f)
	, m_fMaxX(0.0f)
	, m_fMaxZ(0.0f) {
}

CRTNavCellQuad::~CRTNavCellQuad() {
}

CRTNavMeshTerrain* CRTNavCellQuad::GetTerrain() const {
	return static_cast<CRTNavMeshTerrain*>(m_pMesh);
}

uint32_t CRTNavCellQuad::GetTileFlag(const SNavVec3* pPos) {
	return GetTerrain()->GetTileFlag(pPos);
}

const SNavVec3* CRTNavCellQuad::GetNormal(const tagNavPos* pPos) {
	return GetTerrain()->GetTileNormal(pPos);
}

void CRTNavCellQuad::AddEdge(uint8_t byDirection, CRTNavEdge* pEdge) {
	m_avecEdges[byDirection].push_back(pEdge); // 0x009C0BE0
	++m_nEdgeCount;
}

/*
================
CRTNavCellQuad::FindCrossEdge
[RECONSTRUCTED - 0x009C10E0]
An edge counts as crossed when the segment ends lie on different sides of it and the lines meet on the edge
(Line2_Intersect 2 or 3). An edge that is not crossed counts towards "inside" when both segment ends are within
the cell bounds.
================
*/
int32_t CRTNavCellQuad::FindCrossEdge(const SNavLine2* pSegment, CRTNavCell** ppOutCell, CRTNavEdge** ppOutEdge, SNavVec2* pOutPoint) {
	int32_t nInside = 0;
	const SNavVec2 start = { pSegment->x0, pSegment->z0 };
	const SNavVec2 end = { pSegment->x1, pSegment->z1 };

	for (int32_t nDir = 0; nDir < 4; ++nDir) {
		const std::vector<CRTNavEdge*>& vecEdges = m_avecEdges[nDir];
		for (size_t i = 0; i < vecEdges.size(); ++i) {
			CRTNavEdge* pEdge = vecEdges[i];
			const int32_t nEndSide = (pEdge->SideTest(&end, 0.0f) == 2) ? 1 : -1;
			const int32_t nStartSide = (pEdge->SideTest(&start, 0.0f) == 2) ? 1 : -1;
			if (nStartSide * nEndSide < 0) {
				const int32_t nHit = Line2_Intersect(pEdge->GetLine(), pSegment, pOutPoint);
				if (nHit == 2 || nHit == 3) {
					*ppOutCell = pEdge->GetOtherCell(this);
					*ppOutEdge = pEdge;
					return 2;
				}
			} else if (IsSegmentInside(pSegment)) {
				++nInside;
			}
		}
	}

	return (nInside == m_nEdgeCount) ? 1 : 0;
}

/*
================
CRTNavCellQuad::Load
[RECONSTRUCTED - 0x009C1210]
================
*/
bool CRTNavCellQuad::Load(CNavArchive* pArchive) {
	const CRTNavMeshTerrain* pTerrain = GetTerrain();

	m_fMinX = pArchive->ReadValue<float>();
	m_fMinZ = pArchive->ReadValue<float>();
	m_fMaxX = pArchive->ReadValue<float>();
	m_fMaxZ = pArchive->ReadValue<float>();
	m_fCenterX = (m_fMinX + m_fMaxX) * 0.5f;
	m_fCenterZ = (m_fMaxZ + m_fMinZ) * 0.5f;

	const uint8_t byCount = pArchive->ReadValue<uint8_t>();
	m_vecObjects.resize(byCount);
	for (uint8_t i = 0; i < byCount; ++i) {
		const uint16_t wObject = pArchive->ReadValue<uint16_t>();
		if (wObject >= pTerrain->m_vecObjects.size()) {
			// 0x009C1340: vector subscript out of range
			return false;
		}
		m_vecObjects[i] = pTerrain->m_vecObjects[wObject];
	}
	return !pArchive->IsFailed();
}

/*
================
CRTNavCellQuad::NudgeToCenter
[RECONSTRUCTED - 0x009C1050]
================
*/
void CRTNavCellQuad::NudgeToCenter(SNavVec2* pPoint) const {
	NudgeTowards(m_fCenterX, m_fCenterZ, pPoint);
}

/*
================
CRTNavCellQuad::IsSegmentInside
[RECONSTRUCTED - 0x009C1460 / 0x009C14B0]
[min, max) on both axes for both segment ends.
================
*/
bool CRTNavCellQuad::IsSegmentInside(const SNavLine2* pSegment) const {
	if (!(m_fMinX <= pSegment->x0) || !(pSegment->x0 < m_fMaxX) || !(m_fMinZ <= pSegment->z0) || !(pSegment->z0 < m_fMaxZ)) {
		return false;
	}
	return (m_fMinX <= pSegment->x1) && (pSegment->x1 < m_fMaxX) && (m_fMinZ <= pSegment->z1) && (pSegment->z1 < m_fMaxZ);
}

/*
================
CRTNavCellQuad::PurgeGlobalEdges
[RECONSTRUCTED - 0x009C1350]
================
*/
void CRTNavCellQuad::PurgeGlobalEdges() {
	for (int32_t nDir = 0; nDir < 4; ++nDir) {
		std::vector<CRTNavEdge*>& vecEdges = m_avecEdges[nDir];
		vecEdges.erase(std::remove_if(vecEdges.begin(), vecEdges.end(),
			[](const CRTNavEdge* pEdge) { return pEdge->IsInternal() == 0; }), vecEdges.end());
	}
}

// ============================================================================
// CRTNavCellTri
// ============================================================================

CRTNavCellTri::CRTNavCellTri()
	: m_apVertex{ nullptr, nullptr, nullptr }
	, m_fCenterX(0.0f)
	, m_fCenterZ(0.0f)
	, m_wFlag(0)
	, m_nEdgeCount(0)
	, m_apEdge{ nullptr, nullptr, nullptr, nullptr }
	, m_TriData{}
	, m_HeightPlane{}
	, m_fSlope(0.0f)
	, m_bTriDataValid(0)
	, m_bPlaneValid(0)
	, m_bSlopeValid(0)
	, m_byEventZone(0) {
}

CRTNavMeshObj* CRTNavCellTri::GetObjMesh() const {
	return static_cast<CRTNavMeshObj*>(m_pMesh);
}

/*
================
CRTNavCellTri::Init
[RECONSTRUCTED - 0x009C01D0]
================
*/
void CRTNavCellTri::Init(CRTNavMeshObj* pMesh, int32_t nIndex, SNavObjVertex* pV0, SNavObjVertex* pV1, SNavObjVertex* pV2, uint16_t wFlag) {
	m_nIndex = nIndex;
	m_pMesh = pMesh;
	m_apVertex[2] = pV2;
	m_apVertex[0] = pV0;
	m_apVertex[1] = pV1;
	// 0x00B45AB8 is the double 3.0; the sums stay on the x87 stack until the divide
	m_fCenterX = static_cast<float>((static_cast<double>(pV0->pos.x) + pV1->pos.x + pV2->pos.x) / 3.0);
	m_wFlag = wFlag;
	m_byEventZone = 0;
	m_fCenterZ = static_cast<float>((static_cast<double>(pV0->pos.z) + pV1->pos.z + pV2->pos.z) / 3.0);
}

/*
================
CRTNavCellTri::AddEdge
[RECONSTRUCTED - 0x009C0300]
================
*/
void CRTNavCellTri::AddEdge(SNavObjEdge* pEdge) {
	if (m_nEdgeCount <= 3 && pEdge != nullptr) {
		m_apEdge[m_nEdgeCount] = pEdge;
		++m_nEdgeCount;
	}
}

/*
================
CRTNavCellTri::GetTriData
[RECONSTRUCTED - 0x009C0380]
0x009BF780 copies the vertices; 0x009BF7D0 derives normal = normalize(e2z*e1y - e1z*e2y, e1z*e2x - e2z*e1x,
e2y*e1x - e2x*e1y) with e1 = v1 - v0, e2 = v2 - v0, and the plane through v0.
================
*/
const SNavTriData* CRTNavCellTri::GetTriData() {
	if (m_bTriDataValid == 0) {
		SNavTriData& data = m_TriData;
		data.v0 = m_apVertex[0]->pos;
		data.v1 = m_apVertex[1]->pos;
		data.v2 = m_apVertex[2]->pos;

		// Edge components are stored as floats; the cross products are evaluated on the x87 stack (double)
		const double e2x = static_cast<float>(data.v2.x - data.v0.x);
		const double e2y = static_cast<float>(data.v2.y - data.v0.y);
		const double e2z = static_cast<float>(data.v2.z - data.v0.z);
		const double e1x = static_cast<float>(data.v1.x - data.v0.x);
		const double e1y = static_cast<float>(data.v1.y - data.v0.y);
		const double e1z = static_cast<float>(data.v1.z - data.v0.z);

		data.normal.x = static_cast<float>(e2z * e1y - e1z * e2y);
		data.normal.y = static_cast<float>(e1z * e2x - e2z * e1x);
		data.normal.z = static_cast<float>(e2y * e1x - e2x * e1y);
		Vec3_Normalize(&data.normal);

		data.plane.a = data.normal.x;
		data.plane.b = data.normal.y;
		data.plane.c = data.normal.z;
		data.plane.d = static_cast<float>(-(static_cast<double>(data.normal.x) * data.v0.x +
			static_cast<double>(data.v0.y) * data.normal.y + static_cast<double>(data.v0.z) * data.normal.z));
		m_bTriDataValid = 1;
	}
	return &m_TriData;
}

/*
================
CRTNavCellTri::GetHeightPlane
[RECONSTRUCTED - 0x009C03B0]
0x00537E10 with p0 = v0, p1 = v2, p2 = v1.
================
*/
const SNavPlane* CRTNavCellTri::GetHeightPlane() {
	if (m_bPlaneValid == 0) {
		Plane_FromPoints(&m_apVertex[0]->pos, &m_apVertex[2]->pos, &m_apVertex[1]->pos, &m_HeightPlane);
		m_bPlaneValid = 1;
	}
	return &m_HeightPlane;
}

/*
================
CRTNavCellTri::GetSlopeFactor
[RECONSTRUCTED - 0x009C03E0]
================
*/
float CRTNavCellTri::GetSlopeFactor() {
	if (m_bSlopeValid == 0) {
		const SNavPlane* pPlane = GetHeightPlane();
		const float fDot = static_cast<float>(static_cast<double>(pPlane->a) * 0.0 + pPlane->b + 0.0 * pPlane->c);
		const float fAngle = static_cast<float>(std::acos(static_cast<double>(fDot))); // 0x009FBE30, stored at 0x009C0433
		float fSlope = static_cast<float>(1.0 - fAngle / 1.57);                         // 0x00B45E20 (double)
		if (0.0f > fSlope) {
			fSlope = 0.0f;
		} else if (1.0f <= fSlope) {
			fSlope = 1.0f;
		}
		m_bSlopeValid = 1;
		m_fSlope = fSlope;
	}
	return m_fSlope;
}

/*
================
CRTNavCellTri::GetNormal
[RECONSTRUCTED - 0x009C04C0]
The result lives in a function static (0x00D6CB38), like the native code.
[PARTIAL] Dungeon positions use the dungeon block matrix (+0x68); dungeon meshes are not ported, so a dungeon
position never resolves to a triangle and the branch returns the untransformed normal.
================
*/
const SNavVec3* CRTNavCellTri::GetNormal(const tagNavPos* pPos) {
	static SNavVec3 s_vNormal = {};
	const SNavTriData* pData = GetTriData();
	const double nx = pData->normal.x;
	const double ny = pData->normal.y;
	const double nz = pData->normal.z;

	if ((pPos->wRegionID & 0x8000) == 0) {
		const float* m = pPos->pNavMeshInst->matLocalToWorld.m;
		s_vNormal.x = static_cast<float>(m[4] * ny + m[0] * nx + m[8] * nz);
		s_vNormal.y = static_cast<float>(m[5] * ny + m[1] * nx + m[9] * nz);
		s_vNormal.z = static_cast<float>(m[6] * ny + m[2] * nx + m[10] * nz);
	} else {
		s_vNormal = pData->normal;
	}
	return &s_vNormal;
}

/*
================
CRTNavCellTri::GetOwnerKey
[RECONSTRUCTED - 0x009C05F0]
================
*/
intptr_t CRTNavCellTri::GetOwnerKey(const tagNavPos* pPos) {
	return reinterpret_cast<intptr_t>(pPos->pNavCell);
}

/*
================
CRTNavCellTri::GetDungeonBlock
[PARTIAL - 0x009C0600]
Native reads the dungeon instance block (+0xEC) and resolves it with 0x0099A520; dungeons are not ported.
================
*/
intptr_t CRTNavCellTri::GetDungeonBlock(const tagNavPos* /*pPos*/) {
	return -1;
}

/*
================
CRTNavCellTri::IsInside
[RECONSTRUCTED - 0x009C0330 -> 0x009C0620]
The height comes from the triangle plane (0 when the plane is vertical); a point is inside when it is on the inner
side of all three edges (dot >= 0 with c = (e.z*n.y - e.y*n.z, e.x*n.z - e.z*n.x, e.y*n.x - e.x*n.y)).
================
*/
bool CRTNavCellTri::IsInside(SNavVec3* pPos) {
	const SNavTriData* pData = GetTriData();
	const SNavPlane& plane = pData->plane;
	if (plane.b != 0.0f) {
		pPos->y = static_cast<float>(-((static_cast<double>(plane.c) * pPos->z + static_cast<double>(plane.a) * pPos->x + plane.d) / plane.b));
	} else {
		pPos->y = 0.0f;
	}

	// Stores at 0x009C0651.. (edge), 0x009C0681.. (c), 0x009C06B1.. (w) and 0x009C06E5 (dot); products and sums
	// between them are x87 (double)
	const double nx = pData->normal.x;
	const double ny = pData->normal.y;
	const double nz = pData->normal.z;
	const SNavVec3* apFrom[3] = { &pData->v0, &pData->v1, &pData->v2 };
	const SNavVec3* apTo[3] = { &pData->v1, &pData->v2, &pData->v0 };
	for (int32_t i = 0; i < 3; ++i) {
		const float ex = apTo[i]->x - apFrom[i]->x;
		const float ey = apTo[i]->y - apFrom[i]->y;
		const float ez = apTo[i]->z - apFrom[i]->z;
		const float cx = static_cast<float>(ez * ny - ey * nz);
		const float cy = static_cast<float>(ex * nz - ez * nx);
		const float cz = static_cast<float>(ey * nx - ex * ny);
		const float wx = pPos->x - apFrom[i]->x;
		const float wy = pPos->y - apFrom[i]->y;
		const float wz = pPos->z - apFrom[i]->z;
		const float fDot = static_cast<float>(static_cast<double>(wy) * cy + static_cast<double>(wx) * cx + static_cast<double>(wz) * cz);
		if (fDot < 0.0f) {
			return false;
		}
	}
	return true;
}

/*
================
CRTNavCellTri::ApplyHeight
[RECONSTRUCTED - 0x009C0350]
================
*/
void CRTNavCellTri::ApplyHeight(SNavVec3* pPos) {
	const SNavPlane* pPlane = GetHeightPlane();
	if (pPlane->b != 0.0f) {
		pPos->y = static_cast<float>(-((static_cast<double>(pPlane->c) * pPos->z + static_cast<double>(pPlane->a) * pPos->x + pPlane->d) / pPlane->b));
	} else {
		pPos->y = 0.0f;
	}
}

/*
================
CRTNavCellTri::NudgeToCenter
[RECONSTRUCTED - 0x009C0270]
================
*/
void CRTNavCellTri::NudgeToCenter(SNavVec2* pPoint) const {
	NudgeTowards(m_fCenterX, m_fCenterZ, pPoint);
}

/*
================
CRTNavCellTri::ClassifyPoint
[RECONSTRUCTED - 0x009B58D0]
Each edge e = to - from gives c = (e.z, -e.x); dot = w.z*c.z + w.x*c.x with w = point - from.
================
*/
int32_t CRTNavCellTri::ClassifyPoint(const SNavVec2* pPoint) {
	const SNavTriData* pData = GetTriData();
	const SNavVec3* apFrom[3] = { &pData->v0, &pData->v1, &pData->v2 };
	const SNavVec3* apTo[3] = { &pData->v1, &pData->v2, &pData->v0 };
	float afDot[3];
	for (int32_t i = 0; i < 3; ++i) {
		const float ex = apTo[i]->x - apFrom[i]->x;
		const float ez = apTo[i]->z - apFrom[i]->z;
		const float cx = ez - 0.0f;
		const float cz = 0.0f - ex;
		const float wx = pPoint->x - apFrom[i]->x;
		const float wz = pPoint->z - apFrom[i]->z;
		afDot[i] = static_cast<float>(static_cast<double>(wz) * cz + static_cast<double>(wx) * cx);
		if (afDot[i] < 0.0f) {
			return -1;
		}
	}
	if (afDot[0] == 0.0f || afDot[1] == 0.0f || afDot[2] == 0.0f) {
		return 0;
	}
	return 1;
}

} // namespace NavMesh
