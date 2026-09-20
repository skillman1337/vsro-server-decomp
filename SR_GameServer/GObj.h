/**
 * ============================================================================
 * Silkroad Online - Game Object Base Class
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObj.h
 *
 * Implements CGObj and IGObj:
 *   - VTable @ 0x00AE7C44
 *   - RTTI: .?AVCGObj@@ -> .?AVIGObj@@
 *   - CGObj::GetTID @ 0x00485C90 (66 bytes)
 *   - CGObj::GetTypeID @ 0x00485CE0 (19 bytes)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJ_H_
#define _SR_GAMESERVER_GOBJ_H_

#include <cstdint>
#include "ReferenceData.h"
#include "GlobalPos.h"
#include "../ServerCommon/InstanceChar.h"

namespace BSLib {
class CPacket;
}
typedef BSLib::CPacket CPacket;

class CMsgBlock;

class CGame;
extern CGame* g_pGame;

/**
 * 16-bit TypeID bitfield descriptor (Silkroad TID)
 * Proven against machine instructions in 0x00482530 - 0x00482800:
 *   - Bits 0..1 (0x02): Valid entity flag
 *   - Bits 2..4 (0x1C): Class (0x04 = Character)
 *   - Bits 5..6 (0x60): Character Category (0x20 = Player, 0x40 = Non-Player)
 *   - Bits 7..10 (0x780): Non-Player Subcategory (0x080 = Monster, 0x100 = NPC, 0x180 = COS)
 *     CORRECTION (Claude): the legend had 0x080/0x100 swapped. vftable slot 10 @ 0x00482600 tests
 *     0x80 and gates monster rarity in AI::CNest::Hatch; slot 9 @ 0x004825C0 tests 0x100 and is the
 *     NPC early-out in AI::CNest::OnMonsterDead. The IsMonster/IsNPC bodies below were already right.
 *   - Bits 11..15 (0xF800): Specific Subtypes (e.g. 0x800 = Mount, 0x1000 = Growth Pet, 0x2800 = Fellow)
 */
union tagTID {
	uint16_t wType;
	struct {
		uint16_t bValid     : 2; // Bits 0..1
		uint16_t byClass    : 3; // Bits 2..4
		uint16_t byCharType : 2; // Bits 5..6
		uint16_t bySubClass : 4; // Bits 7..10
		uint16_t bySubType  : 5; // Bits 11..15
	};

	constexpr tagTID() : wType(0) {}
	constexpr explicit tagTID(uint16_t val) : wType(val) {}

	inline bool IsValid() const { return (wType & 0x02) != 0; }
	inline bool IsChar() const { return IsValid() && ((wType & 0x1C) == 0x04); }
	inline bool IsPlayer() const { return IsChar() && ((wType & 0x60) == 0x20); }
	inline bool IsNonPlayer() const { return IsChar() && ((wType & 0x60) == 0x40); }
	inline bool IsMonster() const { return IsNonPlayer() && ((wType & 0x780) == 0x80); }
	inline bool IsNPC() const { return IsNonPlayer() && ((wType & 0x780) == 0x100); }
	inline bool IsStructure() const { return IsNonPlayer() && ((wType & 0x780) == 0x100); }
	inline bool IsCOS() const { return IsNonPlayer() && ((wType & 0x780) == 0x180); }
};

/**
 * [RECONSTRUCTED - 0x00561020] (64 bytes, out-of-line header inline; callers 0x005608EB, 0x0058CC70)
 * TID_IsMonsterTypeID4_4
 * Bionic character NPC with TypeID3 1 (monster) and TypeID4 4 ((TID & 0xF800) == 0x2000).
 * AI::CNest::Hatch refuses the party rarity (0x10) for it. What TypeID4 4 denotes is not proven.
 */
inline bool TID_IsMonsterTypeID4_4(const uint16_t* pwTypeID) {
	tagTID tid(*pwTypeID);
	return tid.IsMonster() && (tid.wType & 0xF800) == 0x2000;
}

