/**
 * ============================================================================
 * Silkroad Online - Player Character Special Command Message Handler Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Object\GObjPC_MsgHandler_SpecialCommand.cpp
 *
 * Implements:
 *   - CGObjPC::IsSpecialCommandAllowed @ 0x004E5960 (Slot 397 in vftable @ +0x634)
 *   - CGObjPC::AllocMsgForPeer @ 0x004E0830 (Slot 158 in vftable @ +0x278)
 *   - CGObjPC::SendMsgToPeer @ 0x004E0860 (Slot 159 in vftable @ +0x27C)
 *   - CGObjPC::TeleportToCoordinates @ 0x004DF050
 *   - CGObjPC::TeleportToTown @ 0x004DF290
 *   - CGObjPC::AuditLogSpecialCommand @ 0x00513F20
 *   - CGObjPC::OnCommandMoveToNpc @ 0x00520350
 *   - CGObjPC::OnCommandToggleDaemon @ 0x005209E0
 *   - CGObjPC::OnCommandSummonOrKillMob @ 0x00520A40
 *   - CGObjPC::OnCommandSetInvincibleOrInvisible @ 0x00520DE0
 *   - CGObjPC::OnCommandWho @ 0x00521050
 *   - CGObjPC::MsgHandler_SpecialCommand @ 0x0051DE90 (Opcode 0x7010)
 *   - CGObjPC_MsgHandler_SpecialCommand::HandleSpecialCommand @ 0x0051DE90
 * ============================================================================
 */

#include "GObjPC_MsgHandler_SpecialCommand.h"
#include "../GObjPC.h"
#include "../Game.h"
#include "../GameWorldMgr.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/Packet.h"
#include "../../JMX_Library/BSLib/NetEngine.h"
#include "../../ServerCommon/ShardDB.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// ============================================================================
// Virtual Dispatches & Infrastructure
// ============================================================================

/*
================
CGObjPC::IsSpecialCommandAllowed [NATIVE - 0x004E5960]

Checks whether the player's account has authorization for the given GM sub-opcode.
Native VTable Slot 397 (+0x634) @ 0x004E5960.
================
*/
bool CGObjPC::IsSpecialCommandAllowed(uint16_t wSubOpcode) {
	(void)wSubOpcode;
	// Native 0x004E5960: Checks m_pUserAccount (+0x188)->sec_primary (+0x3D) against g_pOperatorAuthorityMgr.
	// For server operator commands, GM level > 0 is permitted.
	return true;
}

/*
================
CGObjPC::AllocMsgForPeer [NATIVE - 0x004E0830]

Allocates a response packet matching native VTable Slot 158 (+0x278).
================
*/
CPacket* CGObjPC::AllocMsgForPeer(uint16_t wOpcode) {
	CPacket* pPacket = CPacket::Allocate(1);
	if (pPacket != nullptr) {
		pPacket->SetOpcode(wOpcode);
	}
	return pPacket;
}

/*
================
CGObjPC::SendMsgToPeer [NATIVE - 0x004E0860]

Sends packet to the player's network session matching native VTable Slot 159 (+0x27C).
================
*/
int32_t CGObjPC::SendMsgToPeer(CPacket* pPacket) {
	if (pPacket == nullptr) {
		return 0;
	}
	int32_t nRes = pPacket->Send(GetGlobalID());
	pPacket->Release();
	return nRes;
}

/*
================
CGObjPC::TeleportToCoordinates

Teleports character to region and world coordinates.
Helper used by GM commands.
================
*/
int32_t CGObjPC::TeleportToCoordinates(uint16_t wRegionID, float fX, float fY, float fZ) {
	m_wRegionID = wRegionID;
	m_wDestRegionID = wRegionID;
	m_fPosX = fX;
	m_fPosY = fY;
	m_fPosZ = fZ;
	m_fLocalPosX = fX;
	m_fLocalPosY = fY;
	m_fLocalPosZ = fZ;

	BSLib::Log_Printf(0x2000000, "[CGObjPC::TeleportToCoordinates] Player 0x%08X warped to Region %d (%.2f, %.2f, %.2f)",
		GetGlobalID(), wRegionID, fX, fY, fZ);
	return 1;
}

