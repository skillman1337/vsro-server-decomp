/**
 * ============================================================================
 * Silkroad Online - Game Object Character
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjChar.h
 *
 * Implements CGObjChar:
 *   - VTable @ 0x00AEC0A4
 *   - RTTI: .?AVCGObjChar@@
 *   - Native character intrusive list @ 0x00C825F4 - 0x00C82604
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJCHAR_H_
#define _SR_GAMESERVER_GOBJCHAR_H_

#include <cstdint>
#include "CharacterPeriodicJobs.h"
#include <string>
#include "GObj.h"
#include "SkillManager.h"
#include "GParamKeeper.h"
#include "GStorage.h"
#include "GlobalPos.h"
#include "GCharAutoCommandActor.h"
#include "GCharAutoNavigator.h"
#include "GObjMover.h"

namespace BSLib {
class CPacket;
}
typedef BSLib::CPacket CPacket;

class CMsg;
class CSkillManager;
class CGItemEquip;
class CTimedJobManager;
class CGameWorldLayer;
class CCmdSource;
class CGMsgFilter;
struct tagTargetCandidate;
struct tagActiveSkillInstance;

// Intrusive list node embedded at offset +0x178 in CGObjChar
struct tagCharListNode {
	void*            pOwner;  // +0x00: Owning CGObjChar pointer
	int32_t          bInList; // +0x04: 1 if registered in global list, 0 if not
	tagCharListNode* pPrev;   // +0x08: Previous node
	tagCharListNode* pNext;   // +0x0C: Next node
};

// Character metadata / identity descriptor pointed to by +0x34
struct tagCharInfo;

// One abnormal-state record (0x70 bytes) of the array at CGObjChar +0xD38, indexed by the bit number
// of the abnormal flags (+0xD34): 0x004AAB99 reads index 0, 0x004AABD9 index 6, 0x004AABBC index 14.
/**
 * One of the 32 abnormal state slots of a character (0x70 bytes), at +0x0D3C.
 * CORRECTION (Claude): the active flag was placed at +0x64. CGObjChar_ApplyAbnormalStateRecord (0x004A4270)
 * copies the 0x60 bytes of the status record into the slot and then writes +0x60, and
 * CGObjChar_UpdateAbnormalStates (0x004A4390) tests the same +0x60 and reads the start time from +0x64.
 */
struct tagAbnormalStateSlot {
	tagSkillStatusEffect m_Effect;   // +0x00 - +0x5F: the record the hit carried (0x004A4335, memcpy 0x60)
	uint8_t  m_byActive;             // +0x60 (0x004A433E)
	uint8_t  m_by61;                 // +0x61 (0x004A4342)
	uint8_t  m_byUpgraded;           // +0x62: a stronger status replaced the one already there (0x004A42DB)
	uint8_t  m_pad63;                // +0x63
	uint32_t m_dwStartedAt;          // +0x64: GetTickCount when it was applied (0x004A4350)
	uint32_t m_dw68;                 // +0x68
	uint32_t m_dw6C;                 // +0x6C (0x004A4553 clears it with the slot)
};

// Base class for game characters (Native RTTI: .?AVCGObjChar@@)
class CGObjChar : public CGObj {
public:
	CGObjChar();
	virtual ~CGObjChar() override;

	// Overridden type query virtual slots proven against native VTable @ 0x00AEC0A4:
	// Slot 7 @ +0x1C: IsPlayer / IsPC [RECONSTRUCTED - 0x00482560]
	virtual bool IsPlayer() const override;

	// Slot 8 @ +0x20: IsNonPlayer [RECONSTRUCTED - 0x00482590]
	virtual bool IsNonPlayer() const override;

	// Slot 9 @ +0x24: IsNPC / IsStructure / IsBuilding (TID 0x100) [RECONSTRUCTED - 0x004825C0]
	virtual bool IsNPC() const override;
	virtual bool IsStructure() const override;

	// Slot 10 @ +0x28: IsMonster / IsMob (TID 0x80) [RECONSTRUCTED - 0x00482600]
	virtual bool IsMonster() const override;

	// Slot 11 @ +0x2C: IsCOS / IsPet [RECONSTRUCTED - 0x004827B0]
	virtual bool IsCOS() const override;

	// Slot 59 @ +0xEC: GetName [RECONSTRUCTED - 0x004A66D0]
	virtual const char* GetName() const override;

	// Slot 62 @ +0xF8: GetLifeState [RECONSTRUCTED - 0x00485EE0]
	virtual uint8_t GetLifeState() const override;

	// Slot 63 @ +0xFC: GetMotionState [RECONSTRUCTED - 0x004AA590]
	virtual uint8_t GetMotionState() const override;

	// Slot 64 @ +0x100: GetBodyMode [RECONSTRUCTED - 0x004AA5B0]
	virtual uint8_t GetBodyMode() const override;

