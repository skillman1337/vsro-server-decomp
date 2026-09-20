/**
 * ============================================================================
 * Joymax NavMesh - Map Loader
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\MapLoader.cpp
 *
 * Implements:
 *   - CMapLoader::CMapLoader           @ 0x009A7BE0 (static instance 0x00D67FF0)
 *   - CMapLoader::Initialize           @ 0x009A96F0
 *   - CMapLoader::Shutdown             @ 0x009A9C50
 *   - CMapLoader::ReadMapInfo          @ 0x009A8000
 *   - CMapLoader::LoadObjectInfo       @ 0x009A8070
 *   - CMapLoader::LoadObjExtInfo       @ 0x009A8390
 *   - CMapLoader::LoadTile2DInfo       @ 0x009A9280
 *   - CMapLoader::BuildDirectionTable  @ 0x009A9B40
 *   - CMapLoader::IsRegionEnabled      @ 0x009AA000 / 0x009A9FD0
 *   - CMapLoader::LoadTerrain          @ 0x009AA060
 *   - CMapLoader::LoadDungeon          @ 0x009AA1F0
 *   - CMapLoader::CreateInstance       @ 0x009AA330
 *   - CMapLoader::ReleaseInstance      @ 0x009AA520
 *   - CMapLoader::LoadResource         @ 0x009AA540
 *   - CMapLoader::LoadCPD              @ 0x009AA660
 *   - CMapLoader::LoadBSR              @ 0x009AAED0
 *   - CMapLoader::LoadBMS              @ 0x009AB5D0
 *   - CMapLoader::AcquireResource      @ 0x009AC0C0
 *   - CMapLoader::ReleaseResource      @ 0x0098DA60
 * ============================================================================
 */

#include "MapLoader.h"
#include "RTNavMeshDungeon.h"
#include "RTNavMeshObj.h"
#include "RTNavMeshTerrain.h"
#include "../BSLib/BSLog.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace NavMesh {

namespace {

const char* const kMapLoaderFile = "D:\\WORK2005\\Source\\JMX_Library\\NavMesh_new\\MapLoader.cpp";

// The ifo text files start with a 12-byte "JMXV...." signature; the tokenizer starts after it (0x009A818C).
constexpr size_t kIfoHeaderSize = 0x0C;

/**
 * Line reader over an ifo buffer (CStringProcess 0x0098A0D0 / 0x00989C40 / 0x00989D90).
 * Each data line is "<index> 0x<hex> "<string>" ["<string>"]".
 */
class CIfoLineReader {
public:
	explicit CIfoLineReader(const std::vector<uint8_t>& vecData)
		: m_pText(vecData.size() > kIfoHeaderSize ? reinterpret_cast<const char*>(vecData.data()) + kIfoHeaderSize : nullptr)
		, m_nSize(vecData.size() > kIfoHeaderSize ? vecData.size() - kIfoHeaderSize : 0)
		, m_nPos(0) {
	}

