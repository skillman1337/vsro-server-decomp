/**
 * ============================================================================
 * Silkroad Online - Player Character Special Command Message Handler
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Object\GObjPC_MsgHandler_SpecialCommand.h
 *
 * Implements Opcode 0x7010 / Response 0xB010 GM and Operator Commands
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_OBJECT_GOBJPC_MSGHANDLER_SPECIALCOMMAND_H_
#define _SR_GAMESERVER_OBJECT_GOBJPC_MSGHANDLER_SPECIALCOMMAND_H_

#include <cstdint>
#include <string>

class CGObjPC;
namespace BSLib { class CPacket; }
using BSLib::CPacket;

// Sub-Opcodes for 0x7010 (Client Special Command)
enum ESpecialCommandSubOpcode : uint16_t {
	CMD_FIND_PLAYER                 = 0x01, // Find player location ("Pos of %s: %s (%.2f, %.2f, %.2f)")
	CMD_MOVE_TO_TOWN                = 0x02, // Teleport self to town
	CMD_MOVE_TARGET_TO_TOWN         = 0x03, // Teleport target player to town ("MoveTargetToTown: %s")
	CMD_SHOW_ME_STATISTICS          = 0x04, // World entity statistics ("ShowMeStatistics: [PC: %d, NPC: %d, ItemOnGrd: %d")
	CMD_SHOW_ME_CHAR_DATA           = 0x05, // Character stats ("Stat of %s >> Level: %d, Str: %d, Int: %d, HP: %d/%d MP: %d/%d")
	CMD_SUMMON_MONSTER              = 0x06, // Summon monster ("SummonMonster : [%d of %s]")
	CMD_DROP_ITEM                   = 0x07, // Spawn / drop item on ground ("DropItem: [%d of %s]")
	CMD_MOVE_TO_PLAYER              = 0x08, // Warp to target player ("MoveToPlayer: %s [%d, (%f, %f, %f)]")
	CMD_SET_GAME_TIME               = 0x0A, // Set world game time ("SetGameTime to %d")
	CMD_FORCE_KILL_MOB              = 0x0C, // Force kill mob by RefID ("ForceKillMob : [%d of %s]")
	CMD_BAN_PLAYER                  = 0x0D, // Ban / kick player ("BanPlayer: %s")
	CMD_SET_INVISIBLE               = 0x0E, // Toggle GM invisibility
	CMD_SET_INVINCIBLE              = 0x0F, // Toggle GM invincibility
	CMD_MOVE_TO_COORDINATES         = 0x10, // Teleport to coordinates ("MoveTo: [%d, (%f, %f, %f)]")
	CMD_RECALL_USER                 = 0x11, // Recall user to GM position ("Recall User: %s")
	CMD_RECALL_GUILD                = 0x12, // Recall entire guild to GM position ("Recall Guild: %s")
	CMD_QUERY_WEATHER               = 0x13, // Query region weather
	CMD_FIND_OBJECT_BY_ID           = 0x14, // Find game object by GID
	CMD_SPAWN_OBJECT_CONTEXT        = 0x15, // Spawn object context
	CMD_KILL_MOB_BY_NAME            = 0x16, // Kill mob by name ("KillMob: [%s]")
	CMD_WHO_CHAR_TO_ACCOUNT         = 0x19, // Who1: Query account username from character name
	CMD_WHO_ACCOUNT_TO_CHAR         = 0x1A, // Who2: Query character name and nickname from account
	CMD_INSERT_QUEST                = 0x1B, // Start quest: {CALL _InsertQuestinCharByName ( '%s', '%s' )}
	CMD_RESET_QUEST                 = 0x1C, // Reset quests: {CALL _ResetQuestByCharName ( '%s' )}
	CMD_COMPLETE_QUEST              = 0x1D, // Complete quest: {CALL _InsertEndQuestinCharByName ( '%s', '%s' )}
	CMD_REMOVE_QUEST                = 0x1E, // Remove quest: {CALL _RemoveQuestByCharName ( '%s' )}
	CMD_MOVE_TO_NPC                 = 0x1F, // Move to NPC: {CALL _GetNPCPosByCodename ('%s', ?, ?, ?, ?, ? )}
	CMD_SET_GM_MODE                 = 0x20, // Toggle GM mode
	CMD_EVENT_BROADCAST             = 0x21, // Broadcast event packet
	CMD_REG_EVENT_DAEMON            = 0x26, // Register event daemon ("EventDaemon Registered: [%s]")
	CMD_UNREG_EVENT_DAEMON          = 0x2A, // Unregister event daemon ("EventDaemon Unregistered: [%s]")
	CMD_TOGGLE_DAEMON_IPC           = 0x2B, // Toggle daemon state via ShardManager IPC (0x34BB)
	CMD_SET_WEATHER                 = 0x30, // Set weather effect
	CMD_TRIGGER_EVENT_PHASE         = 0x32, // Advance event phase
	CMD_SERVER_NOTICE               = 0x37  // Broadcast server-wide notice (0x3026)
};

// Response Status Codes for 0xB010
enum ESpecialCommandResponseCode : uint16_t {
	RES_SPECIAL_CMD_SUCCESS         = 1,
	RES_SPECIAL_CMD_TEXT            = 2,
	RES_SPECIAL_CMD_ERR_INVALID     = 5,
	RES_SPECIAL_CMD_ERR_NOT_FOUND   = 0x1401,
	RES_SPECIAL_CMD_ERR_FAILED      = 0x1402,
	RES_SPECIAL_CMD_ERR_NO_AUTH     = 0x1403
};

class CGObjPC_MsgHandler_SpecialCommand {
public:
	// [NATIVE - 0x0051DE90]
	// Adapter entry point dispatching to CGObjPC::MsgHandler_SpecialCommand
	static int32_t HandleSpecialCommand(CGObjPC* pPlayer, CPacket* pPacket);
};

#endif // _SR_GAMESERVER_OBJECT_GOBJPC_MSGHANDLER_SPECIALCOMMAND_H_
