/**
 * ============================================================================
 * Joymax NavMesh - Vector / Matrix / Line helpers
 * Original Source: D:\WORK2005\Source\JMX_Library\NavMesh_new (inlined math COMDATs)
 *
 * Implements:
 *   - Vec3_TransformCoord      @ 0x00997330 (80 bytes)   D3DX row-vector transform, no w divide
 *   - Vec3_LengthSq            @ 0x00997580 (33 bytes)
 *   - Vec3_Normalize           @ 0x004328C0 (105 bytes)  (COMDAT shared with SR_GameServer)
 *   - Vec2_Normalize           @ 0x009A77A0 (100 bytes)
 *   - Matrix_RotationY         @ 0x0099ED10 (92 bytes)
 *   - Matrix_InverseAffine     @ 0x0099EAD0 (572 bytes)
 *   - Plane_FromPoints         @ 0x00537E10 (191 bytes)
 *   - Line2_Intersect          @ 0x009BFCC0 (379 bytes)
 *
 * The native code evaluates on the x87 stack with 53-bit precision control (MSVC CRT default FPCW 0x27F), which is
 * IEEE double arithmetic; values are rounded to float only where the native stores them (fstp dword). The port
 * keeps that split: double intermediates, float at each native store. Verified against the native bytes under
 * unicorn (scratchpad navdiff).
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_NAVMATH_H_
#define _JMX_LIBRARY_NAVMESH_NEW_NAVMATH_H_

#include <cmath>
#include <cstring>

namespace NavMesh {

// D3DXVECTOR3 layout (12 bytes)
struct SNavVec3 {
	float x;
	float y;
	float z;
};

// Two floats (x, z) - vertex direction entries, crossing points (8 bytes)
struct SNavVec2 {
	float x;
	float z;
};

// Line segment in the XZ plane (16 bytes): start (x0, z0), end (x1, z1)
struct SNavLine2 {
	float x0;
	float z0;
	float x1;
	float z1;
};

// D3DXPLANE layout (16 bytes)
struct SNavPlane {
	float a;
	float b;
	float c;
	float d;
};

// D3DXMATRIX layout (64 bytes), row-major: m[12..14] is the translation
struct SNavMatrix {
	float m[16];
};

/**
 * [RECONSTRUCTED - 0x00997330] (80 bytes, eax = out, edx = in, ecx = matrix)
 */
inline SNavVec3* Vec3_TransformCoord(SNavVec3* pOut, const SNavVec3* pIn, const SNavMatrix* pMat) {
	const float* m = pMat->m;
	const double x = pIn->x;
	const double y = pIn->y;
	const double z = pIn->z;
	pOut->x = static_cast<float>(m[4] * y + x * m[0] + m[8] * z + m[12]);
	pOut->y = static_cast<float>(m[1] * x + m[5] * y + m[9] * z + m[13]);
	pOut->z = static_cast<float>(m[2] * x + m[6] * y + m[10] * z + m[14]);
	return pOut;
}

/**
 * [RECONSTRUCTED - 0x00997580] (33 bytes, eax = vector)
 */
inline float Vec3_LengthSq(const SNavVec3* pVec) {
	const double x = pVec->x;
	const double y = pVec->y;
	const double z = pVec->z;
	return static_cast<float>(y * y + x * x + z * z);
}

/**
 * [RECONSTRUCTED - 0x004328C0] (105 bytes, esi = vector)
 * A zero-length vector is scaled by 0 (0x004328FD).
 */
inline void Vec3_Normalize(SNavVec3* pVec) {
	const double x = pVec->x;
	const double y = pVec->y;
	const double z = pVec->z;
	const float fLengthSq = static_cast<float>(y * y + x * x + z * z);
	const float fLength = static_cast<float>(std::sqrt(static_cast<double>(fLengthSq))); // 0x00405250
	const float fInv = (fLength > 0.0f) ? static_cast<float>(1.0 / fLength) : 0.0f;
	pVec->x = static_cast<float>(static_cast<double>(pVec->x) * fInv);
	pVec->y = static_cast<float>(static_cast<double>(fInv) * pVec->y);
	pVec->z = static_cast<float>(static_cast<double>(fInv) * pVec->z);
}

