/**
 * ============================================================================
 * Silkroad Online - Interactive NPC Entity
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjNPCNPC.h
 *
 * Implements:
 *   - CGObjNPCNPC @ 0x00AEF450
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJNPCNPC_H_
#define _SR_GAMESERVER_GOBJNPCNPC_H_

#include "GObjChar.h"

class CGObjPC;

class CGObjNPCNPC : public CGObjChar {
public:
	CGObjNPCNPC();
	virtual ~CGObjNPCNPC() override;

	// [RECONSTRUCTED - Native 0x00AEF450]
	// Interactive dialog and shop/teleport options
	bool OnPlayerInteract(CGObjPC* pPlayer);

	uint32_t GetShopID() const;
	uint32_t GetTeleportID() const;

protected:
	uint32_t m_dwShopID;
	uint32_t m_dwTeleportID;
};

#endif // _SR_GAMESERVER_GOBJNPCNPC_H_
