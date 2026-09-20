/**
 * ============================================================================
 * Joymax NavMesh - Terrain Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshTerrain.cpp
 *
 * Implements:
 *   - CRTNavMeshTerrain::CRTNavMeshTerrain    @ 0x0099EE40
 *   - CRTNavMeshTerrain::~CRTNavMeshTerrain   @ 0x0099F080
 *   - CRTNavMeshTerrain::Clear                @ 0x0099F130
 *   - CRTNavMeshTerrain::LinkEdges            @ 0x0099F280
 *   - CRTNavMeshTerrain::PurgeGlobalEdges     @ 0x0099F320
 *   - CRTNavMeshTerrain::Load                 @ 0x0099F370
 *   - CRTNavMeshTerrain::GetTileFlag          @ 0x0099F820
 *   - CRTNavMeshTerrain::GetTileNormal        @ 0x0099F860
 *   - CRTNavMeshTerrain::FindObjectByKey      @ 0x0099FBB0
 *   - NavObj_FindCell                         @ 0x0099FC30
 *   - NavObj_IsInside                         @ 0x0099FCF0
 *   - CRTNavMeshTerrain::FindNavCell          @ 0x0099FD90 (vftable[3])
 *   - CRTNavMeshTerrain::GetCellAt            @ 0x0099FF00
 *   - NavPos_WrapOutdoor                      @ 0x0099FFB0
 *   - CRTNavMeshTerrain::MoveInObject         @ 0x009A0030
 *   - CRTNavMeshTerrain::CheckObjectsOnPath   @ 0x009A05A0
 *   - CRTNavMeshTerrain::Move                 @ 0x009A0710 (vftable[2])
 *   - CRTNavMeshTerrain::FindHeight           @ 0x009A1150 (vftable[1])
 *   - CRTNavMeshTerrain::RegisterEventZones   @ 0x009A1BF0 (vftable[4])
 *   - CRTNavMeshTerrain::GetCell              @ 0x009A7B20
 * ============================================================================
 */

#include "RTNavMeshTerrain.h"
#include "MapLoader.h"
#include "NavArchive.h"
#include "RTNavMeshObj.h"
#include "RegionManagerBody.h"
#include "../BSLib/BSLog.h"

#include <cmath>
#include <cstring>

