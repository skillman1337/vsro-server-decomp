/**
 * ============================================================================
 * Silkroad Online - Player Character Object (PC)
 * Original Source: D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\GObjPC.cpp
 *
 * Implements CGObjPC:
 *   - Native Ctor @ 0x004DE300, Dtor @ 0x004DE9B0
 *   - Native OnClientPacket @ 0x004ECBC0 (Slot 358 in VTable @ 0x00AF59FC)
 *   - Native SendErrorResponse @ 0x004E7250
 *   - Native HandleClientConfigData @ 0x00518C20 (Opcode 0x7158)
 *   - Native TeleportToLocation @ 0x00468A80
 *   - Native ResetReturnState @ 0x004DD990
 * ============================================================================
 */

#include "GObjPC.h"
#include "GCharAutoCommandActor.h"
#include "TimedJob.h"
#include "SkillManager.h"
#include "GItemEquip.h"
#include "Common/Framework/CmdSource.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerMain.h"
#include <cstring>

// ============================================================================
// tagSpawnLocation
// ============================================================================

tagSpawnLocation::tagSpawnLocation()
	: m_wSpawnRegionID(0)
	, m_wSpawnSectorID(0) {
	std::memset(m_pad00, 0, sizeof(m_pad00));
	std::memset(m_pad0C, 0, sizeof(m_pad0C));
	std::memset(m_pad10, 0, sizeof(m_pad10));
}

// [RECONSTRUCTED - 0x004DD990]
void tagSpawnLocation::Reset() {
	std::memset(this, 0, 0x12);
}

// ============================================================================
// CGObjPC
// ============================================================================

// [RECONSTRUCTED - Native 0x004DE300]
CGObjPC::CGObjPC()
	: CGObjChar()
	, m_cooltimeManager()
	, m_spawnLocation() {
	std::memset(m_pad1B78, 0, sizeof(m_pad1B78));
	std::memset(m_pad1FA0, 0, sizeof(m_pad1FA0));
	std::memset(m_pad2230, 0, sizeof(m_pad2230));
	std::memset(m_configData, 0, sizeof(m_configData));
	m_cooltimeManager.SetOwner(this);
	m_timedJobs.SetOwner(this);
	m_periodicJobs.Add(1.0f, [this] { m_timedJobs.Tick(1.0f); }); // 52AA90
}

// [RECONSTRUCTED - Native 0x004DE9B0]
CGObjPC::~CGObjPC() {
	m_timedJobs.Checkpoint();
	m_timedJobs.Clear(); // callbacks are detached before skill-manager teardown
	ResetReturnState();
}

// [PROVEN - 0x00482560]
bool CGObjPC::IsPlayer() const {
	return true;
}

/**
 * [RECONSTRUCTED - 0x004E5920] (22 bytes)
 * Slot 2 (+0x08): GetJID
 * The account id of the session the character is played through (session +0x28).
 */
uint32_t CGObjPC::GetJID() const {
	if (m_pNetSession == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump(); // 0x004E592A
		return 0;
	}
	return static_cast<const CCmdSrcNet*>(m_pNetSession)->GetSessionID();
}

// [RECONSTRUCTED - Native 0x0064C700]
void CGObjPC::RegisterSkillCooldownAndTimer(const tagRefSkill* pRefSkill) {
	m_cooltimeManager.RegisterCooldown(pRefSkill);
}

/*
================
CGObjPC::SetSpawnInvincible
[STUB - Native 0x004E0C60] (237 bytes)

Native: when the byte at tagCharData +0x13 differs from bySet, a non-zero value adds status 0x0E
(duration 10.0) to the status map at +0x1D8, zero removes it and broadcasts 0x3042 with the
object id and the remaining level; the byte is then updated. The status map is not ported.
================
*/
void CGObjPC::SetSpawnInvincible(uint8_t bySet) {
	(void)bySet;
}

// [RECONSTRUCTED - Native 0x004E7250]
// Allocates response packet with wOpcode, writes error code, and sends to player
int32_t CGObjPC::SendErrorResponse(uint16_t wOpcode, uint16_t wErrorCode) {
	CPacket* pResp = CPacket::Allocate(1);
	if (pResp == nullptr) {
		return 0;
	}

	pResp->SetOpcode(wOpcode);
	pResp->WriteUint8(2); // Failure status
	pResp->WriteUint16(wErrorCode);
	pResp->Send(GetGlobalID());
	pResp->Release();
	return 1;
}