/**
 * Interface for game objects (Native RTTI: .?AVIGObj@@)
 */
class IGObj {
public:
	virtual ~IGObj() = default;
	virtual uint32_t GetRefObjID() const = 0; // Slot 1 (+0x04)
	virtual uint32_t GetJID() const = 0;       // Slot 2 (+0x08)
	virtual uint32_t GetGameID() const = 0;    // Slot 3 (+0x0C)
	virtual uint16_t GetTypeID() const = 0;    // Slot 4 (+0x10)
};

// Character status and mode descriptor pointed to by +0x30
struct tagCharData {
	uint8_t m_byLifeState;    // +0x00: Life State (0: Embryo, 1: Alive, 2: Dead, 3: Gone)
	uint8_t m_pad01;          // +0x01
	uint8_t m_byMotionState;  // +0x02: Motion State (0: Stand, 1: Skill, 2: Walk, 3: Run)
	uint8_t m_byBodyMode;     // +0x03: Body Mode (0: Normal, 1: Hwan, 2: Invincible, 3: Invisible, 4: Berserker)
	uint8_t m_byMsgProcState; // +0x04: Msg Proc State (0: Normal, 1: Blocked, 2: Overlapped)
	uint8_t m_pad05;          // +0x05
	uint8_t m_byInteractMode; // +0x06: Interact Mode (0: None, 1: Handshaking, 2: P2P, etc.)
	uint8_t m_byGroupMode;    // +0x07: Group Mode (0: Solo, 1: Party Setup, 2: Party, etc.)
	uint8_t m_pad08[4];       // +0x08 - +0x0B
	uint8_t m_byPvPState;     // +0x0C: PvP State (0: Neutral, 1: Assaulter, 2: Murderer)
	uint8_t m_byBattleState;  // +0x0D: Battle State (0: Peace, 1: Battle)
	uint8_t m_byTransportState;  // +0x0E: 1 while riding a transport (CGObjPC slot 336, 0x004DDCA0)
	uint8_t m_pad0F;             // +0x0F
	uint8_t m_byTeleportState;   // +0x10: slot 72 GetTeleportState (CGObjPC 0x004DDC90); message filter group 11
	uint8_t m_byFilterState11;   // +0x11: message filter group 12 (0x00429844)
	uint8_t m_byFilterState12;   // +0x12: message filter group 13 (0x0042986F)
	uint8_t m_bySpawnInvincible; // +0x13: CGObjPC::SetSpawnInvincible (0x004E0C73)
	uint8_t m_byFilterState14;   // +0x14: message filter group 15 (0x0042989A)
};

/**
 * Base class for all game objects (Native RTTI: .?AVCGObj@@)
 * Native VTable @ 0x00AE7C44 (size 0x64 = 25 slots)
 */
class CGObj : public IGObj {
public:
	CGObj();
	virtual ~CGObj() override;

	// Slot 1 (+0x04) @ 0x004824F0
	virtual uint32_t GetRefObjID() const override;

	// Slot 2 (+0x08) @ 0x0057D4B0
	virtual uint32_t GetJID() const override;

	// Slot 3 (+0x0C) @ 0x00404050
	virtual uint32_t GetGameID() const override;
	uint32_t GetGlobalID() const;
	inline uint32_t GetUniqueID() const { return GetGlobalID(); }

	// Slot 4 (+0x10) @ 0x00485CE0
	virtual uint16_t GetTypeID() const override;

	// Slot 5 (+0x14) @ 0x00482510
	virtual uint32_t GetWorldID() const;

	// Slot 6 (+0x18) @ 0x00482530: IsChar
	virtual bool IsChar() const;

	// Slot 7 (+0x1C) @ 0x00482560: IsPlayer
	virtual bool IsPlayer() const;

	// Slot 8 (+0x20) @ 0x00482590: IsNonPlayer
	virtual bool IsNonPlayer() const;

