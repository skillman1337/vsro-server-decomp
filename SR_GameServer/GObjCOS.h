/**
 * ============================================================================
 * Silkroad Online - Creature of Silkroad (COS) Base Entity
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjCOS.h
 *
 * Implements:
 *   - CGObjCOS @ 0x00AF0BA0 / 0x004D1A50
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJCOS_H_
#define _SR_GAMESERVER_GOBJCOS_H_

#include "GObjChar.h"

class CGObjCOS : public CGObjChar {
public:
	CGObjCOS();
	virtual ~CGObjCOS() override;

	// [RECONSTRUCTED - Native 0x004D1A50]
	// Owner link and vehicle mounting
	uint32_t GetOwnerID() const;
	void SetOwnerID(uint32_t dwOwnerID);

	virtual bool IsCOS() const override;

protected:
	uint32_t m_dwOwnerID;
	uint8_t  m_byCOSType; // 1: Transport, 2: Attack Pet, 3: Grab Pet, 4: Fellow
};

#endif // _SR_GAMESERVER_GOBJCOS_H_
