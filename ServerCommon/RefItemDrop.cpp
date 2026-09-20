/**
 * ============================================================================
 * Silkroad Online - Item Drop Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\RefItemDrop.cpp
 *
 * Implements:
 *   - CRefItemDrop::CalculateDrops @ 0x00724A00
 * ============================================================================
 */

#include "RefItemDrop.h"

CRefItemDrop::CRefItemDrop() {
}

CRefItemDrop::~CRefItemDrop() {
}

/*
================
CRefItemDrop::CalculateDrops
[RECONSTRUCTED - Native 0x00724A00]
================
*/
int32_t CRefItemDrop::CalculateDrops(uint32_t dwMonsterRefID, int32_t nMonsterLevel, int32_t nPlayerLevel, std::vector<tagDropResult>& outDrops) {
	(void)dwMonsterRefID;
	(void)nMonsterLevel;
	(void)nPlayerLevel;
	outDrops.clear();
	return 0;
}