	// Slot 65 @ +0x104: GetParamFloat [RECONSTRUCTED - 0x004AA5C0]
	virtual float GetParamFloat(uint32_t dwParamID) const override;

	// Slot 66 @ +0x108: GetCurrentHP [RECONSTRUCTED - 0x004AA5E0]
	virtual uint32_t GetCurrentHP() const override;

	// Slot 67 @ +0x10C: GetCurrentMP [RECONSTRUCTED - 0x004AA5F0]
	virtual uint32_t GetCurrentMP() const override;

	// Slot 68 @ +0x110: GetMaxHP [RECONSTRUCTED - 0x004A6830]
	virtual uint32_t GetMaxHP() const override;

	// Slot 69 @ +0x114: GetMaxMP [RECONSTRUCTED - 0x004A6850]
	virtual uint32_t GetMaxMP() const override;

	// [RECONSTRUCTED - Native 0x004A66F0]
	// Sets current HP clamped between 0 and GetMaxHP()
	void SetCurrentHP(uint32_t dwHP);

	// [RECONSTRUCTED - Native 0x004A6790]
	// Sets current MP clamped between 0 and GetMaxMP()
	void SetCurrentMP(uint32_t dwMP);

	// [RECONSTRUCTED - Native 0x004A8770] (85 bytes)
	// Slot 194 (+0x308): Deducts HP and MP costs for skills and actions
	virtual int32_t ConsumeResources(int32_t nHP, int32_t nMP, uint32_t dwReason = 4);
	// 4A87D0: signed offsets, alive gate, native 32-bit addition and deferred
	// vital-publication caches. The reason is accumulated only on a change.
	int32_t ApplyHealthAndManaOffset(int32_t hp, int32_t mp, uint16_t reason);

	// Slot 71 @ +0x11C: GetJobState [RECONSTRUCTED - 0x0057E210]
	// 0: None, 1: Merchant, 2: Thief, 3: Hunter
	virtual uint8_t GetJobState() const override;

	// Slot 73 @ +0x124: GetLevel [RECONSTRUCTED - 0x004A6870]
	virtual uint8_t GetLevel() const override;

	// Slot 74 @ +0x128: GetMaxLevel [RECONSTRUCTED - 0x0057E2A0]
	virtual uint8_t GetMaxLevel() const override;

	// Slot 75 @ +0x12C: GetMonsterClass [RECONSTRUCTED - 0x0057F290]
	virtual uint8_t GetMonsterClass() const override;

	// Slot 87 @ +0x15C: ShowDebugMsg [RECONSTRUCTED - 0x0066B100 / CGObjPC override]
	virtual void ShowDebugMsg(const char* pszMsg) override;

	// Slot 91 @ +0x16C: OffsetGold [RECONSTRUCTED - Native 0x0057E540 (Base) / 0x004E4B60 (PC)]
	virtual int64_t OffsetGold(int64_t nOffset, uint32_t dwReason = 9, uint32_t dwParam1 = 1, uint32_t dwParam2 = 0);

	// Slot 93 @ +0x174: OffsetSkillPoint [RECONSTRUCTED - Native 0x0057E5A0 (Base) / 0x004E4C60 (PC)]
	virtual void OffsetSkillPoint(int32_t nOffset, uint8_t byReason = 1);

	// Slot 137 @ +0x224: InquireSameItem [RECONSTRUCTED - Native 0x0057EE10 (Base) / 0x004ED680 (PC)]
	virtual uint32_t InquireSameItem(uint32_t dwStorageType, const char* szCodeName, uint32_t dwMode, uint32_t dwSlot = 0xFFFFFFFF, uint32_t dwUnk = 1);

	// Slot 140 @ +0x230: DelItem_EXT [RECONSTRUCTED - Native 0x0057EEA0 (Base) / 0x004EEB70 (PC)]
	virtual void* DelItem_EXT(uint32_t dwStorageType, uint8_t bySlot, int32_t nCount, uint16_t* pwStatus, uint8_t byReason = 3, uint32_t dwControl = 0);

	// Slot 157 @ +0x274: BackupData [RECONSTRUCTED - Native 0x0057F0E0 (Base) / 0x004E64D0 (PC)]
	virtual void BackupData(uint32_t dwFlags, uint32_t dwParam = 0);

	// [RECONSTRUCTED - Native 0x004E30D0]
	// Recomputes character mastery scaling factors (+0x1CD8 and +0x1CDC)
	void RecomputeMasteryStats();

	int64_t GetGold() const;
	uint32_t GetSkillPoints() const;

	// Slot 192 @ +0x300: GetMonsterType [RECONSTRUCTED - 0x0057F800 / Base IGObj]
	// Returns monster category/type (0 = Normal, 1 = Champion/Party, etc.); base returns 0
	virtual uint8_t GetMonsterType() const override;

	// Slot 211 @ +0x34C: OnTick [RECONSTRUCTED - 0x004A88F0]
	// Advances character timers, buffs, combat status, and movement interpolation
	virtual void OnTick(float fDeltaSec) override;
	CharacterPeriodicJobs m_periodicJobs; // native +18C, portable callback bindings

