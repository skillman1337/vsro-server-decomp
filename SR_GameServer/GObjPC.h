/**
 * ============================================================================
 * Silkroad Online - Player Character Object (PC)
 * Original Source: D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\GObjPC.h
 *
 * Implements CGObjPC:
 *   - Native VTable @ 0x00AF59FC (for CGObjChar)
 *   - Native RTTI: .?AVCGObjPC@@ @ 0x00B572B4
 *   - Derived from CGObjChar
 *   - Slot 358 @ +0x598: OnClientPacket @ 0x004ECBC0 (Primary client packet dispatcher)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJPC_H_
#define _SR_GAMESERVER_GOBJPC_H_

#include <cstdint>
#include <cstring>
#include "GObjChar.h"
#include "../Common/Framework/CmdSource.h"
#include "CCooltimeManager.h"
#include "TimedJob.h"
#include "../JMX_Library/BSLib/Packet.h"

// Spawn location structure matching native offset +0x2210 in CGObjPC
struct tagSpawnLocation {
	uint8_t  m_pad00[0x0A];       // +0x00 - +0x09
	uint16_t m_wSpawnRegionID;   // +0x0A (overall +0x221A)
	uint8_t  m_pad0C[2];          // +0x0C - +0x0D
	uint16_t m_wSpawnSectorID;   // +0x0E (overall +0x221E)
	uint8_t  m_pad10[0x10];       // +0x10 - +0x1F

	tagSpawnLocation();
	void Reset();
};

/**
 * [RECONSTRUCTED - Native VTable @ 0x00AF59FC / RTTI .?AVCGObjPC@@]
 * Silkroad Online - CGObjPC (Player Character Object)
 */
class CGObjPC : public CGObjChar {
public:
	// Native 0x004DE300: Constructor
	CGObjPC();

	// Native 0x004DE9B0: Destructor (Slot 0)
	virtual ~CGObjPC() override;

	// Slot 7 @ +0x1C: IsPlayer (Proven @ 0x00482560)
	virtual bool IsPlayer() const override;

	// Slot 2 @ +0x08: GetJID [RECONSTRUCTED - 0x004E5920]
	// The account the session at +0x188 belongs to; the base class treats the call as illegal, so a player
	// that reaches combat without this override trips the mini dump every time a status is rolled.
	virtual uint32_t GetJID() const override;

	// Slot 158 @ +0x278: AllocMsgForPeer [NATIVE - 0x004E0830]
	virtual CPacket* AllocMsgForPeer(uint16_t wOpcode) override;

	// Slot 159 @ +0x27C: SendMsgToPeer [NATIVE - 0x004E0860]
	virtual int32_t SendMsgToPeer(CPacket* pPacket) override;

	// Slot 91 @ +0x16C: OffsetGold [NATIVE - 0x004E4B60]
	virtual int64_t OffsetGold(int64_t nOffset, uint32_t dwReason = 9, uint32_t dwParam1 = 1, uint32_t dwParam2 = 0) override;

	// Slot 93 @ +0x174: OffsetSkillPoint [NATIVE - 0x004E4C60]
	virtual void OffsetSkillPoint(int32_t nOffset, uint8_t byReason = 1) override;
	// 4E4D00: signed delta; body mode 1 blocks gains, not consumption.
	void ModifyBerserkPoints(int32_t delta, uint8_t reason);

	// Slot 137 @ +0x224: InquireSameItem [NATIVE - 0x004ED680]
	virtual uint32_t InquireSameItem(uint32_t dwStorageType, const char* szCodeName, uint32_t dwMode, uint32_t dwSlot = 0xFFFFFFFF, uint32_t dwUnk = 1) override;

	// Slot 140 @ +0x230: DelItem_EXT [NATIVE - 0x004EEB70]
	virtual void* DelItem_EXT(uint32_t dwStorageType, uint8_t bySlot, int32_t nCount, uint16_t* pwStatus, uint8_t byReason = 3, uint32_t dwControl = 0) override;

	// Slot 157 @ +0x274: BackupData [NATIVE - 0x004E64D0]
	virtual void BackupData(uint32_t dwFlags, uint32_t dwParam = 0) override;

	// Slot 347 @ +0x56C: GetMainWeaponAttackSkillID [NATIVE - 0x004EAE50]
	virtual uint32_t GetMainWeaponAttackSkillID() const override;

	// Slot 358 @ +0x598: OnClientPacket [RECONSTRUCTED - 0x004ECBC0]
	// Main network packet handler for client-initiated player packets
	virtual int32_t OnClientPacket(CPacket* pPacket) override;

	// Slot 72 @ +0x120: GetTeleportState [RECONSTRUCTED - Native 0x004DDC90]
	virtual uint8_t GetTeleportState() const override { return m_pCharData->m_byTeleportState; }

	// Slot 336 @ +0x540: IsRidingTransport [RECONSTRUCTED - Native 0x004DDCA0]
	virtual bool IsRidingTransport() const override;
	virtual void SendParameterStats() override; // AF59FC+538 -> 4EBE30
	CGObjChar* m_pRiddenTransport = nullptr; // Native PC +1D18 (pointer, not GID)
	bool m_hasMovementSpeedOverride = false; // Native +21A4 == 1
	float m_overrideWalkSpeed = 0; // +21A8
	float m_overrideRunSpeed = 0; // +21AC

