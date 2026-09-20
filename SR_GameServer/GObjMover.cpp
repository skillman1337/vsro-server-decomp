/**
 * ============================================================================
 * Silkroad Online - Movement controllers
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjMover.cpp
 *
 * Implements:
 *   - CGObjMover        0x0048BF70 / 0x0048BFC0 / 0x0048BF60 / 0x0048BFE0 / 0x0048BFF0
 *   - CGObjMoverByCmd   0x0048B0B0 and slots 2 - 10
 *   - CGObjMoverByDest  0x0048B1A0 and slots 2 - 10
 * ============================================================================
 */

#include "GObjMover.h"
#include "GObjChar.h"
#include "../JMX_Library/BSLib/BSLog.h"

#include <cmath>

namespace {

// 0x00B45EA8: a single step is never longer than this
const double kMaxStepLength = 160.0;

// 0x00B45DF8: components under this snap to zero so a standing object does not drift
const double kStepEpsilon = 0.009999999776482582;

// 0x00B45C20 / 0x00B45C18: the turn rate per second and one full turn, both stored as doubles
const double kTurnRatePerSec = 3.1415927410125732;
const double kTwoPi          = 6.2831854820251465;

// 0x00B45C38: radians to degrees, as a double
const double kRadToDeg = 57.295780181884766;

// 0x00B45B28: both the smallest destination change worth a new command and the turn threshold
const double kMoveEpsilon = 5.0;

// 0x00B45B68: how close to the destination counts as arrived
const double kArriveRadius = 0.5;

} // namespace

/*
================
CGObjMover::CGObjMover
[RECONSTRUCTED - 0x0048BF70] (18 bytes)
================
*/
CGObjMover::CGObjMover()
	: m_pOwner(nullptr)
	, m_pState(nullptr)
	, m_bMoving(0) {
}

/*
================
CGObjMover::~CGObjMover
[RECONSTRUCTED - 0x0048BFB0] (7 bytes: the vptr write only)
================
*/
CGObjMover::~CGObjMover() {
}

/*
================
CGObjMover::Init
[RECONSTRUCTED - 0x0048BFC0] (23 bytes)
Slot 1. The mover never owns its state: it edits the character's own block in place.
================
*/
int32_t CGObjMover::Init(CGObjChar* pOwner) {
	m_pOwner = pOwner;               // 0x0048BFC4
	m_pState = &pOwner->m_MoveState;  // 0x0048BFC7: owner + 0x154
	return 1;
}

/*
================
CGObjMover::Restore
[RECONSTRUCTED - 0x0048BF60] (13 bytes)
Slot 2. Re-applies the speed mode the state already carries; CGObjMobile::ReadState (0x0048B61E) calls
this right after it has read a movement block off the wire.
================
*/
void CGObjMover::Restore() {
	SetSpeedMode(m_pState->m_bySpeedMode);
}

/*
================
CGObjMover::SetSpeedMode
[RECONSTRUCTED - 0x0048BFE0] (7 bytes)
================
*/
void CGObjMover::SetSpeedMode(uint8_t bySpeedMode) {
	m_pState->m_bySpeedMode = bySpeedMode;
}

/*
================
CGObjMover::ComputeStep
[RECONSTRUCTED - 0x0048BFF0] (183 bytes)
The displacement the owner covers along its facing in fDeltaSec, clamped and snapped near zero.
================
*/
void CGObjMover::ComputeStep(SRO_Vector3D* pOutStep, float fDeltaSec) const {
	double fLength = std::fabs(static_cast<double>(m_pOwner->m_fMoveSpeed) * fDeltaSec); // 0x0048BFFB
	if (fLength > kMaxStepLength) {
		fLength = kMaxStepLength; // 0x0048C02E
	}

	pOutStep->x = static_cast<float>(m_pOwner->m_fDirX * fLength); // 0x0048C044
	pOutStep->y = 0.0f;                                            // 0x0048C048
	pOutStep->z = static_cast<float>(fLength * m_pOwner->m_fDirZ); // 0x0048C055

	if (std::fabs(pOutStep->x) < kStepEpsilon) {
		pOutStep->x = 0.0f; // 0x0048C077
	}
	if (std::fabs(pOutStep->z) < kStepEpsilon) {
		pOutStep->z = 0.0f; // 0x0048C08F
	}

	ASSERT(m_bMoving == 1); // 0x0048C096
}

