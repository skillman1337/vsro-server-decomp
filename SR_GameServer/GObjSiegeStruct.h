/**
 * ============================================================================
 * Silkroad Online - Fortress Siege Structure Entity (Gates, Towers, Flags)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjSiegeStruct.h
 *
 * Implements:
 *   - CGObjSiegeStruct @ 0x00AF04A0
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJSIAGESTRUCT_H_
#define _SR_GAMESERVER_GOBJSIAGESTRUCT_H_

#include "GObjChar.h"

class CGObjSiegeStruct : public CGObjChar {
public:
	CGObjSiegeStruct();
	virtual ~CGObjSiegeStruct() override;

	// [RECONSTRUCTED - Native 0x00AF04A0]
	// Siege structure structural integrity and gate state
	bool IsGateOpen() const;
	void SetGateState(bool bOpen);

	uint32_t GetFortressID() const;

protected:
	uint32_t m_dwFortressID;
	uint8_t  m_byStructType; // 1: Main Gate, 2: Guard Tower, 3: Command Post, 4: Camp
	bool     m_bGateOpen;
};

#endif // _SR_GAMESERVER_GOBJSIAGESTRUCT_H_
