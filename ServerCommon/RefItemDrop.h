/**
 * ============================================================================
 * Silkroad Online - Item Drop Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\RefItemDrop.h
 *
 * Implements:
 *   - CRefItemDrop @ 0x00724A00 / 0x00728470
 * ============================================================================
 */

#ifndef _SERVERCOMMON_REFITEMDROP_H_
#define _SERVERCOMMON_REFITEMDROP_H_

#include <cstdint>
#include <vector>

struct tagDropResult {
	uint32_t dwItemID;
	uint32_t dwCount;
};

class CRefItemDrop {
public:
	CRefItemDrop();
	virtual ~CRefItemDrop();

	// [RECONSTRUCTED - Native 0x00724A00]
	// Calculates drop items from monster reference and level differential
	int32_t CalculateDrops(uint32_t dwMonsterRefID, int32_t nMonsterLevel, int32_t nPlayerLevel, std::vector<tagDropResult>& outDrops);
};

#endif // _SERVERCOMMON_REFITEMDROP_H_
