/*
===========================================================================
Silkroad Online - Game Message Filter Implementation
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GMsgFilter.cpp
Native RTTI: .?AVCGMsgFilter@@ @ 0x00AE0AC4
===========================================================================
*/

#include "GMsgFilter.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/Msg.h"
#include "../JMX_Library/BSLib/BSLog.h"

CGMsgFilter* g_pMsgFilter = nullptr;

static const tagMsgFilterRule s_staticRules[] = {

	// Index 0: Group 0, State 0, Mode 0
	{ 0, 0, {  } },
	// Index 1: Group 0, State 1, Mode 1
	{ 1, 1, {  } },
	// Index 2: Group 0, State 2, Mode 0
	{ 0, 2, { 0x3053, 0x7025, 0x3080, 0x7061, 0x7062, 0x7063, 0x7067, 0x7034, 0x7005, 0x7006, 0x704C, 0x7309, 0x7308, 0x730A, 0x715F, 0x34BF, 0x7121 } },
	// Index 3: Group 0, State 3, Mode 0
	{ 0, 3, {  } },
	// Index 4: Group 1, State 0, Mode 1
	{ 1, 0, {  } },
	// Index 5: Group 1, State 1, Mode 1
	{ 1, 1, { 0x7021, 0x7023, 0x7024 } },
	// Index 6: Group 1, State 2, Mode 1
	{ 1, 2, {  } },
	// Index 7: Group 1, State 3, Mode 1
	{ 1, 3, {  } },
	// Index 8: Group 1, State 4, Mode 1
	{ 1, 4, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x703E, 0x703F, 0x7074, 0x3091 } },
	// Index 9: Group 1, State 8, Mode 1
	{ 1, 8, { 0x7021, 0x7070, 0x704F, 0x7023 } },
	// Index 10: Group 1, State 9, Mode 0
	{ 0, 9, { 0x704C, 0x7025, 0x3053, 0x7005, 0x7006, 0x7045, 0x706A, 0x706B, 0x706C, 0x706D, 0x306E, 0x7069, 0x7074, 0x7070 } },
	// Index 11: Group 1, State 10, Mode 0
	{ 0, 10, { 0x7045, 0x704C, 0x7025, 0x3053, 0x7005, 0x7006, 0x706A, 0x706B, 0x706C, 0x706D, 0x306E, 0x7069, 0x7074, 0x7070 } },
	// Index 12: Group 1, State 11, Mode 1
	{ 1, 11, { 0x7021, 0x7070 } },
	// Index 13: Group 1, State 12, Mode 1
	{ 1, 12, {  } },
	// Index 14: Group 1, State 13, Mode 0
	{ 0, 13, {  } },
	// Index 15: Group 1, State 14, Mode 1
	{ 1, 14, { 0x7021, 0x7023, 0x7024, 0x7070 } },
	// Index 16: Group 1, State 15, Mode 1
	{ 1, 15, { 0x7021, 0x7023, 0x7024, 0x7070, 0x70CB } },
	// Index 17: Group 1, State 19, Mode 0
	{ 0, 19, { 0x7045, 0x704C, 0x7025, 0x3053, 0x7005, 0x7006, 0x706A, 0x706B, 0x706C, 0x706D, 0x306E, 0x7069, 0x7074, 0x7070 } },
	// Index 18: Group 1, State 16, Mode 1
	{ 1, 16, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7074, 0x704F } },
	// Index 19: Group 1, State 17, Mode 1
	{ 1, 17, { 0x7021, 0x7023, 0x7024, 0x704F, 0x70CB } },
	// Index 20: Group 1, State 18, Mode 1
	{ 1, 18, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x703E, 0x703F, 0x7074, 0x3091 } },
	// Index 21: Group 2, State 0, Mode 1
	{ 1, 0, {  } },
	// Index 22: Group 2, State 1, Mode 1
	{ 1, 1, { 0x7060, 0x7046, 0x7059, 0x704F, 0x7150, 0x7151, 0x7155, 0x7157 } },
	// Index 23: Group 2, State 2, Mode 1
	{ 1, 2, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x7045, 0x704C, 0x703E, 0x703F, 0x7074, 0x7059, 0x7081, 0x70B1, 0x70B3, 0x7060, 0x7062, 0x704F, 0x7150, 0x7151, 0x7155, 0x7157 } },
	// Index 24: Group 2, State 6, Mode 1
	{ 1, 6, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x7074, 0x7081, 0x70B1, 0x70B3, 0x7060, 0x704F } },
	// Index 25: Group 2, State 5, Mode 0
	{ 0, 5, { 0x3053, 0x7025, 0x704B, 0x30D4, 0x704F, 0x7202, 0x7203, 0x70EA, 0x70D9, 0x7515 } },
	// Index 26: Group 2, State 4, Mode 1
	{ 1, 4, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x7045, 0x704C, 0x703E, 0x703F, 0x7074, 0x7059, 0x7081, 0x70B1, 0x70B3, 0x7060, 0x7062, 0x704F, 0x70B5, 0x7150, 0x7151, 0x7155, 0x7157 } },
	// Index 27: Group 2, State 7, Mode 1
	{ 1, 7, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x7045, 0x704C, 0x703E, 0x703F, 0x7074, 0x7059, 0x7081, 0x70B1, 0x70B3, 0x7060, 0x7062, 0x704F, 0x7150, 0x7151, 0x7155, 0x7157 } },
	// Index 28: Group 2, State 9, Mode 1
	{ 1, 9, { 0x7021, 0x7023, 0x7024, 0x7070, 0x7046, 0x7045, 0x704C, 0x703E, 0x703F, 0x7074, 0x7059, 0x7081, 0x70B1, 0x70B3, 0x7060, 0x7062, 0x704F, 0x7150, 0x7151, 0x7155, 0x7157 } },
	// Index 29: Group 3, State 0, Mode 1
	{ 1, 0, { 0x7061, 0x7062, 0x7063 } },
	// Index 30: Group 3, State 1, Mode 1
	{ 1, 1, { 0x7060, 0x7061, 0x7062, 0x7063 } },
	// Index 31: Group 3, State 2, Mode 1
	{ 1, 2, { 0x7060 } },
	// Index 32: Group 9, State 1, Mode 1
	{ 1, 1, { 0x7021, 0x7023, 0x7024, 0x7070 } },
	// Index 33: Group 4, State 2, Mode 1
	{ 1, 2, { 0x7070, 0x7074 } },
	// Index 34: Group 11, State 1, Mode 0
	{ 0, 1, { 0x7025, 0x704C, 0x705B, 0x7005, 0x34BF } },
	// Index 35: Group 12, State 4, Mode 0
	{ 0, 4, { 0x7006, 0x7025, 0x704C } },
	// Index 36: Group 12, State 2, Mode 0
	{ 0, 2, {  } },
	// Index 37: Group 12, State 3, Mode 0
	{ 0, 3, {  } },
	// Index 38: Group 12, State 5, Mode 0
	{ 0, 5, {  } },
	// Index 39: Group 13, State 1, Mode 0
	{ 0, 1, { 0x7010 } },
	// Index 40: Group 15, State 1, Mode 0
	{ 0, 1, { 0x7155, 0x7025, 0x706A, 0x706B, 0x706C, 0x706D, 0x306E, 0x7069 } },
	// Index 41: Group 15, State 2, Mode 0
	{ 0, 2, { 0x7150, 0x7151, 0x34A7, 0x7025, 0x706A, 0x706B, 0x706C, 0x706D, 0x306E, 0x7069, 0x716A } },
};
static const size_t s_nStaticRuleCount = sizeof(s_staticRules) / sizeof(s_staticRules[0]);