namespace NavMesh {

namespace {

constexpr float kNavRegionSize = 1920.0f;      // 0x00B45AD0
constexpr float kNavTileSize = 20.0f;          // 0x00B45AC8
constexpr float kNavRegionEdgeMax = 1919.99f;  // 0x00B462F0 (float) / 0x00B462E8 (double)
constexpr float kNavRegionEdgeMin = 0.01f;     // 0x00B45DF8

const char* const kRTNavMeshTerrainFile = "D:\\WORK2005\\Source\\JMX_Library\\NavMesh_new\\RTNavMeshTerrain.cpp";

// Water / ice surface normal returned when the position lies on the surface (0x00C678D4)
const SNavVec3 kNavSurfaceNormal = { 0.0f, 1.0f, 0.0f };

bool TileIndexInRange(int32_t nTileX, int32_t nTileZ) {
	return nTileX >= 0 && nTileZ >= 0 && nTileX < kNavTileCount && nTileZ < kNavTileCount;
}

// trunc(pos / 20) (0x009FBB40): the quotient stays on the x87 stack, so it is not rounded to float first
int32_t TileIndexOf(float fPos) {
	return static_cast<int32_t>(static_cast<double>(fPos) / kNavTileSize);
}

// (pos - tile * 20) / 20 stored as float (0x009A119C / 0x0099F972)
float TileFraction(float fPos, int32_t nTile) {
	return static_cast<float>((static_cast<double>(fPos) - static_cast<double>(nTile) * kNavTileSize) / kNavTileSize);
}

} // namespace

// ============================================================================
// Construction / loading
// ============================================================================

CRTNavMeshTerrain::CRTNavMeshTerrain()
	: m_pSkipInst(nullptr)
	, m_dwOpenCellCount(0)
	, m_pNormalCache(nullptr) {
	m_nMeshType = 1;
	std::memset(m_aTiles, 0, sizeof(m_aTiles));
	std::memset(m_afHeights, 0, sizeof(m_afHeights));
	std::memset(m_abySurfaceType, 0, sizeof(m_abySurfaceType));
	std::memset(m_afSurfaceHeight, 0, sizeof(m_afSurfaceHeight));
}

CRTNavMeshTerrain::~CRTNavMeshTerrain() {
	delete[] m_pNormalCache;
	m_pNormalCache = nullptr;
}

/*
================
CRTNavMeshTerrain::Clear
[RECONSTRUCTED - 0x0099F130]
================
*/
void CRTNavMeshTerrain::Clear(int32_t bKeepData) {
	if (bKeepData != 0) {
		if (m_pNormalCache != nullptr) {
			std::memset(m_pNormalCache, 0, kNavTileCount * kNavTileCount);
		}
		return;
	}

	m_vecGlobalEdges.clear();
	m_vecInternalEdges.clear();
	m_vecCells.clear();
	for (size_t i = 0; i < m_vecObjects.size(); ++i) {
		if (g_pMapLoader != nullptr) {
			g_pMapLoader->ReleaseInstance(m_vecObjects[i]);
		}
	}
	m_vecObjects.clear();
}

/*
================
CRTNavMeshTerrain::LinkEdges
[RECONSTRUCTED - 0x0099F280]
================
*/
void CRTNavMeshTerrain::LinkEdges(int32_t nWhich) {
	if (nWhich == 1) {
		for (size_t i = 0; i < m_vecInternalEdges.size(); ++i) {
			m_vecInternalEdges[i].Link();
		}
	} else if (nWhich == 0) {
		for (size_t i = 0; i < m_vecGlobalEdges.size(); ++i) {
			m_vecGlobalEdges[i].Link();
		}
	}
}

/*
================
CRTNavMeshTerrain::PurgeGlobalEdges
[RECONSTRUCTED - 0x0099F320]
================
*/
void CRTNavMeshTerrain::PurgeGlobalEdges() {
	for (size_t i = 0; i < m_vecCells.size(); ++i) {
		m_vecCells[i].PurgeGlobalEdges();
	}
}

/*
================
CRTNavMeshTerrain::Load
[RECONSTRUCTED - 0x0099F370]
Object list, quad cells, global edges, internal edges (linked immediately), tile map, height map, surface maps,
then each tile's texture index is replaced by the tile2d.ifo flag in bits 16-31.
================
*/
bool CRTNavMeshTerrain::Load(CNavArchive* pArchive, CMapLoader* pLoader) {
	const uint16_t wObjectCount = pArchive->ReadValue<uint16_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecObjects.assign(wObjectCount, nullptr);
	for (uint16_t i = 0; i < wObjectCount; ++i) {
		SNvmObjectEntry entry;
		pArchive->Read(&entry, sizeof(entry));
		SNavMeshInst* pInst = pLoader->CreateInstance(&entry);
		if (pInst == nullptr) {
			// 0x0099F3E3: the native asserts "pNMI" and dereferences the null instance
			BSLib::AssertReport(0x69, kRTNavMeshTerrainFile, "pNMI");
			return false;
		}
		pInst->pTerrain = this;
		m_vecObjects[i] = pInst;

		if (pInst->pResource->GetMeshType() == 3) {
			// [PARTIAL] 0x0099F43D: 0x00997FE0 attaches the dungeon entrance; dungeons are not ported
		} else {
			pInst->pResource->m_pParentTerrain = this;
		}

		const uint16_t wLinkCount = pArchive->ReadValue<uint16_t>();
		if (wLinkCount == 0) {
			pInst->vecLinks.clear();
		} else {
			pInst->vecLinks.resize(wLinkCount);
			pArchive->Read(pInst->vecLinks.data(), static_cast<size_t>(wLinkCount) * sizeof(SNvmObjectLink));
		}
	}

	const uint32_t dwCellCount = pArchive->ReadValue<uint32_t>();
	m_dwOpenCellCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecCells.resize(dwCellCount);
	for (uint32_t i = 0; i < dwCellCount; ++i) {
		CRTNavCellQuad& cell = m_vecCells[i];
		cell.m_nEdgeCount = 0;
		cell.m_fCenterX = 0.0f;
		cell.m_fCenterZ = 0.0f;
		cell.m_pMesh = this;
		cell.m_nIndex = static_cast<int32_t>(i);
		if (!cell.Load(pArchive)) {
			return false;
		}
	}

	const uint32_t dwGlobalEdgeCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecGlobalEdges.resize(dwGlobalEdgeCount);
	for (uint32_t i = 0; i < dwGlobalEdgeCount; ++i) {
		m_vecGlobalEdges[i].Load(pArchive);
		m_vecGlobalEdges[i].m_pMesh = this;
	}

	const uint32_t dwInternalEdgeCount = pArchive->ReadValue<uint32_t>();
	if (pArchive->IsFailed()) {
		return false;
	}
	m_vecInternalEdges.resize(dwInternalEdgeCount);
	for (uint32_t i = 0; i < dwInternalEdgeCount; ++i) {
		m_vecInternalEdges[i].Load(pArchive);
		m_vecInternalEdges[i].m_pMesh = this;
	}

	for (uint32_t i = 0; i < dwInternalEdgeCount; ++i) {
		const NavEdge& data = m_vecInternalEdges[i].m_sData;
		if (data.assocCell[0] >= dwCellCount || ((data.flag & 3) == 0 && data.assocCell[1] >= dwCellCount) ||
			data.assocDir[0] > 3 || ((data.flag & 3) == 0 && data.assocDir[1] > 3)) {
			return false; // 0x009C0A73 / 0x009C0AD8: vector subscript out of range
		}
	}
	LinkEdges(1);

	pArchive->Read(m_aTiles, sizeof(m_aTiles));
	pArchive->Read(m_afHeights, sizeof(m_afHeights));
	pArchive->Read(m_abySurfaceType, sizeof(m_abySurfaceType));
	pArchive->Read(m_afSurfaceHeight, sizeof(m_afSurfaceHeight));
	if (pArchive->IsFailed()) {
		return false;
	}

	const std::vector<STile2DInfo>& vecTile2D = pLoader->GetTile2DInfo();
	for (int32_t nTile = 0; nTile < kNavTileCount * kNavTileCount; ++nTile) {
		const uint16_t wTexture = static_cast<uint16_t>(m_aTiles[nTile].dwFlag >> 16);
		m_aTiles[nTile].dwFlag &= 0x0000FFFF;
		if (wTexture >= vecTile2D.size()) {
			return false; // 0x0099F810: vector subscript out of range
		}
		m_aTiles[nTile].dwFlag |= vecTile2D[wTexture].dwFlag << 16;
	}
	return true;
}

// ============================================================================
// Queries
// ============================================================================

/*
================
CRTNavMeshTerrain::GetCell
[RECONSTRUCTED - 0x009A7B20]
================
*/
CRTNavCellQuad* CRTNavMeshTerrain::GetCell(uint32_t dwIndex) {
	if (dwIndex >= m_vecCells.size()) {
		return nullptr; // 0x009A7B44: vector subscript out of range
	}
	return &m_vecCells[dwIndex];
}

/*
================
CRTNavMeshTerrain::GetTileFlag
[RECONSTRUCTED - 0x0099F820]
The native indexes with trunc(pos / -20) and subtracts; that equals trunc(pos / 20) added. Positions outside the
region read outside the tile array natively; the port reports them blocked.
================
*/
uint32_t CRTNavMeshTerrain::GetTileFlag(const SNavVec3* pPos) const {
	const int32_t nTileZ = TileIndexOf(pPos->z);
	const int32_t nTileX = TileIndexOf(pPos->x);
	if (!TileIndexInRange(nTileX, nTileZ)) {
		return 1;
	}
	return m_aTiles[nTileZ * kNavTileCount + nTileX].dwFlag;
}

/*
================
CRTNavMeshTerrain::GetTileNormal
[RECONSTRUCTED - 0x0099F860]
The tile is split along the diagonal from (x0, z0) to (x1, z1); fz >= fx picks the (h00, h01, h11) triangle.
Both normals are built on first use: (-20 (h11 - h01), 400, 20 (h00 - h01)) and (20 (h00 - h10), 400, -20 (h11 - h10)).
A position standing on a water / ice surface returns the up vector.
================
*/
const SNavVec3* CRTNavMeshTerrain::GetTileNormal(const tagNavPos* pPos) {
	if (m_pNormalCache == nullptr) {
		m_pNormalCache = new uint8_t[0x38400];
		std::memset(m_pNormalCache, 0, kNavTileCount * kNavTileCount);
	}

	const int32_t nTileX = TileIndexOf(pPos->fPosX);
	const int32_t nTileZ = TileIndexOf(pPos->fPosZ);
	if (!TileIndexInRange(nTileX, nTileZ)) {
		return &kNavSurfaceNormal;
	}

	const int32_t nSurface = (nTileZ / 16) * kNavSurfaceCount + (nTileX / 16);
	if ((m_abySurfaceType[nSurface] & 2) != 0 && pPos->fPosY == m_afSurfaceHeight[nSurface]) {
		return &kNavSurfaceNormal;
	}

	const float fX = TileFraction(pPos->fPosX, nTileX);
	const float fZ = TileFraction(pPos->fPosZ, nTileZ);
	const int32_t nTile = nTileZ * kNavTileCount + nTileX;
	SNavVec3* pNormals = reinterpret_cast<SNavVec3*>(m_pNormalCache + kNavTileCount * kNavTileCount + nTile * 2 * sizeof(SNavVec3));

	if (m_pNormalCache[nTile] == 0) {
		m_pNormalCache[nTile] = 1;
		const int32_t nHeight = nTileZ * kNavHeightCount + nTileX;
		const float h00 = m_afHeights[nHeight];
		const float h01 = m_afHeights[nHeight + kNavHeightCount];
		const float h11 = m_afHeights[nHeight + kNavHeightCount + 1];
		const float h10 = m_afHeights[nHeight + 1];

		// v1 = (20, h11 - h01, 0), v2 = (0, h00 - h01, -20), normal = v1 x v2
		const float v1y = h11 - h01;
		const float v2y = h00 - h01;
		pNormals[0].x = -20.0f * v1y - v2y * 0.0f;
		pNormals[0].y = 0.0f * 0.0f - 20.0f * -20.0f;
		pNormals[0].z = 20.0f * v2y - 0.0f * v1y;
		Vec3_Normalize(&pNormals[0]);

		// w1 = (-20, h00 - h10, 0), w2 = (0, h11 - h10, 20), normal = (w2.z w1.y - w1.z w2.y, ..)
		const float w1y = h00 - h10;
		const float w2y = h11 - h10;
		pNormals[1].x = 20.0f * w1y - 0.0f * w2y;
		pNormals[1].y = 0.0f - 20.0f * -20.0f;
		pNormals[1].z = -20.0f * w2y - 0.0f * w1y;
		Vec3_Normalize(&pNormals[1]);
	}

	return (fZ >= fX) ? &pNormals[0] : &pNormals[1];
}

/*
================
CRTNavMeshTerrain::FindObjectByKey
[RECONSTRUCTED - 0x0099FBB0]
================
*/
SNavMeshInst* CRTNavMeshTerrain::FindObjectByKey(uint32_t dwKey) const {
	for (size_t i = 0; i < m_vecObjects.size(); ++i) {
		if (m_vecObjects[i]->GetKey() == dwKey) {
			return m_vecObjects[i];
		}
	}
	return nullptr;
}

/*
================
NavObj_FindCell
[RECONSTRUCTED - 0x0099FC30]
================
*/
CRTNavCellTri* NavObj_FindCell(SNavMeshInst* pInst, SNavVec3* pPos) {
	SNavVec3 local;
	Vec3_TransformCoord(&local, pPos, &pInst->matWorldToLocal);
	if (pInst->pResource->GetMeshType() == 3) {
		return nullptr;
	}
	CRTNavCellTri* pCell = pInst->GetObjResource()->FindCell(&local);
	if (pCell == nullptr) {
		return nullptr;
	}
	SNavVec3 world;
	Vec3_TransformCoord(&world, &local, &pInst->matLocalToWorld);
	pPos->y = world.y;
	return pCell;
}

/*
================
NavObj_IsInside
[RECONSTRUCTED - 0x0099FCF0]
================
*/
int32_t NavObj_IsInside(SNavMeshInst* pInst, const SNavVec3* pPos) {
	SNavVec3 local;
	Vec3_TransformCoord(&local, pPos, &pInst->matWorldToLocal);
	if (pInst->pResource->GetMeshType() == 3) {
		return -1;
	}
	return (pInst->GetObjResource()->IsInside(&local) == 1) ? 1 : 0;
}

/*
================
CRTNavMeshTerrain::FindNavCell
[RECONSTRUCTED - 0x0099FD90]
Starts from the quad cell (with the terrain height), then prefers an object triangle whose height is closer to the
requested one. A terrain result on a blocked tile has no cell.
================
*/
void CRTNavMeshTerrain::FindNavCell(tagNavPos* pPos) {
	g_pRegionManagerBody->m_pLastTerrain = this;

	const float fRequestedY = pPos->fPosY;
	SNavVec3 probe = *pPos->Pos();
	CRTNavCellQuad* pCell = GetCellAt(&probe);
	pPos->pNavCell = pCell;
	pPos->pNavMeshInst = nullptr;
	if (pCell == nullptr) {
		return;
	}

	float fBestDelta = probe.y - fRequestedY;
	for (size_t i = 0; i < pCell->m_vecObjects.size(); ++i) {
		SNavMeshInst* pInst = pCell->m_vecObjects[i];
		CRTNavCellTri* pTri = NavObj_FindCell(pInst, pPos->Pos());
		if (pTri == nullptr) {
			continue;
		}
		const float fDelta = pPos->fPosY - fRequestedY;
		if (std::fabs(fDelta) < std::fabs(fBestDelta)) {
			fBestDelta = fDelta;
			pPos->pNavCell = pTri;
			pPos->pNavMeshInst = pInst;
		}
	}

	const bool bOnTerrain = (pPos->pNavMeshInst == nullptr);
	if (bOnTerrain) {
		*pPos->Pos() = probe;
	}
	pPos->fPosY = fBestDelta + fRequestedY;
	if (bOnTerrain && (GetTileFlag(pPos->Pos()) & 1) != 0) {
		pPos->pNavCell = nullptr;
	}
}

/*
================
CRTNavMeshTerrain::GetCellAt
[RECONSTRUCTED - 0x0099FF00]
================
*/
CRTNavCellQuad* CRTNavMeshTerrain::GetCellAt(SNavVec3* pPos) {
	if (!(0.0f <= pPos->x) || !(pPos->x < kNavRegionSize) || !(0.0f <= pPos->z) || !(pPos->z < kNavRegionSize)) {
		return nullptr;
	}

	const int32_t nTileZ = TileIndexOf(pPos->z);
	const int32_t nTileX = TileIndexOf(pPos->x);
	const uint16_t wCell = static_cast<uint16_t>(m_aTiles[nTileZ * kNavTileCount + nTileX].dwCellIndex);
	if (FindHeight(pPos) == 0) {
		BSLib::AssertReport(0x181, kRTNavMeshTerrainFile, "if(FindHeight(vPos) == FALSE)");
	}
	return GetCell(wCell);
}

/*
================
NavPos_WrapOutdoor
[RECONSTRUCTED - 0x0099FFB0]
Checks x < 0, x >= 1920, then z < 0 (returning at once), then z >= 1920.
================
*/
int32_t NavPos_WrapOutdoor(tagNavPos* pPos) {
	int32_t nChanged = 0;
	uint8_t* pbyRegion = reinterpret_cast<uint8_t*>(&pPos->wRegionID);

	if (!(0.0f <= pPos->fPosX)) {
		pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] - 1);
		pPos->fPosX = pPos->fPosX + kNavRegionSize;
		nChanged = 1;
	} else if (!(kNavRegionSize > pPos->fPosX)) {
		pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] + 1);
		pPos->fPosX = pPos->fPosX - kNavRegionSize;
		nChanged = 1;
	}

	if (!(0.0f <= pPos->fPosZ)) {
		pPos->fPosZ = kNavRegionSize + pPos->fPosZ;
		pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] - 1);
		return 1;
	}
	if (!(pPos->fPosZ < kNavRegionSize)) {
		pPos->fPosZ = pPos->fPosZ - kNavRegionSize;
		pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] + 1);
		return 1;
	}
	return nChanged;
}

