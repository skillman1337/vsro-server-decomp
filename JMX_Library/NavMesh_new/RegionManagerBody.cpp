/**
 * ============================================================================
 * Joymax NavMesh - Region Manager Body
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RegionManagerBody.cpp
 *
 * Implements:
 *   - CRegionManagerBody::CRegionManagerBody   @ 0x0098A1A0 (IRegionManager 0x0098A110)
 *   - CRegionManagerBody::~CRegionManagerBody  @ 0x0098A560
 *   - CRegionManagerBody::Initialize           @ 0x0098A660 (vftable[1])
 *   - CRegionManagerBody::LoadRegionMesh       @ 0x0098A9A0
 *   - CRegionManagerBody::ReleaseRegionMesh    @ 0x0098AA10
 *   - CRegionManagerBody::FindNavMesh          @ 0x0098AA90 / 0x0098DD70 (vftable[31])
 *   - CRegionManagerBody::Release              @ 0x0098AAB0 (vftable[4])
 *   - CRegionManagerBody::LinkAllRegions       @ 0x0098AB70 (vftable[3])
 *   - CRegionManagerBody::AddRegion            @ 0x0098ABA0 / 0x0098DDC0 (vftable[5])
 *   - CRegionManagerBody::RemoveRegion         @ 0x0098ABD0 / 0x0098DEB0 (vftable[6])
 *   - CRegionManagerBody::TriggerEventZone     [PARTIAL - 0x0098AC00]
 *   - CRegionManagerBody::RegisterEventZone    @ 0x0098AE90
 *   - CRegionManagerBody::CheckPointValid      @ 0x0098B1D0 (vftable[11])
 *   - CRegionManagerBody::QueryMovement        @ 0x0098B300 (vftable[12])
 *   - CRegionManagerBody::GetRegionOffset      @ 0x0098B780 (vftable[13])
 *   - CRegionManagerBody::NormalizeOutdoorPos  @ 0x0098B9B0 (vftable[16])
 *   - CRegionManagerBody::IsLineOfSight        @ 0x0098D880 (vftable[30])
 *
 * CORRECTION (Claude): the earlier body kept a plain region map with a fail-closed flag and a QueryMovement that
 * copied the source when a cell was present. The native query walks the meshes cell by cell (CRTNavMesh::Move).
 * ============================================================================
 */

#include "RegionManagerBody.h"
#include "MapLoader.h"
#include "RTNavCell.h"
#include "RTNavMeshObj.h"
#include "RTNavMeshTerrain.h"
#include "../BSLib/BSLog.h"

#include <cctype>
#include <cmath>

