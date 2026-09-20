/**
 * ============================================================================
 * Joymax NavMesh - Navigation Mesh Base Class
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new\RTNavMesh.h
 *
 * Implements:
 *   - CRTNavMesh::CRTNavMesh   @ 0x009C09D0
 *   - CRTNavMesh::~CRTNavMesh  @ 0x009C0A10
 * ============================================================================
 */

#include "RTNavMesh.h"

namespace NavMesh {

/*
================
CRTNavMesh::CRTNavMesh
[RECONSTRUCTED - 0x009C09D0]
================
*/
CRTNavMesh::CRTNavMesh()
	: m_pParentTerrain(nullptr)
	, m_wRegionID(0xFFFF)
	, m_wPad0A(0)
	, m_nMeshType(0) {
}

/*
================
CRTNavMesh::~CRTNavMesh
[RECONSTRUCTED - 0x009C0A10]
================
*/
CRTNavMesh::~CRTNavMesh() {
}

} // namespace NavMesh