	// 0x0098A0D0: next line that is not empty
	bool NextLine(std::string& strLine) {
		while (m_nPos < m_nSize) {
			size_t nEnd = m_nPos;
			while (nEnd < m_nSize && m_pText[nEnd] != '\n' && m_pText[nEnd] != '\0') {
				++nEnd;
			}
			strLine.assign(m_pText + m_nPos, nEnd - m_nPos);
			m_nPos = nEnd + 1;
			while (!strLine.empty() && (strLine.back() == '\r' || strLine.back() == ' ' || strLine.back() == '\t')) {
				strLine.pop_back();
			}
			if (!strLine.empty()) {
				return true;
			}
		}
		return false;
	}

private:
	const char* m_pText;
	size_t      m_nSize;
	size_t      m_nPos;
};

// Parses "<index> 0x<hex>" (index optional) and the quoted strings that follow.
bool ParseIfoLine(const std::string& strLine, bool bHasIndex, uint32_t* pdwHex, std::vector<std::string>& vecQuoted) {
	const char* p = strLine.c_str();
	char* pEnd = nullptr;
	if (bHasIndex) {
		std::strtol(p, &pEnd, 10); // 0x00989C40
		if (pEnd == p) {
			return false;
		}
		p = pEnd;
	}
	*pdwHex = static_cast<uint32_t>(std::strtoul(p, &pEnd, 16)); // sscanf "%x"
	if (pEnd == p) {
		return false;
	}
	p = pEnd;

	vecQuoted.clear();
	for (;;) {
		const char* pOpen = std::strchr(p, '"');
		if (pOpen == nullptr) {
			break;
		}
		const char* pClose = std::strchr(pOpen + 1, '"');
		if (pClose == nullptr) {
			return false;
		}
		vecQuoted.emplace_back(pOpen + 1, pClose - pOpen - 1);
		p = pClose + 1;
	}
	return true;
}

} // namespace

// Static instance (0x00D67FF0) and its pointer (0x00D6CA30), set by the constructor 0x009A7BE0.
static CMapLoader s_MapLoader;
CMapLoader* g_pMapLoader = &s_MapLoader;

CMapLoader::CMapLoader()
	: m_bFileManagerReady(0)
	, m_bOwnsFileManager(0)
	, m_pFileManager(nullptr) {
	std::memset(m_abyMapInfo, 0, sizeof(m_abyMapInfo));
	std::memset(m_abyRegionBits, 0, sizeof(m_abyRegionBits));
	std::memset(m_aDirTable, 0, sizeof(m_aDirTable));
}

CMapLoader::~CMapLoader() {
}

/*
================
CMapLoader::Shutdown
[RECONSTRUCTED - 0x009A9C50]
================
*/
void CMapLoader::Shutdown() {
	m_mapObjExtInfo.clear();
	std::memset(m_abyRegionBits, 0, sizeof(m_abyRegionBits));
	m_bOwnsFileManager = 0;
	m_pFileManager = nullptr;
	m_vecObjectInfo.clear();
	m_vecTile2DInfo.clear();
}

/*
================
CMapLoader::Initialize
[RECONSTRUCTED - 0x009A96F0]
[PARTIAL] dungeon\dungeoninfo.txt (0x009A8B10) is not read: dungeons are not ported.
================
*/
bool CMapLoader::Initialize(CNavFileManager* pFileManager, const char* pszPath) {
	Shutdown();
	m_bFileManagerReady = 1;

	if (pFileManager != nullptr) {
		m_pFileManager = pFileManager;
	} else {
		// 0x009A984C: GFXDllCreateObject(2) rooted at "<path>data\"
		std::string strRoot = (pszPath != nullptr) ? pszPath : "";
		strRoot += "data\\";
		m_OwnFileManager.SetRoot(strRoot);
		m_pFileManager = &m_OwnFileManager;
		m_bOwnsFileManager = 1;
	}

	BuildDirectionTable();

	CNavArchive mapInfo;
	mapInfo.Open(m_pFileManager, "navmesh\\mapinfo.mfo");
	if (!ReadMapInfo(&mapInfo)) {
		return false;
	}

	CNavArchive objectInfo;
	objectInfo.Open(m_pFileManager, "navmesh\\object.ifo");
	if (!LoadObjectInfo(&objectInfo)) {
		return false;
	}

	CNavArchive objExtInfo;
	objExtInfo.Open(m_pFileManager, "navmesh\\objext.ifo");
	if (!LoadObjExtInfo(&objExtInfo)) {
		return false;
	}

	CNavArchive tile2DInfo;
	tile2DInfo.Open(m_pFileManager, "navmesh\\tile2d.ifo");
	if (!LoadTile2DInfo(&tile2DInfo)) {
		return false;
	}
	return true;
}

