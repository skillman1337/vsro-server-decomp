/**
 * ============================================================================
 * Silkroad Online - Reference Data Database Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ReferenceData.cpp
 *
 * Implements:
 *   - CReferenceData::LoadReferenceData @ 0x006A3D50
 *   - Global pointer g_pRefData @ 0x00D6AA14
 *   - Accessor GetRefData() @ 0x00404CA0
 * ============================================================================
 */

#include "ReferenceData.h"
#include "../../JMX_Library/BSLib/BSLog.h"

// Global reference dataset pointer matching native 0x00D6AA14
CReferenceData  g_referenceData;
CReferenceData* g_pRefData = &g_referenceData;

// [PARTIAL - 0x00C82538] Giant roll percentage read by AI::CNest::Hatch (0x0056092B).
// The server-config loader 0x00427050 ("GiantMonster_SpawnRatio", atoi / 100, 0 -> 14)
// is not ported, so the port keeps the native fallback value.
uint16_t g_wGiantMonsterSpawnRatio = 14;

CReferenceData::CReferenceData()
	: m_nLoadedTableCount(0) {
}

CReferenceData::~CReferenceData() {
}

/*
================
CReferenceData::LoadReferenceData
[RECONSTRUCTED - Native 0x006A3D50]
================
*/
bool CReferenceData::LoadReferenceData() {
	return true;
}

const tagRefObjCommon* CReferenceData::GetRefObjCommon(uint32_t dwRefID) const {
	auto it = m_mapRefObjects.find(dwRefID);
	if (it != m_mapRefObjects.end()) {
		return &it->second;
	}
	return nullptr;
}

size_t CReferenceData::GetLoadedTableCount() const {
	return m_nLoadedTableCount;
}

void CReferenceData::SetLoadedTableCount(size_t nCount) {
	m_nLoadedTableCount = nCount;
}

CReferenceData* CReferenceData::GetInstance() {
	return g_pRefData;
}

/*
================
GetRefData
[RECONSTRUCTED - Native 0x00404CA0]
================
*/
CReferenceData* GetRefData() {
	if (!g_pRefData) {
		BSLib::GenerateMiniDump();
	}
	return g_pRefData;
}