namespace NavMesh {

namespace {

constexpr float kNavRegionSize = 1920.0f; // 0x00B45AD0

// [RECONSTRUCTED - 0x0098ADD0] trailing "_r<digits>": strips the suffix and returns the number, otherwise -1
int32_t ParseEventZoneAngle(std::string& strName) {
	int32_t nValue = 0;
	int32_t nScale = 1;
	const int32_t nLength = static_cast<int32_t>(strName.size());
	const char* pData = strName.c_str();
	for (int32_t i = nLength - 1; i >= 0; --i) {
		const char ch = pData[i];
		if (!std::isdigit(static_cast<unsigned char>(ch))) {
			if (ch != 'r' || i <= 0 || pData[i - 1] != '_') {
				return -1;
			}
			strName.erase(static_cast<size_t>(i - 1));
			return nValue;
		}
		nValue += (ch - '0') * nScale;
		nScale *= 10;
	}
	return -1;
}

} // namespace

// Static instance (0x00D67C80) and the interface pointer (0x00CC387C).
static CRegionManagerBody s_RegionManagerBody;
CRegionManagerBody* g_pRegionManagerBody = &s_RegionManagerBody;
IRegionManager*     g_pRegionManager = &s_RegionManagerBody;

CRegionManagerBody::CRegionManagerBody()
	: m_pActor(nullptr)
	, m_pUserData(nullptr)
	, m_pfnEventCallback(nullptr)
	, m_pfnEventCallback2(nullptr)
	, m_dwUnkD0(0)
	, m_dwUnkD4(0)
	, m_pLastTerrain(nullptr)
	, m_pLastInst(nullptr)
	, m_MoveContext()
	, m_dwQueryFlags(0)
	, m_vSiegeGatePoint{ 0.0f, 0.0f, 0.0f }
	, m_bSkipRegionInfo(1)
	, m_dwNextEventZoneID(0xFFFF0000)
	, m_pMapLoader(g_pMapLoader) {
}

CRegionManagerBody::~CRegionManagerBody() {
	for (std::map<std::string, SNavEventZone*>::iterator it = m_mapEventZones.begin(); it != m_mapEventZones.end(); ++it) {
		delete it->second;
	}
	m_mapEventZones.clear();
}

/*
================
CRegionManagerBody::Initialize
[RECONSTRUCTED - 0x0098A660]
CMap_Load (0x00530514) passes (no file manager, server path, skip regioninfo.txt, flags 0).
[PARTIAL] regioninfo.txt (0x0098BE30, only read when not skipped) and objectstring.ifo (0x0098C7F0, fortress
structure positions for the event zone callbacks) are not read.
================
*/
int32_t CRegionManagerBody::Initialize(CNavFileManager* pFileManager, const char* pszPath, int32_t bSkipRegionInfo, int32_t nFlags) {
	m_dwQueryFlags = static_cast<uint32_t>(nFlags);
	m_bSkipRegionInfo = bSkipRegionInfo;
	m_pMapLoader = g_pMapLoader;
	return m_pMapLoader->Initialize(pFileManager, pszPath) ? 1 : 0;
}

/*
================
CRegionManagerBody::LoadRegionMesh
[RECONSTRUCTED - 0x0098A9A0]
================
*/
CRTNavMesh* CRegionManagerBody::LoadRegionMesh(uint16_t wRegionID) {
	if ((wRegionID & 0x8000) != 0) {
		return m_pMapLoader->LoadDungeon(wRegionID);
	}
	return m_pMapLoader->LoadTerrain(wRegionID);
}

/*
================
CRegionManagerBody::ReleaseRegionMesh
[RECONSTRUCTED - 0x0098AA10]
================
*/
void CRegionManagerBody::ReleaseRegionMesh(CRTNavMesh* pMesh) {
	if (pMesh == nullptr) {
		return;
	}
	if (pMesh->GetMeshType() == 1) {
		static_cast<CRTNavMeshTerrain*>(pMesh)->Clear(0);
	}
	delete pMesh;
}

/*
================
CRegionManagerBody::FindNavMesh
[RECONSTRUCTED - 0x0098AA90 -> 0x0098DD70]
================
*/
CRTNavMesh* CRegionManagerBody::FindNavMesh(uint16_t wRegionID) {
	std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.find(wRegionID);
	return (it != m_mapRegions.end()) ? it->second.pMesh : nullptr;
}

/*
================
CRegionManagerBody::Release
[RECONSTRUCTED - 0x0098AAB0]
================
*/
int32_t CRegionManagerBody::Release() {
	for (std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.begin(); it != m_mapRegions.end(); ++it) {
		ReleaseRegionMesh(it->second.pMesh);
	}
	m_mapRegions.clear();
	return 1;
}

/*
================
CRegionManagerBody::LinkAllRegions
[RECONSTRUCTED - 0x0098AB70]
Drops the global edges from every terrain cell (0x0098AA70), then links them again (0x0098AA50) so edges towards
regions loaded later find their neighbour cell.
================
*/
void CRegionManagerBody::LinkAllRegions() {
	for (std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.begin(); it != m_mapRegions.end(); ++it) {
		if (it->second.pMesh->GetMeshType() == 1) {
			static_cast<CRTNavMeshTerrain*>(it->second.pMesh)->PurgeGlobalEdges();
		}
	}
	for (std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.begin(); it != m_mapRegions.end(); ++it) {
		if (it->second.pMesh->GetMeshType() == 1) {
			static_cast<CRTNavMeshTerrain*>(it->second.pMesh)->LinkEdges(0);
		}
	}
}

/*
================
CRegionManagerBody::AddRegion
[RECONSTRUCTED - 0x0098ABA0 -> 0x0098DDC0]
A loaded region only gains a reference.
================
*/
int32_t CRegionManagerBody::AddRegion(uint16_t wRegionID, int32_t bLink) {
	CRTNavMesh* pMesh = nullptr;
	std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.find(wRegionID);
	if (it != m_mapRegions.end()) {
		++it->second.nRefCount;
		pMesh = it->second.pMesh;
	} else {
		pMesh = LoadRegionMesh(wRegionID);
		if (pMesh != nullptr) {
			SRegionTableEntry& entry = m_mapRegions[wRegionID];
			entry.nRefCount = 1;
			entry.pMesh = pMesh;
		}
	}

	if (bLink != 0) {
		LinkAllRegions();
	}
	return (pMesh != nullptr) ? 1 : 0;
}

/*
================
CRegionManagerBody::RemoveRegion
[RECONSTRUCTED - 0x0098ABD0 -> 0x0098DEB0]
The table can park unreferenced regions for reuse when its cache size (+0x24) is set; the server table has none.
================
*/
int32_t CRegionManagerBody::RemoveRegion(uint16_t wRegionID, int32_t bLink) {
	std::map<uint16_t, SRegionTableEntry>::iterator it = m_mapRegions.find(wRegionID);
	if (it != m_mapRegions.end()) {
		if (--it->second.nRefCount <= 0) {
			ReleaseRegionMesh(it->second.pMesh);
			m_mapRegions.erase(it);
		}
	}
	if (bLink != 0) {
		LinkAllRegions();
	}
	return 1;
}

/*
================
CRegionManagerBody::RegisterEventZone
[RECONSTRUCTED - 0x0098AE90]
Looked up by the name as given; a new zone is stored under the name with any "_r<degrees>" suffix removed and,
for such names, lower-cased with an ID from the 0xFFFF0000 counter and the angle in radians.
================
*/
SNavEventZone* CRegionManagerBody::RegisterEventZone(const std::string& strName, void* pOwner) {
	std::map<std::string, SNavEventZone*>::iterator it = m_mapEventZones.find(strName);
	if (it != m_mapEventZones.end()) {
		it->second->pOwner = pOwner;
		return it->second;
	}

	std::string strKey = strName;
	const int32_t nAngle = ParseEventZoneAngle(strKey);
	SNavEventZone* pZone = new SNavEventZone();
	pZone->pOwner = pOwner;
	if (nAngle < 0) {
		pZone->dwID = 0;
		pZone->fAngle = 0.0f;
	} else {
		pZone->dwID = m_dwNextEventZoneID++;
		for (size_t i = 0; i < strKey.size(); ++i) {
			strKey[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(strKey[i])));
		}
		pZone->fAngle = static_cast<float>(static_cast<double>(nAngle) * static_cast<double>(0.01745329238474369f));
	}

