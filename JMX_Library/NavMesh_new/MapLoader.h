/**
 * ============================================================================
 * Joymax NavMesh - Map Loader
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\MapLoader.cpp
 *
 * Implements:
 *   - CMapLoader (g_pMapLoader 0x00D6CA30)
 *       Initialize 0x009A96F0, Shutdown 0x009A9C50, ReadMapInfo 0x009A8000, LoadObjectInfo 0x009A8070,
 *       LoadObjExtInfo 0x009A8390, LoadTile2DInfo 0x009A9280, BuildDirectionTable 0x009A9B40,
 *       IsRegionEnabled 0x009AA000 / 0x009A9FD0, LoadTerrain 0x009AA060, CreateInstance 0x009AA330,
 *       ReleaseInstance 0x009AA520, AcquireResource 0x009AC0C0, ReleaseResource 0x0098DA60,
 *       LoadResource 0x009AA540, LoadCPD 0x009AA660, LoadBSR 0x009AAED0, LoadBMS 0x009AB5D0
 *
 * CORRECTION (Claude): replaces the earlier LoadMapData / UnloadMapData stub.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_MAPLOADER_H_
#define _JMX_LIBRARY_NAVMESH_NEW_MAPLOADER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "NavArchive.h"
#include "NavMath.h"

namespace NavMesh {

class CRTNavMesh;
class CRTNavMeshObj;
class CRTNavMeshTerrain;
struct SNavMeshInst;
struct SNvmObjectEntry;

/**
 * object.ifo entry (0x20)
 */
struct SObjectInfo {
	uint32_t    dwFlag;  // +0x00
	std::string strPath; // +0x04: res\...\x.bsr / .cpd / .bms / dungeon .dof
};

/**
 * tile2d.ifo entry (0x20)
 */
struct STile2DInfo {
	std::string strName; // +0x00: texture file, lower-cased (0x009A9551)
	uint32_t    dwFlag;  // +0x1C: folded into bits 16-31 of each terrain tile flag
};

/**
 * objext.ifo entry (map at +0x2048 keyed by region << 16 | uid)
 */
struct SObjExtInfo {
	std::string strName;     // +0x00
	std::string strEntrance; // +0x1C: dungeon entrance name
};

/**
 * Object resource cache node (map at +0x209C, keyed by the object.ifo entry)
 */
struct SNavResourceEntry {
	int32_t     nRefCount = 0;      // +0x28
	CRTNavMesh* pMesh     = nullptr; // +0x2C
};

class CMapLoader {
public:
	CMapLoader();
	~CMapLoader();

	// [RECONSTRUCTED - 0x009A96F0] (ecx = external file manager or null, edx = this, path on the stack)
	// With no file manager a folder manager is rooted at "<path>data\" (GFXFileManager mode 2).
	bool Initialize(CNavFileManager* pFileManager, const char* pszPath);

	// [RECONSTRUCTED - 0x009A9C50]
	void Shutdown();

	// [RECONSTRUCTED - 0x009AA000] outdoor regions inside the map bounds whose mapinfo bit is set
	bool IsRegionEnabled(uint16_t wRegionID) const;

	// [RECONSTRUCTED - 0x009AA060] navmesh\nv_%04x.nvm
	CRTNavMeshTerrain* LoadTerrain(uint16_t wRegionID);

	// [PARTIAL - 0x009AA1F0] dungeons (dungeon\dungeoninfo.txt, .dof) are not ported: fail closed
	CRTNavMesh* LoadDungeon(uint16_t wRegionID);

	// [RECONSTRUCTED - 0x009AA330]
	SNavMeshInst* CreateInstance(const SNvmObjectEntry* pEntry);

	// [RECONSTRUCTED - 0x009AA520]
	void ReleaseInstance(SNavMeshInst* pInst);

	// [RECONSTRUCTED - 0x009A9BA0] (al = index, edx = this, ecx = out)
	const SNavVec2& GetDirection(uint8_t byIndex) const { return m_aDirTable[byIndex]; }

	CNavFileManager* GetFileManager() const { return m_pFileManager; }
	const std::vector<STile2DInfo>& GetTile2DInfo() const { return m_vecTile2DInfo; }

private:
	// [RECONSTRUCTED - 0x009A8000]
	bool ReadMapInfo(CNavArchive* pArchive);
	// [RECONSTRUCTED - 0x009A8070]
	bool LoadObjectInfo(CNavArchive* pArchive);
	// [RECONSTRUCTED - 0x009A8390]
	bool LoadObjExtInfo(CNavArchive* pArchive);
	// [RECONSTRUCTED - 0x009A9280]
	bool LoadTile2DInfo(CNavArchive* pArchive);
	// [RECONSTRUCTED - 0x009A9B40] 256 (cos, -sin) pairs, 2 pi / 256 apart
	void BuildDirectionTable();

	// [RECONSTRUCTED - 0x009AC0C0]
	CRTNavMesh* AcquireResource(const SObjectInfo* pInfo);
	// [RECONSTRUCTED - 0x0098DA60]
	void ReleaseResource(const SObjectInfo* pInfo);
	// [RECONSTRUCTED - 0x009AA540] dispatch on the last character of the path
	CRTNavMesh* LoadResource(const SObjectInfo* pInfo);
	// [RECONSTRUCTED - 0x009AA660]
	CRTNavMesh* LoadCPD(const std::string& strPath);
	// [RECONSTRUCTED - 0x009AAED0]
	CRTNavMesh* LoadBSR(const std::string& strPath);
	// [RECONSTRUCTED - 0x009AB5D0]
	CRTNavMeshObj* LoadBMS(const std::string& strPath);

private:
	std::vector<STile2DInfo>                  m_vecTile2DInfo;  // +0x04
	uint32_t                                  m_bFileManagerReady; // +0x14
	uint32_t                                  m_bOwnsFileManager;  // +0x18
	CNavFileManager*                          m_pFileManager;   // +0x28
	CNavFileManager                           m_OwnFileManager; // replaces the GFXFileManager.dll object
	uint8_t                                   m_abyMapInfo[0x0C]; // +0x2C: u16 width, u16 height (0x009AA02E)
	uint8_t                                   m_abyRegionBits[0x2000]; // +0x38: bit (0x80 >> (id & 7)) of byte id >> 3
	std::vector<SObjectInfo>                  m_vecObjectInfo;  // +0x2038
	std::map<uint32_t, SObjExtInfo>           m_mapObjExtInfo;  // +0x2048
	std::map<const SObjectInfo*, SNavResourceEntry> m_mapResources; // +0x209C
	SNavVec2                                  m_aDirTable[256]; // +0x20C4
};

extern CMapLoader* g_pMapLoader; // 0x00D6CA30

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_MAPLOADER_H_