/*
================
CRTNavMeshTerrain::FindHeight
[RECONSTRUCTED - 0x009A1150]
Tiles alternate their diagonal by (x + z) parity. A water / ice block (surface type bit 1) raises the height to its
surface. Tiles outside 0..95 are read out of range natively; the port leaves the height unchanged for them.
================
*/
int32_t CRTNavMeshTerrain::FindHeight(SNavVec3* pPos) {
	const int32_t nTileX = TileIndexOf(pPos->x);
	const int32_t nTileZ = TileIndexOf(pPos->z);
	if (!TileIndexInRange(nTileX, nTileZ)) {
		return 1;
	}

	// The fractions are stored as floats; each interpolation below runs on the x87 stack (double) up to the store
	// at 0x009A129C.
	const double fX = TileFraction(pPos->x, nTileX);
	const double fZ = TileFraction(pPos->z, nTileZ);
	const int32_t nHeight = nTileZ * kNavHeightCount + nTileX;
	const double h00 = m_afHeights[nHeight];
	const double h01 = m_afHeights[nHeight + kNavHeightCount];
	const double h11 = m_afHeights[nHeight + kNavHeightCount + 1];
	const double h10 = m_afHeights[nHeight + 1];

	double fY;
	if ((nTileX % 2) == (nTileZ % 2)) {
		if (fZ >= fX) {
			fY = (h00 + fZ * (h01 - h00)) + fX * (h11 - h01);
		} else {
			fY = (h10 + fZ * (h11 - h10)) + (1.0 - fX) * (h00 - h10);
		}
	} else {
		if ((1.0 - fZ) >= fX) {
			fY = (h10 + fZ * (h01 - h00)) + (1.0 - fX) * (h00 - h10);
		} else {
			fY = ((1.0 - fX) * (h01 - h11) + h10) + fZ * (h11 - h10);
		}
	}
	pPos->y = static_cast<float>(fY);

	const int32_t nSurface = (nTileZ / 16) * kNavSurfaceCount + (nTileX / 16);
	if ((m_abySurfaceType[nSurface] & 2) != 0) {
		const float fSurface = m_afSurfaceHeight[nSurface];
		if (!(fSurface <= pPos->y)) {
			pPos->y = fSurface;
		}
	}
	return 1;
}

