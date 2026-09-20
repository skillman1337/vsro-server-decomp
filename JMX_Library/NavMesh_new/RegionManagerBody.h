/**
 * ============================================================================
 * Joymax NavMesh - Region Manager Body
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RegionManagerBody.cpp
 *
 * Implements:
 *   - IRegionManager      .?AUIRegionManager@@ (RTTI 0x00C811EC), vftable 0x00B42F94
 *   - CRegionManagerBody  .?AVCRegionManagerBody@@ (RTTI 0x00C81228), vftable 0x00B431FC,
 *       static instance 0x00D67C80 constructed by 0x0098A1A0, g_pRegionManager 0x00CC387C
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_REGIONMANAGERBODY_H_
#define _JMX_LIBRARY_NAVMESH_NEW_REGIONMANAGERBODY_H_

#include <cstdint>
#include <map>
#include "NavArchive.h"
#include "NavMath.h"
#include "RTNavMesh.h"

namespace NavMesh {

class CMapLoader;
struct SNavEventZone;

/**
 * Event zone record registered by the object meshes (0x0098AE90, map at CRegionManagerBody +0x30)
 */
struct SNavEventZone {
	void*       pOwner = nullptr; // +0x00
	uint32_t    dwID = 0;         // +0x04: 0 unnamed, 0xFFFFxxxx for "_r<angle>" names
	std::string strName;          // +0x08
	float       fAngle = 0.0f;    // +0x24: radians from the "_r<degrees>" suffix
};

/**
 * IRegionManager (vftable 0x00B42F94); only the slots the server port calls are declared.
 */
struct IRegionManager {
	virtual ~IRegionManager() {}                                                                               // [0]

	// [1] 0x0098A660
	virtual int32_t Initialize(CNavFileManager* pFileManager, const char* pszPath, int32_t bSkipRegionInfo, int32_t nFlags) = 0;
	// [3] 0x0098AB70
	virtual void    LinkAllRegions() = 0;
	// [4] 0x0098AAB0
	virtual int32_t Release() = 0;
	// [5] 0x0098ABA0
	virtual int32_t AddRegion(uint16_t wRegionID, int32_t bLink) = 0;
	// [6] 0x0098ABD0
	virtual int32_t RemoveRegion(uint16_t wRegionID, int32_t bLink) = 0;
	// [11] 0x0098B1D0
	virtual int32_t CheckPointValid(tagNavPos* pPos, int32_t nReserved) = 0;
	// [12] 0x0098B300
	virtual int32_t QueryMovement(int32_t nActorMode, int32_t nFlag, const tagNavPos* pSource, tagNavPos* pDest,
		const int32_t* pMaxSteps, void* pActor) = 0;
	// [13] 0x0098B780
	virtual int32_t GetRegionOffset(uint16_t wFromRegion, uint16_t wToRegion, SNavVec3* pOut) = 0;
	// [16] 0x0098B9B0
	virtual void    NormalizeOutdoorPos(tagNavPos* pPos) = 0;
	// [30] 0x0098D880
	virtual int32_t IsLineOfSight(tagNavPos* pFrom, tagNavPos* pTo, int32_t nReserved) = 0;
	// [31] 0x0098AA90
	virtual CRTNavMesh* FindNavMesh(uint16_t wRegionID) = 0;
};

class CRegionManagerBody : public IRegionManager {
public:
	// [RECONSTRUCTED - 0x0098A1A0]
	CRegionManagerBody();
	// [RECONSTRUCTED - 0x0098A560]
	virtual ~CRegionManagerBody() override;

