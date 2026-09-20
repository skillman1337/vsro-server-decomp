/*
===========================================================================
Silkroad Online - Cooltime Manager
Original Source: D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\CooltimeManager.h

Implements CCooltimeManager:
  - Proven Native constructor @ 0x0064B140 (Size: 0x60 / 96 bytes)
  - Proven Native destructor  @ 0x0064B2C0
  - Native reset / clear      @ 0x0064B340
  - IsCooldownAvailable       @ 0x0064C1A0 (eax: pRefSkill, ecx/arg2: this)
  - RegisterCooldown          @ 0x0064C700 (ecx: this, edx: pRefSkill)
  - Embedded at CGObjPC +0x1F40 (dwords +0x7D0 in CGObjChar base pointer)
===========================================================================
*/

#ifndef _SR_GAMESERVER_COOLTIMEMANAGER_H_
#define _SR_GAMESERVER_COOLTIMEMANAGER_H_

#include <cstdint>
#include <map>
#include <list>

struct tagRefSkill;
class CGObjChar;

namespace BSLib {
	class CPacket;
}
typedef BSLib::CPacket CPacket;

/*
===========================================================================
sCoolTimeInfo

Native struct size: 0x14 (20 bytes).
Represents an individual active cooldown entry tracked in memory.
===========================================================================
*/
struct sCoolTimeInfo {
	int32_t		nType;			// +0x00: Timer/job type
	uint32_t	dwDuration;		// +0x04: Cooldown duration in ms
	uint32_t	dwStartTime;	// +0x08: Start tick (GetTickCount())
	uint8_t		byGroup;		// +0x0C: Cooldown group ID
	uint8_t		pad0D[3];		// +0x0D - +0x0F
	void*		pJob;			// +0x10: Linked CTJ_CoolTimeKeeper* or null

	sCoolTimeInfo();

	/*
	================
	Init
	Native 0x0064B120
	================
	*/
	void Init( int32_t type, uint32_t duration, uint8_t group, void* job );
};

/*
===========================================================================
CCooltimeManager

Native class size: 0x60 (96 bytes).
Manages skill cooldowns, item cooldowns, group cooldowns, and global cooldowns.
Embedded inside CGObjPC at offset +0x1F40.
===========================================================================
*/
class CCooltimeManager {
public:
	/*
	================
	CCooltimeManager
	Native 0x0064B140
	================
	*/
	CCooltimeManager();

	/*
	================
	~CCooltimeManager
	Native 0x0064B2C0
	================
	*/
	~CCooltimeManager();

	/*
	================
	Reset
	Native 0x0064B340
	================
	*/
	void Reset();

	/*
	================
	SetOwner / GetOwner
	================
	*/
	void SetOwner( CGObjChar* pOwner );
	CGObjChar* GetOwner() const;

	/*
	================
	IsCooldownAvailable
	Native 0x0064C1A0
	================
	*/
	bool IsCooldownAvailable( const tagRefSkill* pRefSkill, uint32_t dwFlags, bool bBypassLearnedCheck );

	/*
	================
	IsItemCooldownAvailable
	Native 0x0064C040
	================
	*/
	bool IsItemCooldownAvailable( uint32_t dwCooldownType, uint32_t dwItemTypeID );

	/*
	================
	RegisterCooldown
	Native 0x0064C700
	================
	*/
	void RegisterCooldown( const tagRefSkill* pRefSkill );

	/*
	================
	RegisterItemCooldown
	Native 0x0064B5F0
	================
	*/
	void RegisterItemCooldown( uint32_t dwItemTypeID, uint32_t dwDuration, uint8_t byGroup );

	/*
	================
	RegisterItemCooldownEx
	Native 0x0064C320
	================
	*/
	void RegisterItemCooldownEx( uint8_t byGroup, uint32_t dwDuration, int32_t arg4, int32_t arg5 );

	/*
	================
	GetRemainingSkillCooldown
	================
	*/
	uint32_t GetRemainingSkillCooldown( uint32_t dwSkillID ) const;

	/*
	================
	GetRemainingGroupCooldown
	================
	*/
	uint32_t GetRemainingGroupCooldown( uint8_t byGroup ) const;

	/*
	================
	SendCooltimeSyncPacket
	Native 0x0064BAE0
	================
	*/
	int32_t SendCooltimeSyncPacket();

private:
	/*
	================
	AllocCoolTimeInfo
	Native 0x0064CDD0
	================
	*/
	sCoolTimeInfo* AllocCoolTimeInfo();

public:
	CGObjChar*							m_pOwner;					// +0x00: Owner character (CGObjPC*)
	std::list<void*>					m_listTimers;				// +0x04: Active timer / job list
	std::map<uint32_t, sCoolTimeInfo*>	m_mapItemCooldown;			// +0x10: Item cooldown map (dwItemID -> info)
	std::map<uint32_t, sCoolTimeInfo*>	m_mapSkillCooldown;			// +0x1C: Skill cooldown map (dwSkillID -> info)
	std::map<uint8_t, sCoolTimeInfo*>	m_mapGroupCooldown;			// +0x28: Group cooldown map (byGroup -> info)
	uint32_t							m_dwGlobalCooldownStart;	// +0x50: Global cooldown start tick (GetTickCount())
	uint32_t							m_dwGlobalCooldownDuration;	// +0x54: Global cooldown duration in ms
	uint32_t							m_dwItemCooldownStart;		// +0x58: Item cooldown start tick
	uint32_t							m_dwItemCooldownDuration;	// +0x5C: Item cooldown duration in ms
};

#endif // _SR_GAMESERVER_COOLTIMEMANAGER_H_