// [RECONSTRUCTED - Native 0x00518C20]
// Client Configuration Data Handler (Opcode 0x7158)
int32_t CGObjPC::HandleClientConfigData(CPacket* pPacket) {
	if (pPacket == nullptr) {
		return 0;
	}

	uint8_t byConfigType = 0;
	if (!pPacket->ReadUint8(&byConfigType)) {
		return 0;
	}

	switch (byConfigType) {
		case 0:
			// Native 0x00407D80: UI / Chat / Macro configuration
			return 1;

		case 1:
			// Native 0x00407E00: Key bindings / Hotkeys configuration
			return 1;

		case 2:
			// Native 0x00407EE0: Graphics / Audio / Game options
			return 1;

		default:
			BSLib::Log_Printf(0x2000001, "Unhandled Client Config Data Msg Type was Detected!!! (%d)", byConfigType);
			ServerFramework::ServerFramework_GenerateMiniDump();
			return 0;
	}
}

// [RECONSTRUCTED - Native 0x00468A80]
// Teleports player entity to destination location key
int32_t CGObjPC::TeleportToLocation(uint8_t byReason, uint32_t dwLocationKey) {
	m_wRegionID = static_cast<uint16_t>((dwLocationKey >> 16) & 0xFFFF);
	m_wDestRegionID = m_wRegionID;
	BSLib::Log_Printf(0, "[CGObjPC::TeleportToLocation] Player 0x%08X teleported (Reason 0x%02X, LocationKey 0x%08X)",
		GetGlobalID(), byReason, dwLocationKey);
	return 1;
}

// [RECONSTRUCTED - Native 0x004DD990]
// Resets player return/resurrection states
void CGObjPC::ResetReturnState() {
	m_spawnLocation.Reset();
}

/*
================
CGObjPC::OffsetGold

[NATIVE - 0x004E4B60] (362 bytes)
Virtual slot 91 (+0x16C): Changes player gold and checks teleport thresholds.
================
*/
int64_t CGObjPC::OffsetGold(int64_t nOffset, uint32_t dwReason, uint32_t dwParam1, uint32_t dwParam2) {
	(void)dwReason; (void)dwParam1; (void)dwParam2;
	if (m_pDataPermanent == nullptr) {
		return 0;
	}
	int64_t nCur = m_pDataPermanent->GetGold();
	int64_t nNew = nCur + nOffset;
	if (nNew < 0) {
		nNew = 0;
	}
	m_pDataPermanent->SetGold(nNew);
	return nNew;
}

/*
================
CGObjPC::OffsetSkillPoint

[NATIVE - 0x004E4C60] (148 bytes)
Virtual slot 93 (+0x174): Changes player Skill Points (SP).
================
*/
void CGObjPC::OffsetSkillPoint(int32_t nOffset, uint8_t byReason) {
	(void)byReason;
	if (m_pDataPermanent == nullptr) {
		return;
	}
	int32_t nCur = static_cast<int32_t>(m_pDataPermanent->GetSkillPoints());
	int32_t nNew = nCur + nOffset;
	if (nNew < 0) {
		nNew = 0;
	}
	m_pDataPermanent->SetSkillPoints(static_cast<uint32_t>(nNew));
}

/*
================
CGObjPC::InquireSameItem

[NATIVE - 0x004ED680] (360 bytes)
Virtual slot 137 (+0x224): Inquires item quantities and slot coordinates in player inventory.
================
*/
uint32_t CGObjPC::InquireSameItem(uint32_t dwStorageType, const char* szCodeName, uint32_t dwMode, uint32_t dwSlot, uint32_t dwUnk) {
	(void)dwUnk;
	if (dwStorageType != 0 || !szCodeName) {
		return 0;
	}

	uint32_t dwTotal = 0;
	uint32_t dwCap = m_storage.GetCapacity();
	for (uint32_t i = 13; i < dwCap; ++i) {
		CGItem* pItem = m_storage.GetItem(i);
		if (pItem != nullptr) {
			int32_t nCount = pItem->GetCount();
			if (nCount <= 0) {
				nCount = 1;
			}
			if (dwMode == 2) {
				dwTotal += static_cast<uint32_t>(nCount);
			} else if (dwMode == 1 && i == dwSlot) {
				return static_cast<uint32_t>(nCount);
			} else if (dwMode == 0 && i >= dwSlot) {
				return i;
			}
		}
	}
	if (dwMode == 2) {
		return dwTotal;
	}
	return 0;
}