// ============================================================================
// CGObjMoverByCmd
// ============================================================================

/*
================
CGObjMoverByCmd::CGObjMoverByCmd
[RECONSTRUCTED - 0x0048B0B0] (81 bytes)
================
*/
CGObjMoverByCmd::CGObjMoverByCmd() {
}

CGObjMoverByCmd::~CGObjMoverByCmd() {
}

/*
================
CGObjMoverByCmd::Restore
[RECONSTRUCTED - 0x0048C0B0] (37 bytes)
Slot 2. Beyond the speed mode it also puts the owner back on the heading the state carries.
================
*/
void CGObjMoverByCmd::Restore() {
	SetSpeedMode(m_pState->m_bySpeedMode);  // 0x0048C0B8
	m_pOwner->SetAngle(m_pState->m_fAngle); // 0x0048C0D2: owner slot 230
}

/*
================
CGObjMoverByCmd::Update
[RECONSTRUCTED - 0x0048C1B0] (38 bytes)
Slot 3. A steered object only moves while its command says so.
================
*/
int32_t CGObjMoverByCmd::Update(float fDeltaSec, SRO_Vector3D* pOutStep) {
	if (m_bMoving != 1) {
		return 0; // 0x0048C1D1
	}
	ComputeStep(pOutStep, fDeltaSec); // 0x0048C1C4
	return 1;
}

/*
================
CGObjMoverByCmd::SetCommand
[RECONSTRUCTED - 0x0048C140] (106 bytes)
Slot 4. A command that carries a destination brings the region and the coordinates, one that does not
brings the heading and the steering flags; either way the owner is turned and a GO flag starts the walk.
================
*/
int32_t CGObjMoverByCmd::SetCommand(const tagObjMoveCommand* pCommand) {
	m_pState->m_byMoveType = pCommand->m_byMoveType; // 0x0048C14C

	if (pCommand->m_byMoveType == OBJ_MOVE_BY_CMD) {
		m_pState->m_fAngle = pCommand->m_fAngle;   // 0x0048C156
		m_pState->m_byFlags = pCommand->m_byFlags; // 0x0048C15C
	} else {
		m_pState->m_wRegionID = pCommand->m_wRegionID; // 0x0048C168
		m_pState->m_nDestX = pCommand->m_nDestX;       // 0x0048C171
		m_pState->m_nDestY = pCommand->m_nDestY;       // 0x0048C176
		m_pState->m_nDestZ = pCommand->m_nDestZ;       // 0x0048C17C
	}

	m_pOwner->SetAngle(m_pState->m_fAngle); // 0x0048C194: owner slot 230
	if ((m_pState->m_byFlags & OBJ_MOVE_FLAG_GO_FORWARD) != 0) {
		m_bMoving = 1; // 0x0048C1A3
	}
	return 1;
}

/*
================
CGObjMoverByCmd::SetAngle
[RECONSTRUCTED - 0x0048C0E0] (35 bytes)
Slot 5. Turning by hand ends whatever turn the flags had started.
================
*/
void CGObjMoverByCmd::SetAngle(float fAngle) {
	m_pOwner->SetAngle(fAngle);  // 0x0048C0F6: owner slot 230
	m_pState->m_byFlags &= 0x0F; // 0x0048C0FB
}

