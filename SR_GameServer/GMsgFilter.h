/*
===========================================================================
Silkroad Online - Game Message Filter
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GMsgFilter.h
Native RTTI: .?AVCGMsgFilter@@ @ 0x00AE0AC4
Native Global Instance @ 0x00D6A8F8
===========================================================================
*/

#ifndef _SR_GAMESERVER_GMSGFILTER_H_
#define _SR_GAMESERVER_GMSGFILTER_H_

#include <cstdint>
#include <vector>

class CGObjChar;
class CMsg;

/*
===========================================================================
tagMsgFilterRule
Encapsulates an allow-list (mode 0) or block-list (mode 1) rule set.
Native check functions:
  - 0x004279A0: Allow-list check
  - 0x00427940: Block-list check
===========================================================================
*/
struct tagMsgFilterRule {
	uint32_t              m_dwMode;   // 0: Whitelist (Allow only), 1: Blacklist (Block listed)
	uint32_t              m_dwState;  // Associated state index
	std::vector<uint16_t> m_opcodes;  // Filtered opcodes

	bool IsAllowed( uint16_t wOpcode ) const {
		bool bFound = false;
		for ( uint16_t op : m_opcodes ) {
			if ( op == wOpcode ) {
				bFound = true;
				break;
			}
		}
		if ( m_dwMode == 0 ) {
			// Mode 0: Whitelist - must be found to be allowed
			return bFound;
		}
		// Mode 1: Blacklist - must NOT be found to be allowed
		return !bFound;
	}
};

/*
===========================================================================
CGMsgFilter
Global packet filter evaluating character states against opcode permissions.
Native VTable @ 0x00AE0AC4 (size 0x1C0 = 448 bytes)
===========================================================================
*/
class CGMsgFilter {
public:
	// [RECONSTRUCTED - 0x00427C60] single static instance @ 0x00CC3FE0; sets g_pMsgFilter
	CGMsgFilter();

	// [RECONSTRUCTED - 0x00427D20] clears g_pMsgFilter
	virtual ~CGMsgFilter();

	// [RECONSTRUCTED - 0x00427D90] sizes the state tables and registers the 42 rules
	void Initialize();

	// [RECONSTRUCTED - 0x004296A0] (ebx = pMsg, esi = pChar, stack this)
	// CORRECTION (Claude): the three abnormal-state checks of group 1 are independent, the motion
	// state is always checked afterwards, and players are also checked against groups 11, 12, 13
	// and 15. A refused message is handed to slot 358.
	int32_t ValidateMessage( CGObjChar* pChar, CMsg* pMsg ) const;

	// [RECONSTRUCTED - 0x00427B20] (edi = group, stack wOpcode, byState)
	// CORRECTION (Claude): a state outside the table is reported and then faults (vector subscript),
	// it is not silently allowed.
	bool CheckRule( uint32_t dwGroup, uint8_t byState, uint16_t wOpcode ) const;

private:
	// 17 groups of state rules matching native layout at 0x00CC3FE4
	std::vector<const tagMsgFilterRule*> m_groups[17];
};

extern CGMsgFilter* g_pMsgFilter;

#endif // _SR_GAMESERVER_GMSGFILTER_H_