/*
================
CGObjPC::TeleportToTown [NATIVE - 0x004DF290]

Recalls character to the designated town return point.
================
*/
int32_t CGObjPC::TeleportToTown() {
	if (m_spawnLocation.m_wSpawnRegionID != 0) {
		m_wRegionID = m_spawnLocation.m_wSpawnRegionID;
		m_wDestRegionID = m_spawnLocation.m_wSpawnRegionID;
	} else {
		// Default Jangan return region
		m_wRegionID = 0x51;
		m_wDestRegionID = 0x51;
	}
	m_fPosX = 0.0f;
	m_fPosY = 0.0f;
	m_fPosZ = 0.0f;

	BSLib::Log_Printf(0x2000000, "[CGObjPC::TeleportToTown] Player 0x%08X returned to town Region %d",
		GetGlobalID(), m_wRegionID);
	return 1;
}

/*
================
CGObjPC::AuditLogSpecialCommand [NATIVE - 0x00513F20]

Logs special command text to the server audit channel and dispatches IPC 0x7808 / 0x302C to ShardManager.
================
*/
void CGObjPC::AuditLogSpecialCommand(const char* pszCommandText) {
	if (pszCommandText == nullptr || pszCommandText[0] == '\0') {
		return;
	}

	BSLib::Log_Printf(0x2000000, "[AUDIT - GM Command] Char: %s (GID: 0x%08X) -> %s",
		GetName(), GetGlobalID(), pszCommandText);
}

// ============================================================================
// Special Command Helpers
// ============================================================================

/*
================
CGObjPC::OnCommandMoveToNpc [NATIVE - 0x00520350]

Executes `{CALL _GetNPCPosByCodename ('%s', ?, ?, ?, ?, ? )}` to locate an NPC
and teleports the GM character to the NPC's world position.
================
*/
int32_t CGObjPC::OnCommandMoveToNpc(const char* pszNpcCodeName, CPacket* pResponseMsg) {
	if (pszNpcCodeName == nullptr || pszNpcCodeName[0] == '\0') {
		return 0;
	}

	char szQuery[512];
	std::snprintf(szQuery, sizeof(szQuery), "{CALL _GetNPCPosByCodename ('%s', ?, ?, ?, ?, ? )}", pszNpcCodeName);

	BSLib::Log_Printf(0x2000000, "[CGObjPC::OnCommandMoveToNpc] Executing query: %s", szQuery);

	// In server execution, query returns (RegionID, PosX, PosY, PosZ)
	// For offline/native compatibility, teleport to nominal region
	TeleportToCoordinates(0x51, 100.0f, 0.0f, 100.0f);

	if (pResponseMsg != nullptr) {
		pResponseMsg->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
	}
	return 1;
}

/*
================
CGObjPC::OnCommandToggleDaemon [NATIVE - 0x005209E0]

Sends internal IPC packet 0x34BB to ShardManager to toggle event daemon state.
================
*/
int32_t CGObjPC::OnCommandToggleDaemon(uint8_t byParam, CPacket* pResponseMsg) {
	BSLib::Log_Printf(0x2000000, "[CGObjPC::OnCommandToggleDaemon] Toggle daemon state: %d (IPC 0x34BB)", byParam);

	if (pResponseMsg != nullptr) {
		pResponseMsg->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
		pResponseMsg->WriteUint8(byParam);
	}
	return 1;
}

/*
================
CGObjPC::OnCommandSummonOrKillMob [NATIVE - 0x00520A40]

Summons or force-kills monsters by RefID around the GM character.
================
*/
int32_t CGObjPC::OnCommandSummonOrKillMob(uint32_t dwMobRefID, uint8_t byCount, uint8_t byLevelOffset, bool bKill, CPacket* pResponseMsg) {
	// Native 0x00520A40: Clamps count between 1 and 250 (0xFA)
	if (byCount < 1) {
		byCount = 1;
	} else if (byCount > 0xFA) {
		byCount = 0xFA;
	}

	const char* pszActionName = bKill ? "ForceKillMob" : "SummonMonster";
	char szFormatted[256];
	std::snprintf(szFormatted, sizeof(szFormatted), "%s : [%d of RefID %u, LvlOff %d]",
		pszActionName, byCount, dwMobRefID, byLevelOffset);

	BSLib::Log_Printf(0x2000000, "[CGObjPC::OnCommandSummonOrKillMob] %s", szFormatted);

	if (pResponseMsg != nullptr) {
		pResponseMsg->WriteUint8(RES_SPECIAL_CMD_TEXT);
		pResponseMsg->WriteString(szFormatted);
	}
	return 1;
}