/*
================
CMapLoader::ReadMapInfo
[RECONSTRUCTED - 0x009A8000]
================
*/
bool CMapLoader::ReadMapInfo(CNavArchive* pArchive) {
	if (!pArchive->IsOpen()) {
		BSLib::AssertReport(0x2B, kMapLoaderFile, "ar.IsOpen()");
		return false;
	}
	uint8_t abyHeader[0x0C];
	pArchive->Read(abyHeader, sizeof(abyHeader));
	pArchive->Read(m_abyMapInfo, sizeof(m_abyMapInfo));
	pArchive->Read(m_abyRegionBits, sizeof(m_abyRegionBits));
	return !pArchive->IsFailed();
}

/*
================
CMapLoader::LoadObjectInfo
[RECONSTRUCTED - 0x009A8070]
================
*/
bool CMapLoader::LoadObjectInfo(CNavArchive* pArchive) {
	if (!pArchive->IsOpen()) {
		BSLib::AssertReport(0x38, kMapLoaderFile, "hSrc(%s) != INVALID_HANDLE_VALUE");
		return false;
	}
	CIfoLineReader reader(pArchive->GetData());
	std::string strLine;
	reader.NextLine(strLine); // object count

	std::vector<std::string> vecQuoted;
	while (reader.NextLine(strLine)) {
		SObjectInfo info;
		if (!ParseIfoLine(strLine, true, &info.dwFlag, vecQuoted) || vecQuoted.empty()) {
			continue;
		}
		info.strPath = vecQuoted[0];
		m_vecObjectInfo.push_back(info);
	}
	return true;
}

/*
================
CMapLoader::LoadObjExtInfo
[RECONSTRUCTED - 0x009A8390]
================
*/
bool CMapLoader::LoadObjExtInfo(CNavArchive* pArchive) {
	if (!pArchive->IsOpen()) {
		BSLib::AssertReport(0x65, kMapLoaderFile, "hSrc(%s) != INVALID_HANDLE_VALUE");
		return false;
	}
	CIfoLineReader reader(pArchive->GetData());
	std::string strLine;
	reader.NextLine(strLine);
	const long nCount = std::strtol(strLine.c_str(), nullptr, 10);

	std::vector<std::string> vecQuoted;
	while (reader.NextLine(strLine)) {
		uint32_t dwKey = 0;
		if (!ParseIfoLine(strLine, false, &dwKey, vecQuoted) || vecQuoted.size() < 2) {
			continue;
		}
		SObjExtInfo& info = m_mapObjExtInfo[dwKey];
		info.strName = vecQuoted[0];
		info.strEntrance = vecQuoted[1];
	}

	if (static_cast<size_t>(nCount) != m_mapObjExtInfo.size()) {
		BSLib::AssertReport(0x8D, kMapLoaderFile, "lnObjSize == m_sObjExtInfo.size()");
	}
	return true;
}

/*
================
CMapLoader::LoadTile2DInfo
[RECONSTRUCTED - 0x009A9280]
The texture name (second string) is stored lower-cased with the flag.
================
*/
bool CMapLoader::LoadTile2DInfo(CNavArchive* pArchive) {
	if (!pArchive->IsOpen()) {
		BSLib::AssertReport(0x109, kMapLoaderFile, "h != INVALID_HANDLE_VALUE");
		return false;
	}
	CIfoLineReader reader(pArchive->GetData());
	std::string strLine;
	reader.NextLine(strLine);

	std::vector<std::string> vecQuoted;
	while (reader.NextLine(strLine)) {
		STile2DInfo info;
		if (!ParseIfoLine(strLine, true, &info.dwFlag, vecQuoted) || vecQuoted.size() < 2) {
			continue;
		}
		info.strName = vecQuoted[1];
		for (size_t i = 0; i < info.strName.size(); ++i) {
			info.strName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(info.strName[i])));
		}
		m_vecTile2DInfo.push_back(info);
	}
	return true;
}