/*
================
CGObjMoverByCmd::Cancel
[RECONSTRUCTED - 0x0048C110] (42 bytes)
Slot 6. Clears the GO flags, leaves the TURN flags alone and stops the walk.
================
*/
void CGObjMoverByCmd::Cancel(float fAngle) {
	m_pState->m_byFlags &= 0xF0; // 0x0048C11A
	m_pOwner->SetAngle(fAngle);  // 0x0048C12D: owner slot 230
	m_bMoving = 0;               // 0x0048C12F
}

/*
================
CGObjMoverByCmd::GetAngle
[RECONSTRUCTED - 0x0048B160] (7 bytes)
Slot 7.
================
*/
float CGObjMoverByCmd::GetAngle() const {
	return m_pState->m_fAngle;
}

/*
================
CGObjMoverByCmd::IsTurning
[RECONSTRUCTED - 0x0048B170] (15 bytes)
Slot 8.
================
*/
int32_t CGObjMoverByCmd::IsTurning() const {
	return (m_pState->m_byFlags & 0xF0) != 0 ? 1 : 0;
}

/*
================
CGObjMoverByCmd::IsArrived
[RECONSTRUCTED - 0x00559C70] (3 bytes: 33 C0 C3, a folded COMDAT)
Slot 9. A steered object never arrives anywhere by itself.
================
*/
int32_t CGObjMoverByCmd::IsArrived() {
	return 0;
}

/*
================
CGObjMoverByCmd::TickTurn
[RECONSTRUCTED - 0x0048C1E0] (205 bytes)
Slot 10. TURN_LEFT spins the heading forward at pi per second and wraps it down below 2 pi, TURN_RIGHT
spins it back and wraps it up above zero; the owner is then put on the new heading.

Nothing on the server reaches this slot: every site that dispatches through m_apMover (+0x14C) uses slots
2 - 7 only (CGObjMobile::OnTick, ReadState, StopMove, CGObjChar::SetMoveCommand / SetMoveAngle /
SetSpeedMode / IsMoving / GetMoveAngle), and CGObjMoverByCmd::Update never calls it the way the
destination mover's does. The turn a steered character makes arrives with the movement packet instead.
================
*/
void CGObjMoverByCmd::TickTurn(float fDeltaSec) {
	if (IsTurning() == 0) {
		return; // 0x0048C1EC
	}

	if ((m_pState->m_byFlags & OBJ_MOVE_FLAG_TURN_LEFT) != 0) {
		// 0x0048C1FC: evaluated on the x87 stack, only the result is stored back as a float
		m_pState->m_fAngle = static_cast<float>(
			static_cast<double>(fDeltaSec) * kTurnRatePerSec + m_pState->m_fAngle);
		while (m_pState->m_fAngle >= kTwoPi) {
			m_pState->m_fAngle = static_cast<float>(m_pState->m_fAngle - kTwoPi); // 0x0048C22E
		}
	} else if ((m_pState->m_byFlags & OBJ_MOVE_FLAG_TURN_RIGHT) != 0) {
		// 0x0048C246
		m_pState->m_fAngle = static_cast<float>(
			m_pState->m_fAngle - static_cast<double>(fDeltaSec) * kTurnRatePerSec);
		while (0.0 > m_pState->m_fAngle) {
			m_pState->m_fAngle = static_cast<float>(m_pState->m_fAngle + kTwoPi); // 0x0048C27C
		}
	} else {
		return; // 0x0048C244
	}

	m_pOwner->SetAngle(m_pState->m_fAngle); // 0x0048C2A7: owner slot 230
}

// ============================================================================
// CGObjMoverByDest
// ============================================================================

/*
================
CGObjMoverByDest::CGObjMoverByDest
[RECONSTRUCTED - 0x0048B1A0] (81 bytes)
================
*/
CGObjMoverByDest::CGObjMoverByDest()
	: m_wStartRegion(0)
	, m_pad12(0)
	, m_fStartX(0.0f)
	, m_fStartY(0.0f)
	, m_fStartZ(0.0f) {
}

CGObjMoverByDest::~CGObjMoverByDest() {
}