	std::map<std::string, SNavEventZone*>::iterator itKey = m_mapEventZones.find(strKey);
	if (itKey != m_mapEventZones.end()) {
		delete itKey->second;
		itKey->second = pZone;
	} else {
		m_mapEventZones[strKey] = pZone;
	}
	pZone->strName = strKey;
	return pZone;
}

/*
================
CRegionManagerBody::TriggerEventZone
[PARTIAL - 0x0098AC00]
Native: with an actor (+0x04) and a callback (+0xC8), resolves the zone through the structure tables (+0x80 by
object key, +0x58 by name) and calls callback(info, bEnter, actor); CMap_Load installs 0x005301B0, which runs
CGame_ApplyTeleportTransition. Those structure tables come from objectstring.ifo / RegisterStructure (vftable[9]),
which are not ported, so no zone reaches the callback.
================
*/
int32_t CRegionManagerBody::TriggerEventZone(SNavEventZone* /*pZone*/, int32_t /*bEnter*/) {
	return 0;
}

/*
================
CRegionManagerBody::CheckPointValid
[RECONSTRUCTED - 0x0098B1D0]
Outdoor positions are wrapped into [0, 1920) one region step per axis; dungeon positions without a cell are
retried at (x+1, z+1), (x-1, z+1), (x-1, z-1) and (x-3, z-1).
================
*/
int32_t CRegionManagerBody::CheckPointValid(tagNavPos* pPos, int32_t nReserved) {
	m_pActor = reinterpret_cast<void*>(static_cast<intptr_t>(nReserved));
	uint8_t* pbyRegion = reinterpret_cast<uint8_t*>(&pPos->wRegionID);

	if ((pPos->wRegionID & 0x8000) == 0) {
		if (!(0.0f <= pPos->fPosX)) {
			pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] - 1);
			pPos->fPosX = pPos->fPosX + kNavRegionSize;
		}
		if (!(0.0f <= pPos->fPosZ)) {
			pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] - 1);
			pPos->fPosZ = pPos->fPosZ + kNavRegionSize;
		}
		if (!(pPos->fPosX < kNavRegionSize)) {
			pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] + 1);
			pPos->fPosX = pPos->fPosX - kNavRegionSize;
		}
		if (!(pPos->fPosZ < kNavRegionSize)) {
			pPos->fPosZ = pPos->fPosZ - kNavRegionSize;
			pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] + 1);
		}
	}

	CRTNavMesh* pMesh = FindNavMesh(pPos->wRegionID);
	if (pMesh == nullptr) {
		return 0;
	}

	pMesh->FindNavCell(pPos);
	if (pPos->pNavCell == nullptr && (pPos->wRegionID & 0x8000) != 0) {
		pPos->fPosX = pPos->fPosX + 1.0f;
		pPos->fPosZ = 1.0f + pPos->fPosZ;
		pMesh->FindNavCell(pPos);
		if (pPos->pNavCell == nullptr) {
			pPos->fPosX = pPos->fPosX - 2.0f;
			pMesh->FindNavCell(pPos);
			if (pPos->pNavCell == nullptr) {
				pPos->fPosZ = pPos->fPosZ - 2.0f;
				pMesh->FindNavCell(pPos);
				if (pPos->pNavCell == nullptr) {
					pPos->fPosX = pPos->fPosX - 2.0f;
					pMesh->FindNavCell(pPos);
				}
			}
		}
	}
	return (pPos->pNavCell != nullptr) ? 1 : 0;
}

