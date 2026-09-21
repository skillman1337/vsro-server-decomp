/**
 * ============================================================================
 * Silkroad Online - Game Object Character Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjChar.cpp
 *
 * Implements:
 *   - CGObjChar methods
 *   - Global character registry @ 0x00C825F4 - 0x00C82604
 *   - CGObjChar_AddToList @ 0x004AB460
 *   - CGObjChar_RemoveFromList @ 0x004AB4B0
 * ============================================================================
 */

#include "GObjChar.h"
#include "GObjPC.h"
#include "Game.h"
#include "../JMX_Library/BSLib/Packet.h"
#include "../JMX_Library/BSLib/Msg.h"
#include "../JMX_ServerFramework/ServerFramework/ServerProcessBase.h"
#include "GCharAutoCommandActor.h"
#include "SkillManager.h"
#include "GItemEquip.h"
#include "Common/Framework/CmdSource.h"
#include "GMsgFilter.h"
#include "../ServerCommon/ReferenceData.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstring>

// Global Character List Registry (Native 0x00C825F4 - 0x00C82604)
uint32_t         g_dwCharacterCount         = 0;       // @ 0x00C825F4
tagCharListNode* g_pCharacterListHead       = nullptr; // @ 0x00C825F8
tagCharListNode* g_pCharacterListTail       = nullptr; // @ 0x00C825FC
tagCharListNode* g_pCharacterListCurrent    = nullptr; // @ 0x00C82600
uint32_t         g_dwCharacterListIterFlags = 0;       // @ 0x00C82604

/**
 * [RECONSTRUCTED - 0x004AB460]
 * CGObjChar_AddToList
 *
 * Enqueues a character node to the intrusive doubly linked list:
 *   - Increments g_dwCharacterCount
 *   - Sets head and tail if first node
 *   - Links to tail and updates g_pCharacterListTail
 */
void CGObjChar_AddToList(tagCharListNode* pNode) {
	if (!pNode || pNode->bInList) {
		return;
	}

	pNode->bInList = 1;
	g_dwCharacterCount++;

	if (!g_pCharacterListHead) {
		g_pCharacterListHead = pNode;
		g_pCharacterListTail = pNode;
		pNode->pPrev = nullptr;
		pNode->pNext = nullptr;
		return;
	}

	tagCharListNode* pOldTail = g_pCharacterListTail;
	if (pOldTail) {
		pOldTail->pNext = pNode;
	}
	g_pCharacterListTail = pNode;
	pNode->pPrev = pOldTail;
	pNode->pNext = nullptr;
}

/**
 * [RECONSTRUCTED - 0x004AB4B0]
 * CGObjChar_RemoveFromList
 *
 * Safely unlinks a character node from the intrusive list:
 *   - If pNode == g_pCharacterListCurrent, updates iterator & sets flag bit 0
 *   - Decrements g_dwCharacterCount
 *   - Updates head and tail links
 */
void CGObjChar_RemoveFromList(tagCharListNode* pNode) {
	if (!pNode || !pNode->bInList) {
		return;
	}

	pNode->bInList = 0;

	// Check if active iterator is pointing to this node
	if (pNode == g_pCharacterListCurrent) {
		g_dwCharacterListIterFlags |= 1;
		g_pCharacterListCurrent = pNode->pNext;
	}

	if (g_dwCharacterCount > 0) {
		g_dwCharacterCount--;
	}

	tagCharListNode* pPrev = pNode->pPrev;
	tagCharListNode* pNext = pNode->pNext;

	if (!pPrev) {
		g_pCharacterListHead = pNext;
	} else {
		pPrev->pNext = pNext;
	}

	if (pNext) {
		pNext->pPrev = pPrev;
	} else {
		g_pCharacterListTail = pPrev;
	}

	pNode->pNext = nullptr;
	pNode->pPrev = nullptr;
}

// ============================================================================
// CGObjChar Methods
// ============================================================================

CGObjChar::CGObjChar()
	: CGObj()
	, m_asyncJobs([] { return GetTickCount(); })
	, m_MoveState()
	, m_fWalkSpeed(0.0f)
	, m_fRunSpeed(0.0f)
	, m_fMoveSpeed(0.0f)
	, m_AutoCommandActor()
	, m_AutoNavigator()
	, m_StructureApproachPos()
	, m_pNetSession(nullptr)
	, m_paramKeeper()
	, m_bPositionDirty(0)
	, m_pSkillManager(nullptr)
	, m_pBuffManager(nullptr)
	, m_pAttackState(nullptr)
	, m_bActionLocked(0)
	, m_storage(45)
	, m_dwTotalSlots(45)
	, m_pParty(nullptr)
	, m_fExpMultiplier(1.0f)
	, m_pActiveVehicle(nullptr)
	, m_dwActiveVehicleOwner(0)
	, m_nCargoRawGold(0)
	, m_dwCargoTotalCount(0) {
	std::memset(m_padA0, 0, sizeof(m_padA0));

	// 0x0048B2D5 - 0x0048B36C: the CGObjMobile constructor creates one mover of each kind, in this order,
	// and binds each to this character. m_byMoveType picks which one the tick drives; a fresh character is
	// steered (CGObjMobile::Clear 0x0048B468 writes the same 0) and standing still.
	m_apMover[OBJ_MOVE_BY_CMD]  = new CGObjMoverByCmd();  // 0x0048B344 (0x10 bytes)
	m_apMover[OBJ_MOVE_BY_DEST] = new CGObjMoverByDest(); // 0x0048B30D (0x20 bytes)
	for (int32_t i = 0; i < 2; ++i) {
		m_apMover[i]->Init(this); // 0x0048B364: slot 1
	}
	m_MoveState.m_byMoveType = OBJ_MOVE_BY_CMD; // 0x0048B468
	m_bySpeedMode = 1;                          // 0x0048B474: a character is dropped in running

	m_paramKeeper.SetOwner(this);

	m_listNode.pOwner  = this;
	m_listNode.bInList = 0;
	m_listNode.pPrev   = nullptr;
	m_listNode.pNext   = nullptr;

	m_AutoCommandActor.SetOwner(this);
	m_AutoNavigator.SetOwner(this);

	m_pSkillManager = new CSkillManager();
	m_pSkillManager->SetOwner(this);
	m_periodicJobs.Add(0.300000012f, [this] {
		m_pSkillManager->OnTick(0.300000012f);
		UpdateAbnormalStates();
	});
	// 4A6E29..4A6E3E: async actor jobs follow skill queues at the same cadence.
	m_periodicJobs.Add(0.300000012f, [this] { m_asyncJobs.Advance(); });
}

CGObjChar::~CGObjChar() {
	CGObjChar_RemoveFromList(&m_listNode);
	if (m_pSkillManager != nullptr) {
		delete m_pSkillManager;
		m_pSkillManager = nullptr;
	}

	// 0x0048B410 - 0x0048B42A: both movers are deleted and their slots cleared
	for (int32_t i = 0; i < 2; ++i) {
		if (m_apMover[i] != nullptr) {
			delete m_apMover[i];
			m_apMover[i] = nullptr;
		}
	}
}

/**
 * [RECONSTRUCTED - 0x00482560]
 * Slot 7 @ +0x1C: IsPlayer
 * Machine assembly (0x00482560):
 *   Checks (tid.bValid & 2) != 0, (tid.byClass & 0x1C) == 4, (tid.byClass & 0x60) == 0x20
 */
bool CGObjChar::IsPlayer() const {
	const tagTID& tid = GetTID();
	return (tid.bValid & 2) != 0 && (tid.byClass & 0x1C) == 4 && (tid.byClass & 0x60) == 0x20;
}

/**
 * [RECONSTRUCTED - 0x00482590]
 * Slot 8 @ +0x20: IsNonPlayer
 * Machine assembly (0x00482590):
 *   Checks (tid.bValid & 2) != 0, (tid.byClass & 0x1C) == 4, (tid.byClass & 0x60) == 0x40
 */
bool CGObjChar::IsNonPlayer() const {
	const tagTID& tid = GetTID();
	return (tid.bValid & 2) != 0 && (tid.byClass & 0x1C) == 4 && (tid.byClass & 0x60) == 0x40;
}

/**
 * [RECONSTRUCTED - 0x004825C0]
 * Slot 9 @ +0x24: IsNPC / IsStructure (Structure / Gate / Building)
 * Machine assembly (0x004825C0):
 *   Checks (tid.bValid & 2) != 0, (tid.byClass & 0x1C) == 4, (tid.byClass & 0x60) == 0x40, (tid.bySubClass & 0x780) == 0x100
 */
bool CGObjChar::IsNPC() const {
	const tagTID& tid = GetTID();
	return (tid.bValid & 2) != 0 && (tid.byClass & 0x1C) == 4 && (tid.byClass & 0x60) == 0x40 && (tid.bySubClass & 0x780) == 0x100;
}

bool CGObjChar::IsStructure() const {
	return IsNPC();
}

