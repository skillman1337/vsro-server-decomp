/*
===========================================================================
Silkroad Online - Cooltime Manager
Original Source: D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\CooltimeManager.cpp

Implements CCooltimeManager:
  - Proven Native constructor @ 0x0064B140 (Size: 0x60 / 96 bytes)
  - Proven Native destructor  @ 0x0064B2C0
  - Native reset / clear      @ 0x0064B340
  - IsCooldownAvailable       @ 0x0064C1A0 (eax: pRefSkill, ecx/arg2: this)
  - RegisterCooldown          @ 0x0064C700 (ecx: this, edx: pRefSkill)
  - Embedded at CGObjPC +0x1F40 (dwords +0x7D0 in CGObjChar base pointer)
===========================================================================
*/

#include "../JMX_Library/BSLib/Packet.h"
#include "CCooltimeManager.h"
#include "GObjChar.h"
#include "Formulae.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <chrono>
static inline uint32_t GetTickCount() {
	using namespace std::chrono;
	return static_cast<uint32_t>( duration_cast<milliseconds>( steady_clock::now().time_since_epoch() ).count() );
}
#endif

/*
================
sCoolTimeInfo::sCoolTimeInfo
================
*/
sCoolTimeInfo::sCoolTimeInfo()
	: nType( 0 )
	, dwDuration( 0 )
	, dwStartTime( 0 )
	, byGroup( 0 )
	, pJob( nullptr ) {
	pad0D[0] = 0;
	pad0D[1] = 0;
	pad0D[2] = 0;
}

/*
================
sCoolTimeInfo::Init
Native 0x0064B120
================
*/
void sCoolTimeInfo::Init( int32_t type, uint32_t duration, uint8_t group, void* job ) {
	nType		= type;
	dwDuration	= duration;
	dwStartTime	= ::GetTickCount();
	byGroup		= group;
	pJob		= job;
}

/*
================
CCooltimeManager::CCooltimeManager
Native 0x0064B140
================
*/
CCooltimeManager::CCooltimeManager()
	: m_pOwner( nullptr )
	, m_dwGlobalCooldownStart( 0 )
	, m_dwGlobalCooldownDuration( 0 )
	, m_dwItemCooldownStart( 0 )
	, m_dwItemCooldownDuration( 0 ) {
}

/*
================
CCooltimeManager::~CCooltimeManager
Native 0x0064B2C0
================
*/
CCooltimeManager::~CCooltimeManager() {
	Reset();
}

/*
================
CCooltimeManager::Reset
Native 0x0064B340
================
*/
void CCooltimeManager::Reset() {
	for ( auto& pair : m_mapItemCooldown ) {
		delete pair.second;
	}
	m_mapItemCooldown.clear();

	for ( auto& pair : m_mapSkillCooldown ) {
		delete pair.second;
	}
	m_mapSkillCooldown.clear();

	for ( auto& pair : m_mapGroupCooldown ) {
		delete pair.second;
	}
	m_mapGroupCooldown.clear();

	m_listTimers.clear();

	m_dwGlobalCooldownStart = 0;
	m_dwGlobalCooldownDuration = 0;
	m_dwItemCooldownStart = 0;
	m_dwItemCooldownDuration = 0;
}

/*
================
CCooltimeManager::SetOwner
================
*/
void CCooltimeManager::SetOwner( CGObjChar* pOwner ) {
	m_pOwner = pOwner;
}

/*
================
CCooltimeManager::GetOwner
================
*/
CGObjChar* CCooltimeManager::GetOwner() const {
	return m_pOwner;
}

/*
================
CCooltimeManager::AllocCoolTimeInfo
Native 0x0064CDD0
================
*/
sCoolTimeInfo* CCooltimeManager::AllocCoolTimeInfo() {
	return new sCoolTimeInfo();
}

