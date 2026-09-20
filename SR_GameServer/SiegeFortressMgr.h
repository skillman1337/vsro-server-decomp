/**
 * ============================================================================
 * Silkroad Online - Fortress War Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SiegeFortressMgr.h
 *
 * Implements:
 *   - CSiegeFortressMgr @ 0x00B04440
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SIEGEFORTRESSMGR_H_
#define _SR_GAMESERVER_SIEGEFORTRESSMGR_H_

#include <cstdint>
#include <map>

struct tagFortressData {
	uint32_t dwFortressID;
	uint32_t dwOwnerGuildID;
	uint32_t dwTaxRate;
	uint64_t qwAccumulatedTax;
	uint8_t  byState; // 0: Normal, 1: Request, 2: War, 3: Settlement
};

class CSiegeFortressMgr {
public:
	CSiegeFortressMgr();
	virtual ~CSiegeFortressMgr();

	// [RECONSTRUCTED - Native 0x00B04440]
	// Fortress war battle state updates and tax calculation
	void Tick(uint32_t dwElapsedMs);

	bool StartSiegeWar(uint32_t dwFortressID);
	bool EndSiegeWar(uint32_t dwFortressID, uint32_t dwVictorGuildID);

	static CSiegeFortressMgr* GetInstance();

private:
	std::map<uint32_t, tagFortressData> m_mapFortresses;
};

#endif // _SR_GAMESERVER_SIEGEFORTRESSMGR_H_
