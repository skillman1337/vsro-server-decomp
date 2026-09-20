/**
 * ============================================================================
 * Silkroad Online - NPC Shop Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ShopApp.h
 *
 * Implements:
 *   - CShopApp @ 0x00867660
 * ============================================================================
 */

#ifndef _SERVERCOMMON_SHOPAPP_H_
#define _SERVERCOMMON_SHOPAPP_H_

#include <cstdint>

class CGObjPC;

class CShopApp {
public:
	CShopApp();
	virtual ~CShopApp();

	// [RECONSTRUCTED - Native 0x00867660]
	// Processes item purchase transaction from NPC shop
	bool ProcessBuyItem(CGObjPC* pPlayer, uint32_t dwShopID, uint32_t dwTabID, uint32_t dwSlot, uint32_t dwCount);

	// Processes item sale transaction to NPC shop
	bool ProcessSellItem(CGObjPC* pPlayer, uint32_t dwInventorySlot, uint32_t dwCount);
};

#endif // _SERVERCOMMON_SHOPAPP_H_