/*
================
CCooltimeManager::IsCooldownAvailable
Native 0x0064C1A0

eax: pRefSkill
ecx / arg2: this
arg3: dwFlags
arg4: bBypassLearnedCheck

Checks whether the skill can be cast without violating active cooldowns.
Returns true if skill is off cooldown (available to cast).
Returns false if on cooldown (cast must be rejected with 0x3005).
================
*/
bool CCooltimeManager::IsCooldownAvailable( const tagRefSkill* pRefSkill, uint32_t dwFlags, bool bBypassLearnedCheck ) {
	(void)bBypassLearnedCheck;

	// Native 0x0064C1AA: If pRefSkill is null, allow
	if ( pRefSkill == nullptr ) {
		return true;
	}

	// Native 0x0064C1D0: If dwCooldown == 0, allow immediately
	if ( pRefSkill->dwCooldown == 0 ) {
		return true;
	}

	// Native 0x0064C1DC: If dwFlags bit 0x80 is set, bypass cooldown check
	if ( ( dwFlags & 0x80 ) != 0 ) {
		return true;
	}

	const uint32_t dwNow = ::GetTickCount();

	// 1. Group Cooldown Check (Native 0x0064C1E5 - 0x0064C220)
	const uint8_t byGroup = pRefSkill->byCooldownGroup;
	if ( byGroup != 0 ) {
		auto itGroup = m_mapGroupCooldown.find( byGroup );
		if ( itGroup != m_mapGroupCooldown.end() ) {
			sCoolTimeInfo* pGroupInfo = itGroup->second;
			if ( pGroupInfo != nullptr ) {
				const uint32_t dwElapsed = dwNow - pGroupInfo->dwStartTime;
				if ( dwElapsed < pGroupInfo->dwDuration ) {
					return false; // Active group cooldown
				}
				// Expired, clean up
				delete pGroupInfo;
				m_mapGroupCooldown.erase( itGroup );
			}
		}
	}

	// 2. Global Cooldown Check (Native 0x0064C225 - 0x0064C240)
	if ( m_dwGlobalCooldownDuration > 0 ) {
		const uint32_t dwElapsed = dwNow - m_dwGlobalCooldownStart;
		if ( dwElapsed < m_dwGlobalCooldownDuration ) {
			return false; // Active global cooldown
		}
		m_dwGlobalCooldownDuration = 0;
	}

	// 3. Skill-Specific Cooldown Check (Native 0x0064C245 - 0x0064C290)
	auto itSkill = m_mapSkillCooldown.find( pRefSkill->dwSkillID );
	if ( itSkill != m_mapSkillCooldown.end() ) {
		sCoolTimeInfo* pSkillInfo = itSkill->second;
		if ( pSkillInfo != nullptr ) {
			const uint32_t dwElapsed = dwNow - pSkillInfo->dwStartTime;
			if ( dwElapsed < pSkillInfo->dwDuration ) {
				return false; // Active skill cooldown
			}
			// Expired, clean up
			delete pSkillInfo;
			m_mapSkillCooldown.erase( itSkill );
		}
	}

	return true;
}

/*
================
CCooltimeManager::IsItemCooldownAvailable
Native 0x0064C040
================
*/
bool CCooltimeManager::IsItemCooldownAvailable( uint32_t dwCooldownType, uint32_t dwItemTypeID ) {
	(void)dwCooldownType;
	const uint32_t dwNow = ::GetTickCount();

	if ( m_dwItemCooldownDuration > 0 ) {
		const uint32_t dwElapsed = dwNow - m_dwItemCooldownStart;
		if ( dwElapsed < m_dwItemCooldownDuration ) {
			return false;
		}
		m_dwItemCooldownDuration = 0;
	}

	auto it = m_mapItemCooldown.find( dwItemTypeID );
	if ( it != m_mapItemCooldown.end() ) {
		sCoolTimeInfo* pInfo = it->second;
		if ( pInfo != nullptr ) {
			const uint32_t dwElapsed = dwNow - pInfo->dwStartTime;
			if ( dwElapsed < pInfo->dwDuration ) {
				return false;
			}
			delete pInfo;
			m_mapItemCooldown.erase( it );
		}
	}

	return true;
}

/*
================
CCooltimeManager::RegisterCooldown
Native 0x0064C700

ecx: this
edx: pRefSkill
================
*/
void CCooltimeManager::RegisterCooldown( const tagRefSkill* pRefSkill ) {
	if ( pRefSkill == nullptr ) {
		return;
	}

	if ( pRefSkill->dwCooldown == 0 ) {
		return;
	}

	const uint32_t dwNow = ::GetTickCount();

	// 1. Register Skill-Specific Cooldown (Native 0x0064C720 - 0x0064C7A0)
	auto itSkill = m_mapSkillCooldown.find( pRefSkill->dwSkillID );
	if ( itSkill != m_mapSkillCooldown.end() && itSkill->second != nullptr ) {
		itSkill->second->dwStartTime = dwNow;
		itSkill->second->dwDuration = pRefSkill->dwCooldown;
	} else {
		sCoolTimeInfo* pInfo = AllocCoolTimeInfo();
		pInfo->Init( 1, pRefSkill->dwCooldown, pRefSkill->byCooldownGroup, nullptr );
		m_mapSkillCooldown[pRefSkill->dwSkillID] = pInfo;
	}

	// 2. Register Group Cooldown (Native 0x0064C877 - 0x0064C8E0)
	if ( pRefSkill->byCooldownGroup != 0 ) {
		auto itGroup = m_mapGroupCooldown.find( pRefSkill->byCooldownGroup );
		if ( itGroup != m_mapGroupCooldown.end() && itGroup->second != nullptr ) {
			itGroup->second->dwStartTime = dwNow;
			itGroup->second->dwDuration = pRefSkill->dwCooldown;
		} else {
			sCoolTimeInfo* pGroupInfo = AllocCoolTimeInfo();
			pGroupInfo->Init( 2, pRefSkill->dwCooldown, pRefSkill->byCooldownGroup, nullptr );
			m_mapGroupCooldown[pRefSkill->byCooldownGroup] = pGroupInfo;
		}
	}

	// 3. Register Global Cooldown (Native 0x0064C82D - 0x0064C850)
	if ( pRefSkill->dwCooldownGlobal > 0 ) {
		m_dwGlobalCooldownStart = dwNow;
		m_dwGlobalCooldownDuration = pRefSkill->dwCooldownGlobal;
	}

	// 4. Send Cooltime Synchronization Packet to Client (Native 0x0064BAE0)
	SendCooltimeSyncPacket();
}