/*
================
CGObjMoverByDest::Restore
[RECONSTRUCTED - 0x0048C2B0] (29 bytes)
Slot 2. A destination mover takes its heading from the destination, so it simply faces it again.
================
*/
void CGObjMoverByDest::Restore() {
	SetSpeedMode(m_pState->m_bySpeedMode); // 0x0048C2B8
	TickTurn(0.0f);                        // 0x0048C2CA: its own slot 10
}

/*
================
CGObjMoverByDest::Update
[RECONSTRUCTED - 0x0048C4E0] (374 bytes)
Slot 3. Once it has arrived the owner is told so and the walk ends; otherwise the owner keeps facing the
destination, takes the step its speed gives, and that step is shortened so the last one lands exactly there.
================
*/
int32_t CGObjMoverByDest::Update(float fDeltaSec, SRO_Vector3D* pOutStep) {
	if (m_bMoving != 1) {
		return 0; // 0x0048C64D
	}

	if (IsArrived() == 1) {
		m_pOwner->StopMove(0, m_pOwner->GetMoveAngle()); // 0x0048C519: owner slot 300
		return 0;
	}

	TickTurn(fDeltaSec);              // 0x0048C535: its own slot 10
	ComputeStep(pOutStep, fDeltaSec); // 0x0048C547

	// 0x0048C54C - 0x0048C5A9: how much of the way is left, on the plane
	const SRO_Vector3D destPos(static_cast<float>(m_pState->m_nDestX),
		static_cast<float>(m_pState->m_nDestY), static_cast<float>(m_pState->m_nDestZ));
	const SRO_Vector3D ownerPos(m_pOwner->m_fPosX, m_pOwner->m_fPosY, m_pOwner->m_fPosZ);
	SRO_Vector3D remain;
	Pos_RelativePlanar(&remain, m_pOwner->GetRegionID(), &ownerPos, m_pState->m_wRegionID, &destPos);

	// 0x0048C5D1 - 0x0048C616: both sums of squares are rounded to a float before the square root
	const double fStep = std::sqrt(static_cast<double>(static_cast<float>(
		static_cast<double>(pOutStep->y) * pOutStep->y + static_cast<double>(pOutStep->x) * pOutStep->x +
		static_cast<double>(pOutStep->z) * pOutStep->z)));
	const double fLeft = std::sqrt(static_cast<double>(static_cast<float>(
		static_cast<double>(remain.x) * remain.x + static_cast<double>(remain.y) * remain.y +
		static_cast<double>(remain.z) * remain.z)));

	if (fStep > fLeft) {
		*pOutStep = remain; // 0x0048C62B: the last step lands on the destination
	}
	return 1;
}