	// Slot 270 @ +0x438: IsAbilityOrPetCOS [RECONSTRUCTED - 0x004838E0]
	virtual bool IsAbilityOrPetCOS() const;

	// Slot 271 @ +0x43C: IsTradeCOS [RECONSTRUCTED - 0x00483930]
	virtual bool IsTradeCOS() const;
	bool IsCOSType() const;

	// ------------------------------------------------------------------------
	// Movement (CGObjMobile). Every one of these asserts that m_byMoveType is a valid mover index and that
	// the mover it names exists - the constructor creates both, so a live character always has them.
	// ------------------------------------------------------------------------

	// Slot 230 @ +0x398: SetAngle [RECONSTRUCTED - 0x0048BA10]
	// Besides the direction vector it remembers the angle in the move state while there is no destination.
	virtual void SetAngle(float fAngle) override;

	// Slot 231 @ +0x39C: MoveTo [RECONSTRUCTED - 0x0048B660]
	virtual int32_t MoveTo(tagObjLocation destination, uint8_t byMode);

	// Slot 300 @ +0x4B0: StopMove [RECONSTRUCTED - 0x0048B730 base / PARTIAL 0x004A9430 override]
	virtual void StopMove(int32_t bBroadcast, float fAngle);

	// Slot 301 @ +0x4B4: SetMoveCommand [RECONSTRUCTED - 0x0048B6E0]
	virtual int32_t SetMoveCommand(const tagObjMoveCommand* pCommand);

	// Slot 302 @ +0x4B8: SetMoveAngle [RECONSTRUCTED - 0x0048B770]
	virtual void SetMoveAngle(float fAngle);

	// Slot 303 @ +0x4BC: SetSpeedMode [RECONSTRUCTED - 0x0048B7C0]
	virtual void SetSpeedMode(uint8_t bySpeedMode);

	// Slot 304 @ +0x4C0: IsMoving [RECONSTRUCTED - 0x0048B880]
	virtual bool IsMoving() const;

	// Slot 305 @ +0x4C4: MoveByStep [RECONSTRUCTED - 0x0048B920]
	// Commits the displacement a mover produced this tick through MoveTo.
	virtual void MoveByStep(const SRO_Vector3D* pStep);

	// [RECONSTRUCTED - 0x0048B840] the heading the active mover is steering along
	float GetMoveAngle() const;

	// [RECONSTRUCTED - 0x0048B810] m_fMoveSpeed = the walk or the run speed, whichever the mode names
	void ApplyMoveSpeed(uint8_t bySpeedMode);
	// Native slot +4E8, 4AA410; batched parameter-source notification.
	virtual void RefreshMovementSpeeds();
	// Native slot +538 is used only after IsPlayer; overridden by CGObjPC.
	virtual void SendParameterStats() {}

	// Slot 215 @ +0x35C: InitMoveSpeeds [PARTIAL - 0x0048B4D0]
	virtual void InitMoveSpeeds();

	// Slot 308 @ +0x4D0: SetCmdSource [RECONSTRUCTED - 0x004A72B0]
	// Binds CCmdSource command pipeline at offset +0x188
	virtual int32_t SetCmdSource(CCmdSource* pCmdSource);

	// Slot 238 @ +0x3B8: CanBeAttacked [RECONSTRUCTED - 0x00482690]
	virtual int32_t CanBeAttacked() const;

	// Slot 332 @ +0x530: GetExp [RECONSTRUCTED - 0x004A68B0]
	// Returns reward experience points for killing this character
	virtual uint32_t GetExp() const;

	// Slot 344 @ +0x560: GetCollisionRadius [RECONSTRUCTED - 0x004A68C0]
	virtual int32_t GetCollisionRadius() const;

	// Slot 350 @ +0x578: GetStorageItem [RECONSTRUCTED - 0x004A68E0]
	// Retrieves item from character storage/inventory by slot index (e.g. slot 6 = weapon)
	virtual CGItemEquip* GetStorageItem(uint8_t bySlot) const;

	// Slot 358 @ +0x598: OnClientPacket [RECONSTRUCTED - 0x009BF500 (Base default: No-Op) / CGObjPC override @ 0x004ECBC0]
	// Main network packet handler for client-initiated player packets
	virtual int32_t OnClientPacket(CPacket* pPacket);

	// Slot 359 @ +0x59C: GetExpMultiplier [RECONSTRUCTED - 0x009A7330]
	// Returns reward experience multiplier (base default 1.0f)
	virtual float GetExpMultiplier() const;

	// Slot 360 @ +0x5A0: IsAttackLocked [RECONSTRUCTED - 0x004AAB40 / CGObjPC override @ 0x004EF880]
	// Checks if character is action/attack restricted by active debuffs (stun, freeze, sleep)
	virtual bool IsAttackLocked() const;