/**
 * [RECONSTRUCTED - 0x00482600]
 * Slot 10 @ +0x28: IsMonster / IsMob (Monster / NPC Character)
 * Machine assembly (0x00482600):
 *   Checks (tid.bValid & 2) != 0, (tid.byClass & 0x1C) == 4, (tid.byClass & 0x60) == 0x40, (tid.bySubClass & 0x780) == 0x080
 */
bool CGObjChar::IsMonster() const {
	const tagTID& tid = GetTID();
	return (tid.bValid & 2) != 0 && (tid.byClass & 0x1C) == 4 && (tid.byClass & 0x60) == 0x40 && (tid.bySubClass & 0x780) == 0x080;
}

/**
 * [RECONSTRUCTED - 0x004827B0]
 * Slot 11 @ +0x2C: IsCOS
 * Machine assembly (0x004827B0):
 *   Checks (tid.bValid & 2) != 0, (tid.byClass & 0x1C) == 4, (tid.byClass & 0x60) == 0x40, (tid.bySubClass & 0x780) == 0x180
 */
bool CGObjChar::IsCOS() const {
	const tagTID& tid = GetTID();
	return (tid.bValid & 2) != 0 && (tid.byClass & 0x1C) == 4 && (tid.byClass & 0x60) == 0x40 && (tid.bySubClass & 0x780) == 0x180;
}

void* CGObjChar::GetNetSession() const {
	return m_pNetSession;
}

CCmdSource* CGObjChar::GetCmdSource() const {
	return m_pNetSession;
}

/*
================
CGObjChar::SetCmdSource
Slot 308 @ +0x4D0 [RECONSTRUCTED - 0x004A72B0] (41 bytes)

CORRECTION (Claude): a second source is refused whether or not the new one is null, and the
source is not told about its object here (CCmdSrcNet::Init 0x0040B680 does that beforehand).
================
*/
int32_t CGObjChar::SetCmdSource( CCmdSource* pCmdSource ) {
	if ( m_pNetSession != nullptr ) {
		ASSERT( false );
		return 0;
	}
	m_pNetSession = pCmdSource;
	return ( pCmdSource != nullptr ) ? 1 : 0;
}

void* CGObjChar::GetBuffManager() const {
	return m_pBuffManager;
}

tagActiveSkillInstance* CGObjChar::GetAttackState() const {
	return m_pAttackState;
}

bool CGObjChar::IsPC() const {
	return IsPlayer();
}

uint32_t CGObjChar::GetHP() const {
	return GetCurrentHP();
}

uint8_t CGObjChar::GetJobType() const {
	return GetJobState();
}

/**
 * [RECONSTRUCTED - 0x004A66D0]
 * Slot 59 @ +0xEC: GetName
 * Machine assembly (0x004A66D0):
 *   Dispatches to m_pDataPermanent->GetCharName() (slot 11 @ +0x2C)
 */
const char* CGObjChar::GetName() const {
	if (m_pDataPermanent != nullptr) {
		return m_pDataPermanent->GetCharName();
	}
	return "";
}

/**
 * [RECONSTRUCTED - 0x00485EE0]
 * Slot 62 @ +0xF8: GetLifeState
 * Machine assembly (0x00485EE0):
 *   Returns byte at offset +0x00 of m_pCharData (+0x30)
 */
uint8_t CGObjChar::GetLifeState() const {
	if (m_pCharData != nullptr) {
		return *reinterpret_cast<const uint8_t*>(m_pCharData);
	}
	return 1; // LIFESTATE_ALIVE
}

/**
 * [RECONSTRUCTED - 0x004AA590]
 * Slot 63 @ +0xFC: GetMotionState
 * Machine assembly (0x004AA590):
 *   Returns byte at offset +0x02 of m_pCharData (+0x30)
 */
uint8_t CGObjChar::GetMotionState() const {
	if (m_pCharData != nullptr) {
		return *(reinterpret_cast<const uint8_t*>(m_pCharData) + 2);
	}
	return 0; // MOTIONSTATE_STAND
}

/**
 * [RECONSTRUCTED - 0x004AA5B0]
 * Slot 64 @ +0x100: GetBodyMode
 * Machine assembly (0x004AA5B0):
 *   Returns byte at offset +0x03 of m_pCharData (+0x30)
 */
uint8_t CGObjChar::GetBodyMode() const {
	if (m_pCharData != nullptr) {
		return *(reinterpret_cast<const uint8_t*>(m_pCharData) + 3);
	}
	return 0; // BODYMODE_NORMAL
}

/**
 * [RECONSTRUCTED - 0x004AA5C0]
 * Slot 65 @ +0x104: GetParamFloat
 * Machine assembly (0x004AA5C0):
 *   Calls CGParamKeeper_GetParamFloat(dwParamID, &this->m_paramKeeper @ +0x1EC)
 */
float CGObjChar::GetParamFloat(uint32_t dwParamID) const {
	return const_cast<CGParamKeeper&>(m_paramKeeper).GetParamFloat(static_cast<uint16_t>(dwParamID));
}

/**
 * [RECONSTRUCTED - 0x004AA5E0]
 * Slot 66 @ +0x108: GetCurrentHP
 * Machine assembly (0x004AA5E0):
 *   Dispatches to m_pDataPermanent->GetCurrentHP() (slot 14 @ +0x38)
 */