/*
================
CMapLoader::BuildDirectionTable
[RECONSTRUCTED - 0x009A9B40]
================
*/
void CMapLoader::BuildDirectionTable() {
	for (int32_t i = 0; i < 256; ++i) {
		// 0x00B45C70 (double 2*pi/256); the product is stored as a float before sin / cos (0x009A9B5C)
		const float fAngle = static_cast<float>(i * 0.024543600156903267);
		m_aDirTable[i].x = static_cast<float>(std::cos(static_cast<double>(fAngle)));
		m_aDirTable[i].z = -static_cast<float>(std::sin(static_cast<double>(fAngle)));
	}
}

/*
================
CMapLoader::IsRegionEnabled
[RECONSTRUCTED - 0x009AA000 / 0x009A9FD0]
================
*/
bool CMapLoader::IsRegionEnabled(uint16_t wRegionID) const {
	if ((wRegionID & 0x8000) != 0) {
		return false;
	}
	const int16_t sWidth = static_cast<int16_t>(m_abyMapInfo[0] | (m_abyMapInfo[1] << 8));
	const int16_t sHeight = static_cast<int16_t>(m_abyMapInfo[2] | (m_abyMapInfo[3] << 8));
	if (static_cast<int16_t>(wRegionID & 0xFF) >= sWidth || static_cast<int32_t>(wRegionID >> 8) >= sHeight) {
		return false;
	}
	return (m_abyRegionBits[wRegionID >> 3] & (0x80 >> (wRegionID & 7))) != 0;
}

/*
================
CMapLoader::LoadTerrain
[RECONSTRUCTED - 0x009AA060]
================
*/
CRTNavMeshTerrain* CMapLoader::LoadTerrain(uint16_t wRegionID) {
	if (!IsRegionEnabled(wRegionID)) {
		return nullptr;
	}

	char szPath[0x400];
	std::snprintf(szPath, sizeof(szPath), "navmesh\\nv_%04x.nvm", wRegionID);
	CNavArchive archive;
	if (!archive.Open(m_pFileManager, szPath)) {
		return nullptr;
	}
	uint8_t abyHeader[0x0C];
	archive.Read(abyHeader, sizeof(abyHeader));

	CRTNavMeshTerrain* pTerrain = new CRTNavMeshTerrain(); // pool 0x009AC4F0 (Clear(1))
	pTerrain->m_wRegionID = wRegionID;
	if (!pTerrain->Load(&archive, this)) {
		pTerrain->Clear(0);
		delete pTerrain;
		return nullptr;
	}
	return pTerrain;
}

/*
================
CMapLoader::LoadDungeon
[PARTIAL - 0x009AA1F0]
Dungeon navmeshes (dungeoninfo.txt, .dof) are not ported: no dungeon region loads.
================
*/
CRTNavMesh* CMapLoader::LoadDungeon(uint16_t /*wRegionID*/) {
	return nullptr;
}

/*
================
CMapLoader::CreateInstance
[RECONSTRUCTED - 0x009AA330]
[PARTIAL] dungeon entrances (.dof) get a CRTNavMeshDungeon placeholder resource without a dungeon link, so the
terrain still loads and every move into the entrance fails.
================
*/
SNavMeshInst* CMapLoader::CreateInstance(const SNvmObjectEntry* pEntry) {
	if (pEntry->dwObjectID >= m_vecObjectInfo.size()) {
		BSLib::AssertReport(0x4DF, kMapLoaderFile, "obj.dwID < m_sObjectInfo.size()");
		return nullptr;
	}
	const SObjectInfo* pInfo = &m_vecObjectInfo[pEntry->dwObjectID];

	CRTNavMesh* pResource = AcquireResource(pInfo);
	if (pResource == nullptr) {
		// 0x009AA505: MessageBoxA(path, "Load Fail(NavMesh Obj)")
		BSLib::Log_Printf(0x2000000, "Load Fail(NavMesh Obj): %s\n", pInfo->strPath.c_str());
		return nullptr;
	}

	SNavMeshInst* pInst = new SNavMeshInst(); // pool 0x009AC880
	pInst->pResource = pResource;
	pInst->entry = *pEntry;
	Matrix_RotationY(&pInst->matLocalToWorld, pEntry->fYaw);
	pInst->matLocalToWorld.m[12] = pEntry->vPos.x;
	pInst->matLocalToWorld.m[13] = pEntry->vPos.y;
	pInst->matLocalToWorld.m[14] = pEntry->vPos.z;
	SNavMatrix inverse;
	Matrix_InverseAffine(&pInst->matLocalToWorld, &inverse);
	pInst->matWorldToLocal = inverse;
	return pInst;
}