/*
================
CRegionManagerBody::QueryMovement
[RECONSTRUCTED - 0x0098B300]
Moves from the source towards the destination (converted into the source region). A move that enters or leaves an
object (result 0x04 / 0x10) without being blocked and still has 5 units to go continues from the transition point,
at most 7 legs. The result fails when the destination leaves the region bounds or has no cell.
================
*/
int32_t CRegionManagerBody::QueryMovement(int32_t nActorMode, int32_t nFlag, const tagNavPos* pSource, tagNavPos* pDest,
	const int32_t* pMaxSteps, void* pActor) {
	if (pSource->wRegionID == pDest->wRegionID && pSource->fPosX == pDest->fPosX && pSource->fPosY == pDest->fPosY &&
		pSource->fPosZ == pDest->fPosZ) {
		*pDest = *pSource;
		return 0;
	}

	m_pActor = pActor;
	if ((m_dwQueryFlags & 1) != 0) {
		m_MoveContext.dstCopy.pNavCell = nullptr;
		m_MoveContext.dstCopy.pNavMeshInst = nullptr;
		m_MoveContext.pLastObjEdge = nullptr;
	}
	m_MoveContext.dwLinkEdgeIndex = 0;
	m_MoveContext.nStepsLeft = (pMaxSteps != nullptr) ? *pMaxSteps : 0x7FFFFFFF;

	tagNavPos source = *pSource;
	if (pDest->wRegionID != source.wRegionID) {
		const int32_t nDX = static_cast<int32_t>(pDest->wRegionID & 0xFF) - static_cast<int32_t>(source.wRegionID & 0xFF);
		const int32_t nDZ = static_cast<int32_t>(pDest->wRegionID >> 8) - static_cast<int32_t>(source.wRegionID >> 8);
		pDest->fPosX = static_cast<float>(nDX) * kNavRegionSize + pDest->fPosX;
		pDest->fPosZ = static_cast<float>(nDZ) * kNavRegionSize + pDest->fPosZ;
		pDest->wRegionID = source.wRegionID;
	}

	if (m_MoveContext.nStepsLeft > 10) {
		// The differences are compared on the x87 stack without a float store (0x0098B442 / 0x0098B451)
		if (static_cast<double>(pDest->fPosX) - source.fPosX > kNavRegionSize ||
			static_cast<double>(pDest->fPosZ) - source.fPosZ > kNavRegionSize) {
			return 0x10000000;
		}
	}

	const tagNavPos destSaved = *pDest;
	int32_t nResult = 0;
	int32_t nLegs = 0;
	for (;;) {
		CRTNavCell* pCell = source.pNavCell;
		if (pCell == nullptr) {
			// 0x0098B4B3 dereferences the source cell; callers pass resolved positions
			return 0x10000000;
		}
		CRTNavMesh* pMesh = pCell->m_pMesh;
		++nLegs;
		pDest->pNavCell = pCell;
		pDest->pNavMeshInst = source.pNavMeshInst;

		CRTNavMesh* pMover = pMesh;
		if (pMesh->m_pParentTerrain != nullptr) {
			if ((source.wRegionID & 0x8000) != 0) {
				pMover = pMesh->m_pParentTerrain;
			} else {
				pMover = source.pNavMeshInst->pTerrain;
			}
		}

		nResult = pMover->Move(nActorMode, nFlag, &source, pDest, &m_MoveContext);
		if ((nResult & 0x10000001) != 0 || (nResult & 0x14) == 0) {
			break;
		}

		SNavVec3 remaining = { destSaved.fPosX - source.fPosX, destSaved.fPosY - source.fPosY, destSaved.fPosZ - source.fPosZ };
		if (Vec3_LengthSq(&remaining) < 25.0f) {
			break;
		}

		source = *pDest;
		*pDest = destSaved;
		if (pDest->wRegionID != source.wRegionID) {
			const int32_t nDX = static_cast<int32_t>(pDest->wRegionID & 0xFF) - static_cast<int32_t>(source.wRegionID & 0xFF);
			const int32_t nDZ = static_cast<int32_t>(pDest->wRegionID >> 8) - static_cast<int32_t>(source.wRegionID >> 8);
			pDest->wRegionID = source.wRegionID;
			pDest->fPosX = static_cast<float>(nDX) * kNavRegionSize + pDest->fPosX;
			pDest->fPosZ = static_cast<float>(nDZ) * kNavRegionSize + pDest->fPosZ;
		}
		if (nLegs > 6) {
			return 0x10000000;
		}
	}

	if ((m_dwQueryFlags & 1) != 0 && m_MoveContext.pLastObjEdge != nullptr) {
		m_MoveContext.srcCopy = *pSource;
		m_MoveContext.dstCopy = *pDest;
	}

	if ((nResult & 0x10000000) != 0 || (pDest->wRegionID & 0x8000) != 0) {
		return nResult;
	}
	if (!(pDest->fPosX < kNavRegionSize) || !(pDest->fPosZ < kNavRegionSize) || !(0.0f <= pDest->fPosX) || !(0.0f <= pDest->fPosZ)) {
		return 0x10000000;
	}
	if (pDest->pNavCell == nullptr) {
		return 0x10000000;
	}
	return nResult;
}

