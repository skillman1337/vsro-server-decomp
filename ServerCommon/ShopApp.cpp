/**
 * ============================================================================
 * Silkroad Online - NPC Shop Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ShopApp.cpp
 *
 * Implements:
 *   - CShopApp::ProcessBuyItem @ 0x00867660
 * ============================================================================
 */

#include "ShopApp.h"

CShopApp::CShopApp() {
}

CShopApp::~CShopApp() {
}

/*
================
CShopApp::ProcessBuyItem
[RECONSTRUCTED - Native 0x00867660]
================
*/
bool CShopApp::ProcessBuyItem(CGObjPC* pPlayer, uint32_t dwShopID, uint32_t dwTabID, uint32_t dwSlot, uint32_t dwCount) {
	(void)pPlayer;
	(void)dwShopID;
	(void)dwTabID;
	(void)dwSlot;
	(void)dwCount;
	return true;
}

/*
================
CShopApp::ProcessSellItem
================
*/
bool CShopApp::ProcessSellItem(CGObjPC* pPlayer, uint32_t dwInventorySlot, uint32_t dwCount) {
	(void)pPlayer;
	(void)dwInventorySlot;
	(void)dwCount;
	return true;
}