	// Slot 374 @ +0x5D8: GetTimedJobManager [RECONSTRUCTED - 0x00559C70 (Base returns nullptr) / CGObjPC override @ 0x004DDD50 (returns +0x1DFC)]
	virtual CTimedJobManager* GetTimedJobManager() { return nullptr; }

	// Slot 376 @ +0x5E0: DispatchMsg [RECONSTRUCTED - 0x004B0D70 / CGObjPC override @ 0x0050EEE0]
	// Routes SR_MSG opcodes through the message map built by 0x004B0B70.
	virtual void DispatchMsg(CMsg* pMsg);

	// Slot 377 @ +0x5E4: 0x7021 [STUB - 0x004B0EA0]
	virtual void OnMsg_7021(CMsg* pMsg);

	// Slot 378 @ +0x5E8: 0x7022 [STUB - 0x004B1210]
	virtual void OnMsg_7022(CMsg* pMsg);

	// Slot 379 @ +0x5EC: 0x7023 [STUB - 0x004B12A0]
	virtual void OnMsg_7023(CMsg* pMsg);

	// Slot 380 @ +0x5F0: 0x7024 [STUB - 0x004B1360]
	virtual void OnMsg_7024(CMsg* pMsg);

	// Slot 381 @ +0x5F4: 0x704F [STUB - 0x004B1450]
	virtual void OnMsg_704F(CMsg* pMsg);

	// Slot 382 @ +0x5F8: 0x7025 [STUB - 0x004B1750]
	virtual void OnMsg_7025(CMsg* pMsg);

	// Slot 383 @ +0x5FC: OnMsg_SkillAction [RECONSTRUCTED - 0x004B21B0]
	// Internal 0x7070 posted by the command actor: executes the skill use.
	virtual void OnMsg_SkillAction(CMsg* pMsg);

	// Slot 384 @ +0x600: 0x7091 [STUB - 0x004B1630]
	virtual void OnMsg_7091(CMsg* pMsg);

	// Slot 385 @ +0x604: OnMsg_ActionCommand [RECONSTRUCTED - 0x004B21E0]
	// CORRECTION (Claude): was TickActionSession. It is the 0x7074 message handler: ProcessCommand,
	// then the unconsumed-payload check for network messages.
	virtual void OnMsg_ActionCommand(CMsg* pMsg);

	// Slot 386 @ +0x608: 0x7034 [STUB - base 0x00825E50, CGObjPC override]
	virtual void OnMsg_7034(CMsg* pMsg);

	// [PARTIAL - Native 0x004B0DD0] default entry of the SR_MSG map
	void OnMsg_Unhandled(CMsg* pMsg);

	// Slot 387 @ +0x60C: CanPickupItem [RECONSTRUCTED - 0x004A6980 / CGObjPC override @ 0x00526AC0]
	// CORRECTION (Claude): was CanUseItem. Its only caller is the command actor's pick-up handler
	// (0x004AE0E9), which then posts 0x7034 type 6 (ground to inventory).
	virtual uint16_t CanPickupItem(uint32_t dwItemID);

	// Slot 388 @ +0x610: FilterCheck [RECONSTRUCTED - 0x004AA310]
	// Validates message against CGMsgFilter table
	virtual int32_t FilterCheck(CMsg* pMsg);

	// Slot 390 @ +0x618: Deactivate [RECONSTRUCTED - 0x004AB3D0]
	// Disarms character, clears active targets, and cancels actions before removal/destruction
	virtual void Deactivate();

	// Subsystem accessors
	CGCharAutoCommandActor* GetAutoCommandActor() { return &m_AutoCommandActor; }

	// [RECONSTRUCTED - Native 0x004AAB90] (eax = this) frozen: flag bit 0 with its record active
	bool IsFrozen() const { return (m_dwAbnormalFlags & 0x0001) != 0 && m_aAbnormalState[0].m_byActive != 0; }

	// [RECONSTRUCTED - Native 0x004AABB0] (eax = this) stunned: flag bit 14 with its record active
	bool IsStunned() const { return (m_dwAbnormalFlags & 0x4000) != 0 && m_aAbnormalState[14].m_byActive != 0; }

	// [RECONSTRUCTED - Native 0x004AABD0] (eax = this) asleep: flag bit 6 with its record active
	bool IsAsleep() const { return (m_dwAbnormalFlags & 0x0040) != 0 && m_aAbnormalState[6].m_byActive != 0; }

	// [RECONSTRUCTED - Native 0x004AAB60] (ecx = this)
	// Message-filter state of the disabling abnormal: 10 frozen, 9 stunned, 0x13 asleep, 0x15 none.
	uint8_t GetAbnormalFilterState() const {
		if (IsFrozen()) return 0x0A;
		if (IsStunned()) return 0x09;
		return IsAsleep() ? 0x13 : 0x15;
	}