/*
================
CMapLoader::ReleaseInstance
[RECONSTRUCTED - 0x009AA520]
================
*/
void CMapLoader::ReleaseInstance(SNavMeshInst* pInst) {
	if (pInst == nullptr) {
		return;
	}
	const CRTNavMesh* pResource = pInst->pResource;
	const SObjectInfo* pInfo = nullptr;
	if (pResource != nullptr && pResource->GetMeshType() == 2) {
		pInfo = static_cast<const CRTNavMeshObj*>(pResource)->m_pObjectInfo;
	} else if (pResource != nullptr && pResource->GetMeshType() == 3) {
		pInfo = static_cast<const CRTNavMeshDungeon*>(pResource)->m_pObjectInfo;
	}
	ReleaseResource(pInfo);
	delete pInst;
}

/*
================
CMapLoader::AcquireResource
[RECONSTRUCTED - 0x009AC0C0]
================
*/
CRTNavMesh* CMapLoader::AcquireResource(const SObjectInfo* pInfo) {
	std::map<const SObjectInfo*, SNavResourceEntry>::iterator it = m_mapResources.find(pInfo);
	if (it != m_mapResources.end()) {
		++it->second.nRefCount;
		return it->second.pMesh;
	}

	CRTNavMesh* pMesh = LoadResource(pInfo);
	if (pMesh == nullptr) {
		return nullptr;
	}
	if (pMesh->GetMeshType() == 2) {
		static_cast<CRTNavMeshObj*>(pMesh)->m_pObjectInfo = pInfo; // 0x009AA3E5
	} else if (pMesh->GetMeshType() == 3) {
		static_cast<CRTNavMeshDungeon*>(pMesh)->m_pObjectInfo = pInfo;
	}
	SNavResourceEntry& entry = m_mapResources[pInfo];
	entry.nRefCount = 1;
	entry.pMesh = pMesh;
	return pMesh;
}

/*
================
CMapLoader::ReleaseResource
[RECONSTRUCTED - 0x0098DA60]
The native cache can park unreferenced resources on a free list for reuse; the port releases them.
================
*/
void CMapLoader::ReleaseResource(const SObjectInfo* pInfo) {
	std::map<const SObjectInfo*, SNavResourceEntry>::iterator it = m_mapResources.find(pInfo);
	if (it == m_mapResources.end()) {
		return;
	}
	if (--it->second.nRefCount > 0) {
		return;
	}
	delete it->second.pMesh; // unload callback 0x009AA620
	m_mapResources.erase(it);
}

/*
================
CMapLoader::LoadResource
[RECONSTRUCTED - 0x009AA540]
================
*/
CRTNavMesh* CMapLoader::LoadResource(const SObjectInfo* pInfo) {
	if (pInfo->strPath.empty()) {
		BSLib::AssertReport(0x52D, kMapLoaderFile, "NULL");
		return nullptr;
	}
	switch (pInfo->strPath.back()) {
	case 'd':
		return LoadCPD(pInfo->strPath);
	case 'f': {
		// [PARTIAL] 0x009AA920: .dof dungeon; placeholder so the owning terrain can load
		CRTNavMeshDungeon* pDungeon = new CRTNavMeshDungeon();
		return pDungeon;
	}
	case 'r':
		return LoadBSR(pInfo->strPath);
	case 's':
		return LoadBMS(pInfo->strPath);
	default:
		BSLib::AssertReport(0x52D, kMapLoaderFile, "NULL");
		return nullptr;
	}
}