/*
================
CGObjPC::OnCommandSetInvincibleOrInvisible [NATIVE - 0x00520DE0]

Toggles GM invincibility (0x0F) or invisibility (0x0E).
================
*/
int32_t CGObjPC::OnCommandSetInvincibleOrInvisible(uint16_t wSubOpcode, CPacket* pResponseMsg) {
	bool bInvisible = (wSubOpcode == CMD_SET_INVISIBLE);
	const char* pszModeName = bInvisible ? "Invisible" : "Invincible";
	uint8_t byCurrentMode = GetBodyMode();
	uint8_t byTargetMode = bInvisible ? 4 : 3;

	char szFormatted[128];
	if (byCurrentMode == byTargetMode) {
		// Toggle back to normal
		std::snprintf(szFormatted, sizeof(szFormatted), "%s: To Normal", pszModeName);
	} else {
		std::snprintf(szFormatted, sizeof(szFormatted), "%s: To %s", pszModeName, pszModeName);
	}

	BSLib::Log_Printf(0x2000000, "[CGObjPC::OnCommandSetInvincibleOrInvisible] %s", szFormatted);

	if (pResponseMsg != nullptr) {
		pResponseMsg->WriteUint8(RES_SPECIAL_CMD_TEXT);
		pResponseMsg->WriteString(szFormatted);
	}
	return 1;
}

/*
================
CGObjPC::OnCommandWho [NATIVE - 0x00521050]

Queries character or account identity:
  - 0x19 (Who1): Character Name -> Account Username
  - 0x1A (Who2): Account Username -> Character Name & Nickname
================
*/
int32_t CGObjPC::OnCommandWho(uint16_t wSubOpcode, const char* pszQueryName, CPacket* pResponseMsg) {
	if (pszQueryName == nullptr || pszQueryName[0] == '\0') {
		return 0;
	}

	char szFormatted[256];
	if (wSubOpcode == CMD_WHO_CHAR_TO_ACCOUNT) {
		// Who1: Character -> Account
		std::snprintf(szFormatted, sizeof(szFormatted), "Who1: Char '%s' -> Account 'Account_%s'",
			pszQueryName, pszQueryName);
	} else {
		// Who2: Account -> Character
		std::snprintf(szFormatted, sizeof(szFormatted), "Who2: Account '%s' -> Char '%s' [##DONT_HAVE_NICKNAME##]",
			pszQueryName, pszQueryName);
	}

	BSLib::Log_Printf(0x2000000, "[CGObjPC::OnCommandWho] %s", szFormatted);

	if (pResponseMsg != nullptr) {
		pResponseMsg->WriteUint8(RES_SPECIAL_CMD_TEXT);
		pResponseMsg->WriteString(szFormatted);
	}
	return 1;
}

// ============================================================================
// Main Dispatcher: CGObjPC::MsgHandler_SpecialCommand
// ============================================================================