	// Slot 9 (+0x24) @ 0x004825C0: IsNPC / IsStructure (TID 0x100)
	virtual bool IsNPC() const;
	virtual bool IsStructure() const;

	// Slot 10 (+0x28) @ 0x00482600: IsMonster / IsMob (TID 0x80)
	virtual bool IsMonster() const;

	// Slot 11 (+0x2C) @ 0x004827B0: IsCOS
	virtual bool IsCOS() const;

	// Slot 12 (+0x30) @ 0x004827F0: IsActiveVehicle
	virtual bool IsActiveVehicle() const;

	// Slot 13 (+0x34) @ 0x00482840: IsAttackCOS - the COS kind that fights alongside its owner.
	// The bytes test TID valid, class 4, category 0x40, subcategory 0x180 and specific 0x800;
	// SkillCombat_RollAbnormalStatus refuses every abnormal status on such a target (0x005906A6).
	virtual bool IsAttackCOS() const;

	// Slot 59 (+0xEC) @ 0x0057E060: GetName / GetCharName (Illegal invocation handler on base IGObj)
	virtual const char* GetName() const;

	// Slot 62 (+0xF8) @ 0x00485EE0: GetLifeState (0: Embryo, 1: Alive, 2: Dead, 3: Gone/Despawn)
	virtual uint8_t GetLifeState() const;

	// Slot 63 (+0xFC) @ 0x0057E120: GetMotionState (Illegal invocation handler)
	virtual uint8_t GetMotionState() const;

	// Slot 64 (+0x100) @ 0x0057E150: GetBodyMode (Illegal invocation handler)
	virtual uint8_t GetBodyMode() const;

	// Slot 65 (+0x104) @ 0x0057E180: GetParamFloat (Illegal invocation handler)
	virtual float GetParamFloat(uint32_t dwParamID) const;

	// Slot 66 (+0x108) @ 0x0057E1B0: GetCurrentHP (Illegal invocation handler)
	virtual uint32_t GetCurrentHP() const;

	// Slot 67 (+0x10C) @ 0x0057E1E0: GetCurrentMP (Illegal invocation handler)
	virtual uint32_t GetCurrentMP() const;

	// Slot 68 (+0x110) @ 0x0057E1B0: GetMaxHP (Illegal invocation handler)
	virtual uint32_t GetMaxHP() const;

	// Slot 69 (+0x114) @ 0x0057E1E0: GetMaxMP (Illegal invocation handler)
	virtual uint32_t GetMaxMP() const;

	// Slot 71 (+0x11C) @ 0x0057E210: GetJobState (Illegal invocation handler)
	virtual uint8_t GetJobState() const;

	// Slot 72 (+0x120) @ 0x0057E240: GetTeleportState (Illegal invocation handler)
	virtual uint8_t GetTeleportState() const;

	// Slot 73 (+0x124) @ 0x0057E270: GetLevel (Illegal invocation handler)
	virtual uint8_t GetLevel() const;

	// Slot 74 (+0x128) @ 0x0057E2A0: GetMaxLevel (Illegal invocation handler)
	virtual uint8_t GetMaxLevel() const;

	// Slot 75 (+0x12C) @ 0x0057F290: GetMonsterClass (Illegal invocation handler)
	virtual uint8_t GetMonsterClass() const;

	// Slot 87 (+0x15C) @ 0x0057E480: ShowDebugMsg / OutputChatMsg
	virtual void ShowDebugMsg(const char* pszMsg);

	// Slot 186 (+0x2E8) @ 0x0057F6B0: IHaveTradeItem (Illegal invocation handler)
	virtual bool IHaveTradeItem() const;

	// Slot 192 (+0x300) @ 0x0057F800: GetMonsterType (Illegal invocation handler)
	virtual uint8_t GetMonsterType() const;

	// Slot 211 (+0x34C) @ 0x00484D10: Base entity simulation tick
	virtual void OnTick(float fDeltaSec);