/*
================
CMapLoader::LoadCPD
[RECONSTRUCTED - 0x009AA660]
Header 0x9B29E0 ("JMXVCPD 0101" + fields); the u32 at +0x0C locates the length-prefixed BSR path.
================
*/
CRTNavMesh* CMapLoader::LoadCPD(const std::string& strPath) {
	CNavArchive archive;
	if (!archive.Open(m_pFileManager, strPath.c_str())) {
		return nullptr;
	}
	uint8_t abyMagic[0x0C];
	archive.Read(abyMagic, sizeof(abyMagic));
	const uint32_t dwOffset = archive.ReadValue<uint32_t>();
	if (archive.IsFailed() || dwOffset == 0 || !archive.Seek(dwOffset)) {
		return nullptr;
	}
	std::string strBSR;
	if (!archive.ReadString(strBSR) || strBSR.empty()) {
		return nullptr;
	}
	return LoadBSR(strBSR);
}

/*
================
CMapLoader::LoadBSR
[RECONSTRUCTED - 0x009AAED0]
Header 0x009B27E0 ("JMXVRES 0109" + 13 u32); the u32 at +0x28 locates the length-prefixed collision bms path.
The simplified-mesh export branch (+0x2088, "data_simple_mesh\") is a tool feature and is off in the server.
================
*/
CRTNavMesh* CMapLoader::LoadBSR(const std::string& strPath) {
	CNavArchive archive;
	if (!archive.Open(m_pFileManager, strPath.c_str())) {
		return nullptr;
	}
	uint8_t abyHeader[0x40];
	archive.Read(abyHeader, sizeof(abyHeader));
	uint32_t dwOffset = 0;
	std::memcpy(&dwOffset, abyHeader + 0x28, sizeof(dwOffset));
	if (archive.IsFailed() || dwOffset == 0 || !archive.Seek(dwOffset)) {
		return nullptr;
	}
	std::string strBMS;
	if (!archive.ReadString(strBMS)) {
		return nullptr;
	}
	return LoadBMS(strBMS);
}

/*
================
CMapLoader::LoadBMS
[RECONSTRUCTED - 0x009AB5D0]
Header 0x3C bytes read into [esp+0x14] (0x009AB754, one push active): the u32 at +0x28 ([esp+0x3C] after the
call) locates the collision mesh, the u32 at +0x38 ([esp+0x4C]) holds its struct options.
================
*/
CRTNavMeshObj* CMapLoader::LoadBMS(const std::string& strPath) {
	CNavArchive archive;
	if (!archive.Open(m_pFileManager, strPath.c_str())) {
		return nullptr;
	}
	uint8_t abyHeader[0x3C];
	archive.Read(abyHeader, sizeof(abyHeader));
	uint32_t dwOffset = 0;
	uint32_t dwStructOption = 0;
	std::memcpy(&dwOffset, abyHeader + 0x28, sizeof(dwOffset));
	std::memcpy(&dwStructOption, abyHeader + 0x38, sizeof(dwStructOption));
	if (archive.IsFailed() || dwOffset == 0 || !archive.Seek(dwOffset)) {
		return nullptr;
	}

	CRTNavMeshObj* pMesh = new CRTNavMeshObj(); // pool 0x009AC6D0
	if (!pMesh->Load(&archive, this, dwStructOption)) {
		delete pMesh; // 0x0098E9A0
		return nullptr;
	}
	return pMesh;
}

} // namespace NavMesh