	// [RECONSTRUCTED - Native 0x004AC890] (eax = this)
	// Cast range bonus: parameter 0x21 truncated to a word.
	uint16_t GetCastRangeBonus();

	// [RECONSTRUCTED - Native 0x004EC220] (eax = this)
	// Item in equipment slot 7 (arrows / bolts / shield) when it is an equipment item, else null.
	CGItem* GetEquippedAmmo() const;

	// [RECONSTRUCTED - Native 0x004EC250] (eax = this)
	// Count (slot 314) of the slot-7 item; -1 when the slot is empty.
	// CORRECTION (Claude): GetEquippedWeaponType reads the same item; it is the ammunition count.
	int32_t GetEquippedAmmoCount() const;

	// [RECONSTRUCTED - Native 0x004EAD40] (ecx = this, eax = out)
	// TID of the weapon in equipment slot 6, or 0 when the slot is empty or the item is marked (+0x190).
	uint16_t GetEquippedPrimaryWeaponTID() const;

	// [STUB - Native 0x004F10D0] (eax = this, stack dwStructureRefID)
	// Asks the navigation mesh for a point from which the fortress structure can be attacked and
	// stores it in m_StructureApproachPos (+0x2284).
	int32_t AssignStructureApproachPos(uint32_t dwStructureRefID);

	// Slot 402 @ +0x648 [STUB - Native 0x004B9AB0]
	// Reference id the structure approach search is keyed by.
	virtual uint32_t GetFortressStructureRefID() const;

	// Slot 353 @ +0x584: GetAvatarStorageItem [RECONSTRUCTED - Native 0x004A6910]
	virtual CGItem*  GetAvatarStorageItem(uint32_t dwSlot);

	// Slot 372 @ +0x5D0: IsMainWeaponUsable (Default base true, overridden by CGObjPC @ 0x004EC5A0)
	virtual bool     IsMainWeaponUsable() const;

	// Slot 399 @ +0x63C: IsSecondaryWeaponUsable (Default base true, overridden by CGObjPC @ 0x004EC5D0)
	virtual bool     IsSecondaryWeaponUsable() const;

	// Slot 400 @ +0x640: IsEquipmentSlotUsable (Default base true, overridden by CGObjPC @ 0x004EC600)
	virtual bool     IsEquipmentSlotUsable(uint32_t dwSlot) const;

	// Slot 355 @ +0x58C: CanSelectTarget [RECONSTRUCTED - Native 0x004A98A0]
	virtual bool CanSelectTarget(CGObjChar* pTarget, uint32_t dwParam);

	// Slot 393 @ +0x624: GetCombatPermission [RECONSTRUCTED - Native 0x004AA640]
	virtual uint32_t GetCombatPermission(CGObjChar* pTarget, uint32_t dwMode, uint32_t* pErrorCode);

	// Slot 336 @ +0x540: IsRidingTransport (base 0x00559C70 returns 0, CGObjPC @ 0x004DDCA0)
	// CORRECTION (Claude): was IsInvulnerableOrDead. The PC body tests the transport at +0x1D18 and
	// riding flag tagCharData +0x0E; the command actor refuses attacks and casts while it is set.
	virtual bool IsRidingTransport() const;

	// Slot 271 @ +0x43C: IsPickPetCOS [RECONSTRUCTED - Native 0x00483930]
	virtual bool IsPickPetCOS() const;

	// Slot 244 @ +0x3D0: IsFortressHeart [RECONSTRUCTED - Native 0x00482970]
	virtual bool IsFortressHeart() const;

	// Slot 405 @ +0x654: IsSiegeTargetRestricted (Default false, overridden by CGObjPC @ 0x0052BC10)
	virtual bool IsSiegeTargetRestricted() const;

	// Slot 253 @ +0x3F4: IsDropUsable1 (Default base true)
	virtual bool IsDropUsable1() const;

	// Slot 254 @ +0x3F8: IsDropUsable2 (Default base true)
	virtual bool IsDropUsable2() const;

	// Slot 335 @ +0x53C: IsGM (Default base false, overridden by CGObjPC @ 0x004E5950)
	virtual bool IsGM() const;

	// Slot 350 @ +0x578: GetTransportVehicle (Default nullptr)
	virtual CGObjChar* GetTransportVehicle(uint32_t dwSlot = 8) const;

	// Slot 261 @ +0x414: IsVehicleActive (Default false)
	virtual bool IsVehicleActive() const;

	// Consume projectile ammo (arrows/bolts) - Native 0x004EC290
	virtual bool ConsumeAmmo(uint32_t dwCount = 1) { (void)dwCount; return true; }

	// Register skill cooldown and timer - Native 0x0064C700
	virtual void RegisterSkillCooldownAndTimer(const tagRefSkill* pRefSkill) { (void)pRefSkill; }

	// Get CooltimeManager (embedded in CGObjPC at +0x1F40)
	virtual class CCooltimeManager* GetCooltimeManager() { return nullptr; }