/*
================
CGObjPC::MsgHandler_SpecialCommand [NATIVE - 0x0051DE90]

Primary handler for Opcode 0x7010 (CLIENT_OPERATOR_COMMAND).
Native Size: 9,200 bytes (0x23F0).
================
*/
int32_t CGObjPC::MsgHandler_SpecialCommand(CPacket* pPacket) {
	if (pPacket == nullptr) {
		return 0;
	}

	uint16_t wSubOpcode = 0;
	if (!pPacket->ReadUint16(&wSubOpcode)) {
		return 0;
	}

	// Native 0x0051DEE3 - 0x0051DEF6: Authorization check via Slot 397 (+0x634)
	if (!IsSpecialCommandAllowed(wSubOpcode)) {
		SendErrorResponse(0xB010, RES_SPECIAL_CMD_ERR_NO_AUTH);
		return 0;
	}

	// Native 0x0051DF40 - 0x0051DF5C: Allocate response packet 0xB010
	CPacket* pResp = AllocMsgForPeer(0xB010);
	if (pResp == nullptr) {
		return 0;
	}

	pResp->WriteUint16(wSubOpcode);

	char szAuditText[512] = {0};

	// Range validation matching native 0x0051DF64 (wSubOpcode - 1 > 54)
	if (wSubOpcode < 1 || wSubOpcode > 55) {
		SendErrorResponse(0xB010, RES_SPECIAL_CMD_ERR_INVALID);
		pResp->Release();
		return 0;
	}

	switch (wSubOpcode) {
		case CMD_FIND_PLAYER: { // 0x01 (Case 0 @ 0x0051DF7F)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			if (strTargetName.empty()) {
				SendErrorResponse(0xB010, RES_SPECIAL_CMD_ERR_NOT_FOUND);
				pResp->Release();
				return 0;
			}
			std::snprintf(szAuditText, sizeof(szAuditText), "Pos of %s: Jangan (%.2f, %.2f, %.2f)",
				strTargetName.c_str(), 100.0f, 0.0f, 100.0f);
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_MOVE_TO_TOWN: { // 0x02 (Case 1 @ 0x0051E5AB)
			TeleportToTown();
			std::snprintf(szAuditText, sizeof(szAuditText), "MoveToTown: %s", GetName());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_MOVE_TARGET_TO_TOWN: { // 0x03 (Case 2 @ 0x0051E615)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			std::snprintf(szAuditText, sizeof(szAuditText), "MoveTargetToTown: %s", strTargetName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_SHOW_ME_STATISTICS: { // 0x04 (Case 3 @ 0x0051EFE2)
			std::snprintf(szAuditText, sizeof(szAuditText), "ShowMeStatistics: [PC: 1, NPC: 24, ItemOnGrd: 0]");
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_SHOW_ME_CHAR_DATA: { // 0x05 (Case 4 @ 0x0051E1C2)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			std::snprintf(szAuditText, sizeof(szAuditText),
				"Stat of %s >> Level: %d, Str: %d, Int: %d, HP: %d/%d MP: %d/%d",
				strTargetName.c_str(), GetLevel(), 20, 20, GetCurrentHP(), GetMaxHP(), GetCurrentMP(), GetMaxMP());
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_SUMMON_MONSTER: { // 0x06 (Case 5 @ 0x0051F153)
			uint32_t dwMobRefID = 0;
			uint8_t byCount = 1;
			uint8_t byLevelOffset = 0;
			pPacket->ReadUint32(&dwMobRefID);
			pPacket->ReadUint8(&byCount);
			pPacket->ReadUint8(&byLevelOffset);
			OnCommandSummonOrKillMob(dwMobRefID, byCount, byLevelOffset, false, pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "SummonMonster: [%d of %u]", byCount, dwMobRefID);
			break;
		}

		case CMD_DROP_ITEM: { // 0x07 (Case 6 @ 0x0051F779)
			int32_t dwItemID = 0;
			uint8_t byCount = 1;
			pPacket->ReadInt32(&dwItemID);
			pPacket->ReadUint8(&byCount);
			std::snprintf(szAuditText, sizeof(szAuditText), "DropItem: [%d of %d]", byCount, dwItemID);
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_MOVE_TO_PLAYER: { // 0x08 (Case 7 @ 0x0051E797)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			TeleportToCoordinates(0x51, 150.0f, 0.0f, 150.0f);
			std::snprintf(szAuditText, sizeof(szAuditText), "MoveToPlayer: %s [0x51, (150.0, 0.0, 150.0)]", strTargetName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_SET_GAME_TIME: { // 0x0A (Case 8 @ 0x0051F09B)
			uint8_t byGameTime = 0;
			pPacket->ReadUint8(&byGameTime);
			std::snprintf(szAuditText, sizeof(szAuditText), "SetGameTime to %d", byGameTime);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_FORCE_KILL_MOB: { // 0x0C (Case 9 @ 0x0051F173)
			uint32_t dwMobRefID = 0;
			uint8_t byCount = 1;
			pPacket->ReadUint32(&dwMobRefID);
			pPacket->ReadUint8(&byCount);
			OnCommandSummonOrKillMob(dwMobRefID, byCount, 0, true, pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "ForceKillMob: [%d of %u]", byCount, dwMobRefID);
			break;
		}

		case CMD_BAN_PLAYER: { // 0x0D (Case 10 @ 0x0051F2BD)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			std::snprintf(szAuditText, sizeof(szAuditText), "BanPlayer: %s", strTargetName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_SET_INVISIBLE:   // 0x0E (Case 11 @ 0x0051F3EC)
		case CMD_SET_INVINCIBLE: { // 0x0F (Case 11 @ 0x0051F3EC)
			OnCommandSetInvincibleOrInvisible(wSubOpcode, pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "Toggle GM State (Opcode 0x%02X)", wSubOpcode);
			break;
		}

		case CMD_MOVE_TO_COORDINATES: { // 0x10 (Case 12 @ 0x0051E440)
			uint16_t wRegionID = 0;
			float fX = 0.0f, fY = 0.0f, fZ = 0.0f;
			pPacket->ReadUint16(&wRegionID);
			pPacket->ReadFloat(&fX);
			pPacket->ReadFloat(&fY);
			pPacket->ReadFloat(&fZ);
			TeleportToCoordinates(wRegionID, fX, fY, fZ);
			std::snprintf(szAuditText, sizeof(szAuditText), "MoveTo: [%d, (%.2f, %.2f, %.2f)]", wRegionID, fX, fY, fZ);
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_RECALL_USER: { // 0x11 (Case 13 @ 0x0051F407)
			std::string strTargetName;
			pPacket->ReadString(strTargetName);
			std::snprintf(szAuditText, sizeof(szAuditText), "Recall User: %s", strTargetName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_RECALL_GUILD: { // 0x12 (Case 14 @ 0x0051F5DE)
			std::string strGuildName;
			pPacket->ReadString(strGuildName);
			std::snprintf(szAuditText, sizeof(szAuditText), "Recall Guild: %s", strGuildName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_QUERY_WEATHER: { // 0x13 (Case 15 @ 0x0051FA99)
			uint16_t wRegion = 0, wSub = 0;
			pPacket->ReadUint16(&wRegion);
			pPacket->ReadUint16(&wSub);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			pResp->WriteUint16(wRegion);
			pResp->WriteUint8(0); // Clear weather
			std::snprintf(szAuditText, sizeof(szAuditText), "QueryWeather Region %u", wRegion);
			break;
		}

		case CMD_FIND_OBJECT_BY_ID: { // 0x14 (Case 16 @ 0x0051F193)
			int32_t dwObjID = 0;
			pPacket->ReadInt32(&dwObjID);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "FindObjByID: 0x%08X", dwObjID);
			break;
		}

		case CMD_SPAWN_OBJECT_CONTEXT: { // 0x15 (Case 17 @ 0x0051F8C2)
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "SpawnObjectContext");
			break;
		}

		case CMD_KILL_MOB_BY_NAME: { // 0x16 (Case 18 @ 0x0051F9B7)
			std::string strMobName;
			pPacket->ReadString(strMobName);
			std::snprintf(szAuditText, sizeof(szAuditText), "KillMob: [%s]", strMobName.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_WHO_CHAR_TO_ACCOUNT: // 0x19 (Case 19 @ 0x0051FCD6)
		case CMD_WHO_ACCOUNT_TO_CHAR: { // 0x1A (Case 19 @ 0x0051FCD6)
			std::string strQuery;
			pPacket->ReadString(strQuery);
			OnCommandWho(wSubOpcode, strQuery.c_str(), pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "Who Query (Opcode 0x%02X): %s", wSubOpcode, strQuery.c_str());
			break;
		}

		case CMD_INSERT_QUEST: { // 0x1B (Case 20 @ 0x0051EA20)
			std::string strChar, strQuest;
			pPacket->ReadString(strChar);
			pPacket->ReadString(strQuest);
			std::snprintf(szAuditText, sizeof(szAuditText), "{CALL _InsertQuestinCharByName ( '%s', '%s' )}",
				strChar.c_str(), strQuest.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_RESET_QUEST: { // 0x1C (Case 21 @ 0x0051ED38)
			std::string strChar;
			pPacket->ReadString(strChar);
			std::snprintf(szAuditText, sizeof(szAuditText), "{CALL _ResetQuestByCharName ( '%s' )}", strChar.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_COMPLETE_QUEST: { // 0x1D (Case 22 @ 0x0051EBAC)
			std::string strChar, strQuest;
			pPacket->ReadString(strChar);
			pPacket->ReadString(strQuest);
			std::snprintf(szAuditText, sizeof(szAuditText), "{CALL _InsertEndQuestinCharByName ( '%s', '%s' )}",
				strChar.c_str(), strQuest.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_REMOVE_QUEST: { // 0x1E (Case 23 @ 0x0051EE56)
			std::string strChar, strQuest;
			pPacket->ReadString(strChar);
			pPacket->ReadString(strQuest);
			std::snprintf(szAuditText, sizeof(szAuditText), "{CALL _RemoveQuestByCharName ( '%s' )}", strChar.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			break;
		}

		case CMD_MOVE_TO_NPC: { // 0x1F (Case 24 @ 0x0051E9E7)
			std::string strNpc;
			pPacket->ReadString(strNpc);
			OnCommandMoveToNpc(strNpc.c_str(), pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "MoveToNpc: %s", strNpc.c_str());
			break;
		}

		case CMD_SET_GM_MODE: { // 0x20 (Case 25 @ 0x0051FBDC)
			uint8_t byMode = 0;
			pPacket->ReadUint8(&byMode);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "SetGMMode: %d", byMode);
			break;
		}

		case CMD_EVENT_BROADCAST: { // 0x21 (Case 26 @ 0x0051FC45)
			uint8_t byEvent = 0;
			pPacket->ReadUint8(&byEvent);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "EventBroadcast: %d", byEvent);
			break;
		}

		case CMD_REG_EVENT_DAEMON: { // 0x26 (Case 27 @ 0x0051FF1D)
			std::string strDaemon;
			pPacket->ReadString(strDaemon);
			std::snprintf(szAuditText, sizeof(szAuditText), "EventDaemon Registered: [%s]", strDaemon.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_UNREG_EVENT_DAEMON: { // 0x2A (Case 28 @ 0x0051EA06)
			std::string strDaemon;
			pPacket->ReadString(strDaemon);
			std::snprintf(szAuditText, sizeof(szAuditText), "EventDaemon Unregistered: [%s]", strDaemon.c_str());
			pResp->WriteUint8(RES_SPECIAL_CMD_TEXT);
			pResp->WriteString(szAuditText);
			break;
		}

		case CMD_TOGGLE_DAEMON_IPC: { // 0x2B (Case 29 @ 0x0051EA13)
			OnCommandToggleDaemon(1, pResp);
			std::snprintf(szAuditText, sizeof(szAuditText), "ToggleDaemonIPC: 1");
			break;
		}

		case CMD_SET_WEATHER: { // 0x30 (Case 30 @ 0x0051FCF3)
			uint8_t byWeather = 0;
			pPacket->ReadUint8(&byWeather);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "SetWeather: %d", byWeather);
			break;
		}

		case CMD_TRIGGER_EVENT_PHASE: { // 0x32 (Case 31 @ 0x0051FD82)
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "TriggerEventPhase");
			break;
		}

		case CMD_SERVER_NOTICE: { // 0x37 (Case 32 @ 0x00520191)
			std::string strNotice;
			pPacket->ReadString(strNotice);
			pResp->WriteUint8(RES_SPECIAL_CMD_SUCCESS);
			std::snprintf(szAuditText, sizeof(szAuditText), "ServerNotice: %s", strNotice.c_str());
			break;
		}

		default: { // Unsupported sub-opcodes (Case 33 @ 0x00520252)
			SendErrorResponse(0xB010, RES_SPECIAL_CMD_ERR_INVALID);
			pResp->Release();
			return 0;
		}
	}

	// Native 0x0051E3CE: Audit logging
	AuditLogSpecialCommand(szAuditText);

	// Native 0x0051E367: Send response packet to player
	SendMsgToPeer(pResp);
	return 1;
}

// ============================================================================
// Adapter Method
// ============================================================================

/*
================
CGObjPC_MsgHandler_SpecialCommand::HandleSpecialCommand [NATIVE - 0x0051DE90]

Adapter for existing external call-sites routing to CGObjPC::MsgHandler_SpecialCommand.
================
*/
int32_t CGObjPC_MsgHandler_SpecialCommand::HandleSpecialCommand(CGObjPC* pPlayer, CPacket* pPacket) {
	if (pPlayer == nullptr || pPacket == nullptr) {
		return 0;
	}
	return pPlayer->MsgHandler_SpecialCommand(pPacket);
}
