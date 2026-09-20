/**
 * ============================================================================
 * Silkroad Online - Game Server Message Block
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\MsgBlock.h
 *
 * RTTI-proven classes (SR_GameServer.exe):
 *   - CMsgBlock         .?AVCMsgBlock@@         vftable 0x00AF7F90 (7 slots, [4] pure), ctor 0x00533520
 *   - CMsgBlockObject   .?AVCMsgBlockObject@@   vftable 0x00AF7FB8
 *   - CMsgBlockTerrain  .?AVCMsgBlockTerrain@@  vftable 0x00AF7FD8, sizeof 0x98; 36 of them (a 6 x 6 grid
 *     over the region's 1920 units, 320.0 per block at 0x00B45AE8) make up one CRgnTerrain.
 *
 * A message block is the unit of visibility: every packet an object broadcasts goes to the players inside
 * its own block and inside that block's neighbours (CMsgBlock::SendPacketToLayer 0x00534150).
 *
 * The per-layer records have no RTTI; tagMsgBlockLayer and tagMsgBlockLayerTable are reconstruction names.
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_MSGBLOCK_H_
#define _SR_GAMESERVER_MSGBLOCK_H_

#include <cstdint>
#include <map>
#include <vector>
#include "GameWorld.h"
#include "GlobalPos.h"

namespace BSLib {
class CPacket;
}
typedef BSLib::CPacket CPacket;

class CGObj;
class CRegion;

/**
 * [RECONSTRUCTED - 0x005340E0 / 0x00534340 / 0x005342A0] (Size: 0x1C)
 * tagMsgBlockLayer
 * The objects of one game world layer inside one message block. Players and everything else are kept apart
 * because only the players are ever sent anything (0x005340FC picks the map by IsPlayer).
 */
struct tagMsgBlockLayer {
	// [RECONSTRUCTED - 0x005342A0] hands every player of this layer to the packet's recipient list
	void AppendRecipients(CPacket* pPacket);

	uint16_t wPCCount = 0;                        // +0x00: raised by OnObjectEnter for a player (0x0053448C)
	uint16_t pad02 = 0;                           // +0x02
	std::map<uint32_t, CGObj*> m_mapPC;       // +0x04 - +0x0F: players, keyed by game id (0x0053410A)
	std::map<uint32_t, CGObj*> m_mapNonPC;    // +0x10 - +0x1B: everything else (0x0053412C)
};

/**
 * [RECONSTRUCTED - 0x00533260 / 0x00533340] (Size: 0x30)
 * tagMsgBlockLayerTable
 * Layer 0 is stored inline; layers 1..n are in the vector.
 */
struct tagMsgBlockLayerTable {
	// Native 0x00533340 (125 bytes, ebx = this, ax = wLayerID)
	tagMsgBlockLayer* GetLayer(uint16_t wLayerID);

	std::vector<tagMsgBlockLayer> m_vecLayer; // +0x00 - +0x13: layers 1..n
	tagMsgBlockLayer              m_Layer0;   // +0x14 - +0x2F
};

/**
 * [PARTIAL - 0x00533520, vftable 0x00AF7F90] (Size: 0x58)
 * CMsgBlock
 * Not ported: the object set at +0x1C and the whole enter-view protocol the native runs as an object
 * arrives (0x3015 spawn, 0x3017 / 0x3018 / 0x3019 peer lists) - see CMsgBlock::OnObjectEnter.
 */
class CMsgBlock {
public:
	// Native 0x00533520 (126 bytes, this on the stack)
	CMsgBlock();

	// vftable[0]: scalar deleting destructor 0x005335A0, body 0x00533610
	virtual ~CMsgBlock();

	// vftable[2] @ 0x00534340: an object arrives in this block
	virtual void OnObjectEnter(CGObj* pObj, CMsgBlock* pOldBlock, int32_t nMode);

	// vftable[3] @ 0x005344B0: an object leaves for pNewBlock
	virtual void OnObjectLeave(CGObj* pObj, CMsgBlock* pNewBlock);

	// vftable[4]: whether a position still falls inside this block. Pure in the base; the native takes the
	// location by value (retn 0x18), which is how CGObj::StepMovement hands it over (0x00485998).
	virtual int32_t IsInside(tagObjLocation pos) const = 0;

	// vftable[6] @ 0x00534150: collects the recipients of one layer from this block and its neighbours
	virtual void SendPacketToLayer(uint16_t wLayerID, CPacket* pPacket);

	// [RECONSTRUCTED - 0x00533900] a block of a proxy region never receives broadcasts
	int32_t IsBroadcastEnabled() const;

	// [RECONSTRUCTED - 0x005341E0] wLayerID 0xFFFF means every layer of the table
	void AppendLayerRecipients(uint16_t wLayerID, CPacket* pPacket);

	// Layer-map bookkeeping. 0x00534050 is a despawn broadcaster, NOT this erase.
	void AddObjectToLayer(CGObj* pObj);
	void RemoveObjectFromLayer(CGObj* pObj);

	// Native 0x005347C0 (20 bytes, ecx = this, eax = pContext)
	uint16_t GetPCCount(const tagRegionContext* pContext);

public:
	CRegion*                m_pRegion;       // +0x04: the region this block belongs to (0x00533901)
	// +0x08 / +0x0A: where this block sits in its region's grid. They live in the base even though only
	// CMsgBlockTerrain reads them (0x005376D6 / 0x005376E7); the base constructor clears both as one dword.
	uint16_t                m_wBlockZ;       // +0x08
	uint16_t                m_wBlockX;       // +0x0A
	std::vector<CMsgBlock*> m_vecNeighbour;  // +0x0C - +0x1B: first +0x10, last +0x14 (0x0053416D)
	uint8_t                 pad1C[0x0C];     // +0x1C - +0x27: the object set (0x00534A90)
	tagMsgBlockLayerTable   m_LayerTable;    // +0x28 (constructor 0x00533260)
};

/**
 * [PARTIAL - vftable 0x00AF7FD8] (Size: 0x98)
 * CMsgBlockTerrain
 * One cell of a region's 6 x 6 grid.
 */
class CMsgBlockTerrain : public CMsgBlock {
public:
	CMsgBlockTerrain();
	virtual ~CMsgBlockTerrain() override;

	// vftable[4] @ 0x005376C0
	virtual int32_t IsInside(tagObjLocation pos) const override;
};

// 0x00B45AE8: a region is 1920 units across and a terrain block covers 320 of them
const double kMsgBlockSize = 320.0;

#endif // _SR_GAMESERVER_MSGBLOCK_H_