	// Slot 393 @ +0x624: CheckCombatPermission (Native 0x004AA640)
	virtual bool CheckCombatPermission(CGObjChar* pTarget, uint32_t dwMode, uint32_t* pError) {
		(void)pTarget; (void)dwMode;
		if (pError) *pError = 0;
		return true;
	}

	virtual void Notice(uint32_t dwCode) { (void)dwCode; }

	virtual bool IsFreePVPModeActive() const { return false; }
	virtual uint8_t GetAbnormalImmunityFlags() const { return 0; }
	virtual bool IsCharacter() const { return IsChar(); }
	virtual bool IsInteractiveWorldItem() const { return false; }
	virtual uint32_t GetCombatLevel() const { return GetLevel(); }
	uint32_t GetLastAttackSkillID() const { return m_dwLastAttackSkillID; }
	CSkillManager* GetSkillManager() { return m_pSkillManager; }
	const CSkillManager* GetSkillManager() const { return m_pSkillManager; }
	void* GetParty() const { return m_pParty; }
	bool HasParty() const { return m_pParty != nullptr; }
	virtual bool SameRelation(CGObjChar* pOther) const {
		if (!pOther) return false;
		if (m_pParty != nullptr && m_pParty == pOther->m_pParty) return true;
		return false;
	}

	// Slot 158 @ +0x278: AllocMsgForPeer (overridden by CGObjPC @ 0x004E0800)
	virtual CPacket* AllocMsgForPeer(uint16_t wOpcode);

	// Slot 159 @ +0x27C: SendMsgToPeer (overridden by CGObjPC @ 0x004E0860)
	virtual int32_t SendMsgToPeer(CPacket* pPacket);

	// Slot 319 @ +0x4FC: ApplyHit [PARTIAL - Native 0x004A7500]
	// Native forwards all arguments through world-controller +0x28, then actor
	// +0x4F4 (normal worlds) or +0x4F8 (siege). Current HP subtraction does not
	// implement those policies, attribution, or downstream death callbacks.
	virtual int32_t ApplyHit(CGObjChar* pAttacker, int32_t nDamage1, int32_t nDamage2, int32_t nFlag1, int32_t nFlag2);

	// Slot 347 @ +0x56C: GetMainWeaponAttackSkillID (overridden by CGObjPC @ 0x004EAE50)
	virtual uint32_t GetMainWeaponAttackSkillID() const;

	// Helper for party ID lookup (Native 0x004EA280)
	uint32_t GetPartyID() const;

	// Broadcast packet to nearby observer sessions in the region (Native 0x00487C80 / 0x00488210)
	// CORRECTION (Claude): SendPacketToNearbySessions was declared here and only ever reached the character's
	// own session. The native has it on CGObj (0x00484D90), where it broadcasts through the message block.

	// Status and position dirty flag management (proven @ 0x004A5C73: |= 0x100)
	void SetDirtyFlags(uint16_t wFlags);
	uint16_t GetDirtyFlags() const;

	// [RECONSTRUCTED - Native 0x004FD7F0]
	// Returns active transport / COS vehicle associated with character
	CGObj* GetActiveVehicle() const;

	// [PARTIAL - Native 0x004A4270] (288 bytes)
	// Writes one rolled status into its slot and publishes the new mask. Returns 1 when it was applied.
	int32_t ApplyAbnormalStateRecord(const tagSkillStatusEffect& effect);

	// [PARTIAL - Native 0x004A4390] (465 bytes)
	// Expires the slots whose duration has run out; driven by the 0.3 s skill queue job (0x004A9977).
	void UpdateAbnormalStates();

	// [PARTIAL - Native 0x004A5660] (93 bytes)
	// Ends one running status, rebuilds the mask from the slots that are left and publishes it.
	void ClearAbnormalStateSlot(uint8_t byStatusIndex);

	// [RECONSTRUCTED - Native 0x004A5C60] (422 bytes)
	// Publishes the whole abnormal state block to the owning client as 0x30D2. Only a player has anywhere
	// to send it: slots 158 / 159 are the illegal-invocation handlers on every other character.
	void SendAbnormalStateUpdate();

public:
	// Note: m_dwGlobalID (+0x08), m_pCharData (+0x30), m_pDataPermanent (+0x34), m_dwWorldID (+0x78)
	// are inherited directly from base CGObj.
	// CORRECTION (Claude): the region and the coordinates are the tail of the navigation location block
	// CGObj::EnterWorld fills at +0x7C (cell +0x7C, mesh instance +0x80, region +0x84, x / y / z +0x88 - +0x90).
	// They used to be separate members, which left the resolved cell behind and made every navmesh query
	// (movement, line of sight) start from a null cell. The names below alias the inherited block.
	uint16_t&        m_wRegionID   = m_Location.wRegionID; // +0x84: Region ID (word)
	uint16_t&        m_wSectorID   = m_Location.wRegionID; // +0x84: Sector ID alias
	float&           m_fLocalPosX  = m_Location.fPosX;     // +0x88: Local X coordinate
	float&           m_fPosX       = m_Location.fPosX;
	float&           m_fLocalPosY  = m_Location.fPosY;     // +0x8C: Local Y coordinate (elevation)
	float&           m_fPosY       = m_Location.fPosY;
	float&           m_fLocalPosZ  = m_Location.fPosZ;     // +0x90: Local Z coordinate
	float&           m_fPosZ       = m_Location.fPosZ;
	// m_pGameWorld (+0x98), m_pGameWorldLayer (+0x9C) and m_pWorldContextController (+0xD8) are inherited
	// from CGObj, which is where CGObj::EnterWorld resolves them.
	uint8_t          m_padA0[0xAC];         // +0xA0 - +0x14B