	// Slot 18 (+0x48) @ 0x00482D80: IsItem (TID bit 1 clear, class 3)
	// CORRECTION (Claude): BN labelled 0x00482D80 CGObj_IsSkillObject; the command actor's pick-up
	// path (0x004AD12E) uses it to validate a ground item.
	virtual bool IsItem() const;

	// Slot 248 (+0x3E0) @ 0x00482AB0: IsFortressStructure (character, NPC class, subclass 5)
	// CORRECTION (Claude): was IsItemDrop. The test is TID class 1 / type NPC / subclass 5, the fortress
	// structure kind; the command actor engages such targets from the approach point at +0x2284.
	virtual bool IsFortressStructure() const;

	// Native @ 0x00485C90 (66 bytes): GetTID
	tagTID GetTID() const;

	// Accessor to permanent instance data (+0x34)
	inline CInstanceChar* GetDataPermanent() const { return m_pDataPermanent; }

	// Tactics / Nest index accessor @ +0x2C
	uint32_t GetTacticsIndex() const;

	// Authentic lifecycle methods
	bool     Spawn(uint32_t dwGameID, CInstanceChar* pDataPermanent, uint32_t dwWorldID); // [0x00484B80]

	// [0x00485360] dwWorldID packs the game world in its low word and the layer in its high word, the way it is
	// stored at +0x78; a location without a cell is resolved against the navmesh.
	bool     EnterWorld(uint32_t dwWorldID, const tagObjLocation& location, float fAngle);

	// Slot 231 (+0x39C) @ 0x00485740: byMode 7 places the object at the destination, any other mode walks
	// the navmesh to it. Returns 0 when the move fails, 2 when it was not blocked and 1 when it was.
	// CORRECTION (Claude): a virtual slot - CGObjChar overrides it (0x0048B660) to report the stop.
	virtual int32_t MoveTo(tagObjLocation destination, uint8_t byMode);

	// [0x004858E0] commits a reached position and moves the object between region cell nodes
	int32_t  StepMovement(const tagObjLocation& location, uint8_t byMode);

	// Slot 236 (+0x3B0) @ 0x00485BB0: hands the object from the message block it stands in over to another
	// one. CORRECTION (Claude): the native takes three arguments (retn 0xC) - the new block, the mode slot
	// 299 is told about, and the mode the two blocks are told about (0x00485BD4).
	int32_t  SetCellNode(CMsgBlock* pNewBlock, int32_t nMode299, int32_t nEnterMode);
	const char* GetCodeName() const;                                                      // [0x004824C0]
	uint32_t GetCountry() const;                                                          // [0x00482490]

	// Slot 230 (+0x398) @ 0x00482440: puts the object on a heading - m_fDirX / m_fDirY / m_fDirZ.
	// CORRECTION (Claude): this is a virtual slot, not a plain member. CGObjChar overrides it (0x0048BA10)
	// to also remember the angle in the movement block, and every CGObjMover reaches the owner through it.
	virtual void SetAngle(float fAngle);

	// [RECONSTRUCTED - 0x00485C10] (11 bytes) the region word out of the navigation location (+0x84)
	uint16_t GetRegionID() const { return m_Location.wRegionID; }

	// The game world layer, the high word of m_dwWorldID. Every message block indexes its layer table with
	// it straight off +0x7A (0x005340E4, 0x0053434C, 0x00533F7A).
	uint16_t GetLayerID() const { return static_cast<uint16_t>(m_dwWorldID >> 16); }

	// [RECONSTRUCTED - 0x00484D90] (81 bytes) hands the packet to every player sharing this object's cell
	// node and its neighbours. CGObjChar used to declare it; the native has it on CGObj.
	int32_t SendPacketToNearbySessions(CPacket* pPacket);

	// [RECONSTRUCTED - 0x00485C20] (98 bytes) the heading that points from here at a position in wRegion
	float    GetAngleToPosition(uint16_t wRegion, const SRO_Vector3D* pPos) const;