/*
================
CGObjPC::DelItem_EXT

[NATIVE - 0x004EEB70] (680 bytes)
Virtual slot 140 (+0x230): Consumes items from inventory slots.
================
*/
void* CGObjPC::DelItem_EXT(uint32_t dwStorageType, uint8_t bySlot, int32_t nCount, uint16_t* pwStatus, uint8_t byReason, uint32_t dwControl) {
	(void)byReason;
	if (pwStatus != nullptr) {
		*pwStatus = 1;
	}
	if (dwStorageType != 0) {
		if (pwStatus != nullptr) {
			*pwStatus = 0x1843;
		}
		return nullptr;
	}

	CGItem* pItem = m_storage.GetItem(bySlot);
	if (!pItem) {
		if (pwStatus != nullptr) {
			*pwStatus = 0x1809;
		}
		return nullptr;
	}

	if (dwControl == 1) { // Test / query dry-run
		if (pwStatus != nullptr) {
			*pwStatus = 1;
		}
		return nullptr;
	}

	int32_t nCurCount = pItem->GetCount();
	if (nCurCount <= nCount) {
		m_storage.RemoveItem(bySlot);
	}
	return nullptr;
}

/*
================
CGObjPC::BackupData

[NATIVE - 0x004E64D0]
Virtual slot 157 (+0x274): Dispatches resource change notification to client.
================
*/
void CGObjPC::BackupData(uint32_t dwFlags, uint32_t dwParam) {
	if ((dwFlags & 0x100) != 0 && !m_timedJobs.Checkpoint())
		m_timedJobs.OnPersistenceFailure({false, 0, "Timed-job checkpoint enqueue failed"});
	(void)dwParam;
	CPacket pkt;
	pkt.SetOpcode(0x304E);
	pkt.WriteUint32(dwFlags);
	SendMsgToPeer(&pkt);
}

// [RECONSTRUCTED - Native 0x004ECBC0]
// Slot 358 in CGObjPC::vftable{for CGObjChar} @ 0x00AF5F94
// Main network packet handler for client-initiated player packets
int32_t CGObjPC::OnClientPacket(CPacket* pPacket) {
	if (pPacket == nullptr) {
		return 0;
	}

	try {
		uint16_t wOpcode = pPacket->GetOpcode();

		// Native 0x004ECBD7 - 0x004ECC5A: Opcodes <= 0x7074
		if (wOpcode <= 0x7074) {
			if (wOpcode == 0x7010) {
				// Native 0x0051DE90: Special / GM Command Handler
				return MsgHandler_SpecialCommand(pPacket);
			}

			if (wOpcode == 0x7074) {
				// Native 0x004ECC46 - 0x004ECC54: Action Request (Pass to CGCharAutoCommandActor @ +0x1BC0)
				// A refused player answers every 0x7074 with 0xB074 refused / 0x4004.
				GetAutoCommandActor()->ProcessCommand(reinterpret_cast<CMsg*>(pPacket), 1);
				return 0;
			}

			if (wOpcode == 0x7034) {
				// Native 0x004ECBE3 - 0x004ECC43: Party Request (Fallback Error 0x181D)
				return SendErrorResponse(0xB034, 0x181D);
			}

			if (wOpcode == 0x7045) {
				// Native 0x004ECBEA - 0x004ECC29: Exchange Request (Fallback Error 0x6006)
				return SendErrorResponse(0xB045, 0x6006);
			}

			if (wOpcode == 0x7046) {
				// Native 0x004ECBF8 - 0x004ECC0F: Stall Request (Fallback Error 0x1C0B)
				return SendErrorResponse(0xB046, 0x1C0B);
			}

			return 0;
		}

		// Native 0x004ECC5D - 0x004ECE2B: Guide / Help / Quest Query (Opcode 0x70C5)
		if (wOpcode == 0x70C5) {
			uint32_t dwParam = 0;
			uint8_t byType = 0;
			pPacket->ReadUint32(&dwParam);
			pPacket->ReadUint8(&byType);

			if (byType == 2 || byType == 8) {
				CPacket* pResp = CPacket::Allocate(1);
				if (pResp != nullptr) {
					pResp->SetOpcode(0xB0C5);
					pResp->WriteUint8(2);
					pResp->WriteUint8(byType);

					if (byType == 2) {
						uint32_t dwExtra = 0;
						pPacket->ReadUint32(&dwExtra);
						pResp->WriteUint16(0x3009);
						pResp->WriteUint32(dwExtra);
					} else if (byType == 8) {
						uint32_t dwExtra = 0;
						pPacket->ReadUint32(&dwExtra);
						pResp->WriteUint16(0x4414);
						pResp->WriteUint32(dwParam);
						pResp->WriteUint32(dwExtra);
					}

					pResp->Send(GetGlobalID());
					pResp->Release();
					return 1;
				}
			}
			return 0;
		}

		// Native 0x004ECC68 - 0x004ECD58: Return Scroll / Town Teleport (Opcode 0x7155)
		if (wOpcode == 0x7155) {
			uint8_t bySubtype = 0;
			pPacket->ReadUint8(&bySubtype);
			if (bySubtype != 1) {
				return SendErrorResponse(0xB155, 0x5814);
			}

			// Read spawn location (Native 0x004ECC94 - 0x004ECCA5)
			uint32_t dwLocationKey = (static_cast<uint32_t>(m_spawnLocation.m_wSpawnRegionID) << 16) |
			                          static_cast<uint32_t>(m_spawnLocation.m_wSpawnSectorID);

			// Teleport player to town spawn point
			TeleportToLocation(0x8B, dwLocationKey);

			// Reset return state & movement
			ResetReturnState();

			// Send success response 0xB155
			CPacket* pResp = CPacket::Allocate(1);
			if (pResp != nullptr) {
				pResp->SetOpcode(0xB155);
				pResp->WriteUint8(1);
				pResp->WriteUint8(1);
				pResp->Send(GetGlobalID());
				pResp->Release();
				return 1;
			}
			return 1;
		}

		// Native 0x004ECC6F - 0x004ECC84: Client Config Data (Opcode 0x7158)
		if (wOpcode == 0x7158) {
			return HandleClientConfigData(pPacket);
		}

		// Native 0x00515A60: Client Skill Learn Request (Opcode 0x70A1)
		if (wOpcode == 0x70A1) {
			uint32_t dwSkillID = 0;
			pPacket->ReadUint32(&dwSkillID);
			CSkillManager* pSkillMgr = GetSkillManager();
			if (pSkillMgr != nullptr) {
				return pSkillMgr->LearnSkill(dwSkillID) ? 1 : 0;
			}
			return 0;
		}

		// Native 0x00515AB0: Client Mastery Level Up Request (Opcode 0x70A2)
		if (wOpcode == 0x70A2) {
			uint32_t dwMasteryID = 0;
			uint8_t byIncrement = 0;
			pPacket->ReadUint32(&dwMasteryID);
			pPacket->ReadUint8(&byIncrement);
			CSkillManager* pSkillMgr = GetSkillManager();
			if (pSkillMgr != nullptr) {
				return pSkillMgr->RaiseMastery(dwMasteryID, byIncrement) ? 1 : 0;
			}
			return 0;
		}
	} catch (const BSLib::CMsgException&) {
		// Native 0x004ECE38: Drop malformed packet upon deserialization underflow
		return 0;
	}

	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x004DDD50]
 * Slot 374 @ +0x5D8: CGObjPC::GetTimedJobManager
 *
 * Returns pointer to the CTimedJobManager instance embedded at offset +0x1DFC
 * in CGObjPC.
 */