	// CORRECTION (Claude): +0x154 is not a loose set of fields, it is one tagObjMoveCommand - the block both
	// movers bind to in CGObjMover::Init (0x0048BFC7) and the shape SetCommand copies (0x0048C140). Its first
	// byte doubles as the index into m_apMover (0x0048B891) and the angle at +0x164 used to be padding.
	CGObjMover*       m_apMover[2]; // +0x14C: [0] by command, [1] by destination (ctor 0x0048B2E3)
	tagObjMoveCommand m_MoveState;  // +0x154 - +0x16B
	float             m_fWalkSpeed; // +0x16C: refObj +0xE4 (0x0048B4F0)
	float             m_fRunSpeed;  // +0x170: refObj +0xE6 (0x0048B50F)
	float             m_fMoveSpeed; // +0x174: the one the mode picks (0x0048B81A / 0x0048B82C)
	tagCharListNode   m_listNode;   // +0x178: Intrusive global list node

	// The names the rest of the port already used for the fields of that block
	uint8_t&  m_byMovementType  = m_MoveState.m_byMoveType;  // +0x154
	uint8_t&  m_bySpeedMode     = m_MoveState.m_bySpeedMode; // +0x155
	uint16_t& m_wDestRegionID   = m_MoveState.m_wRegionID;   // +0x156
	uint16_t& m_wTargetSectorID = m_MoveState.m_wRegionID;
	int32_t&  m_nDestPosX       = m_MoveState.m_nDestX;      // +0x158
	int32_t&  m_nDestPosY       = m_MoveState.m_nDestY;      // +0x15C
	int32_t&  m_nDestPosZ       = m_MoveState.m_nDestZ;      // +0x160
	float&    m_fMoveAngle      = m_MoveState.m_fAngle;      // +0x164
	uint8_t&  m_byMovementFlags = m_MoveState.m_byFlags;     // +0x168

	// CORRECTION (Claude): both are embedded objects, not heap pointers (0x004A6BBC, 0x004A89D5).
	CGCharAutoCommandActor m_AutoCommandActor;  // +0x1BC0
	CGCharAutoNavigator    m_AutoNavigator;     // +0x1BF4
	tagObjLocation         m_StructureApproachPos; // +0x2284: set by AssignStructureApproachPos

	// Proven native character subsystem and state pointers:
	CCmdSource*      m_pNetSession;         // +0x188: Pointer to network session interface (CCmdSource/CCmdSrcNet)
	tagActiveSkillInstance* m_pActiveCastInstance = nullptr; // +0x1E8: Active cast instance pointer
	CGParamKeeper    m_paramKeeper;         // +0x1EC: Character parameter & stat keeper (native @ +0x1EC)
	uint32_t m_dwPublishedHP = 0;           // +0xA00
	uint32_t m_dwPublishedMP = 0;           // +0xA04
	uint16_t         m_wStatusDirtyFlags = 0; // +0xA0E: Status and position dirty bitmask (proven @ 0x004A5C73: |= 0x100)
	uint8_t          m_bPositionDirty = 0;    // +0xA0E: Position / movement sync dirty flag
	CSkillManager*   m_pSkillManager;       // +0xA30: CSkillManager instance pointer (embedded @ +0xA30 in native binary)
	uint32_t         m_dwPathCheckFlag;     // +0xC04: Path movement check flag (Native 0x005862E0)
	// CORRECTION (Claude): +0xC08 is the skill instance being cast, not a vehicle. CGObjChar::OnTick
	// (0x004A8996) and IsAttackLocked (0x004AAB40) read its execution (+0x18) and reference (+0x08).
	tagActiveSkillInstance* m_pCastingInstance = nullptr; // +0xC08
	void*            m_pBuffManager;        // +0xC08: Buff manager pointer
	tagActiveSkillInstance* m_pAttackState; // +0xC10: continuous attack instance (0x004ADD47)
	tagTargetCandidate* m_pRestrictedActionTarget = nullptr; // +0xC18: Restricted cast/action target (verified @ 0x0058CC70)
	uint32_t         m_dwFieldC34 = 0;      // +0xC34: Continuous channeling damage state (Native 0x0058721F)
	uint8_t          m_bActionLocked;       // +0xC44: Stun / action lock flag
	uint32_t         m_dwLastAttackSkillID = 0; // +0xD1C: Last attack skill ID (verified @ 0x0058CC70)
	uint32_t         m_dwAbnormalFlags = 0;     // +0xD34: Active abnormal condition bitmask (verified @ 0x0058CC70)
	tagAbnormalStateSlot m_aAbnormalState[32] = {}; // +0xD3C (the block is +0xD30: owner, mask, then the slots)
	void*            m_pAbnormalStatus = nullptr; // +0x112C: Abnormal status descriptor (verified @ 0x0050FD60)
	CGStorage        m_storage;             // +0x1C10: Character storage / inventory container
	CGStorage        m_avatarStorage;       // +0x1C34: Avatar inventory container
	uint32_t         m_dwTotalSlots;        // +0x1C30: Total slots count in storage
	void*            m_pParty;              // +0x1CB8: Pointer to party object
	CGObjChar*       m_pOwnerChar = nullptr;// +0x1CD8: Owning player actor pointer for pets/COS (verified @ 0x0058CC70)
	union {
		uint8_t  m_byMobSubtype;        // +0x1CD8: Monster subtype (flags & 0x0F: 6 = Champion/Giant)
		float    m_fExpMultiplier;      // +0x1CD8: Player experience gain multiplier (default 1.0f)
	};