	// Authentic object capability and permission checks (tagRefObjCommon + 0x8C)
	bool CanTrade() const;       // [0x00483D50]
	bool CanSell() const;        // [0x00483D80]
	bool CanStore() const;       // [0x00483DB0]
	bool CanExchange() const;    // [0x00483DE0]
	bool CanPK() const;          // [0x00483E10]
	bool CanRepair() const;      // [0x00483E50]
	bool CanEnchant() const;     // [0x00483E80]
	bool CanAlchemy() const;     // [0x00483EB0]
	bool CanSocket() const;      // [0x00483EE0]
	bool CanEquip() const;       // [0x00483F10]
	bool CanUse() const;         // [0x00483F40]
	bool CanThrow() const;       // [0x00483F70]
	bool CanReinforce() const;   // [0x00483FA0]
	bool CanDeconstruct() const; // [0x00483FD0]

	// Extended subtype discriminators
	bool IsFellowCOS() const;              // [0x00483980]
	bool IsFortressSmallTower() const;      // [0x004832A0]
	bool IsFortressBigTower() const;        // [0x004832F0]
	bool IsFortressCommandTower() const;    // [0x00483340]
	bool IsFortressShieldStructure() const; // [0x00483390]
	bool IsOuterGateObject() const;         // [0x00483530]
	bool IsInnerGateObject() const;         // [0x00483580]
	bool IsGuildWarFlagObject() const;      // [0x00483710]
	bool IsBattleCampFlagObject() const;    // [0x00483760]

public:
	union {
		uint32_t m_dwGameID;       // +0x08: Runtime Unique Entity Game ID
		uint32_t m_dwGlobalID;     // +0x08: Global Entity ID alias
	};
	uint8_t         m_pad0C[0x18];    // +0x0C - +0x23
	float           m_fDirX;          // +0x24: Direction cos(angle)
	float           m_fDirY;          // +0x28: Direction 0.0f
	float           m_fDirZ;          // +0x2C: Direction sin(angle)
	uint32_t        m_dwTacticsIndex = 0; // +0x2C: Tactics / nest slot index
	tagCharData*    m_pCharData;      // +0x30: Specialized state block
	CInstanceChar*  m_pDataPermanent; // +0x34: Pointer to permanent data descriptor
	uint32_t        m_dwMoveQueryFlag; // +0x38: the flag CGObj::MoveTo passes to QueryMovement (0x0048578B)
	uint8_t         m_pad3C[0x3C];    // +0x3C - +0x77
	uint32_t        m_dwWorldID;      // +0x78: World ID

	// +0x7C: the navigation location CGObj::EnterWorld resolves and CGObj::MoveTo advances - navmesh cell
	// (+0x7C), mesh instance (+0x80), region (+0x84) and the position (+0x88 - +0x90). CGObjChar names the
	// last four fields m_wRegionID / m_fPosX / m_fPosY / m_fPosZ.
	tagObjLocation  m_Location;

	// +0x98 / +0x9C / +0xD8: the game world, its layer and the world context controller CGObj::EnterWorld
	// resolves (0x004854A8 / 0x004854DF / 0x00485505). CGObjChar used to declare the layer itself.
	class CGameWorld*      m_pGameWorld = nullptr;             // +0x98
	class CGameWorldLayer* m_pGameWorldLayer = nullptr;        // +0x9C
	void*                  m_pWorldContextController = nullptr; // +0xD8

	// +0x128 / +0x12C: the region the object is registered in and the message block it stands in inside that
	// region (0x004859A2 - 0x00485A85). Cleared when the object is placed by CGObj::MoveTo mode 7.
	// CORRECTION (Claude): +0x12C was a void*; every use of it dispatches through CMsgBlock's vtable
	// (0x00484DBC slot 6, 0x00485BC9 slot 3, 0x00485BE0 slot 2, 0x00485995 slot 4).
	class CRegion*         m_pRegion = nullptr;                // +0x128
	CMsgBlock*             m_pMsgBlock = nullptr;              // +0x12C