uint32_t CGObjChar::GetCurrentHP() const {
	if (m_pDataPermanent != nullptr) {
		return m_pDataPermanent->GetCurrentHP();
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004AA5F0]
 * Slot 67 @ +0x10C: GetCurrentMP
 * Machine assembly (0x004AA5F0):
 *   Dispatches to m_pDataPermanent->GetCurrentMP() (slot 15 @ +0x3C)
 */
uint32_t CGObjChar::GetCurrentMP() const {
	if (m_pDataPermanent != nullptr) {
		return m_pDataPermanent->GetCurrentMP();
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004A6830]
 * Slot 68 @ +0x110: GetMaxHP
 * Machine assembly (0x004A6830):
 *   Calls GetParamFloat(3) and converts float to uint32_t via CRT_ftol
 */
uint32_t CGObjChar::GetMaxHP() const {
	return static_cast<uint32_t>(GetParamFloat(3));
}

/**
 * [RECONSTRUCTED - 0x004A6850]
 * Slot 69 @ +0x114: GetMaxMP
 * Machine assembly (0x004A6850):
 *   Calls GetParamFloat(4) and converts float to uint32_t via CRT_ftol
 */
uint32_t CGObjChar::GetMaxMP() const {
	return static_cast<uint32_t>(GetParamFloat(4));
}

/*
================
CGObjChar::SetCurrentHP

[RECONSTRUCTED - Native 0x004A66F0]
Sets current HP clamped between 0 and GetMaxHP().
CORRECTION (Claude): native line 364 (0x16C) is CLAMP(nHP, 0, GetMaxHP()) with signed compares
(0x004A6700 jge, 0x004A674E jl) and GetMaxHP re-evaluated as in the macro. The old version had no lower bound
or log. The native store goes through (this+0x34)->vftable[16] (0x004A6765); the port keeps m_pDataPermanent.
================
*/
void CGObjChar::SetCurrentHP(uint32_t dwHP) {
	int32_t nHP = static_cast<int32_t>(dwHP);
	CLAMP(nHP, 0, static_cast<int32_t>(GetMaxHP()));
	if (m_pDataPermanent != nullptr) {
		m_pDataPermanent->SetCurrentHP(static_cast<uint32_t>(nHP));
	}
}

/*
================
CGObjChar::SetCurrentMP

[RECONSTRUCTED - Native 0x004A6790]
Sets current MP clamped between 0 and GetMaxMP().
CORRECTION (Claude): native line 369 (0x171) is CLAMP(nMP, 0, GetMaxMP()) with signed compares; the native store
is (this+0x34)->vftable[17] (0x004A6805).
================
*/
void CGObjChar::SetCurrentMP(uint32_t dwMP) {
	int32_t nMP = static_cast<int32_t>(dwMP);
	CLAMP(nMP, 0, static_cast<int32_t>(GetMaxMP()));
	if (m_pDataPermanent != nullptr) {
		m_pDataPermanent->SetCurrentMP(static_cast<uint32_t>(nMP));
	}
}

/*
================
CGObjChar::ConsumeResources

[RECONSTRUCTED - Native 0x004A8770] (85 bytes)
Slot 194 (+0x308): Deducts HP and MP costs for skills and actions.
Ensures HP cost is clamped so character retains at least 1 HP unless suicidal.
================
*/
int32_t CGObjChar::ConsumeResources(int32_t nHP, int32_t nMP, uint32_t dwReason) {
    if (nHP < 0) nHP = 0;
    else if (nHP != 0 && nHP >= static_cast<int32_t>(GetCurrentHP()))
        nHP = static_cast<int32_t>(GetCurrentHP() - 1u);
    if (nMP < 0) nMP = 0;
    return ApplyHealthAndManaOffset(static_cast<int32_t>(0u - uint32_t(nHP)),
        static_cast<int32_t>(0u - uint32_t(nMP)), static_cast<uint16_t>(dwReason));
}

int32_t CGObjChar::ApplyHealthAndManaOffset(int32_t hp, int32_t mp, uint16_t reason) {
    uint32_t result = GetLifeState();
    if (result != 1 || (hp == 0 && mp == 0)) return static_cast<int32_t>(result);
    bool changedHP = false;
    if (hp != 0) {
        const uint32_t next = GetCurrentHP() + uint32_t(hp);
        if (GetCurrentHP() != GetMaxHP()) m_dwPublishedHP = GetCurrentHP();
        SetCurrentHP(next);
        result = GetCurrentHP();
        changedHP = m_dwPublishedHP != result;
    }
    if (mp != 0) {
        const uint32_t next = GetCurrentMP() + uint32_t(mp);
        if (GetCurrentMP() != GetMaxMP()) m_dwPublishedMP = GetCurrentMP();
        SetCurrentMP(next);
        result = GetCurrentMP();
    }
    if (changedHP || (mp != 0 && m_dwPublishedMP != result)) {
        m_wStatusDirtyFlags |= reason;
        result = (result & 0xffff0000u) | reason; // native incidental EAX result
    }
    return static_cast<int32_t>(result);
}

/*
================
CGObjChar::OffsetGold

[RECONSTRUCTED - Native 0x0057E540]
Virtual slot 91 (+0x16C): Changes permanent character gold by signed 64-bit offset.
================
*/
int64_t CGObjChar::OffsetGold(int64_t nOffset, uint32_t dwReason, uint32_t dwParam1, uint32_t dwParam2) {
	(void)dwReason; (void)dwParam1; (void)dwParam2;
	if (m_pDataPermanent != nullptr) {
		int64_t nCur = m_pDataPermanent->GetGold();
		int64_t nNew = nCur + nOffset;
		if (nNew < 0) {
			nNew = 0;
		}
		m_pDataPermanent->SetGold(nNew);
		return nNew;
	}
	return 0;
}

/*
================
CGObjChar::OffsetSkillPoint

[RECONSTRUCTED - Native 0x0057E5A0]
Virtual slot 93 (+0x174): Changes permanent character Skill Points (SP) by signed offset.
================
*/
void CGObjChar::OffsetSkillPoint(int32_t nOffset, uint8_t byReason) {
	(void)byReason;
	if (m_pDataPermanent != nullptr) {
		int32_t nCur = static_cast<int32_t>(m_pDataPermanent->GetSkillPoints());
		int32_t nNew = nCur + nOffset;
		if (nNew < 0) {
			nNew = 0;
		}
		m_pDataPermanent->SetSkillPoints(static_cast<uint32_t>(nNew));
	}
}

/*
================
CGObjChar::InquireSameItem

[RECONSTRUCTED - Native 0x0057EE10]
Virtual slot 137 (+0x224): Base item inquiry method across storage slots.
================
*/
uint32_t CGObjChar::InquireSameItem(uint32_t dwStorageType, const char* szCodeName, uint32_t dwMode, uint32_t dwSlot, uint32_t dwUnk) {
	(void)dwStorageType; (void)szCodeName; (void)dwMode; (void)dwSlot; (void)dwUnk;
	return 0;
}

/*
================
CGObjChar::DelItem_EXT

[RECONSTRUCTED - Native 0x0057EEA0]
Virtual slot 140 (+0x230): Base item deletion method from storage.
================
*/
void* CGObjChar::DelItem_EXT(uint32_t dwStorageType, uint8_t bySlot, int32_t nCount, uint16_t* pwStatus, uint8_t byReason, uint32_t dwControl) {
	(void)dwStorageType; (void)bySlot; (void)nCount; (void)byReason; (void)dwControl;
	if (pwStatus != nullptr) {
		*pwStatus = 1;
	}
	return nullptr;
}

/*
================
CGObjChar::BackupData

[RECONSTRUCTED - Native 0x0057F0E0]
Virtual slot 157 (+0x274): Base resource / character dirty data backup and notify.
================
*/
void CGObjChar::BackupData(uint32_t dwFlags, uint32_t dwParam) {
	(void)dwFlags; (void)dwParam;
}

/*
================
CGObjChar::RecomputeMasteryStats

[RECONSTRUCTED - Native 0x004E30D0] (494 bytes)
Calculates character mastery scaling factors (+0x1CD8 and +0x1CDC) based on character
level vs highest learned mastery level.
================
*/
void CGObjChar::RecomputeMasteryStats() {
	if (!m_pSkillManager) {
		return;
	}
	uint8_t byMaxMastery = m_pSkillManager->GetMaxMasteryLevel();
	if (byMaxMastery > 120) {
		m_fExpMultiplier = 1.0f;
		return;
	}
	uint8_t byLevel = GetLevel();
	float fDiff = static_cast<float>(static_cast<int32_t>(byLevel) - static_cast<int32_t>(byMaxMastery));
	float fVal = 1.0f - (fDiff * 0.1f);
	if (fVal < 0.1f) {
		fVal = 0.1f;
	}
	if (fVal > 1.9f) {
		fVal = 1.9f;
	}
	m_fExpMultiplier = fVal;
}

int64_t CGObjChar::GetGold() const {
	return m_pDataPermanent != nullptr ? m_pDataPermanent->GetGold() : 0;
}

uint32_t CGObjChar::GetSkillPoints() const {
	return m_pDataPermanent != nullptr ? m_pDataPermanent->GetSkillPoints() : 0;
}

/**
 * [RECONSTRUCTED - 0x004A6870]
 * Slot 73 @ +0x124: GetLevel
 */
uint8_t CGObjChar::GetLevel() const {
	return static_cast<uint8_t>(GetParamFloat(0));
}

/**
 * [RECONSTRUCTED - 0x0057E2A0]
 * Slot 74 @ +0x128: GetMaxLevel
 */
uint8_t CGObjChar::GetMaxLevel() const {
	return GetLevel();
}

/**
 * [RECONSTRUCTED - 0x0057F290]
 * Slot 75 @ +0x12C: GetMonsterClass
 */
uint8_t CGObjChar::GetMonsterClass() const {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057F800 / Base IGObj]
 * Slot 192 @ +0x300: GetMonsterType
 *
 * In base IGObj, this is an illegal invocation asserting on player characters.
 * In base CGObjChar, returns 0 (normal). Overridden in CGObjMob.
 */
uint8_t CGObjChar::GetMonsterType() const {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004A68B0]
 * Slot 332 @ +0x530: GetExp
 * Dispatches to CDataPermanent::GetExp() (slot 13 @ +0x34).
 */
uint32_t CGObjChar::GetExp() const {
	if (m_pDataPermanent) {
		return m_pDataPermanent->GetExp();
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x009A7330]
 * Slot 359 @ +0x59C: GetExpMultiplier
 * Returns default reward experience multiplier (1.0f).
 */
float CGObjChar::GetExpMultiplier() const {
	return 1.0f;
}

/**
 * [RECONSTRUCTED - 0x0057E210]
 * Slot 71 @ +0x11C: GetJobState
 * Returns trade job state (0: None, 1: Merchant, 2: Thief, 3: Hunter)
 */
uint8_t CGObjChar::GetJobState() const {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004FD7F0]
 * GetActiveVehicle
 * Returns active vehicle / transport mount COS associated with character (+0x1CE8)
 */
CGObj* CGObjChar::GetActiveVehicle() const {
	return m_pActiveVehicle;
}

void CGObjChar::SetActiveVehicle(CGObj* pVehicle) {
	m_pActiveVehicle = pVehicle;
}

void CGObjChar::SetCargoStats(uint64_t nRawGold, uint32_t dwTotalCount) {
	m_nCargoRawGold = nRawGold;
	m_dwCargoTotalCount = dwTotalCount;
}

/**
 * [RECONSTRUCTED - 0x004D2910] (116 bytes)
 * CGObjChar_CalculateCargoRawGoldValue
 *
 * Loops through character/vehicle storage slots (+0x1C10) up to m_dwTotalSlots (+0x1C30).
 * Queries item count (+0x4E8) and price (+0x3A8), accumulating 64-bit total cargo gold.
 *
 * Machine Disassembly (0x004D2910):
 *   004d2910  mov   ebp, [arg1+0x1c30] ; m_dwTotalSlots
 *   004d291b  call  CGStorage_GetItem  ; (arg1+0x1c10, ebx)
 *   004d2924  call  [eax+0x4e8]        ; pItem->GetCount()
 *   004d2931  call  [eax+0x3a8]        ; pItem->GetPrice()
 *   004d2937  imul  edx:eax            ; price * count
 *   004d2943  add   result, eax        ; 64-bit sum
 */
uint64_t CGObjChar::CalculateCargoRawGoldValue() const {
	uint64_t totalGold = 0;
	for (uint32_t slot = 0; slot < m_dwTotalSlots; ++slot) {
		CGItem* pItem = m_storage.GetItem(slot);
		if (pItem != nullptr) {
			uint32_t count = static_cast<uint32_t>(pItem->GetCount());
			uint32_t price = pItem->GetPrice();
			totalGold += (static_cast<uint64_t>(price) * count);
		}
	}
	const_cast<CGObjChar*>(this)->m_nCargoRawGold = totalGold;
	return totalGold;
}

/**
 * [RECONSTRUCTED - 0x004D2D20] (114 bytes)
 * CGObjChar_GetCargoTotalCount
 *
 * Loops through storage slots, checking if items are specialty goods (+0x58) or trade goods (+0x404),
 * and sums total cargo pack count (+0x4E8).
 *
 * Machine Disassembly (0x004D2D20):
 *   004d2d20  mov   ebp, [arg1+0x1c30] ; m_dwTotalSlots
 *   004d2d31  call  CGStorage_GetItem  ; (arg1+0x1c10, ebx)
 *   004d2d3a  call  [eax+0x58]         ; pItem->IsSpecialtyGoods()
 *   004d2d48  call  [eax+0x404]        ; pItem->IsTradeItem()
 *   004d2d53  call  [eax+0x4e8]        ; pItem->GetCount()
 *   004d2d58  add   result, eax
 */
uint32_t CGObjChar::GetCargoTotalCount() const {
	uint32_t totalPacks = 0;
	for (uint32_t slot = 0; slot < m_dwTotalSlots; ++slot) {
		CGItem* pItem = m_storage.GetItem(slot);
		if (pItem != nullptr) {
			if (pItem->IsSpecialtyGoods() || pItem->IsTradeItem()) {
				totalPacks += static_cast<uint32_t>(pItem->GetCount());
			}
		}
	}
	const_cast<CGObjChar*>(this)->m_dwCargoTotalCount = totalPacks;
	return totalPacks;
}

uint8_t CGObjChar::GetCOSRarity() const {
	if (m_pDataPermanent && m_pDataPermanent->m_pRefObjCommon) {
		return m_pDataPermanent->m_pRefObjCommon->m_byCOS_Rarity;
	}
	return 0;
}

uint16_t CGObjChar::GetCOSSlotCapacityFromRef() const {
	if (m_pDataPermanent && m_pDataPermanent->m_pRefObjCommon) {
		return m_pDataPermanent->m_pRefObjCommon->m_wCOSSlotCapacity;
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0066B100 / CGObjPC override]
 * Slot 87 @ +0x15C: ShowDebugMsg
 */
void CGObjChar::ShowDebugMsg(const char* pszMsg) {
	// Base implementation is a no-op; overridden in CGObjPC to dispatch message to client
	(void)pszMsg;
}

/**
 * [RECONSTRUCTED - 0x0048BA10] (95 bytes)
 * Slot 230 @ +0x398: SetAngle
 *
 * The direction vector the way CGObj sets it, and - while the character is steered rather than walking to
 * a destination - the heading is also written back into the move state so the two never drift apart.
 */
void CGObjChar::SetAngle(float fAngle) {
	CGObj::SetAngle(fAngle);                        // 0x0048BA1E - 0x0048BA59

	if (m_MoveState.m_byMoveType == OBJ_MOVE_BY_CMD) {
		m_MoveState.m_fAngle = fAngle;              // 0x0048BA62
	}
}

/**
 * [RECONSTRUCTED - 0x0048B660] (119 bytes)
 * Slot 231 @ +0x39C: MoveTo
 *
 * CGObj walks the navmesh; a move that was not simply refused (2) and left the character standing still
 * ends the walk and tells everyone about it.
 */
int32_t CGObjChar::MoveTo(tagObjLocation destination, uint8_t byMode) {
	const int32_t nResult = CGObj::MoveTo(destination, byMode); // 0x0048B69C

	if (nResult != 2 && IsMoving()) {   // 0x0048B6A3 / 0x0048B6B2: slot 304
		StopMove(1, GetMoveAngle());    // 0x0048B6CD: slot 300
	}
	return nResult;
}

/**
 * [PARTIAL - 0x0048B730 (base) / 0x004A9430 (override)] (63 / 288 bytes)
 * Slot 300 @ +0x4B0: StopMove
 *
 * A character that is not moving has nothing to stop. The base cancels BOTH movers, not just the active
 * one, so whichever one the character switches to next starts from a standstill.
 */
void CGObjChar::StopMove(int32_t bBroadcast, float fAngle) {
	if (!IsMoving()) {
		return; // 0x004A943F
	}

	// 0x004A9451: CGObjMobile::StopMove
	ASSERT(m_MoveState.m_byMoveType < 2);
	m_apMover[OBJ_MOVE_BY_DEST]->Cancel(fAngle); // 0x0048B754
	m_apMover[OBJ_MOVE_BY_CMD]->Cancel(fAngle);  // 0x0048B769

	// [PARTIAL] 0x004A9456 - 0x004A954C: a visible character broadcasts the 0xB023 stop carrying its
	// position, the location (+0x7C - +0x90) is copied into the last-stop block at +0xA10, a dead or gone
	// character has its action state reset through slot 343 (+0x55C), and the command source at +0x188 is
	// told the walk ended. None of those are wired here yet, so the flag has nothing to switch on.
	(void)bBroadcast;
}

/**
 * [RECONSTRUCTED - 0x0048B6E0] (65 bytes)
 * Slot 301 @ +0x4B4: SetMoveCommand
 *
 * Note which mover is picked: the one the COMMAND names, not the one the character is currently on - a
 * character walking to a destination takes a steering command by handing it to the steered mover, which
 * then rewrites m_byMoveType through its own SetCommand.
 */
int32_t CGObjChar::SetMoveCommand(const tagObjMoveCommand* pCommand) {
	ASSERT(m_MoveState.m_byMoveType < 2); // 0x0048B6E3

	if (GetLifeState() != 1) {
		return 0; // 0x0048B6FD: slot 62, only a living character takes a movement command
	}
	return m_apMover[pCommand->m_byMoveType]->SetCommand(pCommand); // 0x0048B71F
}

/**
 * [RECONSTRUCTED - 0x0048B770] (72 bytes)
 * Slot 302 @ +0x4B8: SetMoveAngle
 */
void CGObjChar::SetMoveAngle(float fAngle) {
	ASSERT(m_MoveState.m_byMoveType < 2);                 // 0x0048B773
	ASSERT(m_apMover[m_MoveState.m_byMoveType] != nullptr); // 0x0048B788

	m_apMover[m_MoveState.m_byMoveType]->SetAngle(fAngle); // 0x0048B7B2
}

/**
 * [RECONSTRUCTED - 0x0048B7C0] (75 bytes)
 * Slot 303 @ +0x4BC: SetSpeedMode
 */
void CGObjChar::SetSpeedMode(uint8_t bySpeedMode) {
	ASSERT(m_MoveState.m_byMoveType < 2);                  // 0x0048B7C3
	ASSERT(m_apMover[m_MoveState.m_byMoveType] != nullptr); // 0x0048B7D8

	m_apMover[m_MoveState.m_byMoveType]->SetSpeedMode(bySpeedMode); // 0x0048B7F9
	ApplyMoveSpeed(bySpeedMode);                                     // 0x0048B802
}

/**
 * [RECONSTRUCTED - 0x0048B810] (40 bytes)
 * ApplyMoveSpeed
 */
void CGObjChar::ApplyMoveSpeed(uint8_t bySpeedMode) {
	if (bySpeedMode == 0) {
		m_fMoveSpeed = m_fWalkSpeed; // 0x0048B81A
	} else if (bySpeedMode == 1) {
		m_fMoveSpeed = m_fRunSpeed;  // 0x0048B82C
	} else {
		ASSERT(false);               // 0x0048B833
	}
}

/**
 * [RECONSTRUCTED - 0x0048B880] (58 bytes)
 * Slot 304 @ +0x4C0: IsMoving
 */
bool CGObjChar::IsMoving() const {
	ASSERT(m_MoveState.m_byMoveType < 2);                   // 0x0048B883
	ASSERT(m_apMover[m_MoveState.m_byMoveType] != nullptr); // 0x0048B898

	return m_apMover[m_MoveState.m_byMoveType]->m_bMoving != 0; // 0x0048B8B5
}

/**
 * [RECONSTRUCTED - 0x0048B920] (171 bytes)
 * Slot 305 @ +0x4C4: MoveByStep
 *
 * The step a mover produced this tick is added to the character's own location and the whole location goes
 * back through MoveTo, so the navmesh still gets to refuse or clip it.
 */
void CGObjChar::MoveByStep(const SRO_Vector3D* pStep) {
	tagObjLocation destination = m_Location;                // 0x0048B929 - 0x0048B98C
	destination.fPosX = m_Location.fPosX + pStep->x;        // 0x0048B94B
	destination.fPosY = m_Location.fPosY + pStep->y;        // 0x0048B96D
	destination.fPosZ = m_Location.fPosZ + pStep->z;        // 0x0048B97A

	MoveTo(destination, 0); // 0x0048B9C1: slot 231, the pushed mode is 0
}

/**
 * [RECONSTRUCTED - 0x0048B840] (60 bytes)
 * GetMoveAngle
 */
float CGObjChar::GetMoveAngle() const {
	ASSERT(m_MoveState.m_byMoveType < 2);                   // 0x0048B841
	ASSERT(m_apMover[m_MoveState.m_byMoveType] != nullptr); // 0x0048B856

	return m_apMover[m_MoveState.m_byMoveType]->GetAngle(); // 0x0048B878
}

/**
 * [PARTIAL - 0x0048B4D0] (149 bytes)
 * Slot 215 @ +0x35C: InitMoveSpeeds
 *
 * The walk and run speeds come out of the reference object (+0xE4 / +0xE6) and the character is dropped in
 * facing a random way. Not ported: the reference lookup goes through m_pDataPermanent->m_pRefObjCommon,
 * whose speed fields this port does not carry yet, so the speeds are left as the constructor set them.
 */
void CGObjChar::InitMoveSpeeds() {
	// 0x0048B515: rand() / 32767.0 rounded to a float, then scaled by 2 pi (0x00B45C18) on the x87 stack.
	// The range's low bound is added right after (0x0048B541, the literal 0.0 at 0x00B45AB0).
	const float fRand = static_cast<float>(std::rand() / 32767.0);
	const float fAngle = static_cast<float>(static_cast<double>(fRand) * 6.2831854820251465);

	SetAngle(fAngle); // 0x0048B552: slot 230
	SetSpeedMode(1);  // 0x0048B560: slot 303, a character is dropped in running
}

/**
 * [RECONSTRUCTED - 0x009BF500]
 * Slot 358 @ +0x598: OnClientPacket
 * Native 0x009BF500: Base character no-op stub for client-directed network packets
 */
int32_t CGObjChar::OnClientPacket(CPacket* /*pPacket*/) {
	return 0;
}

/*
================
CGObjChar::OnMsg_ActionCommand
Slot 385 @ +0x604 [PARTIAL - 0x004B21E0] (197 bytes)

0x7074 from the client. A player whose network message was not fully read by the actor is
reported to the packet-rate monitor (g_d6a964: 0x00527C80 / 0x00527D40) and removed from the
world with reason 4 (CGObjPC 0x004DF050); neither is ported yet. The message is always consumed.
================
*/
void CGObjChar::OnMsg_ActionCommand(CMsg* pMsg) {
	m_AutoCommandActor.ProcessCommand(pMsg, 0);

	if (IsPlayer() && pMsg->m_nRefCount == 1 && pMsg->m_wWriteOffset != pMsg->m_wReadOffset) {
		static_cast<CGObjPC*>(this)->Recall(4);
	}
	pMsg->Consume();
}

/*
================
CGObjChar::OnMsg_SkillAction
Slot 383 @ +0x5FC [RECONSTRUCTED - 0x004B21B0] (48 bytes)
================
*/
void CGObjChar::OnMsg_SkillAction(CMsg* pMsg) {
	m_pSkillManager->OnMsgSkillAction(pMsg);
	if (IsPlayer()) {
		static_cast<CGObjPC*>(this)->SetSpawnInvincible(0);
	}
}

/*
================
CGObjChar::OnMsg_7021 .. OnMsg_7091, OnMsg_7034
Slots 377 - 382, 384, 386 [STUB]

Registered in the SR_MSG map (0x004B0B70) but not ported yet; they consume the message so the
unconsumed-message check stays quiet, and say so in the log.
================
*/
static void CGObjChar_ConsumeUnportedMsg(CMsg* pMsg, const char* pszHandler) {
	BSLib::Log_Printf(0, "[STUB] %s: SR_MSG 0x%x not ported", pszHandler, pMsg->GetOpcode());
	pMsg->Consume();
}

void CGObjChar::OnMsg_7021(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B0EA0"); }
void CGObjChar::OnMsg_7022(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B1210"); }
void CGObjChar::OnMsg_7023(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B12A0"); }
void CGObjChar::OnMsg_7024(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B1360"); }
void CGObjChar::OnMsg_704F(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B1450"); }
void CGObjChar::OnMsg_7025(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B1750"); }
void CGObjChar::OnMsg_7091(CMsg* pMsg) { CGObjChar_ConsumeUnportedMsg(pMsg, "CGObjChar 0x004B1630"); }

// Slot 386: the base body is the shared illegal-virtual stub (0x00825E50).
void CGObjChar::OnMsg_7034(CMsg* pMsg) {
	ASSERT(false);
	pMsg->Consume();
}

/**
 * [RECONSTRUCTED - 0x004A88F0]
 * Slot 211 @ +0x34C: OnTick
 * Native 0x004A88F0: Advances active character status, movement interpolation, and combat timers.
 *
 * Native order: motion/cast movement gates, eligible player command actors,
 * base mobile movement, periodic jobs, then dirty-state publication. The
 * motion/cast gates and complete dirty publication remain partial below.
 */
void CGObjChar::OnTick(float fDeltaSec) {

	// [PARTIAL] 0x004A8928 - 0x004A89BD: while knocked back / knocked down (motion 8 / 16) and moving,
	// CSkillManager 0x0059E840 updates the owner speed and the tick ends; a moving character casting
	// a targeted skill (+0xC08 -> +0x18 -> +0x08 -> +0x65 == 2) is stopped through slot 300. Neither the
	// knockback speed update nor the motion state it keys off is ported yet.

	if (IsPlayer() && m_pCharData->m_byMsgProcState != 1) {
		uint16_t wError = 0;
		m_AutoCommandActor.Update(&wError);
		m_AutoNavigator.Update();
	}

	// 0x004A89F7: CGObjMobile::OnTick (0x0048B8C0). CGObj::OnTick runs first, then the active mover gets
	// this tick's slice and whatever step it produced is committed.
	CGObj::OnTick(fDeltaSec);                       // 0x0048B8CE
	if (m_MoveState.m_byMoveType != OBJ_MOVE_NONE) { // 0x0048B8D3
		SRO_Vector3D step;
		CGObjMover* pMover = m_apMover[m_MoveState.m_byMoveType];      // 0x0048B8E7
		if (pMover->Update(fDeltaSec, &step) == 1) {                    // 0x0048B8FC: slot 3
			MoveByStep(&step);                                          // 0x0048B912: slot 305
		}
	}

	// 4A8A09 runs periodic jobs AFTER the base mobile/movement update.
	m_periodicJobs.Tick(fDeltaSec);

	// [PARTIAL] 0x004A8A0E sends the dirty status (+0xA0E) through slot 313 and clears it.
	if (m_bPositionDirty != 0) {
		m_bPositionDirty = 0;
	}
}

/**
 * [RECONSTRUCTED - 0x004AB3D0]
 * Slot 390 @ +0x618: Deactivate
 *
 * Machine Disassembly (0x004AB3D0):
 *   004ab3d0  push  esi
 *   004ab3d1  mov   esi, ecx
 *   004ab3d3  mov   eax, [esi]
 *   004ab3d5  mov   edx, [eax+0x28] ; slot 10 @ +0x28: IsNPC()
 *   004ab3d8  call  edx
 *   004ab3da  test  eax, eax
 *   004ab3dc  je    0x4ab3f0
 *   004ab3de  mov   ecx, esi
 *   004ab3e0  call  CGObjNPC_HasSpawnRef ; 0x004C2700: checks *(esi+0x1D18) != 0
 *   004ab3e5  test  eax, eax
 *   004ab3e7  je    0x4ab3f0
 *   004ab3e9  mov   eax, esi
 *   004ab3eb  call  CGObjNPC_ClearSpawnRef ; 0x004C2730: sets *(esi+0x1D18) = 0
 *   004ab3f0  add   esi, 0x178 ; &m_listNode
 *   004ab3f6  call  CGObjChar_RemoveFromList ; 0x004AB4B0
 *   004ab3fb  pop   esi
 *   004ab3fc  retn
 */
void CGObjChar::Deactivate() {
	if (IsNPC() && m_dwActiveVehicleOwner != 0) {
		m_dwActiveVehicleOwner = 0;
	}
	CGObjChar_RemoveFromList(&m_listNode);
}

/**
 * [RECONSTRUCTED - 0x004838E0]
 * Slot 270 @ +0x438: IsAbilityOrPetCOS
 * Checks if entity is Growth Pet / Attack Pet / Ability COS (TID subtype 0x1800).
 */
bool CGObjChar::IsAbilityOrPetCOS() const {
	const tagTID& tid = GetTID();
	return tid.bValid == 2 && tid.byClass == 4 && tid.bySubClass == 0x180 && tid.bySubType == 0x1800;
}

/**
 * [RECONSTRUCTED - 0x00483930]
 * Slot 271 @ +0x43C: IsTradeCOS
 * Checks if entity is Trade Vehicle / Transport Beast COS (TID subtype 0x2000).
 */
bool CGObjChar::IsTradeCOS() const {
	const tagTID& tid = GetTID();
	return tid.bValid == 2 && tid.byClass == 4 && tid.bySubClass == 0x180 && tid.bySubType == 0x2000;
}

/**
 * [RECONSTRUCTED - 0x004A68E0]
 * Slot 350 @ +0x578: GetStorageItem
 *
 * Machine Operations (0x004A68E0):
 *   004a68e0  jmp   CGStorage_GetItem(arg1 + 0x1c10, arg2)
 */
CGItemEquip* CGObjChar::GetStorageItem(uint8_t bySlot) const {
	return reinterpret_cast<CGItemEquip*>(const_cast<CGStorage&>(m_storage).GetItem(bySlot));
}

/**
 * [RECONSTRUCTED - 0x00482690]
 * Slot 238 @ +0x3B8: CanBeAttacked
 *
 * Machine Disassembly (0x00482690):
 *   00482698  call  CGObj_GetTID
 *   0048269d  movzx eax, word [eax]
 *   004826a0  test  al, 0x2
 *   004826a3  je    return_0
 *   004826a7  and   cl, 0x1c; cmp cl, 0x4; jne return_0
 *   004826b1  and   dl, 0x60; cmp dl, 0x40; jne return_0
 *   004826bb  and   ecx, 0x780; cmp ecx, 0x80; jne return_0
 *   004826c9  and   eax, 0xf800; cmp eax, 0x800; jne return_0
 *   004826d5  mov   eax, 1; retn
 */
int32_t CGObjChar::CanBeAttacked() const {
	const tagTID& tid = GetTID();
	if ((tid.bValid & 2) != 0 &&
	    (tid.byClass & 0x1C) == 4 &&
	    (tid.byClass & 0x60) == 0x40 &&
	    (tid.bySubClass & 0x780) == 0x080 &&
	    (tid.bySubType & 0xF800) == 0x800) {
		return 1;
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x004A68C0]
 * Slot 344 @ +0x560: GetCollisionRadius
 *
 * Machine Disassembly (0x004A68C0):
 *   004a68c0  mov   esi, [pThis+0x34] ; m_pDataPermanent
 *   004a68c3  cmp   dword [esi+0x18], 0 ; m_pRefObjCommon
 *   004a68c7  jne   0x4a68ce
 *   004a68c9  call  ServerFramework_GenerateMiniDump
 *   004a68ce  mov   eax, [esi+0x18]
 *   004a68d1  movzx eax, byte [eax+0xee] ; m_wCollisionRadius / m_byCollisionRadius
 *   004a68d8  retn
 */
int32_t CGObjChar::GetCollisionRadius() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 10;
	}
	return static_cast<int32_t>(m_pDataPermanent->m_pRefObjCommon->m_wCollisionRadius);
}

/**
 * [RECONSTRUCTED - 0x004A6980 / CGObjPC override @ 0x00526AC0]
 * Slot 387 @ +0x60C: CanPickupItem
 *
 * Machine Disassembly (0x004A6980):
 *   004a6980  mov   ax, 2
 *   004a6984  retn
 * Base non-player implementation unconditionally returns 2 (unsupported).
 */
uint16_t CGObjChar::CanPickupItem(uint32_t /*dwItemID*/) {
	return 2;
}

/**
 * [RECONSTRUCTED - 0x004AAB40 / CGObjPC override @ 0x004EF880]
 * Slot 360 @ +0x5A0: IsAttackLocked
 *
 * Machine Disassembly (0x004AAB40):
 *   004aab40  cmp   dword [arg1+0xc08], 0 ; m_pCastingInstance
 *   004aab47  je    0x4aab4e
 *   004aab49  mov   eax, 1
 *   004aab4e  cmp   byte [arg1+0xc44], 0 ; m_bActionLocked
 *   004aab55  setne al
 *   004aab58  retn
 */
bool CGObjChar::IsAttackLocked() const {
	if (m_pCastingInstance != nullptr) {
		return true;
	}
	return m_bActionLocked != 0;
}

/**
 * [RECONSTRUCTED - Native 0x004A6910]
 * Slot 353 @ +0x584: GetAvatarStorageItem
 */
CGItem* CGObjChar::GetAvatarStorageItem(uint32_t dwSlot) {
	return m_avatarStorage.GetItem(dwSlot);
}

/**
 * Slot 372 @ +0x5D0: IsMainWeaponUsable
 * Default base implementation returns true (monsters/NPCs have innate attacks)
 */
bool CGObjChar::IsMainWeaponUsable() const {
	return true;
}

/**
 * Slot 399 @ +0x63C: IsSecondaryWeaponUsable
 * Default base implementation returns true
 */
bool CGObjChar::IsSecondaryWeaponUsable() const {
	return true;
}

/**
 * Slot 400 @ +0x640: IsEquipmentSlotUsable
 * Default base implementation returns true
 */
bool CGObjChar::IsEquipmentSlotUsable(uint32_t dwSlot) const {
	(void)dwSlot;
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0059EF90 / 0x0059EFF0 / 0x0059F0C0]
 * CancelActiveBuff
 * Native 0x0059EF90 iterates the active buff list at CSkillManager + 0x268
 * Native 0x0059EFF0 deactivates the buff by clearing its retirement marker at offset +0x10
 * Native 0x0059F0C0 coordinates the search and cancellation
 */
bool CGObjChar::CancelActiveBuff(uint32_t dwRefSkillID, uint32_t dwRecordSkillID) {
	(void)dwRecordSkillID;
	if (!m_pSkillManager) {
		return false;
	}
	return m_pSkillManager->CancelBuffBySkillID(dwRefSkillID) != nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x004A98A0]
 * Slot 355 @ +0x58C: CanSelectTarget
 */
bool CGObjChar::CanSelectTarget(CGObjChar* pTarget, uint32_t dwParam) {
	if (!pTarget) {
		return false;
	}
	(void)dwParam;
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x004AA640]
 * Slot 393 @ +0x624: GetCombatPermission
 */
uint32_t CGObjChar::GetCombatPermission(CGObjChar* pTarget, uint32_t dwMode, uint32_t* pErrorCode) {
	if (!pTarget) {
		if (pErrorCode) *pErrorCode = 0x3003; // SKILL_ERR_INVALID_TARGET
		return 0;
	}
	(void)dwMode;
	if (pErrorCode) *pErrorCode = 0;
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x00559C70]
 * Slot 336 @ +0x540: IsRidingTransport
 * Base implementation returns false (overridden by CGObjPC @ 0x004DDCA0)
 */
bool CGObjChar::IsRidingTransport() const {
	return false;
}

CPacket* CGObjChar::AllocMsgForPeer(uint16_t wOpcode) {
	CPacket* pPkt = CPacket::Allocate(1);
	if (pPkt) {
		pPkt->SetOpcode(wOpcode);
	}
	return pPkt;
}

int32_t CGObjChar::SendMsgToPeer(CPacket* pPacket) {
	if (pPacket) {
		pPacket->Release();
	}
	return 1;
}

int32_t CGObjChar::ApplyHit(CGObjChar* pAttacker, int32_t nDamage1, int32_t nDamage2, uint32_t reason, void* hitContext) {
	// Partial actor adapter. Common native handlers 52A240/52D460 reject self
	// and dead targets before the signed positive-damage test. HP changes use
	// 4A87D0, including cached vitals and dirty-reason publication, not SetHP.
	// World dispatch, attribution and death callbacks remain unresolved here.
	(void)nDamage2;
	(void)hitContext;
	if (pAttacker == this || GetLifeState() == 2) return 1;
	if (nDamage1 > 0) {
		ApplyHealthAndManaOffset(-nDamage1, 0, static_cast<uint16_t>(reason));
	}
	return 1;
}

/**
 * [PARTIAL - Native 0x004A4270] (288 bytes)
 * CGObjChar::ApplyAbnormalStateRecord
 *
 * Writes one status the hit rolled into its slot: a status that is stronger than the one already there marks
 * the slot as upgraded, the record is copied in whole (0x60 bytes), the slot starts counting from now and the
 * status bit joins the character's mask.
 * Not ported: the per-status callback table at the block's +0xE0C (0x004A4357 calls entry 0 of it) and the
 * player versus player context sub_004E6590 writes into the copy at +0x5C.
 */
int32_t CGObjChar::ApplyAbnormalStateRecord(const tagSkillStatusEffect& effect) {
	if (m_pDataPermanent == nullptr || GetLifeState() != 1) {
		return 0; // 0x004A4283
	}

	ASSERT(effect.m_byStatusIndex <= 0x1F); // 0x004A4293
	if (effect.m_byStatusIndex > 0x1F) {
		return 0;
	}

	tagAbnormalStateSlot& slot = m_aAbnormalState[effect.m_byStatusIndex];

	// 0x004A42A6: a status already running is only marked when the new one is stronger
	if (effect.m_wLevel != 0) {
		if (slot.m_byActive == 1 && !(effect.m_wLevel <= slot.m_Effect.m_wLevel)) {
			slot.m_byUpgraded = 1;
		}
	} else if (effect.m_byGrade != 0) {
		if (slot.m_byActive == 1 && !(effect.m_byGrade <= slot.m_Effect.m_byGrade)) {
			slot.m_byUpgraded = 1;
		}
	}

	// 0x004A42E1: the caster has to still be around
	CGObjChar* pCaster = ObjMgr_FindByID(effect.m_dwCasterID);
	if (pCaster == nullptr || !pCaster->IsChar()) {
		return 0;
	}

	slot.m_Effect = effect;          // 0x004A4335
	slot.m_byActive = 1;             // 0x004A433E
	slot.m_by61 = 0;
	slot.m_dwStartedAt = GetTickCount(); // 0x004A4350
	slot.m_dw68 = 0;
	m_dwAbnormalFlags |= g_adwAbnormalStatusBit[effect.m_byStatusIndex]; // 0x004A4365
	return 1;
}

/**
 * [PARTIAL - Native 0x004A5660] (93 bytes)
 * CGObjChar::ClearAbnormalStateSlot
 *
 * Ends one running status: the slot is marked cured (+0x61 = 1, which is what tells the client it was removed
 * rather than timed out), the mask is folded back together from the slots that are still active and the block
 * is published.
 * Not ported: the per-status callback at the block's +0xE0C (0x004A5690 calls it with 2).
 */
void CGObjChar::ClearAbnormalStateSlot(uint8_t byStatusIndex) {
	if (byStatusIndex > 0x1F) {
		return;
	}

	tagAbnormalStateSlot& slot = m_aAbnormalState[byStatusIndex];
	if (slot.m_byActive == 0) {
		return; // 0x004A5671
	}

	slot.m_byUpgraded = 0; // 0x004A5675
	slot.m_byActive = 0;
	slot.m_by61 = 1;       // 0x004A567D: cured, not expired
	slot.m_dw6C = 0;

	uint32_t dwMask = 0;
	for (uint32_t i = 0; i < 32; ++i) {
		if (m_aAbnormalState[i].m_byActive != 0) {
			dwMask |= g_adwAbnormalStatusBit[i]; // 0x004A56A5
		}
	}
	m_dwAbnormalFlags = dwMask;  // 0x004A56B3
	SendAbnormalStateUpdate();   // 0x004A56B6
}

/**
 * [PARTIAL - Native 0x004A5C60] (422 bytes)
 * CGObjChar::SendAbnormalStateUpdate
 *
 * Publishes the whole abnormal state block to the owning client as 0x30D2: the mask, then for every slot that
 * is still running its total duration and how much of it has gone, both in hundredths, and the payload word
 * the status carries. A slot that is active but whose bit has already left the mask is what the client reads
 * as "this one just ended": it contributes the two timing values, no payload, and the slot is retired here.
 *
 * Only a player has anywhere to send this - slots 158 and 159 are the illegal-invocation handlers on every
 * other character (0x0057F110 / 0x0057F140), which is why the native leaves after the IsPlayer test.
 *
 * Not ported: the per-status callback the block keeps at +0xE0C, which the retiring branch invokes with 2
 * (0x004A5DDB).
 */
void CGObjChar::SendAbnormalStateUpdate() {
	m_wStatusDirtyFlags |= 0x100; // 0x004A5C73

	if (!IsPlayer()) {
		return; // 0x004A5C88: slot 7
	}

	CPacket* pPacket = AllocMsgForPeer(0x30D2); // 0x004A5CA0: slot 158
	if (pPacket == nullptr) {
		return;
	}
	pPacket->WriteUint32(m_dwAbnormalFlags); // 0x004A5CAF

	if (m_dwAbnormalFlags != 0) { // 0x004A5CB4
		const uint32_t dwNow = GetTickCount();

		for (uint32_t i = 0; i < 32; ++i) {
			tagAbnormalStateSlot& slot = m_aAbnormalState[i];
			if (slot.m_byActive == 0) {
				continue; // 0x004A5CD0
			}

			// 0x004A5CDD - 0x004A5D5C: both are unsigned, divided at double width and truncated toward zero
			const uint32_t dwTotal = static_cast<uint32_t>(
				static_cast<double>(slot.m_Effect.m_dwDuration) / 100.0);
			const uint32_t dwElapsed = static_cast<uint32_t>(
				static_cast<double>(dwNow - slot.m_dwStartedAt) / 100.0);

			pPacket->WriteUint32(dwTotal);                              // 0x004A5D6C
			pPacket->WriteUint16(static_cast<uint16_t>(dwElapsed));     // 0x004A5D7D

			if ((m_dwAbnormalFlags & slot.m_Effect.m_dwStatusBit) != 0) { // 0x004A5D85
				if (slot.m_Effect.m_byCategory == 2) {
					pPacket->WriteUint16(slot.m_Effect.m_byGrade); // 0x004A5DA8
				} else {
					pPacket->WriteUint16(slot.m_Effect.m_wLevel);  // 0x004A5DBA
				}
			} else {
				// 0x004A5DC5: the bit is already gone, so this send is the slot's last one
				slot.m_byUpgraded = 0;
				slot.m_byActive = 0;
				slot.m_by61 = 1;
				slot.m_dw6C = 0;
			}
		}
	}

	SendMsgToPeer(pPacket); // 0x004A5DFA: slot 159, which takes the packet over and releases it
}

/**
 * [PARTIAL - Native 0x004A4390] (465 bytes)
 * CGObjChar::UpdateAbnormalStates
 *
 * Drops the slots whose duration has run out and rebuilds the mask from the ones that remain. When the status
 * at index 0x18 ends it takes 6 and 7 with it, and one roll in four also ends 0x0E (0x004A4426 - 0x004A44F0).
 * The block is published only when a slot actually ended or the mask came out different (0x004A4540).
 * Not ported: the per-status callback the native invokes with 2 as each slot ends, the block's own teardown
 * flag at +0xE8C (0x004A4523) and the byte the publish path writes to the owner at +0xA0C.
 */
void CGObjChar::UpdateAbnormalStates() {
	if (m_dwAbnormalFlags == 0) {
		return; // 0x004A439C
	}

	uint32_t dwRemaining = 0;
	bool bAnyEnded = false; // 0x004A451F: the byte the loop raises as each slot ends
	const uint32_t dwNow = GetTickCount();

	for (uint32_t i = 0; i < 32; ++i) {
		tagAbnormalStateSlot& slot = m_aAbnormalState[i];
		if (slot.m_byActive == 0) {
			continue; // 0x004A43B4
		}

		const bool bStillSet = (m_dwAbnormalFlags & slot.m_Effect.m_dwStatusBit) != 0;
		const bool bExpired = !bStillSet ||
			(dwNow - slot.m_dwStartedAt) > slot.m_Effect.m_dwDuration; // 0x004A43E1

		if (!bExpired) {
			dwRemaining |= g_adwAbnormalStatusBit[i]; // 0x004A4500
			continue;
		}

		slot.m_byUpgraded = 0;
		slot.m_byActive = 0;
		slot.m_by61 = 0;
		slot.m_dw6C = 0;
		bAnyEnded = true;

		// 0x004A4426: the hidden status carries three more down with it
		if (i == 0x18) {
			CSkillManager* pSkillMgr = GetSkillManager();
			if (m_aAbnormalState[0x0E].m_byActive != 0 && pSkillMgr != nullptr &&
				pSkillMgr->RollProbability(25, 0)) {
				m_aAbnormalState[0x0E].m_byUpgraded = 0;
				m_aAbnormalState[0x0E].m_byActive = 0;
				m_aAbnormalState[0x0E].m_by61 = 0;
				m_aAbnormalState[0x0E].m_dw6C = 0;
				dwRemaining &= ~0x4000u;
			}
			if (m_aAbnormalState[7].m_byActive != 0) {
				m_aAbnormalState[7].m_byUpgraded = 0;
				m_aAbnormalState[7].m_byActive = 0;
				m_aAbnormalState[7].m_by61 = 0;
				m_aAbnormalState[7].m_dw6C = 0;
				dwRemaining &= ~0x80u;
			}
			if (m_aAbnormalState[6].m_byActive != 0) {
				m_aAbnormalState[6].m_byUpgraded = 0;
				m_aAbnormalState[6].m_byActive = 0;
				m_aAbnormalState[6].m_by61 = 0;
				m_aAbnormalState[6].m_dw6C = 0;
				dwRemaining &= ~0x40u;
			}
		}
	}

	if (!bAnyEnded && m_dwAbnormalFlags == dwRemaining) {
		return; // 0x004A4547: nothing to tell the client
	}
	m_dwAbnormalFlags = dwRemaining; // 0x004A4552
	SendAbnormalStateUpdate();       // 0x004A4555
}

/**
 * [RECONSTRUCTED - Native 0x004EA280]
 * CGObjChar::GetPartyID
 * Returns Party ID from m_pParty at offset +0x1CB8 (or 0 if not in a party)
 */
uint32_t CGObjChar::GetPartyID() const {
	if (m_pParty == nullptr) {
		return 0;
	}
	// Native 0x004EA280: return *(m_pParty + 8)
	return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(m_pParty) + 8);
}

uint32_t CGObjChar::GetMainWeaponAttackSkillID() const {
	return 0;
}

bool CGObjChar::IsPickPetCOS() const {
	return IsTradeCOS(); // Slot 271 @ +0x43C (0x00483930)
}

bool CGObjChar::IsFortressHeart() const {
	return false; // Slot 244 @ +0x3D0 (0x00482970)
}

bool CGObjChar::IsSiegeTargetRestricted() const {
	return false; // Slot 405 @ +0x654 (Default false)
}

bool CGObjChar::IsDropUsable1() const {
	return true; // Slot 253 @ +0x3F4
}

bool CGObjChar::IsDropUsable2() const {
	return true; // Slot 254 @ +0x3F8
}

bool CGObjChar::IsGM() const {
	return false; // Slot 335 @ +0x53C
}

CGObjChar* CGObjChar::GetTransportVehicle(uint32_t dwSlot) const {
	(void)dwSlot;
	return nullptr; // Slot 350 @ +0x578
}

bool CGObjChar::IsVehicleActive() const {
	return false; // Slot 261 @ +0x414
}

/*
================
CGObjChar::FilterCheck
Slot 388 @ +0x610 [RECONSTRUCTED - 0x004AA310] (38 bytes)
================
*/
int32_t CGObjChar::FilterCheck( CMsg* pMsg ) {
	ASSERT( g_pMsgFilter != nullptr );
	return g_pMsgFilter->ValidateMessage( this, pMsg );
}

/*
================
CGObjChar SR_MSG map
[RECONSTRUCTED - table 0x00CCF110 (0x800 entries), built by 0x004B0B70]

Every entry starts as the unhandled handler 0x004B0DD0; ten opcodes are then registered, each
through a thunk that calls the virtual slot (0x004B23C0 - 0x004B2440, 0x0051CB30).
CORRECTION (Claude): the previous switch handled only 0x7070 / 0x7074 and just logged the rest.
================
*/
typedef void (CGObjChar::*PFN_CHAR_MSG_HANDLER)(CMsg* pMsg);

static PFN_CHAR_MSG_HANDLER s_apfnCharMsgMap[0x800];

// [RECONSTRUCTED - 0x004B0B70]
static bool CGObjChar_BuildMsgMap() {
	for (int32_t i = 0; i < 0x800; ++i) {
		s_apfnCharMsgMap[i] = &CGObjChar::OnMsg_Unhandled;
	}

	struct tagEntry {
		uint16_t             wOpcode;
		PFN_CHAR_MSG_HANDLER pfnHandler;
	};
	static const tagEntry s_aEntries[] = {
		{ 0x7021, &CGObjChar::OnMsg_7021 },
		{ 0x7022, &CGObjChar::OnMsg_7022 },
		{ 0x7023, &CGObjChar::OnMsg_7023 },
		{ 0x7024, &CGObjChar::OnMsg_7024 },
		{ 0x7025, &CGObjChar::OnMsg_7025 },
		{ 0x7070, &CGObjChar::OnMsg_SkillAction },
		{ 0x704F, &CGObjChar::OnMsg_704F },
		{ 0x7091, &CGObjChar::OnMsg_7091 },
		{ 0x7034, &CGObjChar::OnMsg_7034 },
		{ 0x7074, &CGObjChar::OnMsg_ActionCommand },
	};
	for (const tagEntry& entry : s_aEntries) {
		PFN_CHAR_MSG_HANDLER& pfnSlot = s_apfnCharMsgMap[entry.wOpcode & 0x7FF];
		ASSERT(pfnSlot == &CGObjChar::OnMsg_Unhandled);
		pfnSlot = entry.pfnHandler;
	}
	return true;
}

static const bool s_bCharMsgMapBuilt = CGObjChar_BuildMsgMap();

// Native 0x00D6DE14: index of the message being dispatched.
uint16_t g_wCharMsgMapIndex = 0;

/*
================
CGObjChar::DispatchMsg
Slot 376 @ +0x5E0 [RECONSTRUCTED - 0x004B0D70] (86 bytes)
================
*/
void CGObjChar::DispatchMsg( CMsg* pMsg ) {
	uint16_t wOpcode = pMsg->GetOpcode();
	uint16_t wIndex = static_cast<uint16_t>( wOpcode & 0x7FF );
	g_wCharMsgMapIndex = wIndex;

	if ( ( wOpcode & 0xFFF ) != wIndex ) {
		OnMsg_Unhandled( pMsg );
		return;
	}
	( this->*s_apfnCharMsgMap[wIndex] )( pMsg );
}

/*
================
CGObjChar::OnMsg_Unhandled
[PARTIAL - 0x004B0DD0] (204 bytes)

Logs, consumes, and sets the life state to 3 (gone) through slot 124. For players it also logs the
trade-abuse line when a transport is attached (0x004FD7F0, slot 219), logs the unknown sender and
removes the player from the world with reason 4 (0x004DF050). Slot 124 and slot 219 are not ported.
================
*/
void CGObjChar::OnMsg_Unhandled( CMsg* pMsg ) {
	BSLib::Log_Printf( 0, "Unhandled Game SR_MSG: 0x%x [data size: %d]", pMsg->GetOpcode(), *pMsg->m_pwPayloadLength & 0x7FFF );
	pMsg->Consume();

	if ( IsPlayer() ) {
		BSLib::Log_Printf( 0x2000001, "unknown packet sender detected!!! will be disconnected (char_name: %s)", GetName() );
		static_cast<CGObjPC*>( this )->Recall( 4 );
	}
}

/*
================
CGObjChar_PumpNetworkMsg
[RECONSTRUCTED - 0x004A8AD0] (259 bytes)

CORRECTION (Claude): a message is consumed (read offset = write offset) after it is handled, not
rewound, and every message goes through CServerProcessBase_CheckUnconsumedMessage before it is
freed. A message received while the character refuses messages (tagCharData +0x04 == 1) goes
straight to slot 358.
================
*/
uint32_t CGObjChar_PumpNetworkMsg( CGObjChar* pChar ) {
	CCmdSource* pCmdSource = pChar->GetCmdSource();
	if ( pCmdSource == nullptr ) {
		return 0;
	}

	CMsg* pMsg = nullptr;
	while ( pCmdSource->PopCommand( pMsg ) == 1 ) {
		g_dwLatestProcessedMsgID = pMsg->GetOpcode();

		if ( pChar->GetCharData()->m_byMsgProcState == 1 ) {
			pChar->OnClientPacket( reinterpret_cast<CPacket*>( pMsg ) );
			pMsg->Consume();
		} else if ( pChar->FilterCheck( pMsg ) == 1 ) {
			pChar->DispatchMsg( pMsg );
		} else {
			pMsg->Consume();
		}

		ServerFramework::CServerProcessBase_CheckUnconsumedMessage( pMsg );
		pChar->GetCmdSource()->FreeMsg( pMsg );
	}
	return 1;
}

/*
================
CGObjChar::GetCastRangeBonus
[RECONSTRUCTED - 0x004AC890] (62 bytes)
================
*/
uint16_t CGObjChar::GetCastRangeBonus() {
	return static_cast<uint16_t>( static_cast<int32_t>( m_paramKeeper.GetParamFloat( 0x21 ) ) );
}

/*
================
CGObjChar::GetEquippedPrimaryWeaponTID
[RECONSTRUCTED - 0x004EAD40] (55 bytes)
================
*/
uint16_t CGObjChar::GetEquippedPrimaryWeaponTID() const {
	CGItem* pWeapon = const_cast<CGStorage&>( m_storage ).GetItem( 6 );
	if ( pWeapon == nullptr ) {
		return 0;
	}
	// 4EAD63 checks the derived broken flag, not current durability.
	if ( static_cast<CGItemEquip*>( pWeapon )->IsBroken() ) {
		return 0;
	}
	return pWeapon->GetTID().wType;
}

/*
================
CGObjChar::GetEquippedAmmo
[RECONSTRUCTED - 0x004EC220] (41 bytes)
================
*/
CGItem* CGObjChar::GetEquippedAmmo() const {
	CGItem* pItem = const_cast<CGStorage&>( m_storage ).GetItem( 7 );
	if ( pItem == nullptr ) {
		return nullptr;
	}
	return pItem->IsEquipItem() ? pItem : nullptr;
}

/*
================
CGObjChar::GetEquippedAmmoCount
[RECONSTRUCTED - 0x004EC250] (58 bytes)
================
*/
int32_t CGObjChar::GetEquippedAmmoCount() const {
	CGItem* pItem = const_cast<CGStorage&>( m_storage ).GetItem( 7 );
	if ( pItem == nullptr ) {
		return -1;
	}
	ASSERT( pItem->IsEquipItem() );
	return pItem->GetCount();
}

/*
================
CGObjChar::AssignStructureApproachPos
[STUB - 0x004F10D0] (94 bytes)

Native: looks the structure up in g_pRefData +0xD0, then asks the navigation mesh (0x00CC387C
slot 28) for a point from the owner's position (+0x7C) within the structure's attack radius
(+0xDC), stored at +0x2284. Neither lookup is ported: fail closed, so the attack is refused.
================
*/
int32_t CGObjChar::AssignStructureApproachPos( uint32_t dwStructureRefID ) {
	(void)dwStructureRefID;
	return 0;
}

/*
================
CGObjChar::GetFortressStructureRefID
Slot 402 @ +0x648 [STUB - 0x004B9AB0]
================
*/
uint32_t CGObjChar::GetFortressStructureRefID() const {
	return 0;
}