	CGObj*           m_pActiveVehicle = nullptr; // +0x1CE8: Active transport / COS vehicle pointer
	uint32_t         m_dwActiveVehicleOwner = 0; // +0x1D18: NPC / vehicle spawn descriptor
	uint64_t         m_nCargoRawGold = 0;        // Total cargo price * count accumulation
	uint32_t         m_dwCargoTotalCount = 0;    // Total cargo goods count

	void             SetActiveVehicle(CGObj* pVehicle);
	void             SetCargoStats(uint64_t nRawGold, uint32_t dwTotalCount);
	uint64_t         CalculateCargoRawGoldValue() const;
	uint32_t         GetCargoTotalCount() const;
	uint8_t          GetCOSRarity() const;
	uint16_t         GetCOSSlotCapacityFromRef() const;

	void TeleportToCoordinates(uint16_t wRegion, float fX, float fY, float fZ) {
		m_wRegionID = wRegion;
		m_fPosX = fX;
		m_fPosY = fY;
		m_fPosZ = fZ;
	}
	void CancelActionSession(uint8_t bySlot, uint32_t dwSkillID) { (void)bySlot; (void)dwSkillID; }
	void RemoveActiveSkillInstance(void* pInstance) { (void)pInstance; }
	bool IsArmorSlotEquipped(uint32_t dwSlot) const { (void)dwSlot; return true; }
	bool IsSecondaryWeaponEquipped() const { return IsSecondaryWeaponUsable(); }
	bool IsMainWeaponEquipped() const { return IsMainWeaponUsable(); }
	CGItem* GetEquippedItemBySlot(uint32_t dwSlot) { return GetAvatarStorageItem(dwSlot); }

	void*          GetNetSession() const;
	CCmdSource*    GetCmdSource() const;
	void*          GetBuffManager() const;
	tagActiveSkillInstance* GetAttackState() const;

	bool           IsPC() const;
	uint32_t       GetHP() const;
	uint8_t        GetJobType() const;

	// Native 0x0059EF90 & 0x0059EFF0: Active buff search and deactivation
	bool     CancelActiveBuff(uint32_t dwRefSkillID, uint32_t dwRecordSkillID = 0);
};

inline void CGObjChar::SetDirtyFlags(uint16_t wFlags) {
	m_wStatusDirtyFlags |= wFlags;
}

inline uint16_t CGObjChar::GetDirtyFlags() const {
	return m_wStatusDirtyFlags;
}

// Global Character List Registry (Native 0x00C825F4 - 0x00C82604)
extern uint32_t         g_dwCharacterCount;          // @ 0x00C825F4
extern tagCharListNode* g_pCharacterListHead;        // @ 0x00C825F8
extern tagCharListNode* g_pCharacterListTail;        // @ 0x00C825FC
extern tagCharListNode* g_pCharacterListCurrent;     // @ 0x00C82600
extern uint32_t         g_dwCharacterListIterFlags;  // @ 0x00C82604

// [RECONSTRUCTED - 0x004AB460]
void CGObjChar_AddToList(tagCharListNode* pNode);

// [RECONSTRUCTED - 0x004AB4B0]
void CGObjChar_RemoveFromList(tagCharListNode* pNode);

// [RECONSTRUCTED - 0x004A8AD0]
// Pumps and dispatches queued incoming network messages for an active character session
uint32_t CGObjChar_PumpNetworkMsg(CGObjChar* pChar);

#endif // _SR_GAMESERVER_GOBJCHAR_H_
