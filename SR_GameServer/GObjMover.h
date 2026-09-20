/**
 * ============================================================================
 * Silkroad Online - Movement controllers
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjMover.h
 *
 * RTTI-proven classes (SR_GameServer.exe):
 *   - CGObjMover        vftable 0x00AE87C4, 0x10 bytes; slots 3..10 pure, slots 1 and 2 implemented
 *   - CGObjMoverByCmd   vftable 0x00AE8764, 0x10 bytes - the mover a steering command drives
 *   - CGObjMoverByDest  vftable 0x00AE8794, 0x20 bytes - the mover that walks to a destination
 *
 * CGObjMobile's constructor (0x0048B2E3 - 0x0048B36C) creates both, in this order, and calls slot 1 on
 * each with the owner. Both bind to the SAME block - the character's own tagObjMoveCommand at +0x154 -
 * and the first byte of that block, m_byMoveType, picks which of the two the tick drives
 * (CGObjMobile::OnTick 0x0048B8D3, CGObjChar::IsMoving 0x0048B891).
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJMOVER_H_
#define _SR_GAMESERVER_GOBJMOVER_H_

#include <cstdint>
#include "GlobalPos.h"

class CGObjChar;

// tagObjMoveCommand::m_byMoveType - also the index into CGObjChar::m_apMover (0x0048B891)
enum eObjMoveType {
	OBJ_MOVE_BY_CMD  = 0,    // walk along the owner's facing, steered by GO_/TURN_ flags
	OBJ_MOVE_BY_DEST = 1,    // walk to a destination
	OBJ_MOVE_NONE    = 0xFF, // idle; CGObjMobile::OnTick drives no mover at all (0x0048B8D3)
};

// tagObjMoveCommand::m_byFlags, as CGObjMoverByCmd reads them (0x0048C0FB, 0x0048C11A, 0x0048C1F8)
enum eObjMoveFlag {
	OBJ_MOVE_FLAG_GO_FORWARD  = 0x01,
	OBJ_MOVE_FLAG_GO_BACKWARD = 0x02,
	OBJ_MOVE_FLAG_TURN_LEFT   = 0x10,
	OBJ_MOVE_FLAG_TURN_RIGHT  = 0x20,
};

/**
 * [RECONSTRUCTED - 0x0048BFC7 / 0x0048C140 / 0x0048C330] (Size: 0x18)
 * tagObjMoveCommand
 * The movement state a character carries at +0x154 and the shape of the command both movers are given:
 * CGObjMoverByCmd::SetCommand and CGObjMoverByDest::SetCommand copy one into the other field by field.
 */
struct tagObjMoveCommand {
	uint8_t  m_byMoveType;   // +0x00: eObjMoveType
	uint8_t  m_bySpeedMode;  // +0x01: 0 walk, 1 run
	uint16_t m_wRegionID;    // +0x02: the destination's region
	int32_t  m_nDestX;       // +0x04: integer world coordinates (the native fild's them, 0x0048C362)
	int32_t  m_nDestY;       // +0x08
	int32_t  m_nDestZ;       // +0x0C
	float    m_fAngle;       // +0x10: the direction to walk in when there is no destination
	uint8_t  m_byFlags;      // +0x14: eObjMoveFlag
	uint8_t  m_pad15[3];     // +0x15 - +0x17
};

/**
 * [RECONSTRUCTED - 0x0048BF70, vftable 0x00AE87C4] (Size: 0x10)
 * CGObjMover
 */
class CGObjMover {
public:
	// Native 0x0048BF70: vptr and three zeroed dwords
	CGObjMover();

	// vftable[0] @ 0x0048BFB0 (0x0048BF90 is the scalar deleting thunk)
	virtual ~CGObjMover();

	// vftable[1] @ 0x0048BFC0: binds the owner and its movement block (+0x154)
	virtual int32_t Init(CGObjChar* pOwner);

	// vftable[2] @ 0x0048BF60: re-applies the state to the owner after CGObjMobile::ReadState loaded one
	virtual void Restore();

	// vftable[3]: the step for this tick; returns 1 when pOutStep was filled
	virtual int32_t Update(float fDeltaSec, SRO_Vector3D* pOutStep) = 0;

	// vftable[4]: takes a new movement command
	virtual int32_t SetCommand(const tagObjMoveCommand* pCommand) = 0;