/*
================
CGMsgFilter::CGMsgFilter
[RECONSTRUCTED - 0x00427C60] (146 bytes)
================
*/
CGMsgFilter::CGMsgFilter() {
	ASSERT( g_pMsgFilter == nullptr );
	g_pMsgFilter = this;
	Initialize();
}

/*
================
CGMsgFilter::~CGMsgFilter
[RECONSTRUCTED - 0x00427D20] (105 bytes)
================
*/
CGMsgFilter::~CGMsgFilter() {
	g_pMsgFilter = nullptr;
}

// Native static instance @ 0x00CC3FE0, constructed by the CRT initialiser 0x00ACB130.
static CGMsgFilter s_MsgFilter;

/*
================
CGMsgFilter::Initialize
[RECONSTRUCTED - 0x00427D90] (6407 bytes)

Each used group is pre-sized (0x00429930) before its rules are registered (0x00427B80, which reports
an out-of-range or already-registered state).
================
*/
void CGMsgFilter::Initialize() {
	static const struct { uint32_t dwGroup; uint32_t dwStates; } s_aGroupSizes[] = {
		{ 0, 4 }, { 1, 21 }, { 2, 10 }, { 3, 5 }, { 9, 2 }, { 4, 8 }, { 11, 4 }, { 12, 7 }, { 13, 2 }, { 15, 3 },
	};
	for ( const auto& size : s_aGroupSizes ) {
		m_groups[size.dwGroup].assign( size.dwStates, nullptr );
	}

	static const uint8_t s_abyRuleGroup[] = {
		0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3,
		9,
		4,
		11,
		12, 12, 12, 12,
		13,
		15, 15,
	};
	ASSERT( sizeof( s_abyRuleGroup ) == s_nStaticRuleCount );

	for ( size_t i = 0; i < s_nStaticRuleCount; ++i ) {
		const tagMsgFilterRule* pRule = &s_staticRules[i];
		std::vector<const tagMsgFilterRule*>& group = m_groups[s_abyRuleGroup[i]];
		ASSERT( pRule->m_dwState < group.size() );
		ASSERT( group.at( pRule->m_dwState ) == nullptr );
		group.at( pRule->m_dwState ) = pRule;
	}
}