/*
================
CGObjMoverByDest::SetCommand
[RECONSTRUCTED - 0x0048C330] (298 bytes)
Slot 4. A destination outside the neighbouring regions is refused, and so is one within five units of the
destination the owner is already walking to. Where the walk begins is recorded because IsArrived needs it.
================
*/
int32_t CGObjMoverByDest::SetCommand(const tagObjMoveCommand* pCommand) {
	if (IsRegionNear(pCommand->m_wRegionID) == 0) {
		return 0; // 0x0048C355
	}

	// 0x0048C362 - 0x0048C3AE: how far the new destination sits from the one already in the state
	const float fOldX = static_cast<float>(m_pState->m_nDestX);
	const float fOldZ = static_cast<float>(m_pState->m_nDestZ);
	const float fDX = static_cast<float>(pCommand->m_nDestX) - fOldX;
	const float fDZ = static_cast<float>(pCommand->m_nDestZ) - fOldZ;
	const double fDistance = std::sqrt(static_cast<double>(static_cast<float>(
		static_cast<double>(fDX) * fDX + static_cast<double>(fDZ) * fDZ)));

	if (!(fDistance > kMoveEpsilon) && m_pOwner->IsMoving()) {
		return 0; // 0x0048C3D7: too small a correction while the owner is already walking
	}

	m_pState->m_byMoveType = pCommand->m_byMoveType; // 0x0048C3E2
	if (pCommand->m_byMoveType == OBJ_MOVE_BY_CMD) {
		m_pState->m_fAngle = pCommand->m_fAngle;   // 0x0048C3EC
		m_pState->m_byFlags = pCommand->m_byFlags; // 0x0048C3F2
	} else {
		m_pState->m_wRegionID = pCommand->m_wRegionID; // 0x0048C3FB
		m_pState->m_nDestX = pCommand->m_nDestX;       // 0x0048C402
		m_pState->m_nDestY = pCommand->m_nDestY;       // 0x0048C408
		m_pState->m_nDestZ = pCommand->m_nDestZ;       // 0x0048C40E
	}

	m_wStartRegion = m_pOwner->GetRegionID(); // 0x0048C422
	m_fStartX = m_pOwner->m_fPosX;            // 0x0048C42E
	m_fStartY = m_pOwner->m_fPosY;            // 0x0048C434
	m_fStartZ = m_pOwner->m_fPosZ;            // 0x0048C43D

	TickTurn(0.0f); // 0x0048C448: its own slot 10, face the destination
	m_bMoving = 1;  // 0x0048C450
	return 1;
}

/*
================
CGObjMoverByDest::SetAngle
[RECONSTRUCTED - 0x009BF500] (5 bytes: 33 C0 C2 04 00, a folded COMDAT)
Slot 5. A destination mover takes its heading from the destination, never from a command.
================
*/
void CGObjMoverByDest::SetAngle(float /*fAngle*/) {
}

/*
================
CGObjMoverByDest::Cancel
[RECONSTRUCTED - 0x0048C460] (117 bytes)
Slot 6. The walk ends where the owner stands: its current position becomes the destination.
================
*/
void CGObjMoverByDest::Cancel(float fAngle) {
	m_pState->m_nDestX = static_cast<int32_t>(m_pOwner->m_fPosX); // 0x0048C492 (CRT_ftol)
	m_pState->m_nDestY = static_cast<int32_t>(m_pOwner->m_fPosY); // 0x0048C49D
	m_pState->m_nDestZ = static_cast<int32_t>(m_pOwner->m_fPosZ); // 0x0048C4A9
	m_pOwner->SetAngle(fAngle);                                   // 0x0048C4C4: owner slot 230
	m_bMoving = 0;                                                // 0x0048C4C6
}

/*
================
CGObjMoverByDest::GetAngle
[RECONSTRUCTED - 0x0048C760] (37 bytes)
Slot 7. The heading the owner is actually facing.
================
*/
float CGObjMoverByDest::GetAngle() const {
	return static_cast<float>(std::atan2(static_cast<double>(m_pOwner->m_fDirZ),
		static_cast<double>(m_pOwner->m_fDirX))); // 0x0048C774
}

/*
================
CGObjMoverByDest::IsTurning
[RECONSTRUCTED - 0x0048C6C0] (147 bytes)
Slot 8. Still turning while the owner's facing is more than five degrees off the line to the destination.
================
*/
int32_t CGObjMoverByDest::IsTurning() const {
	if (m_bMoving == 0) {
		return 0; // 0x0048C6D1
	}

	// 0x0048C6D8: its own slot 7, kept at double width across the call (0x0048C70E fstp qword)
	const double fFacing = GetAngle();

	const SRO_Vector3D destPos(static_cast<float>(m_pState->m_nDestX), 0.0f,
		static_cast<float>(m_pState->m_nDestZ));
	const float fWanted = m_pOwner->GetAngleToPosition(m_pState->m_wRegionID, &destPos); // 0x0048C712

	const float fDegrees = std::fabs(static_cast<float>((fFacing - fWanted) * kRadToDeg)); // 0x0048C721
	return (fDegrees > kMoveEpsilon) ? 1 : 0;                                              // 0x0048C733
}

