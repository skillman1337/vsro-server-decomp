/**
 * ============================================================================
 * Silkroad Online - Pet Entity (Attack & Grab Pets)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjCOS_Pet.h
 *
 * Implements:
 *   - CGObjCOS_Pet @ 0x00AF19D8
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJCOS_PET_H_
#define _SR_GAMESERVER_GOBJCOS_PET_H_

#include "GObjCOS.h"

class CGObjCOS_Pet : public CGObjCOS {
public:
	CGObjCOS_Pet();
	virtual ~CGObjCOS_Pet() override;

	// [RECONSTRUCTED - Native 0x00AF19D8]
	// Pet inventory and auto-pickup simulation
	bool PickupGroundItem(uint32_t dwItemEntityID);

	uint32_t GetHunger() const;
	void FeedPet(uint32_t dwFoodAmount);

private:
	uint32_t m_dwHunger;
	uint32_t m_dwMaxHunger;
};

#endif // _SR_GAMESERVER_GOBJCOS_PET_H_