/*
================
CRTNavMeshTerrain::RegisterEventZones
[PARTIAL - 0x009A1BF0]
Native walks the placed objects and registers their event zone areas with the region manager (0x009A15C0 /
0x0098D2C0) for the teleport / structure callbacks. The registries behind those callbacks are not ported
(see CRegionManagerBody::TriggerEventZone), so nothing is registered.
================
*/
int32_t CRTNavMeshTerrain::RegisterEventZones() {
	return 1;
}

// ============================================================================
// Movement
// ============================================================================

/*
================
CRTNavMeshTerrain::MoveInObject
[RECONSTRUCTED - 0x009A0030]
Runs the object's Move in its local space. bProbe keeps positions unchanged when the object reports nothing.
Leaving through a link edge (result 2) continues on the linked object; wrapping into a neighbour region matches
the same object there by region / uid; landing on terrain resolves the quad cell again.
[PARTIAL] Dungeon resources (type 3) are not ported and fail the move.
================
*/
int32_t CRTNavMeshTerrain::MoveInObject(tagNavPos* pSource, SNavMeshInst* pInst, int32_t nActorMode, int32_t bProbe, int32_t nFlag,
	tagNavPos* pDest, SNavMoveContext* pContext) {
	const SNavVec3 sourceWorld = *pSource->Pos();
	const SNavVec3 destWorld = *pDest->Pos();
	Vec3_TransformCoord(pSource->Pos(), &sourceWorld, &pInst->matWorldToLocal);
	Vec3_TransformCoord(pDest->Pos(), &destWorld, &pInst->matWorldToLocal);
	g_pRegionManagerBody->m_pLastInst = pInst;

	if (pInst->pResource->GetMeshType() == 3) {
		// [PARTIAL] 0x009A0197: dungeon block move through pInst->pDungeonLink (+0xF0)
		*pDest->Pos() = destWorld;
		pDest->pNavMeshInst = nullptr;
		return 0x10000000;
	}

	const int32_t nResult = pInst->pResource->Move(nActorMode, nFlag, pSource, pDest, pContext);
	if ((nResult & 0x80) != 0) {
		SNavVec3& gate = g_pRegionManagerBody->m_vSiegeGatePoint;
		const SNavVec3 localGate = gate;
		Vec3_TransformCoord(&gate, &localGate, &pInst->matLocalToWorld);
	}

	if (nResult == 0 && bProbe != 0) {
		*pSource->Pos() = sourceWorld;
		*pDest->Pos() = destWorld;
		return 0;
	}

	if ((nResult & 4) != 0) {
		pDest->pNavMeshInst = pInst;
	}
	{
		const SNavVec3 localDest = *pDest->Pos();
		Vec3_TransformCoord(pDest->Pos(), &localDest, &pInst->matLocalToWorld);
	}

	if ((nResult & 2) != 0) {
		const int32_t nEdge = static_cast<int32_t>(pContext->dwLinkEdgeIndex);
		if (nEdge < 0 || nEdge >= 0xFFFF) {
			return 0x10000000;
		}
		const SNvmObjectLink* pLink = nullptr;
		for (size_t i = 0; i < pInst->vecLinks.size(); ++i) {
			if (pInst->vecLinks[i].wEdge == static_cast<uint32_t>(nEdge)) {
				pLink = &pInst->vecLinks[i];
				break;
			}
		}
		if (pLink == nullptr || pLink->wLinkObject == 0xFFFF) {
			return 0x10000000;
		}
		if (pLink->wLinkObject >= m_vecObjects.size()) {
			return 0x10000000; // 0x009A03BE: vector subscript out of range
		}
		SNavMeshInst* pOther = m_vecObjects[pLink->wLinkObject];
		if (pOther == nullptr) {
			return 0x10000000;
		}

		const SNavVec3 worldDest = *pDest->Pos();
		Vec3_TransformCoord(pDest->Pos(), &worldDest, &pOther->matWorldToLocal);
		if (pOther->pResource->GetMeshType() == 3) {
			BSLib::AssertReport(0x1FB, kRTNavMeshTerrainFile, "NULL");
		} else {
			pDest->pNavCell = pOther->GetObjResource()->FindLinkEntryCell(pLink->wLinkEdge, pDest->Pos());
		}
		const SNavVec3 localOther = *pDest->Pos();
		Vec3_TransformCoord(pDest->Pos(), &localOther, &pOther->matLocalToWorld);
		pDest->pNavMeshInst = pOther;
	}

	CRTNavMeshTerrain* pMesh = this;
	if (NavPos_WrapOutdoor(pDest) != 0) {
		pMesh = static_cast<CRTNavMeshTerrain*>(g_pRegionManager->FindNavMesh(pDest->wRegionID));
		if (pMesh == nullptr) {
			return 0x10000000;
		}
		if (pDest->pNavCell != nullptr) {
			SNavMeshInst* pOther = pMesh->FindObjectByKey(pInst->GetKey());
			if (pOther == nullptr) {
				return 0x10000000;
			}
			const SNavVec3 before = *pDest->Pos();
			pDest->pNavCell = NavObj_FindCell(pOther, pDest->Pos());
			if (pDest->pNavCell == nullptr) {
				return (NavObj_IsInside(pOther, pDest->Pos()) != 0) ? 0x10000000 : 0x20000000;
			}
			SNavVec3 delta = { before.x - pDest->fPosX, before.y - pDest->fPosY, before.z - pDest->fPosZ };
			if (Vec3_LengthSq(&delta) > 1.0f) {
				return 0x10000000;
			}
			pDest->pNavMeshInst = pOther;
		}
	}

	CRTNavCell* pCell = pDest->pNavCell;
	if (pCell != nullptr && !pCell->IsTerrainCell()) {
		return nResult;
	}

	if ((pMesh->GetTileFlag(pDest->Pos()) & 1) != 0) {
		return 0x10000000;
	}
	pMesh->FindHeight(pDest->Pos());
	pDest->pNavCell = pMesh->GetCellAt(pDest->Pos());
	pDest->pNavMeshInst = nullptr;
	return nResult;
}

