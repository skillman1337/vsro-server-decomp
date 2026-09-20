/**
 * ============================================================================
 * Silkroad Online - World Coordinate & Spatial Displacement Math Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GlobalPos.cpp
 *
 * Implements:
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

#include "GlobalPos.h"
#include "../JMX_ServerFramework/ServerFramework/ServerMain.h"
#include <cmath>
#include <cstdlib>

/*
================
Pos_Relative3D

[RECONSTRUCTED - Native 0x00430BA0] (206 bytes)
Calculates displacement vector from pFromPos (in wFromRegion) to pToPos (in wToRegion).
================
*/
void Pos_Relative3D(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
) {
	if (!pOutVec || !pFromPos || !pToPos) {
		return;
	}

	uint16_t fromDungeon = (wFromRegion >> 15);
	uint16_t toDungeon = (wToRegion >> 15);

	// Incompatible dungeon / outdoor space
	if (fromDungeon != toDungeon) {
		pOutVec->x = SRO_INCOMPATIBLE_POS;
		pOutVec->y = SRO_INCOMPATIBLE_POS;
		pOutVec->z = SRO_INCOMPATIBLE_POS;
		return;
	}

	// Dungeon coordinates: direct Cartesian displacement
	if (fromDungeon != 0) {
		pOutVec->x = pToPos->x - pFromPos->x;
		pOutVec->y = pToPos->y - pFromPos->y;
		pOutVec->z = pToPos->z - pFromPos->z;
		return;
	}

	// Outdoor coordinates: sector difference shifted by 1920.0f
	int32_t sectorX = static_cast<int32_t>(wToRegion & 0xFF) - static_cast<int32_t>(wFromRegion & 0xFF);
	int32_t sectorZ = static_cast<int32_t>(wToRegion >> 8) - static_cast<int32_t>(wFromRegion >> 8);

	pOutVec->x = static_cast<float>(sectorX) * SRO_REGION_WIDTH + pToPos->x - pFromPos->x;
	pOutVec->y = pToPos->y - pFromPos->y;
	pOutVec->z = static_cast<float>(sectorZ) * SRO_REGION_WIDTH + pToPos->z - pFromPos->z;
}

/*
================
Pos_RelativePlanar

[RECONSTRUCTED - Native 0x00430AD0] (194 bytes)
Calculates 2D horizontal displacement vector (forcing Y elevation to 0.0f).
================
*/
void Pos_RelativePlanar(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
) {
	if (!pOutVec || !pFromPos || !pToPos) {
		return;
	}

	uint16_t fromDungeon = (wFromRegion >> 15);
	uint16_t toDungeon = (wToRegion >> 15);

	if (fromDungeon != toDungeon) {
		pOutVec->x = SRO_INCOMPATIBLE_POS;
		pOutVec->y = SRO_INCOMPATIBLE_POS;
		pOutVec->z = SRO_INCOMPATIBLE_POS;
		return;
	}

	if (fromDungeon != 0) {
		pOutVec->x = pToPos->x - pFromPos->x;
		pOutVec->y = 0.0f;
		pOutVec->z = pToPos->z - pFromPos->z;
		return;
	}

	int32_t sectorX = static_cast<int32_t>(wToRegion & 0xFF) - static_cast<int32_t>(wFromRegion & 0xFF);
	int32_t sectorZ = static_cast<int32_t>(wToRegion >> 8) - static_cast<int32_t>(wFromRegion >> 8);

	pOutVec->x = static_cast<float>(sectorX) * SRO_REGION_WIDTH + pToPos->x - pFromPos->x;
	pOutVec->y = 0.0f;
	pOutVec->z = static_cast<float>(sectorZ) * SRO_REGION_WIDTH + pToPos->z - pFromPos->z;
}

/*
================
Pos_SectorDifference

[RECONSTRUCTED - Native 0x00430C70] (108 bytes)
Calculates sector count offsets between two outdoor regions.
================
*/
bool Pos_SectorDifference(
	float* pOutDiff,
	uint16_t wFromRegion,
	uint16_t wToRegion
) {
	if (!pOutDiff) {
		return false;
	}

	uint16_t fromDungeon = (wFromRegion >> 15);
	uint16_t toDungeon = (wToRegion >> 15);

	if (fromDungeon == toDungeon && fromDungeon == 0) {
		pOutDiff[0] = static_cast<float>(static_cast<int32_t>(wToRegion & 0xFF) - static_cast<int32_t>(wFromRegion & 0xFF));
		pOutDiff[1] = static_cast<float>(static_cast<int32_t>(wToRegion >> 8) - static_cast<int32_t>(wFromRegion >> 8));
		return true;
	}

	pOutDiff[0] = SRO_INCOMPATIBLE_POS;
	pOutDiff[1] = SRO_INCOMPATIBLE_POS;
	return false;
}