/*
================
CGMsgFilter::CheckRule
[RECONSTRUCTED - 0x00427B20] (88 bytes)
================
*/
bool CGMsgFilter::CheckRule( uint32_t dwGroup, uint8_t byState, uint16_t wOpcode ) const {
	const std::vector<const tagMsgFilterRule*>& group = m_groups[dwGroup];
	ASSERT( byState < group.size() );
	const tagMsgFilterRule* pRule = group.at( byState );
	if ( pRule == nullptr ) {
		return true;
	}
	return pRule->IsAllowed( wOpcode );
}

/*
================
CGMsgFilter::ValidateMessage
[RECONSTRUCTED - 0x004296A0] (576 bytes)
================
*/
int32_t CGMsgFilter::ValidateMessage( CGObjChar* pChar, CMsg* pMsg ) const {
	const uint16_t wOpcode = pMsg->GetOpcode();
	bool bAllowed = CheckRule( 0, pChar->GetLifeState(), wOpcode );

	if ( bAllowed && pChar->GetAbnormalFilterState() != 0x15 ) {
		if ( pChar->IsFrozen() ) {
			bAllowed = CheckRule( 1, 0x0A, wOpcode );
		}
		if ( bAllowed && pChar->IsStunned() ) {
			bAllowed = CheckRule( 1, 0x09, wOpcode );
		}
		if ( bAllowed && pChar->IsAsleep() ) {
			bAllowed = CheckRule( 1, 0x13, wOpcode );
		}
	}
	bAllowed = bAllowed && CheckRule( 1, pChar->GetMotionState(), wOpcode );

	if ( bAllowed && pChar->IsPlayer() ) {
		const tagCharData* pCharData = pChar->GetCharData();
		bAllowed = CheckRule( 2, pCharData->m_byInteractMode, wOpcode ) &&
			CheckRule( 3, pCharData->m_byGroupMode, wOpcode ) &&
			CheckRule( 4, pChar->GetBodyMode(), wOpcode ) &&
			CheckRule( 11, pChar->GetTeleportState(), wOpcode ) &&
			CheckRule( 12, pCharData->m_byFilterState11, wOpcode ) &&
			CheckRule( 13, pCharData->m_byFilterState12, wOpcode ) &&
			CheckRule( 15, pCharData->m_byFilterState14, wOpcode );
	}

	if ( !bAllowed ) {
		pChar->OnClientPacket( reinterpret_cast<BSLib::CPacket*>( pMsg ) );
		return 0;
	}
	return 1;
}