/*
================
CCooltimeManager::RegisterItemCooldown
Native 0x0064B5F0
================
*/
void CCooltimeManager::RegisterItemCooldown( uint32_t dwItemTypeID, uint32_t dwDuration, uint8_t byGroup ) {
	const uint32_t dwNow = ::GetTickCount();

	auto it = m_mapItemCooldown.find( dwItemTypeID );
	if ( it != m_mapItemCooldown.end() && it->second != nullptr ) {
		it->second->dwStartTime = dwNow;
		it->second->dwDuration = dwDuration;
	} else {
		sCoolTimeInfo* pInfo = AllocCoolTimeInfo();
		pInfo->Init( 3, dwDuration, byGroup, nullptr );
		m_mapItemCooldown[dwItemTypeID] = pInfo;
	}
}

/*
================
CCooltimeManager::RegisterItemCooldownEx
Native 0x0064C320
================
*/
void CCooltimeManager::RegisterItemCooldownEx( uint8_t byGroup, uint32_t dwDuration, int32_t arg4, int32_t arg5 ) {
	(void)arg4;
	(void)arg5;
	m_dwItemCooldownStart = ::GetTickCount();
	m_dwItemCooldownDuration = dwDuration;

	if ( byGroup != 0 ) {
		auto it = m_mapGroupCooldown.find( byGroup );
		if ( it != m_mapGroupCooldown.end() && it->second != nullptr ) {
			it->second->dwStartTime = m_dwItemCooldownStart;
			it->second->dwDuration = dwDuration;
		} else {
			sCoolTimeInfo* pInfo = AllocCoolTimeInfo();
			pInfo->Init( 4, dwDuration, byGroup, nullptr );
			m_mapGroupCooldown[byGroup] = pInfo;
		}
	}
}

/*
================
CCooltimeManager::GetRemainingSkillCooldown
================
*/
uint32_t CCooltimeManager::GetRemainingSkillCooldown( uint32_t dwSkillID ) const {
	auto it = m_mapSkillCooldown.find( dwSkillID );
	if ( it != m_mapSkillCooldown.end() && it->second != nullptr ) {
		const uint32_t dwNow = ::GetTickCount();
		const uint32_t dwElapsed = dwNow - it->second->dwStartTime;
		if ( dwElapsed < it->second->dwDuration ) {
			return it->second->dwDuration - dwElapsed;
		}
	}
	return 0;
}

/*
================
CCooltimeManager::GetRemainingGroupCooldown
================
*/
uint32_t CCooltimeManager::GetRemainingGroupCooldown( uint8_t byGroup ) const {
	auto it = m_mapGroupCooldown.find( byGroup );
	if ( it != m_mapGroupCooldown.end() && it->second != nullptr ) {
		const uint32_t dwNow = ::GetTickCount();
		const uint32_t dwElapsed = dwNow - it->second->dwStartTime;
		if ( dwElapsed < it->second->dwDuration ) {
			return it->second->dwDuration - dwElapsed;
		}
	}
	return 0;
}

/*
================
CCooltimeManager::SendCooltimeSyncPacket
Native 0x0064BAE0

Builds and transmits opcode 0xB0BD (Cooltime sync packet) to the player client.
Wire format:
  uint16_t wCount: number of active cooldown entries
  for each entry:
    uint32_t dwType / ID
    uint32_t dwRemainingMs
================
*/
int32_t CCooltimeManager::SendCooltimeSyncPacket() {
	if ( m_pOwner == nullptr || !m_pOwner->IsPlayer() ) {
		return 0;
	}

	CPacket* pPacket = m_pOwner->AllocMsgForPeer( 0xB0BD );
	if ( pPacket == nullptr ) {
		return 0;
	}

	const uint32_t dwNow = ::GetTickCount();
	uint16_t wCount = 0;

	// Count active skill cooldowns
	for ( const auto& pair : m_mapSkillCooldown ) {
		if ( pair.second != nullptr && ( dwNow - pair.second->dwStartTime < pair.second->dwDuration ) ) {
			wCount++;
		}
	}

	pPacket->WriteUint16( wCount );
	for ( const auto& pair : m_mapSkillCooldown ) {
		if ( pair.second != nullptr ) {
			const uint32_t dwElapsed = dwNow - pair.second->dwStartTime;
			if ( dwElapsed < pair.second->dwDuration ) {
				pPacket->WriteUint32( pair.first );
				pPacket->WriteUint32( pair.second->dwDuration - dwElapsed );
			}
		}
	}

	return m_pOwner->SendMsgToPeer( pPacket );
}