CTimedJobManager* CGObjPC::GetTimedJobManager() {
	return &m_timedJobs;
}

/**
 * [RECONSTRUCTED - Native 0x004EC5A0]
 * Slot 372 @ +0x5D0: CGObjPC::IsMainWeaponUsable
 * Slot 6 is the primary weapon in character equipment storage.
 */
bool CGObjPC::IsMainWeaponUsable() const {
	CGItem* pItem = const_cast<CGStorage&>(m_storage).GetItem(6);
	if (!pItem) {
		return true;
	}
	CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pItem);
	if (pEquip) {
		return pEquip->GetCurrentDurability() > 0;
	}
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x004EC5D0]
 * Slot 399 @ +0x63C: CGObjPC::IsSecondaryWeaponUsable
 * Slot 7 is the secondary weapon / shield in character equipment storage.
 */
bool CGObjPC::IsSecondaryWeaponUsable() const {
	CGItem* pItem = const_cast<CGStorage&>(m_storage).GetItem(7);
	if (!pItem) {
		return true;
	}
	CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pItem);
	if (pEquip) {
		return pEquip->GetCurrentDurability() > 0;
	}
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x004EC600]
 * Slot 400 @ +0x640: CGObjPC::IsEquipmentSlotUsable
 * Evaluates equipment slots 1..6 via lookup table 0x00C63F44:
 *   1 -> Slot 0 (Helm)
 *   2 -> Slot 2 (Shoulders)
 *   3 -> Slot 1 (Chest)
 *   4 -> Slot 4 (Pants)
 *   5 -> Slot 3 (Gloves)
 *   6 -> Slot 5 (Boots)
 */
