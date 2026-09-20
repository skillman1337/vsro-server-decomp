/**
 * ============================================================================
 * Silkroad Online - Monster Entity
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjMob.h
 *
 * Implements:
 *   - CGObjMob @ 0x00AEDD60
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJMOB_H_
#define _SR_GAMESERVER_GOBJMOB_H_

#include "GObjChar.h"

class CGObjMob : public CGObjChar {
public:
	CGObjMob();
	virtual ~CGObjMob() override;

	// [RECONSTRUCTED - Native 0x00AEDD60]
	// Monster aggro table and drop dispatch on death
	virtual void OnDeath(CGObjChar* pKiller);

	uint8_t GetMobRank() const;
	void SetMobRank(uint8_t byRank);

protected:
	uint8_t  m_byMobRank; // 0: Normal, 1: Champion, 2: Unique, 3: Giant, 4: Titan, 5: Elite
	uint32_t m_dwSpawnHiveID;
};

#endif // _SR_GAMESERVER_GOBJMOB_H_