	// Slot 335 @ +0x53C: IsGM [RECONSTRUCTED - Native 0x004E5950]
	virtual bool IsGM() const override;

	// Slot 405 @ +0x654: IsSiegeTargetRestricted [RECONSTRUCTED - Native 0x0052BC10]
	virtual bool IsSiegeTargetRestricted() const override;

	// Slot 372 @ +0x5D0: IsMainWeaponUsable [RECONSTRUCTED - Native 0x004EC5A0]
	virtual bool IsMainWeaponUsable() const override;

	// Slot 376 @ +0x5E0: DispatchSRMsg [RECONSTRUCTED - 0x0050EEE0]
	// Player-specific network message router (intercepts 0x7070, 0x7074, fallback to CGObjChar)
	virtual void DispatchMsg(CMsg* pMsg) override;

	// Slot 397 @ +0x634: IsSpecialCommandAllowed [NATIVE - 0x004E5960]
	virtual bool IsSpecialCommandAllowed(uint16_t wSubOpcode);

	// Slot 399 @ +0x63C: IsSecondaryWeaponUsable [RECONSTRUCTED - Native 0x004EC5D0]
	virtual bool IsSecondaryWeaponUsable() const override;

	// Slot 400 @ +0x640: IsEquipmentSlotUsable [RECONSTRUCTED - Native 0x004EC600]
	virtual bool IsEquipmentSlotUsable(uint32_t dwSlot) const override;

	// [STUB - Native 0x004E0C60] (237 bytes) (stack this, bySet)
	// Sets or clears status 0x0E (tagCharData +0x13); clearing it broadcasts 0x3042. Called with 0
	// after every skill action the player starts (0x004B21D5).
	void SetSpawnInvincible(uint8_t bySet);

	// Native 0x004E7250: SendErrorResponse
	// Allocates response packet with wOpcode, writes error code, and sends to player
	int32_t SendErrorResponse(uint16_t wOpcode, uint16_t wErrorCode);

	// Native 0x0051DE90: MsgHandler_SpecialCommand (Opcode 0x7010)
	int32_t MsgHandler_SpecialCommand(CPacket* pPacket);

	// Special Command Helpers (0x00520350 - 0x00521050)
	int32_t OnCommandMoveToNpc(const char* pszNpcCodeName, CPacket* pResponseMsg);
	int32_t OnCommandToggleDaemon(uint8_t byParam, CPacket* pResponseMsg);
	int32_t OnCommandSummonOrKillMob(uint32_t dwMobRefID, uint8_t byCount, uint8_t byLevelOffset, bool bKill, CPacket* pResponseMsg);
	int32_t OnCommandSetInvincibleOrInvisible(uint16_t wSubOpcode, CPacket* pResponseMsg);
	int32_t OnCommandWho(uint16_t wSubOpcode, const char* pszQueryName, CPacket* pResponseMsg);
	void    AuditLogSpecialCommand(const char* pszCommandText);

	// Helper for GM commands
	int32_t TeleportToCoordinates(uint16_t wRegionID, float fX, float fY, float fZ);

	// [RECONSTRUCTED - Native 0x004DF050] (568 bytes)
	// Recalls / teleports player to recall point or penalty location (reason 4 = macro / violation)
	int32_t Recall(uint8_t byReason);

	// Native 0x004DF290: TeleportToTown
	int32_t TeleportToTown();

	// Native 0x00518C20: HandleClientConfigData (Opcode 0x7158)
	int32_t HandleClientConfigData(CPacket* pPacket);

	// Native 0x00468A80: TeleportToLocation
	int32_t TeleportToLocation(uint8_t byReason, uint32_t dwLocationKey);

	// Native 0x004DD990: ResetReturnState
	void ResetReturnState();

	// Sends client system notice / warning message
	void Notice(uint32_t dwNoticeID);

	// [RECONSTRUCTED - Native 0x004DDD50]
	// Slot 374 @ +0x5D8: Returns CTimedJobManager embedded at +0x1DFC
	virtual CTimedJobManager* GetTimedJobManager() override;

	// Register skill cooldown and timer - Native 0x0064C700
	virtual void RegisterSkillCooldownAndTimer(const tagRefSkill* pRefSkill) override;

	// Returns CCooltimeManager embedded at +0x1F40
	virtual CCooltimeManager* GetCooltimeManager() override { return &m_cooltimeManager; }

public:
	// Portable owned projection of the manager constructed at 4DE3A1.
	// Padding is not a constructed C++ object and cannot be used as a manager.
	CTimedJobManager m_timedJobs;
	// Exact struct layout matching native binary bytes (+0x1B78 - +0x2300):
	uint8_t          m_pad1B78[0x3C8];      // +0x1B78 - +0x1F3F: Preceding subsystems
	CCooltimeManager m_cooltimeManager;     // +0x1F40 - +0x1F9F: Native CCooltimeManager (Size: 0x60)
	uint8_t          m_pad1FA0[0x270];      // +0x1FA0 - +0x220F: Trailing subsystems
	tagSpawnLocation m_spawnLocation;       // +0x2210: Town return / resurrection spawn point
	uint8_t          m_pad2230[0xA0];       // +0x2230 - +0x22CF: Inventory, trade, exchange states
	uint8_t          m_configData[0x40];    // +0x22D0: Client config / hotkey data
};

#endif // _SR_GAMESERVER_GOBJPC_H_