/**
 * [RECONSTRUCTED - 0x009A77A0] (100 bytes, esi = two floats)
 */
inline void Vec2_Normalize(SNavVec2* pVec) {
	const double x = pVec->x;
	const double z = pVec->z;
	const float fLengthSq = static_cast<float>(x * x + z * z);
	const float fLength = static_cast<float>(std::sqrt(static_cast<double>(fLengthSq)));
	const float fInv = (fLength > 0.0f) ? static_cast<float>(1.0 / fLength) : 0.0f;
	pVec->x = static_cast<float>(static_cast<double>(pVec->x) * fInv);
	pVec->z = static_cast<float>(static_cast<double>(fInv) * pVec->z);
}

/**
 * [RECONSTRUCTED - 0x0099ED10] (92 bytes, esi = out matrix, float yaw on the stack)
 * m[0] = cos, m[2] = sin, m[8] = -sin, m[10] = cos (sin 0x009FBF80, cos 0x009FC0B0).
 */
inline SNavMatrix* Matrix_RotationY(SNavMatrix* pOut, float fYaw) {
	const float fSin = static_cast<float>(std::sin(static_cast<double>(fYaw)));
	const float fCos = static_cast<float>(std::cos(static_cast<double>(fYaw)));
	std::memset(pOut->m, 0, sizeof(pOut->m));
	pOut->m[15] = 1.0f;
	pOut->m[5] = 1.0f;
	pOut->m[0] = fCos;
	pOut->m[2] = fSin;
	pOut->m[8] = -fSin;
	pOut->m[10] = fCos;
	return pOut;
}

/**
 * [RECONSTRUCTED - 0x0099EAD0] (572 bytes, edi = in, esi = out; returns out)
 * Only affine matrices (m[3], m[7], m[11] within 0.001 of 0 and m[15] within 0.001 of 1) are inverted;
 * anything else yields the identity.
 */
inline SNavMatrix* Matrix_InverseAffine(const SNavMatrix* pIn, SNavMatrix* pOut) {
	const float* a = pIn->m;
	float* o = pOut->m;
	std::memset(o, 0, sizeof(pOut->m));
	o[15] = 1.0f;
	o[10] = 1.0f;
	o[5] = 1.0f;
	o[0] = 1.0f;

	const float kEpsilon = 0.001f;
	if (std::fabs(a[15] - 1.0f) > kEpsilon || std::fabs(a[3]) > kEpsilon || std::fabs(a[7]) > kEpsilon ||
		std::fabs(a[11]) > kEpsilon) {
		return pOut;
	}

	const float c0 = a[10] * a[5] - a[9] * a[6];
	const float c1 = a[10] * a[4] - a[8] * a[6];
	const float c2 = a[9] * a[4] - a[8] * a[5];
	const float fInvDet = 1.0f / (a[0] * c0 - a[1] * c1 + a[2] * c2);
	const float fNegInvDet = -fInvDet;

	o[0] = c0 * fInvDet;
	o[1] = (a[10] * a[1] - a[9] * a[2]) * fNegInvDet;
	o[2] = (a[6] * a[1] - a[5] * a[2]) * fInvDet;
	o[3] = 0.0f;
	o[4] = c1 * fNegInvDet;
	o[5] = (a[10] * a[0] - a[8] * a[2]) * fInvDet;
	o[6] = (a[6] * a[0] - a[4] * a[2]) * fNegInvDet;
	o[7] = 0.0f;
	o[8] = c2 * fInvDet;
	o[9] = fNegInvDet * (a[9] * a[0] - a[8] * a[1]);
	o[10] = fInvDet * (a[5] * a[0] - a[4] * a[1]);
	o[11] = 0.0f;
	o[12] = -(o[0] * a[12] + o[4] * a[13] + a[14] * o[8]);
	o[13] = -(o[9] * a[14] + o[5] * a[13] + a[12] * o[1]);
	o[14] = -(o[10] * a[14] + o[6] * a[13] + a[12] * o[2]);
	o[15] = 1.0f;
	return pOut;
}