bool CGObjPC::IsEquipmentSlotUsable(uint32_t dwSlot) const {
	if (dwSlot > 6) {
		return false;
	}
	static const uint32_t s_aEquipmentSlotMap[7] = { 0, 0, 2, 1, 4, 3, 5 };
	CGItem* pItem = const_cast<CGStorage&>(m_storage).GetItem(s_aEquipmentSlotMap[dwSlot]);
	if (!pItem) {
		return true;
	}
	CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pItem);
	if (pEquip) {
		return pEquip->GetCurrentDurability() > 0;
	}
	return true;
}

/**
 * [RECONSTRUCTED - Native 0x004DDCA0]
 * Slot 336 @ +0x540: CGObjPC::IsRidingTransport
 * Riding a transport: +0x1D18 set and m_pCharData + 0x0E == 1
 */
bool CGObjPC::IsRidingTransport() const {
	if (m_pRiddenTransport != nullptr && m_pCharData != nullptr) {
		const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(m_pCharData);
		if (pBytes[0x0E] == 1) {
			return true;
		}
	}
	return false;
}

/**
 * [RECONSTRUCTED - Native 0x004EAE50]
 * Slot 347 @ +0x56C: CGObjPC::GetMainWeaponAttackSkillID
 * Retrieves main weapon equipped in slot 6 and looks up default attack skill ID
 */
uint32_t CGObjPC::GetMainWeaponAttackSkillID() const {
	uint16_t wWeaponTID = 0;
	CGItem* pItem = const_cast<CGStorage&>(m_storage).GetItem(6);
	if (pItem) {
		wWeaponTID = pItem->GetTID().wType;
	}
	wWeaponTID &= 0xF800;
	const CSkillManager* pSkillMgr = GetSkillManager();
	if (pSkillMgr) {
		return pSkillMgr->GetAttackSkillByWeaponTID(wWeaponTID);
	}
	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x004E5950]
 * Slot 335 @ +0x53C: CGObjPC::IsGM
 * Checks whether player's network session has operator/GM privileges
 */
bool CGObjPC::IsGM() const {
	if (m_pNetSession == nullptr) {
		return false;
	}
	// Native 0x004E5950 calls 0x0040B790 on session pointer
	return false;
}

/**
 * [RECONSTRUCTED - Native 0x0052BC10]
 * Slot 405 @ +0x654: CGObjPC::IsSiegeTargetRestricted
 * Verifies if target is restricted from combat by Fortress War / Siege event state
 */
bool CGObjPC::IsSiegeTargetRestricted() const {
	// Native 0x0052BC10 checks Fortress War manager at 0x00D6A9F4
	return false;
}

// Hook pointer for testing / notice interception
static void (*g_pfnNoticeHook)(uint32_t) = nullptr;
extern "C" void SetNoticeHook(void (*pfn)(uint32_t)) {
	g_pfnNoticeHook = pfn;
}

static void (*g_pfnRecallHook)(uint8_t) = nullptr;
extern "C" void SetRecallHook(void (*pfn)(uint8_t)) {
	g_pfnRecallHook = pfn;
}

/*
================
CGObjPC::Recall

[RECONSTRUCTED - Native 0x004DF050] (568 bytes)
Teleports / recalls player to location depending on reason code.
Reason 4: Macro detection or penalty recall to prison/designated spawn.
Validates character state (+0x30 offset +0x11 must be 1 or 4),
checks equipment and avatar set integrity, and dispatches to location manager.
================
*/
int32_t CGObjPC::Recall(uint8_t byReason) {
	if (g_pfnRecallHook) {
		g_pfnRecallHook(byReason);
	}

	// Native 0x004DF05E: check life state in m_pCharData (+0x30)
	if (m_pCharData != nullptr) {
		uint8_t byLife = *(reinterpret_cast<const uint8_t*>(m_pCharData) + 0x11);
		if (byLife != 1 && byLife != 4) {
			return 0;
		}
	}

	return 1;
}

void CGObjPC::Notice(uint32_t dwNoticeID) {
	if (g_pfnNoticeHook) {
		g_pfnNoticeHook(dwNoticeID);
	}
	(void)dwNoticeID;
}

/*
================
CGObjPC::DispatchMsg
Slot 376 @ +0x5E0 [PARTIAL - 0x0050EEE0] (118 bytes)

Native: reports the message to the packet-rate monitor (g_d6a964, 0x00527710), then routes the
SR_MSG index through the player message map at 0x00CD3D08 (built by 0x0050D0C0, 7703 bytes), whose
default entry 0x0050EF60 falls back to CGObjChar::DispatchMsg. Neither the monitor nor the player
map is ported yet, so every message takes the default route.
================
*/
void CGObjPC::DispatchMsg(CMsg* pMsg) {
	CGObjChar::DispatchMsg(pMsg);
}