/*
================
CRegionManagerBody::GetRegionOffset
[RECONSTRUCTED - 0x0098B780]
================
*/
int32_t CRegionManagerBody::GetRegionOffset(uint16_t wFromRegion, uint16_t wToRegion, SNavVec3* pOut) {
	const int32_t nDX = static_cast<int32_t>(wToRegion & 0xFF) - static_cast<int32_t>(wFromRegion & 0xFF);
	const int32_t nDZ = static_cast<int32_t>(wToRegion >> 8) - static_cast<int32_t>(wFromRegion >> 8);
	pOut->x = static_cast<float>(nDX) * kNavRegionSize;
	pOut->z = kNavRegionSize * static_cast<float>(nDZ);
	pOut->y = 0.0f;
	return 1;
}

/*
================
CRegionManagerBody::NormalizeOutdoorPos
[RECONSTRUCTED - 0x0098B9B0]
================
*/
void CRegionManagerBody::NormalizeOutdoorPos(tagNavPos* pPos) {
	if ((pPos->wRegionID & 0x8000) != 0) {
		return;
	}
	uint8_t* pbyRegion = reinterpret_cast<uint8_t*>(&pPos->wRegionID);
	while (0.0f > pPos->fPosX) {
		pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] - 1);
		pPos->fPosX = pPos->fPosX + kNavRegionSize;
	}
	while (0.0f > pPos->fPosZ) {
		pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] - 1);
		pPos->fPosZ = pPos->fPosZ + kNavRegionSize;
	}
	while (pPos->fPosX >= kNavRegionSize) {
		pbyRegion[0] = static_cast<uint8_t>(pbyRegion[0] + 1);
		pPos->fPosX = pPos->fPosX - kNavRegionSize;
	}
	while (pPos->fPosZ >= kNavRegionSize) {
		pbyRegion[1] = static_cast<uint8_t>(pbyRegion[1] + 1);
		pPos->fPosZ = pPos->fPosZ - kNavRegionSize;
	}
}

/*
================
CRegionManagerBody::IsLineOfSight
[RECONSTRUCTED - 0x0098D880]
Repeats the movement query while it hops across linked objects (result 0x02), at most 6 times, moving pFrom to each
reached point. A siege-gate crossing (0x80) that is not on a siege object (0x100) blocks the line between two
terrain positions.
================
*/
int32_t CRegionManagerBody::IsLineOfSight(tagNavPos* pFrom, tagNavPos* pTo, int32_t /*nReserved*/) {
	if (pFrom->pNavCell == nullptr || pTo->pNavCell == nullptr) {
		return 0;
	}

	tagNavPos to = *pTo;
	int32_t nResult = 0;
	for (int32_t nHop = 0;;) {
		nResult = QueryMovement(0, 1, pFrom, &to, nullptr, nullptr);
		if ((nResult & 0x10000000) != 0 || (nResult & 2) == 0) {
			break;
		}
		*pFrom = to;
		to = *pTo;
		if (++nHop >= 6) {
			break;
		}
	}

	if ((nResult & 0x80) == 0) {
		return ((nResult & 0x10000001) == 0) ? 1 : 0;
	}
	if ((nResult & 0x100) == 0) {
		if (pFrom->pNavCell->IsTerrainCell() && pTo->pNavCell->IsTerrainCell()) {
			return 0;
		}
	}
	return 1;
}

} // namespace NavMesh