	// vftable[5]: turns the owner without moving it
	virtual void SetAngle(float fAngle) = 0;

	// vftable[6]: ends the movement where the owner currently stands
	virtual void Cancel(float fAngle) = 0;

	// vftable[7]: the angle the mover is steering along
	virtual float GetAngle() const = 0;

	// vftable[8]: whether the owner still has to turn
	virtual int32_t IsTurning() const = 0;

	// vftable[9]: whether the destination has been reached
	virtual int32_t IsArrived() = 0;

	// vftable[10]: advances the turn for this tick
	virtual void TickTurn(float fDeltaSec) = 0;

	// [RECONSTRUCTED - 0x0048BFE0] writes the mode back into the block the mover is bound to
	void SetSpeedMode(uint8_t bySpeedMode);

	// [RECONSTRUCTED - 0x0048BFF0] the displacement the owner's speed and facing give in fDeltaSec
	void ComputeStep(SRO_Vector3D* pOutStep, float fDeltaSec) const;

public:
	CGObjChar*         m_pOwner;  // +0x04 (0x0048BFC4)
	tagObjMoveCommand* m_pState;  // +0x08: the owner's own block at +0x154 (0x0048BFCC)
	int32_t            m_bMoving; // +0x0C: read by CGObjChar::IsMoving (0x0048B8B5)
};

/**
 * [RECONSTRUCTED - 0x0048B0B0, vftable 0x00AE8764] (Size: 0x10)
 * CGObjMoverByCmd
 * The mover a steering command drives: it walks along the owner's facing and turns it while a turn flag
 * is set. It never arrives anywhere by itself.
 */
class CGObjMoverByCmd : public CGObjMover {
public:
	CGObjMoverByCmd();
	virtual ~CGObjMoverByCmd() override;

	virtual void    Restore() override;                                       // 0x0048C0B0
	virtual int32_t Update(float fDeltaSec, SRO_Vector3D* pOutStep) override;  // 0x0048C1B0
	virtual int32_t SetCommand(const tagObjMoveCommand* pCommand) override;    // 0x0048C140
	virtual void    SetAngle(float fAngle) override;                          // 0x0048C0E0
	virtual void    Cancel(float fAngle) override;                            // 0x0048C110
	virtual float   GetAngle() const override;                                // 0x0048B160
	virtual int32_t IsTurning() const override;                               // 0x0048B170
	virtual int32_t IsArrived() override;                                     // 0x00559C70, returns 0
	virtual void    TickTurn(float fDeltaSec) override;                       // 0x0048C1E0
};

/**
 * [RECONSTRUCTED - 0x0048B1A0, vftable 0x00AE8794] (Size: 0x20)
 * CGObjMoverByDest
 * The mover that walks to a destination: it faces it, steps along that facing and stops on arrival.
 */
class CGObjMoverByDest : public CGObjMover {
public:
	CGObjMoverByDest();
	virtual ~CGObjMoverByDest() override;

	virtual void    Restore() override;                                       // 0x0048C2B0
	virtual int32_t Update(float fDeltaSec, SRO_Vector3D* pOutStep) override;  // 0x0048C4E0
	virtual int32_t SetCommand(const tagObjMoveCommand* pCommand) override;    // 0x0048C330
	virtual void    SetAngle(float fAngle) override;                          // 0x009BF500, no operation
	virtual void    Cancel(float fAngle) override;                            // 0x0048C460
	virtual float   GetAngle() const override;                                // 0x0048C760
	virtual int32_t IsTurning() const override;                               // 0x0048C6C0
	virtual int32_t IsArrived() override;                                     // 0x0048C790
	virtual void    TickTurn(float fDeltaSec) override;                       // 0x0048C660

	// [RECONSTRUCTED - 0x0048C2D0] the destination has to be in the owner's own region or a neighbour
	int32_t IsRegionNear(uint16_t wRegionID) const;

public:
	uint16_t m_wStartRegion; // +0x10 (0x0048C422): where the walk began - IsArrived needs it
	uint16_t m_pad12;        // +0x12
	float    m_fStartX;      // +0x14 (0x0048C42E)
	float    m_fStartY;      // +0x18 (0x0048C434)
	float    m_fStartZ;      // +0x1C (0x0048C43D)
};

#endif // _SR_GAMESERVER_GOBJMOVER_H_