/*
================
CGObjMoverByDest::IsArrived
[RECONSTRUCTED - 0x0048C790] (402 bytes)
Slot 9. Arrived once the owner stands within half a unit of the destination, or once the destination and
the place the walk started from lie on the same side of it - which means the owner has walked past it.
================
*/
int32_t CGObjMoverByDest::IsArrived() {
	const SRO_Vector3D destPos(static_cast<float>(m_pState->m_nDestX),
		static_cast<float>(m_pState->m_nDestY), static_cast<float>(m_pState->m_nDestZ));
	const SRO_Vector3D ownerPos(m_pOwner->m_fPosX, m_pOwner->m_fPosY, m_pOwner->m_fPosZ);

	SRO_Vector3D toDest;
	Pos_RelativePlanar(&toDest, m_pOwner->GetRegionID(), &ownerPos, m_pState->m_wRegionID, &destPos);

	// 0x0048C80C - 0x0048C82C
	const double fLeft = std::sqrt(static_cast<double>(static_cast<float>(
		static_cast<double>(toDest.y) * toDest.y + static_cast<double>(toDest.x) * toDest.x +
		static_cast<double>(toDest.z) * toDest.z)));
	if (!(fLeft > kArriveRadius)) {
		return 1; // 0x0048C83C
	}

	// 0x0048C849 - 0x0048C8E5: the same vector again, and the one back to where the walk started
	const SRO_Vector3D startPos(m_fStartX, m_fStartY, m_fStartZ);
	SRO_Vector3D toStart;
	Pos_RelativePlanar(&toStart, m_pOwner->GetRegionID(), &ownerPos, m_wStartRegion, &startPos);

	// 0x0048C8EA - 0x0048C8FD: the planar dot product of the two
	const float fDot = static_cast<float>(
		static_cast<double>(toDest.z) * toStart.z + static_cast<double>(toDest.x) * toStart.x);
	return (fDot > 0.0f) ? 1 : 0; // 0x0048C907
}

/*
================
CGObjMoverByDest::TickTurn
[RECONSTRUCTED - 0x0048C660] (82 bytes)
Slot 10. Faces the owner straight at the destination; the elapsed time plays no part.
================
*/
void CGObjMoverByDest::TickTurn(float /*fDeltaSec*/) {
	const SRO_Vector3D destPos(static_cast<float>(m_pState->m_nDestX), 0.0f,
		static_cast<float>(m_pState->m_nDestZ));
	const float fAngle = m_pOwner->GetAngleToPosition(m_pState->m_wRegionID, &destPos); // 0x0048C696
	m_pOwner->SetAngle(fAngle);                                                          // 0x0048C6A7
}

/*
================
CGObjMoverByDest::IsRegionNear
[RECONSTRUCTED - 0x0048C2D0] (88 bytes)
The destination has to lie in the owner's own region or in one right next to it, on both axes.
================
*/
int32_t CGObjMoverByDest::IsRegionNear(uint16_t wRegionID) const {
	const uint16_t wOwnRegion = m_pOwner->GetRegionID(); // 0x0048C2D9

	int32_t nDeltaX = static_cast<int32_t>(wRegionID & 0xFF) - static_cast<int32_t>(wOwnRegion & 0xFF);
	if (nDeltaX < 0) {
		nDeltaX = -nDeltaX; // 0x0048C2F9: cdq / xor / sub
	}
	if (nDeltaX >= 2) {
		return 0; // 0x0048C321
	}

	int32_t nDeltaZ = static_cast<int32_t>(wRegionID >> 8) - static_cast<int32_t>(wOwnRegion >> 8);
	if (nDeltaZ < 0) {
		nDeltaZ = -nDeltaZ; // 0x0048C30D
	}
	if (nDeltaZ >= 2) {
		return 0; // 0x0048C321
	}
	return 1; // 0x0048C317
}