	tagCharData*    GetCharData() const { return m_pCharData; }
};

// Global object manager lookup functions matching native
class CGObjChar;
CGObjChar* ObjMgr_FindByID(uint32_t dwObjectID);         // [0x00485D90]
CGObj*     ObjMgr_FindByUniqueID(uint32_t dwUniqueID);   // [0x00485DF0]
CGObj*     ObjMgr_FindByName(const char* pszName);       // [0x00485E20]
bool       ObjMgr_DestroyObject(CGObj* pObj);             // [0x00485E50]

// Intrusive object list node embedded at offset +0x14C in CGSkillObject, CGEventDaemon, CGObjStruct
struct tagObjListNode {
	void*           pOwner;  // +0x00: Owning CGObj pointer
	int32_t         bInList; // +0x04: 1 if registered in global list, 0 if not
	tagObjListNode* pPrev;   // +0x08: Previous node
	tagObjListNode* pNext;   // +0x0C: Next node
};

/**
 * Active Skill Object (Projectiles, Field Effects, Summons)
 * Native VTable @ 0x00AE8804
 */
class CGSkillObject : public CGObj {
public:
	CGSkillObject();
	virtual ~CGSkillObject() override;
	virtual uint8_t GetLifeState() const override;
	virtual void OnTick(float fDeltaSec) override; // Slot 211 (+0x34C) [RECONSTRUCTED - 0x0048CEA0]

public:
	uint8_t        m_pad94[0xB8]; // +0x94 - +0x14B (+0x7C - +0x93 is CGObj::m_Location)
	tagObjListNode m_listNode;    // +0x14C: Intrusive global list node
};

/**
 * Active Event Daemon (World / Quest / System Event Listeners)
 * Native VTable @ 0x00B023C4
 */
class CGEventDaemon : public CGObj {
public:
	CGEventDaemon();
	virtual ~CGEventDaemon() override;
	virtual uint8_t GetLifeState() const override;
	virtual void OnTick(float fDeltaSec) override; // Slot 211 (+0x34C) [RECONSTRUCTED - 0x00611EB0]

public:
	uint8_t        m_pad94[0xB8]; // +0x94 - +0x14B (+0x7C - +0x93 is CGObj::m_Location)
	tagObjListNode m_listNode;    // +0x14C: Intrusive global list node
};

/**
 * Active World Structure (Fortress Siege Gates, Towers, Static Objects)
 * Native VTable @ 0x00AF7534
 */
class CGObjStruct : public CGObj {
public:
	CGObjStruct();
	virtual ~CGObjStruct() override;
	virtual uint8_t GetLifeState() const override;
	virtual void OnTick(float fDeltaSec) override; // Slot 211 (+0x34C) [RECONSTRUCTED - 0x00484D10]

public:
	uint8_t        m_pad94[0xB8]; // +0x94 - +0x14B (+0x7C - +0x93 is CGObj::m_Location)
	tagObjListNode m_listNode;    // +0x14C: Intrusive global list node
};

// Global entity intrusive lists
extern tagObjListNode* g_pSkillObjectListHead;        // @ 0x00C825E0
extern tagObjListNode* g_pSkillObjectListCurrent;     // @ 0x00C825E8
extern uint32_t        g_dwSkillObjectListIterFlags;  // @ 0x00C825EC

extern tagObjListNode* g_pEventDaemonListHead;        // @ 0x00C826E4
extern tagObjListNode* g_pEventDaemonListCurrent;     // @ 0x00C826EC
extern uint32_t        g_dwEventDaemonListIterFlags;  // @ 0x00C826F0

extern tagObjListNode* g_pObjStructListHead;          // @ 0x00C82610
extern tagObjListNode* g_pObjStructListCurrent;       // @ 0x00C82618
extern uint32_t        g_dwObjStructListIterFlags;    // @ 0x00C8261C

#endif // _SR_GAMESERVER_GOBJ_H_