/*
================
CRTNavMeshTerrain::CheckObjectsOnPath
[RECONSTRUCTED - 0x009A05A0]
Probes every object of the source cell (actor mode = nFlag, flag 1, unlimited steps).
================
*/
int32_t CRTNavMeshTerrain::CheckObjectsOnPath(tagNavPos* pSource, tagNavPos* pDest, int32_t nFlag, SNavMeshInst* pSkipInst) {
	SNavMoveContext context;
	context.nStepsLeft = 0x7FFFFFFF;

	CRTNavCellQuad* pCell = static_cast<CRTNavCellQuad*>(pSource->pNavCell);
	for (size_t i = 0; i < pCell->m_vecObjects.size(); ++i) {
		SNavMeshInst* pInst = pCell->m_vecObjects[i];
		if (pInst == pSkipInst) {
			continue;
		}
		tagNavPos source = *pSource;
		tagNavPos dest = *pDest;
		if (MoveInObject(&source, pInst, nFlag, 1, 1, &dest, &context) != 0) {
			return 1;
		}
	}
	return 0;
}

/*
================
CRTNavMeshTerrain::Move
[RECONSTRUCTED - 0x009A0710]
Per cell: probe the cell's objects and keep the result nearest to the source; otherwise cross the cell edge the
segment leaves through. Blocked edges stop just inside the cell, global edges continue in the neighbour region.
================
*/
int32_t CRTNavMeshTerrain::Move(int32_t nActorMode, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) {
	g_pRegionManagerBody->m_pLastTerrain = this;
	SNavVec3* pDestPos = pDest->Pos();

	while (pSource->pNavMeshInst == nullptr) {
		const tagNavPos destSaved = *pDest;
		tagNavPos sourceSaved = *pSource;
		int32_t nBest = 0;
		float fBestDist = 0.0f;

		CRTNavCellQuad* pSourceCell = static_cast<CRTNavCellQuad*>(pSource->pNavCell);
		for (size_t i = 0; i < pSourceCell->m_vecObjects.size(); ++i) {
			SNavMeshInst* pInst = pSourceCell->m_vecObjects[i];
			if (m_pSkipInst == pInst) {
				continue;
			}
			tagNavPos source = sourceSaved;
			tagNavPos dest = destSaved;
			const int32_t nResult = MoveInObject(&source, pInst, nActorMode, 1, nFlag, &dest, pContext);
			if (nResult == 0 || nResult == 0x20000000) {
				continue;
			}
			SNavVec3 delta = { sourceSaved.fPosX - dest.fPosX, sourceSaved.fPosY - dest.fPosY, sourceSaved.fPosZ - dest.fPosZ };
			const float fDist = Vec3_LengthSq(&delta);
			if (nBest == 0 || fDist < fBestDist) {
				fBestDist = fDist;
				nBest = nResult;
				*pDest = dest;
				*pSource = source;
			}
		}

		if (nBest != 0) {
			if ((nBest & 0x10000000) != 0 || (nBest & 1) == 0 || (nFlag & 1) == 0) {
				return nBest;
			}
			if (CheckObjectsOnPath(&sourceSaved, pDest, nFlag, nullptr) != 0) {
				return 0x10000000;
			}
			*pSource = sourceSaved;
		}

		pDest->pNavMeshInst = nullptr;
		CRTNavCellQuad* pCell = static_cast<CRTNavCellQuad*>(pSource->pNavCell);
		if (pCell->m_fMinX <= pDestPos->x && pDestPos->x < pCell->m_fMaxX && pCell->m_fMinZ <= pDestPos->z && pDestPos->z < pCell->m_fMaxZ) {
			pDest->pNavCell = pCell;
			FindHeight(pDestPos);
			return nBest;
		}

		const SNavLine2 segment = { pSource->fPosX, pSource->fPosZ, pDestPos->x, pDest->fPosZ };
		CRTNavCell* pOutCell = pCell;
		CRTNavEdge* pOutEdge = nullptr;
		SNavVec2 cross = { 0.0f, 0.0f };
		const int32_t nCross = pCell->FindCrossEdge(&segment, &pOutCell, &pOutEdge, &cross);

		if (nCross == 0) {
			cross.x = pSource->fPosX;
			cross.z = pSource->fPosZ;
			static_cast<CRTNavCellQuad*>(pSource->pNavCell)->NudgeToCenter(&cross);
			pDestPos->x = cross.x;
			pDest->fPosZ = cross.z;
			FindHeight(pDestPos);
			pDest->pNavCell = pSource->pNavCell;
			return nBest;
		}
		if (nCross == 1) {
			pDest->pNavCell = pOutCell;
			return nBest;
		}
		if (nCross != 2) {
			BSLib::AssertReport(0x33D, kRTNavMeshTerrainFile, "NULL");
			continue;
		}

		if (pContext->nStepsLeft == 0) {
			return 0x20;
		}
		--pContext->nStepsLeft;

		if (pOutEdge->IsBlocked() != 0) {
			static_cast<CRTNavCellQuad*>(pSource->pNavCell)->NudgeToCenter(&cross);
			pDestPos->x = cross.x;
			pDest->fPosZ = cross.z;
			pDest->pNavCell = pSource->pNavCell;
			if (CheckObjectsOnPath(pSource, pDest, nFlag, nullptr) != 0) {
				*pDestPos = *pSource->Pos();
				return 1;
			}
			FindHeight(pDestPos);
			return 1;
		}

		if (pOutEdge->IsGlobal() != 0) {
			pSource->fPosX = cross.x;
			pSource->fPosZ = cross.z;
			if (pOutCell == nullptr) {
				return 0x10000000;
			}

			const uint16_t wRegionB = pOutCell->m_pMesh->m_wRegionID;
			const uint16_t wRegionA = pSource->pNavCell->m_pMesh->m_wRegionID;
			SNavVec3 offset;
			g_pRegionManager->GetRegionOffset(wRegionA, wRegionB, &offset);
			if (std::fabs(offset.x) > 1.0f) {
				if (!(cross.x < kNavRegionEdgeMax)) {
					cross.x = 0.0f;
				} else if (!(cross.x > kNavRegionEdgeMin)) {
					cross.x = kNavRegionEdgeMax;
				}
			}
			if (std::fabs(offset.z) > 1.0f) {
				if (!(cross.z < kNavRegionEdgeMax)) {
					cross.z = 0.0f;
				} else if (!(cross.z > kNavRegionEdgeMin)) {
					cross.z = kNavRegionEdgeMax;
				}
			}

			tagNavPos newDest = *pDest;
			static_cast<CRTNavCellQuad*>(pOutCell)->NudgeToCenter(&cross);
			pDestPos->x = cross.x;
			pDest->fPosZ = cross.z;
			pDest->pNavCell = pOutCell;
			pDest->wRegionID = wRegionB;
			if (wRegionB != newDest.wRegionID) {
				const int32_t nDX = static_cast<int32_t>(newDest.wRegionID & 0xFF) - static_cast<int32_t>(wRegionB & 0xFF);
				const int32_t nDZ = static_cast<int32_t>(newDest.wRegionID >> 8) - static_cast<int32_t>(wRegionB >> 8);
				newDest.wRegionID = wRegionB;
				newDest.fPosX = static_cast<float>(nDX) * kNavRegionSize + newDest.fPosX;
				newDest.fPosZ = kNavRegionSize * static_cast<float>(nDZ) + newDest.fPosZ;
			}

			CRTNavMesh* pNewMesh = g_pRegionManager->FindNavMesh(wRegionB);
			if (pNewMesh == nullptr) {
				return 0xFFFF;
			}

			if ((nFlag & 1) != 0) {
				tagNavPos newSource = *pSource;
				newSource.pNavCell = pOutCell;
				if (newSource.wRegionID != pDest->wRegionID) {
					const int32_t nDX = static_cast<int32_t>(newSource.wRegionID & 0xFF) - static_cast<int32_t>(pDest->wRegionID & 0xFF);
					const int32_t nDZ = static_cast<int32_t>(newSource.wRegionID >> 8) - static_cast<int32_t>(pDest->wRegionID >> 8);
					newSource.wRegionID = newDest.wRegionID;
					newSource.fPosX = static_cast<float>(nDX) * kNavRegionSize + newSource.fPosX;
					newSource.fPosZ = kNavRegionSize * static_cast<float>(nDZ) + newSource.fPosZ;
				}
				if (static_cast<CRTNavMeshTerrain*>(pNewMesh)->CheckObjectsOnPath(&newSource, pDest, nFlag, nullptr) != 0) {
					return 0x10000000;
				}
			}

			const int32_t nResult = pNewMesh->Move(nActorMode, nFlag, pDest, &newDest, pContext);
			*pDest = newDest;
			return nResult;
		}

		static_cast<CRTNavCellQuad*>(pOutCell)->NudgeToCenter(&cross);
		pSource->fPosX = cross.x;
		pSource->fPosZ = cross.z;
		if ((nFlag & 1) != 0) {
			if (CheckObjectsOnPath(&sourceSaved, pSource, nFlag, m_pSkipInst) != 0) {
				return 0x10000000;
			}
		}
		pSource->pNavCell = pOutCell;
	}

	SNavMeshInst* pInst = pSource->pNavMeshInst;
	if (pInst->pResource->GetMeshType() == 2 && pInst->GetObjResource()->m_bEventOnly != 0) {
		tagNavPos source = *pSource;
		tagNavPos dest = *pDest;
		source.pNavCell = GetCellAt(source.Pos());
		source.pNavMeshInst = nullptr;
		SNavMoveContext context = *pContext;
		m_pSkipInst = pInst;
		const int32_t nTerrain = Move(nActorMode, nFlag, &source, &dest, &context);
		m_pSkipInst = nullptr;
		if ((nTerrain & 0x10000001) != 0) {
			return 0x10000000;
		}
	}
	return MoveInObject(pSource, pInst, nActorMode, 0, nFlag, pDest, pContext);
}

} // namespace NavMesh