	virtual int32_t Initialize(CNavFileManager* pFileManager, const char* pszPath, int32_t bSkipRegionInfo, int32_t nFlags) override;
	virtual void    LinkAllRegions() override;
	virtual int32_t Release() override;
	virtual int32_t AddRegion(uint16_t wRegionID, int32_t bLink) override;
	virtual int32_t RemoveRegion(uint16_t wRegionID, int32_t bLink) override;
	virtual int32_t CheckPointValid(tagNavPos* pPos, int32_t nReserved) override;
	virtual int32_t QueryMovement(int32_t nActorMode, int32_t nFlag, const tagNavPos* pSource, tagNavPos* pDest,
		const int32_t* pMaxSteps, void* pActor) override;
	virtual int32_t GetRegionOffset(uint16_t wFromRegion, uint16_t wToRegion, SNavVec3* pOut) override;
	virtual void    NormalizeOutdoorPos(tagNavPos* pPos) override;
	virtual int32_t IsLineOfSight(tagNavPos* pFrom, tagNavPos* pTo, int32_t nReserved) override;
	virtual CRTNavMesh* FindNavMesh(uint16_t wRegionID) override;

	// [RECONSTRUCTED - 0x0098AE90] registers an event zone name used by an object mesh
	SNavEventZone* RegisterEventZone(const std::string& strName, void* pOwner);

	// [PARTIAL - 0x0098AC00] (eax = zone) event zone trigger. The teleport / structure registries that route a
	// trigger to the callback (+0x58, +0x80, +0xDC from objectstring.ifo, 0x0098C7F0) are not ported, so no zone
	// blocks a move: returns 0 like the native code when no actor or callback is set.
	int32_t TriggerEventZone(SNavEventZone* pZone, int32_t bEnter);

private:
	// [RECONSTRUCTED - 0x0098A9A0] region table load callback
	CRTNavMesh* LoadRegionMesh(uint16_t wRegionID);
	// [RECONSTRUCTED - 0x0098AA10] region table unload callback
	void ReleaseRegionMesh(CRTNavMesh* pMesh);

public:
	/**
	 * CRegionTable node (hash map at +0x08; add 0x0098DDC0, remove 0x0098DEB0, find 0x0098DD70)
	 */
	struct SRegionTableEntry {
		int32_t     nRefCount = 0;       // node +0x10
		CRTNavMesh* pMesh     = nullptr; // node +0x14
	};

	void*                                   m_pActor;          // +0x04: actor of the current query (0x0098B37F)
	std::map<uint16_t, SRegionTableEntry>   m_mapRegions;      // +0x08: CRegionTable (load callback +0x24, unload +0x28)
	std::map<std::string, SNavEventZone*>   m_mapEventZones;   // +0x30
	void*                                   m_pUserData;       // +0xC4: slots 20 / 21
	void*                                   m_pfnEventCallback;  // +0xC8: slot 7 (CMap_Load passes 0x005301B0)
	void*                                   m_pfnEventCallback2; // +0xCC: slot 8
	uint32_t                                m_dwUnkD0;         // +0xD0: slot 22
	uint32_t                                m_dwUnkD4;         // +0xD4: slot 26
	CRTNavMeshTerrain*                      m_pLastTerrain;    // +0x104 (0x00D67D84)
	SNavMeshInst*                           m_pLastInst;       // +0x108 (0x00D67D88)
	SNavMoveContext                         m_MoveContext;     // +0x10C
	uint32_t                                m_dwQueryFlags;    // +0x154: bit 0 keeps the last query in the context
	SNavVec3                                m_vSiegeGatePoint; // +0x15C (0x00D67DDC)
	int32_t                                 m_bSkipRegionInfo; // +0x168
	uint32_t                                m_dwNextEventZoneID; // +0x158: starts at 0xFFFF0000 (0x0098A2DC)
	CMapLoader*                             m_pMapLoader;      // g_pMapLoader 0x00D6CA30
};

// Global Region Manager Pointer (Native @ 0x00CC387C)
extern IRegionManager*     g_pRegionManager;
extern CRegionManagerBody* g_pRegionManagerBody; // static instance 0x00D67C80

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_REGIONMANAGERBODY_H_