/**
 * [RECONSTRUCTED - 0x00537E10] (191 bytes, edi = p0, eax = p1, ecx = p2, ebx = out plane)
 * n = normalize((p1 - p0) x (p2 - p0)), d = -dot(n, p0)
 */
inline void Plane_FromPoints(const SNavVec3* p0, const SNavVec3* p1, const SNavVec3* p2, SNavPlane* pOut) {
	const float ax = static_cast<float>(static_cast<double>(p1->x) - p0->x);
	const float ay = static_cast<float>(static_cast<double>(p1->y) - p0->y);
	const float az = static_cast<float>(static_cast<double>(p1->z) - p0->z);
	const float bx = static_cast<float>(static_cast<double>(p2->x) - p0->x);
	const float by = static_cast<float>(static_cast<double>(p2->y) - p0->y);
	const float bz = static_cast<float>(static_cast<double>(p2->z) - p0->z);

	SNavVec3 n;
	n.x = static_cast<float>(static_cast<double>(az) * by - static_cast<double>(bz) * ay);
	n.y = static_cast<float>(static_cast<double>(bz) * ax - static_cast<double>(az) * bx);
	n.z = static_cast<float>(static_cast<double>(ay) * bx - static_cast<double>(ax) * by);
	Vec3_Normalize(&n);

	pOut->a = n.x;
	pOut->b = n.y;
	pOut->c = n.z;
	pOut->d = static_cast<float>(-(static_cast<double>(n.x) * p0->x + static_cast<double>(n.y) * p0->y + static_cast<double>(n.z) * p0->z));
}

/**
 * [RECONSTRUCTED - 0x009BFCC0] (379 bytes, eax = line A, ecx = line B, edx = optional intersection out)
 * Returns 0 when the lines are collinear, 5 when parallel, otherwise tA / tB are the parameters on A / B:
 *   2 both in [0, 1], 3 only tA in [0, 1], 4 only tB in [0, 1], 1 neither.
 * The intersection is written from line B (0x009BFD93).
 */
inline int Line2_Intersect(const SNavLine2* pA, const SNavLine2* pB, SNavVec2* pOut) {
	const float fDiffZ = static_cast<float>(static_cast<double>(pB->z0) - pA->z0);
	const float fADX = static_cast<float>(static_cast<double>(pA->x1) - pA->x0);
	const float fDiffX = static_cast<float>(static_cast<double>(pB->x0) - pA->x0);
	const float fADZ = static_cast<float>(static_cast<double>(pA->z1) - pA->z0);
	const float fBDX = static_cast<float>(static_cast<double>(pB->x1) - pB->x0);
	const float fBDZ = static_cast<float>(static_cast<double>(pB->z1) - pB->z0);

	const float fNumB = static_cast<float>(static_cast<double>(fADX) * fDiffZ - static_cast<double>(fADZ) * fDiffX);
	const float fDenom = static_cast<float>(static_cast<double>(fADZ) * fBDX - static_cast<double>(fADX) * fBDZ);
	if (fDenom == 0.0f) {
		return (fNumB != 0.0f) ? 5 : 0;
	}

	const float fTB = static_cast<float>(static_cast<double>(fNumB) / fDenom);
	const float fTA = static_cast<float>((static_cast<double>(fDiffZ) * fBDX - static_cast<double>(fDiffX) * fBDZ) / fDenom);
	if (pOut != nullptr) {
		pOut->x = static_cast<float>(static_cast<double>(fBDX) * fTB + pB->x0);
		pOut->z = static_cast<float>(static_cast<double>(fBDZ) * fTB + pB->z0);
	}

	const bool bInA = (fTA >= 0.0f && fTA <= 1.0f);
	const bool bInB = (fTB >= 0.0f && fTB <= 1.0f);
	if (bInB && bInA) {
		return 2;
	}
	if (bInA) {
		return 3;
	}
	if (bInB) {
		return 4;
	}
	return 1;
}

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_NAVMATH_H_