/*
================
Pos_RegionsCompatible

[RECONSTRUCTED - Native 0x00430CE0] (85 bytes)
Returns true if regions are in same partition (dungeon vs outdoor) and within adjacent sectors.
================
*/
bool Pos_RegionsCompatible(
	uint16_t wRegion1,
	uint16_t wRegion2
) {
	uint16_t d1 = (wRegion1 >> 15);
	uint16_t d2 = (wRegion2 >> 15);

	if (d1 != d2) {
		return false;
	}

	if (d1 != 0) {
		return true;
	}

	int32_t diffX = std::abs(static_cast<int32_t>(wRegion1 & 0xFF) - static_cast<int32_t>(wRegion2 & 0xFF));
	if (diffX <= 1) {
		int32_t diffZ = std::abs(static_cast<int32_t>(wRegion1 >> 8) - static_cast<int32_t>(wRegion2 >> 8));
		if (diffZ <= 1) {
			return true;
		}
	}

	return false;
}

/*
================
Pos_Relative3D_AssertCompatible

[RECONSTRUCTED - Native 0x00430D40] (197 bytes)
Same as Pos_Relative3D, but triggers MiniDump if regions are incompatible.
================
*/
void Pos_Relative3D_AssertCompatible(
	SRO_Vector3D* pOutVec,
	uint16_t wFromRegion,
	const SRO_Vector3D* pFromPos,
	uint16_t wToRegion,
	const SRO_Vector3D* pToPos
) {
	if (!pOutVec || !pFromPos || !pToPos) {
		return;
	}

	uint16_t fromDungeon = (wFromRegion >> 15);
	uint16_t toDungeon = (wToRegion >> 15);

	if (fromDungeon != toDungeon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}

	if (fromDungeon != 0) {
		pOutVec->x = pToPos->x - pFromPos->x;
		pOutVec->y = pToPos->y - pFromPos->y;
		pOutVec->z = pToPos->z - pFromPos->z;
		return;
	}

	int32_t sectorX = static_cast<int32_t>(wToRegion & 0xFF) - static_cast<int32_t>(wFromRegion & 0xFF);
	int32_t sectorZ = static_cast<int32_t>(wToRegion >> 8) - static_cast<int32_t>(wFromRegion >> 8);

	pOutVec->x = static_cast<float>(sectorX) * SRO_REGION_WIDTH + pToPos->x - pFromPos->x;
	pOutVec->y = pToPos->y - pFromPos->y;
	pOutVec->z = static_cast<float>(sectorZ) * SRO_REGION_WIDTH + pToPos->z - pFromPos->z;
}

/*
================
Pos_NormalizeOutdoorRegion

[RECONSTRUCTED - Native 0x00430E10] (156 bytes)
Adjusts coordinates into [0.0f, 1920.0f) by advancing or decrementing region indices.
================
*/
void Pos_NormalizeOutdoorRegion(
	uint16_t* pwRegion,
	SRO_Vector3D* pPos
) {
	if (!pwRegion || !pPos) {
		return;
	}

	// Dungeon coordinates are not normalized across sector boundaries
	if ((*pwRegion & 0x8000) != 0) {
		return;
	}

	uint8_t sectorX = static_cast<uint8_t>(*pwRegion & 0xFF);
	uint8_t sectorZ = static_cast<uint8_t>(*pwRegion >> 8);

	while (pPos->x < 0.0f) {
		sectorX -= 1;
		pPos->x += SRO_REGION_WIDTH;
	}

	while (pPos->z < 0.0f) {
		sectorZ -= 1;
		pPos->z += SRO_REGION_WIDTH;
	}

	while (pPos->x >= SRO_REGION_WIDTH) {
		sectorX += 1;
		pPos->x -= SRO_REGION_WIDTH;
	}

	while (pPos->z >= SRO_REGION_WIDTH) {
		sectorZ += 1;
		pPos->z -= SRO_REGION_WIDTH;
	}

	*pwRegion = static_cast<uint16_t>(sectorX | (static_cast<uint16_t>(sectorZ) << 8));
}

/*
================
Vec3_Normalize

[RECONSTRUCTED - Native 0x004328C0] (105 bytes)
Normalizes 3D vector to unit magnitude.
================
*/
void Vec3_Normalize(SRO_Vector3D* pVec) {
	if (!pVec) {
		return;
	}

	float fLenSq = pVec->x * pVec->x + pVec->y * pVec->y + pVec->z * pVec->z;
	float fLen = std::sqrt(fLenSq);

	if (fLen > 0.00001f) {
		float fScale = 1.0f / fLen;
		pVec->x *= fScale;
		pVec->y *= fScale;
		pVec->z *= fScale;
	}
}

/*
================
Pos_EncodeCoord3D

[RECONSTRUCTED - Native 0x0048C930] (32 bytes)
Truncates float coordinates into 32-bit signed integer representations.
================
*/
void Pos_EncodeCoord3D(
	const SRO_Vector3D* pInPos,
	int32_t* pOutCoords
) {
	if (!pInPos || !pOutCoords) {
		return;
	}

	pOutCoords[0] = static_cast<int32_t>(pInPos->x);
	pOutCoords[1] = static_cast<int32_t>(pInPos->y);
	pOutCoords[2] = static_cast<int32_t>(pInPos->z);
}
