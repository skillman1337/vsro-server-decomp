/**
 * ============================================================================
 * Joymax NavMesh - Dungeon Navigation Mesh
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMeshDungeon.cpp
 *
 * Implements:
 *   - CRTNavMeshDungeon  .?AVCRTNavMeshDungeon@@ (RTTI 0x00C81270), vftables 0x00B43594 (CRTNavMesh) /
 *       0x00B435B4 (IDungeon), size 0xA8, constructor 0x00998AD0, loader 0x00998DE0
 *
 * [PARTIAL] Dungeon navmeshes (.dof blocks, dungeoninfo.txt, block links) are not ported. The class only exists as
 * the type 3 resource of dungeon entrance objects placed on terrain, so those terrains load and every query that
 * would enter the dungeon fails closed.
 * CORRECTION (Claude): replaces the earlier BuildMesh / GetHeightAt / RayCast stub, which returned success without data.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHDUNGEON_H_
#define _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHDUNGEON_H_

#include <cstdint>
#include "RTNavMesh.h"

namespace NavMesh {

struct SObjectInfo;

class CRTNavMeshDungeon : public CRTNavMesh {
public:
	CRTNavMeshDungeon();
	virtual ~CRTNavMeshDungeon() override;

	// [RECONSTRUCTED - 0x00537ED0] vftable[1]
	virtual int32_t FindHeight(SNavVec3* /*pPos*/) override { return 0; }
	// [PARTIAL - 0x00999CF0] vftable[2]
	virtual int32_t Move(int32_t nActorMode, int32_t nFlag, tagNavPos* pSource, tagNavPos* pDest, SNavMoveContext* pContext) override;
	// [PARTIAL - 0x00999AA0] vftable[3]
	virtual void FindNavCell(tagNavPos* pPos) override;
	// [RECONSTRUCTED - 0x005ECFD0] vftable[4]
	virtual int32_t RegisterEventZones() override { return 1; }

public:
	const SObjectInfo* m_pObjectInfo; // +0x10: object.ifo entry of the entrance (0x009AA3E5)
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_RTNAVMESHDUNGEON_H_
