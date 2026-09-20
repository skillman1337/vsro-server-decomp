/**
 * ============================================================================
 * Silkroad Online - World Coordinate & Spatial Displacement Math
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GlobalPos.h
 *
 * Implements:
 *   - SRO_Vector3D / SRO_Position
 *   - Pos_Relative3D @ 0x00430BA0 (206 bytes)
 *   - Pos_RelativePlanar @ 0x00430AD0 (194 bytes)
 *   - Pos_SectorDifference @ 0x00430C70 (108 bytes)
 *   - Pos_RegionsCompatible @ 0x00430CE0 (85 bytes)
 *   - Pos_Relative3D_AssertCompatible @ 0x00430D40 (197 bytes)
 *   - Pos_NormalizeOutdoorRegion @ 0x00430E10 (156 bytes)
 *   - Vec3_Normalize @ 0x004328C0 (105 bytes)
 *   - Pos_EncodeCoord3D @ 0x0048C930 (32 bytes)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GLOBALPOS_H_
#define _SR_GAMESERVER_GLOBALPOS_H_

#include <cstdint>
#include <cmath>
#include "../JMX_Library/NavMesh_new/RTNavMesh.h"

// Native region dimension constant @ 0x00B45AD0
constexpr float SRO_REGION_WIDTH = 1920.0f;

// Native invalid/incompatible distance sentinel float @ 0x00B45C44
constexpr float SRO_INCOMPATIBLE_POS = 9.999999680285692e37f;

// 3D Cartesian vector
struct SRO_Vector3D {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	SRO_Vector3D() = default;
	SRO_Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// Region-anchored world coordinate
struct SRO_Position {
	uint16_t     wRegionID = 0;
	SRO_Vector3D pos;

	SRO_Position() = default;
	SRO_Position(uint16_t region, float x, float y, float z)
		: wRegionID(region), pos(x, y, z) {}
};

/**
 * [RECONSTRUCTED - 0x00560450 / 0x005604B0 / 0x00560A40 / 0x00560F0D] (x86 size 0x18)
 * tagObjLocation
 * Navigation-resolved position: AI::CNest::m_Pos (+0x10), the spawn position of
 * CMonster_SpawnInstance and the region counter argument.
 * CORRECTION (Claude): this was a second definition of NavMesh::tagNavPos with a DWORD at +0x04, so on x64
 * CRegionManagerBody::CheckPointValid read wRegionID and the coordinates at the wrong offsets. +0x04 is not
 * the slot-11 argument (0x0098B1DD writes this+0x04); CRTNavMeshTerrain slot 3 clears pos+0x04 at 0x0099FDDC.
 */
typedef NavMesh::tagNavPos tagObjLocation;

/**
 * [RECONSTRUCTED - Native 0x00430BA0] (206 bytes)
 * Pos_Relative3D
 *
 * Computes the 3D displacement vector from origin to destination across Silkroad world regions:
 *   - If dungeon region flag (bit 15: 0x8000) does not match between regions, returns SRO_INCOMPATIBLE_POS.
 *   - If inside dungeon (bit 15 set), returns simple Cartesian subtraction (to - from).
 *   - If in outdoor world, computes sector differences:
 *       sectorX = (to.region & 0xFF) - (from.region & 0xFF)
 *       sectorZ = (to.region >> 8) - (from.region >> 8)
 *       dx = sectorX * 1920.0 + to.x - from.x
 *       dy = to.y - from.y
 *       dz = sectorZ * 1920.0 + to.z - from.z
 */
void Pos_Relative3D(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
);

/**
 * [RECONSTRUCTED - Native 0x00430AD0] (194 bytes)
 * Pos_RelativePlanar
 *
 * Computes 2D planar displacement vector (Y elevation forced to 0.0f).
 */
void Pos_RelativePlanar(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
);

/**
 * [RECONSTRUCTED - Native 0x00430C70] (108 bytes)
 * Pos_SectorDifference
 *
 * Computes 2D sector index differences between two outdoor regions.
 */
bool Pos_SectorDifference(
	float* pOutDiff,
	uint16_t wFromRegion,
	uint16_t wToRegion
);

/**
 * [RECONSTRUCTED - Native 0x00430CE0] (85 bytes)
 * Pos_RegionsCompatible
 *
 * Checks if two regions belong to the same space (both dungeon or both outdoor)
 * and if outdoor, whether their sector distance is <= 1 (adjacent or same region).
 */
bool Pos_RegionsCompatible(
	uint16_t wRegion1,
	uint16_t wRegion2
);

/**
 * [RECONSTRUCTED - Native 0x00430D40] (197 bytes)
 * Pos_Relative3D_AssertCompatible
 *
 * Same calculation as Pos_Relative3D, but triggers MiniDump if regions are incompatible.
 */
void Pos_Relative3D_AssertCompatible(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
);

/**
 * [RECONSTRUCTED - Native 0x00430E10] (156 bytes)
 * Pos_NormalizeOutdoorRegion
 *
 * Normalizes local coordinates within [0.0f, 1920.0f) by wrapping across outdoor regions:
 *   - While x < 0.0f: sectorX -= 1, x += 1920.0f
 *   - While z < 0.0f: sectorZ -= 1, z += 1920.0f
 *   - While x >= 1920.0f: sectorX += 1, x -= 1920.0f
 *   - While z >= 1920.0f: sectorZ += 1, z -= 1920.0f
 */
void Pos_NormalizeOutdoorRegion(
	uint16_t* pwRegion,
	SRO_Vector3D* pPos
);

/**
 * [RECONSTRUCTED - Native 0x004328C0] (105 bytes)
 * Vec3_Normalize
 *
 * Normalizes 3D vector to unit length.
 */
void Vec3_Normalize(SRO_Vector3D* pVec);

/**
 * [RECONSTRUCTED - Native 0x0048C930] (32 bytes)
 * Pos_EncodeCoord3D
 *
 * Truncates floating-point coordinates to integer positions for wire protocol transmission.
 */
void Pos_EncodeCoord3D(
	const SRO_Vector3D* pInPos,
	int32_t* pOutCoords
);

#endif // _SR_GAMESERVER_GLOBALPOS_H_
