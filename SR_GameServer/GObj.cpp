/**
 * ============================================================================
 * Silkroad Online - Game Object Base Class Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObj.cpp
 *
 * Implements:
 *   - CGObj member functions
 *   - CGObj::GetTID @ 0x00485C90 (66 bytes)
 *   - CGObj::GetTypeID @ 0x00485CE0 (19 bytes)
 *   - CGObj::GetRefObjID @ 0x004824F0 (31 bytes)
 *   - IGObj::GetJID @ 0x0057D4B0 (37 bytes)
 *   - CGObj::GetGameID @ 0x00404050 (4 bytes)
 *   - CGObj::GetWorldID @ 0x00482510 (12 bytes)
 *   - CGObj::IsChar @ 0x00482530 (38 bytes)
 *   - CGObj::IsPlayer @ 0x00482560 (48 bytes)
 *   - CGObj::IsNonPlayer @ 0x00482590 (48 bytes)
 *   - CGObj::IsMonster @ 0x004825C0 (64 bytes)
 *   - CGObj::IsNPC @ 0x00482600 (64 bytes)
 *   - CGObj::IsCOS @ 0x004827B0 (64 bytes)
 *   - CGObj::IsActiveVehicle @ 0x004827F0 (80 bytes)
 *   - CGObj::IsAttackCOS @ 0x00482840 (79 bytes)
 *   - CGObj::GetCodeName @ 0x004824C0 (36 bytes)
 *   - CGObj::GetCountry @ 0x00482490 (48 bytes)
 *   - CGObj::SetAngle @ 0x00482440 (58 bytes)
 *   - CGObj::GetAngleToPosition @ 0x00485C20 (98 bytes)
 *   - CGObj::Spawn @ 0x00484B80 (364 bytes)
 *   - CGObj::EnterWorld @ 0x00485360 (985 bytes)
 *   - CGObj::MoveTo @ 0x00485740 (340 bytes)
 *   - CGObj::StepMovement @ 0x004858E0 (346 bytes)
 *   - CGObj::SetCellNode @ 0x00485BB0 (89 bytes)
 * ============================================================================
 */

#include "GObj.h"
#include "GObjChar.h"
#include "Game.h"
#include "GameWorld.h"
#include "GameWorldMgr.h"
#include "Map.h"
#include "Region.h"
#include "WorldMap.h"
#include "MsgBlock.h"
#include "../JMX_Library/BSLib/Packet.h"
#include "../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstring>
#include <cmath>

// Global entity intrusive lists
tagObjListNode* g_pSkillObjectListHead       = nullptr; // @ 0x00C825E0
tagObjListNode* g_pSkillObjectListCurrent    = nullptr; // @ 0x00C825E8
uint32_t        g_dwSkillObjectListIterFlags = 0;       // @ 0x00C825EC

tagObjListNode* g_pEventDaemonListHead       = nullptr; // @ 0x00C826E4
tagObjListNode* g_pEventDaemonListCurrent    = nullptr; // @ 0x00C826EC
uint32_t        g_dwEventDaemonListIterFlags = 0;       // @ 0x00C826F0

tagObjListNode* g_pObjStructListHead         = nullptr; // @ 0x00C82610
tagObjListNode* g_pObjStructListCurrent      = nullptr; // @ 0x00C82618
uint32_t        g_dwObjStructListIterFlags   = 0;       // @ 0x00C8261C

// ============================================================================
// Construction / Destruction
// ============================================================================

CGObj::CGObj()
	: m_dwGameID(0)
	, m_fDirX(1.0f)
	, m_fDirY(0.0f)
	, m_fDirZ(0.0f)
	, m_pCharData(nullptr)
	, m_pDataPermanent(nullptr)
	, m_dwWorldID(0)
	, m_dwMoveQueryFlag(0) {
	std::memset(m_pad0C, 0, sizeof(m_pad0C));
	std::memset(m_pad3C, 0, sizeof(m_pad3C));
}

CGObj::~CGObj() {
	// Destructor @ 0x004848E0
}

uint32_t CGObj::GetGlobalID() const {
	return m_dwGameID;
}

uint32_t CGObj::GetTacticsIndex() const {
	return m_dwTacticsIndex;
}

// ============================================================================
// Virtual Method Implementations (VTable @ 0x00AE7C44)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x004824F0]
 * Slot 1 (+0x04): GetRefObjID
 * Reads m_dwRefObjID from m_pDataPermanent->m_pRefObjCommon (+0x18).
 */
uint32_t CGObj::GetRefObjID() const {
	CInstanceChar* pData = m_pDataPermanent;
	if (!pData || !pData->m_pRefObjCommon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return (pData && pData->m_pRefObjCommon) ? pData->m_pRefObjCommon->m_dwRefObjID : 0;
	}
	return pData->m_pRefObjCommon->m_dwRefObjID;
}

/**
 * [RECONSTRUCTED - 0x0057D4B0]
 * Slot 2 (+0x08): GetJID
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint32_t CGObj::GetJID() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetJID Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00404050]
 * Slot 3 (+0x0C): GetGameID
 */
uint32_t CGObj::GetGameID() const {
	return m_dwGameID;
}

/**
 * [RECONSTRUCTED - 0x00485CE0]
 * Slot 4 (+0x10): GetTypeID
 */
uint16_t CGObj::GetTypeID() const {
	return GetTID().wType;
}

/**
 * [RECONSTRUCTED - 0x00482510]
 * Slot 5 (+0x14): GetWorldID
 */
uint32_t CGObj::GetWorldID() const {
	return m_dwWorldID;
}

/**
 * [RECONSTRUCTED - 0x00482530]
 * Slot 6 (+0x18): IsChar
 */
bool CGObj::IsChar() const {
	return GetTID().IsChar();
}

/**
 * [RECONSTRUCTED - 0x00482560]
 * Slot 7 (+0x1C): IsPlayer
 */
bool CGObj::IsPlayer() const {
	return GetTID().IsPlayer();
}

/**
 * [RECONSTRUCTED - 0x00482590]
 * Slot 8 (+0x20): IsNonPlayer
 */
bool CGObj::IsNonPlayer() const {
	return GetTID().IsNonPlayer();
}

/**
 * [RECONSTRUCTED - 0x004825C0]
 * Slot 9 (+0x24): IsNPC / IsStructure (TID 0x100)
 */
bool CGObj::IsNPC() const {
	return GetTID().IsNPC();
}

bool CGObj::IsStructure() const {
	return GetTID().IsStructure();
}

/**
 * [RECONSTRUCTED - 0x00482600]
 * Slot 10 (+0x28): IsMonster / IsMob (TID 0x80)
 */
bool CGObj::IsMonster() const {
	return GetTID().IsMonster();
}

/**
 * [RECONSTRUCTED - 0x004827B0]
 * Slot 11 (+0x2C): IsCOS
 */
bool CGObj::IsCOS() const {
	return GetTID().IsCOS();
}

/**
 * [RECONSTRUCTED - 0x004827F0]
 * Slot 12 (+0x30): IsActiveVehicle
 * Verifies (TID & 0xF800) == 0x1000 for riding mounts / transport vehicles.
 */
bool CGObj::IsActiveVehicle() const {
	tagTID tid = GetTID();
	return tid.IsNonPlayer() && tid.IsCOS() && ((tid.wType & 0xF800) == 0x1000);
}

/**
 * [RECONSTRUCTED - 0x00482840]
 * Slot 13 (+0x34): IsAttackCOS
 * The TID of an attack companion: valid, character class, non player, COS subcategory and specific 0x800.
 */
bool CGObj::IsAttackCOS() const {
	tagTID tid = GetTID();
	return tid.IsCOS() && ((tid.wType & 0xF800) == 0x800);
}

/**
 * [RECONSTRUCTED - 0x0057E060]
 * Slot 59 (+0xEC): GetName / GetCharName
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
const char* CGObj::GetName() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetCharName Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00485EE0]
 * Slot 62 (+0xF8): GetLifeState
 */
uint8_t CGObj::GetLifeState() const {
	return m_pCharData ? m_pCharData->m_byLifeState : 0;
}

/**
 * [RECONSTRUCTED - 0x0057E120]
 * Slot 63 (+0xFC): GetMotionState
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetMotionState() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetMotionState Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E150]
 * Slot 64 (+0x100): GetBodyMode
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetBodyMode() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetBodyMode Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E180]
 * Slot 65 (+0x104): GetParamFloat
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
float CGObj::GetParamFloat(uint32_t dwParamID) const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetParam(%d) Entered! TypeID: %u", dwParamID, static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0.0f;
}

/**
 * [RECONSTRUCTED - 0x0057E1B0]
 * Slot 66 (+0x108): GetCurrentHP
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint32_t CGObj::GetCurrentHP() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetCurHP Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E1E0]
 * Slot 67 (+0x10C): GetCurrentMP
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint32_t CGObj::GetCurrentMP() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetCurMP Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E1B0]
 * Slot 68 (+0x110): GetMaxHP
 */
uint32_t CGObj::GetMaxHP() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetCurHP Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E1E0]
 * Slot 69 (+0x114): GetMaxMP
 */
uint32_t CGObj::GetMaxMP() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetCurMP Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E210]
 * Slot 71 (+0x11C): GetJobState
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetJobState() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetJobState Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E240]
 * Slot 72 (+0x120): GetTeleportState
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetTeleportState() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetTeleportState Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E270]
 * Slot 73 (+0x124): GetLevel (Illegal invocation handler)
 */
uint8_t CGObj::GetLevel() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetLevel Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E2A0]
 * Slot 74 (+0x128): GetMaxLevel
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetMaxLevel() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetMaxLevel Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057F290]
 * Slot 75 (+0x12C): GetMonsterClass
 * Illegal invocation handler for base IGObj: logs error with TypeID and triggers mini dump.
 */
uint8_t CGObj::GetMonsterClass() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetMonsterClass Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0057E480]
 * Slot 87 (+0x15C): ShowDebugMsg / OutputChatMsg
 * Illegal invocation handler for base IGObj.
 */
void CGObj::ShowDebugMsg(const char* pszMsg) {
	(void)pszMsg;
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::OutputChatMsg Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
}

/**
 * [RECONSTRUCTED - 0x0057F6B0]
 * Slot 186 (+0x2E8): IHaveTradeItem
 * Illegal invocation handler for base IGObj.
 */
bool CGObj::IHaveTradeItem() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::IHaveTradeItem Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return false;
}

/**
 * [RECONSTRUCTED - 0x0057F800]
 * Slot 192 (+0x300): GetMonsterType
 * Illegal invocation handler for base IGObj.
 */
uint8_t CGObj::GetMonsterType() const {
	BSLib::Log_Printf(0x2000001, "## Illegal ##  IGObj::GetMonsterType Entered! TypeID: %u", static_cast<uint32_t>(GetTypeID()));
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00484D10]
 * Slot 211 (+0x34C): Base entity simulation tick
 */
void CGObj::OnTick(float fDeltaSec) {
	(void)fDeltaSec;
}

/**
 * [RECONSTRUCTED - 0x00482D80] (38 bytes)
 * Slot 18 (+0x48): IsItem
 */
bool CGObj::IsItem() const {
	uint16_t wType = GetTID().wType;
	return (wType & 0x02) == 0 && (wType & 0x1C) == 0x0C;
}

/**
 * [RECONSTRUCTED - 0x00482AB0] (64 bytes)
 * Slot 248 (+0x3E0): IsFortressStructure
 */
bool CGObj::IsFortressStructure() const {
	tagTID tid = GetTID();
	uint16_t wType = tid.wType;
	return ((wType & 0x02) != 0 && (wType & 0x1C) == 0x04 && (wType & 0x60) == 0x40 && (wType & 0x780) == 0x280);
}

// ============================================================================
// Member Functions
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00485C90] (66 bytes)
 * GetTID
 */
tagTID CGObj::GetTID() const {
	CInstanceChar* pData = m_pDataPermanent;
	if (!pData) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		BSLib::Log_Printf(0x2000001, "CGObj::GetTID() GetDataPermanent Error");
		return tagTID(0);
	}

	if (!pData->m_pRefObjCommon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return tagTID(0);
	}

	return tagTID(pData->m_pRefObjCommon->m_wTypeID);
}

/**
 * [RECONSTRUCTED - 0x004824C0] (36 bytes)
 * GetCodeName
 */
const char* CGObj::GetCodeName() const {
	CInstanceChar* pData = m_pDataPermanent;
	if (!pData || !pData->m_pRefObjCommon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return "";
	}
	return pData->m_pRefObjCommon->m_strCodeName.c_str();
}

/**
 * [RECONSTRUCTED - 0x00482490] (48 bytes)
 * GetCountry
 */
uint32_t CGObj::GetCountry() const {
	CInstanceChar* pData = m_pDataPermanent;
	if (!pData || !pData->m_pRefObjCommon) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	return pData->m_pRefObjCommon->m_dwCountry;
}

/**
 * [RECONSTRUCTED - 0x00482440] (58 bytes)
 * SetAngle
 */
void CGObj::SetAngle(float fAngle) {
	m_fDirX = std::cos(fAngle);
	m_fDirY = 0.0f;
	m_fDirZ = std::sin(fAngle);
}

/**
 * [RECONSTRUCTED - 0x00485C20] (98 bytes)
 * GetAngleToPosition
 * The heading that points from where this object stands at pPos in wRegion. The relative vector is planar,
 * so the elevation plays no part, and the angle comes straight out of atan2(dz, dx) (0x00485C6E).
 */
float CGObj::GetAngleToPosition(uint16_t wRegion, const SRO_Vector3D* pPos) const {
	const SRO_Vector3D ownPos(m_Location.fPosX, m_Location.fPosY, m_Location.fPosZ); // 0x00485C29
	SRO_Vector3D relative;
	Pos_RelativePlanar(&relative, GetRegionID(), &ownPos, wRegion, pPos);            // 0x00485C5E

	return static_cast<float>(std::atan2(static_cast<double>(relative.z),
		static_cast<double>(relative.x)));                                          // 0x00485C6E
}

/**
 * [RECONSTRUCTED - 0x00484B80] (364 bytes)
 * Spawn
 * Native assert at line 382: "m_GID._LocalID < ::pow((double)2, MAX_LOCALOBJID_BITS)"
 */
bool CGObj::Spawn(uint32_t dwGameID, CInstanceChar* pDataPermanent, uint32_t dwWorldID) {
	if (!pDataPermanent) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	m_dwGameID = dwGameID;
	uint32_t dwLocalID = dwGameID & 0x1FFFFF; // 21 bits (MAX_LOCALOBJID_BITS)
	if (static_cast<double>(dwLocalID) >= std::pow(2.0, 21.0)) {
		BSLib::Log_Printf(0x2000000, "GOBJID is Overflow!! [Max: %d]", 1 << 21);
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	m_pDataPermanent = pDataPermanent;
	m_dwWorldID = dwWorldID;
	return true;
}

/**
 * [PARTIAL - 0x00485360] (985 bytes)
 * EnterWorld
 * Stores the location, resolves it against the navmesh when it carries no cell, turns the object to the entry
 * angle, resolves the game world / layer / world context controller and finally places it through MoveTo mode 7.
 * The world entry events (0x0063D4A0 / 0x0063D190 with codes 0 and 2) and the instance game world registration
 * (0x005F7330) are not ported.
 */
bool CGObj::EnterWorld(uint32_t dwWorldID, const tagObjLocation& location, float fAngle) {
	const uint16_t wGameWorldID = static_cast<uint16_t>(dwWorldID & 0xFFFF);
	const uint16_t wLayerID = static_cast<uint16_t>(dwWorldID >> 16);

	m_Location = location;

	// 0x00485399: slot 11 drops an unresolved position onto the navmesh, giving it its cell and height
	if (m_Location.pNavCell == nullptr &&
		NavMesh::g_pRegionManager->CheckPointValid(&m_Location, 0) == 0) {
		BSLib::Log_Printf(0x2000001,
			"CGObj::EnterWorld Failed!!! at ResolveCellAndHeight() GAMEWORLDID[%d,%d], Pos(%.3f,%.3f,%.3f)",
			wGameWorldID, wLayerID, location.fPosX, location.fPosY, location.fPosZ);
		return false;
	}

	SetAngle(fAngle); // slot 230 (0x00482440)

	ASSERT(wGameWorldID >= 1);          // 0x004853F7
	ASSERT(g_pGameWorldMgr != nullptr); // 0x00485416
	if (g_pGameWorldMgr == nullptr) {
		return false;
	}

	m_dwWorldID = dwWorldID;

	m_pGameWorld = g_pGameWorldMgr->FindGameWorld(wGameWorldID);
	if (m_pGameWorld == nullptr) {
		BSLib::Log_Printf(0x2000001, "CGObj::EnterWorld Failed!!! at GetWorld() GAMEWORLDID[%d,%d]",
			wGameWorldID, wLayerID);
		return false;
	}

	m_pGameWorldLayer = m_pGameWorld->GetLayer(wLayerID);
	if (m_pGameWorldLayer == nullptr) {
		BSLib::Log_Printf(0x2000001, "CGObj::EnterWorld Failed!!! at GetWorldLayer() GAMEWORLDID[%d,%d]",
			wGameWorldID, wLayerID);
		return false;
	}

	// 0x00485505: the world context controller (CGameWorld vftable[23]) is not ported, so the object keeps none
	m_pWorldContextController = nullptr;

	if (MoveTo(m_Location, 7) == 0) {
		BSLib::Log_Printf(0x2000001,
			"CGObj::EnterWorld Failed!!! at MoveTo() GAMEWORLDID[%d,%d] Pos(%.3f,%.3f, %.3f) Reason(%d) Err(%d)",
			wGameWorldID, wLayerID, m_Location.fPosX, m_Location.fPosY, m_Location.fPosZ, 0, 0);
		return false;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x00485740] (340 bytes)
 * MoveTo
 * Mode 7 places the object: the destination is validated on its own and replaces the location, dropping the
 * region and the cell node. Any other mode asks the region manager to walk from the current location to the
 * destination; a blocked query (0x10000000) leaves the object where it stood.
 */
int32_t CGObj::MoveTo(tagObjLocation destination, uint8_t byMode) {
	int32_t nMoveResult = 0;

	if (byMode != 7) {
		// 0x0048575E: players and their COS move as actor mode 1, everything else as 0
		const int32_t nActorMode = (IsPlayer() || IsCOS()) ? 1 : 0;
		nMoveResult = NavMesh::g_pRegionManager->QueryMovement(nActorMode,
			static_cast<int32_t>(m_dwMoveQueryFlag), &m_Location, &destination, nullptr, this);

		if ((nMoveResult & 0x10000000) != 0) {
			destination = m_Location; // 0x004857A9: the object keeps the position it had
			ShowDebugMsg("MoveTo");   // slot 148 (0x00485FD0) mirrors the location into the debug window
			return 0;
		}
	} else {
		SetCellNode(nullptr, 0, 0); // slot 236 (0x00485BB0): leave the current cell node

		if (NavMesh::g_pRegionManager->CheckPointValid(&destination, 0) == 0) {
			return 0;
		}
		m_Location = destination;
		m_pRegion = nullptr;         // 0x004857BB
		m_pMsgBlock = nullptr; // 0x004857C2
	}

	if (StepMovement(destination, byMode) == 0) {
		return 0;
	}
	if ((nMoveResult & 1) == 0) {
		return 2;
	}
	ShowDebugMsg("MoveTo");
	return 1;
}

/**
 * [PARTIAL - 0x004858E0] (346 bytes)
 * StepMovement
 * Commits a reached position into the location block and keeps the object in the cell node of the region that
 * owns it: while the position stays inside the current cell (mode 3, or a cell that still accepts it) nothing
 * else happens, otherwise the region of the new position takes the object over.
 * The region objects (CRgnTerrain and its CMsgBlocks) are not ported and CMap has no world map to look one up
 * in, so the position is committed without that bookkeeping - which feeds the visibility lists, not where the
 * object stands.
 */
int32_t CGObj::StepMovement(const tagObjLocation& location, uint8_t byMode) {
	if (location.pNavCell == nullptr) {
		return 0; // 0x004858EA: a position without a cell is never committed
	}

	CRegion* pRegion = nullptr;
	if (g_pMap != nullptr && g_pGameWorldMgr != nullptr) {
		const tagRegionContext* pContext = reinterpret_cast<const tagRegionContext*>(&m_dwWorldID);
		CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(pContext->wGameWorldID);
		CWorldMap* pWorldMap = (pGameWorld != nullptr) ? pGameWorld->m_pWorldMap : nullptr;
		if (pWorldMap != nullptr) {
			pRegion = pWorldMap->FindRegion(location.wRegionID); // 0x00485A02
		}
	}

	// 0x00485A4F: entering another region hands the object over to it, and the cell node that region assigns is
	// what slot 236 stores. Neither exists in the port, so only the position is committed.
	if (pRegion != nullptr && pRegion != m_pRegion) {
		m_pRegion = pRegion;
		m_pMsgBlock = nullptr;
	}

	m_Location = location; // 0x00485A85 - 0x00485AA0
	(void)byMode;          // mode 3 keeps the object in its cell node (0x0048595A)
	return 1;
}

/**
 * [PARTIAL - 0x00485BB0] (89 bytes)
 * SetCellNode
 * Slot 236. The block the object is leaving is told first, then the one it is arriving in, and only then is
 * the new block stored. Both notifications are what keeps the layer maps - and with them everybody's view -
 * in step with where the object stands.
 * Not ported: the store itself goes through slot 299 (+0x4AC) in the native (0x00485BFA), which this port
 * does not carry, so m_pMsgBlock is written here.
 */
int32_t CGObj::SetCellNode(CMsgBlock* pNewBlock, int32_t nMode299, int32_t nEnterMode) {
	if (m_pMsgBlock != nullptr) {
		m_pMsgBlock->OnObjectLeave(this, pNewBlock); // 0x00485BCE: slot 3
	}
	if (pNewBlock != nullptr) {
		pNewBlock->OnObjectEnter(this, m_pMsgBlock, nEnterMode); // 0x00485BE8: slot 2
	}

	(void)nMode299;         // 0x00485BFA: the argument slot 299 is given alongside the new block
	m_pMsgBlock = pNewBlock;
	return 1;
}

/**
 * [PARTIAL - 0x00484D90] (81 bytes)
 * SendPacketToNearbySessions
 * Everything a player has to see about this object goes out through here: the message block the object
 * stands in collects the players of the object's own layer from itself and from its neighbours.
 * CORRECTION (Claude): this used to live on CGObjChar and only ever reached the object's own session, so
 * nothing any character did was visible to anybody else.
 * Not ported: the tail hands the packet to g_pNetEngine slot 19 (0x00484DDD) for the wire flush; the port's
 * recipients are sent to directly, so there is nothing left to flush.
 */
int32_t CGObj::SendPacketToNearbySessions(CPacket* pPacket) {
	if (pPacket == nullptr) {
		return 0;
	}

	if (m_pMsgBlock != nullptr) {                              // 0x00484D91
		m_pMsgBlock->SendPacketToLayer(GetLayerID(), pPacket);  // 0x00484DC0: slot 6
	}

	pPacket->ResetReadCursors(); // 0x00484DC3
	return 1;
}

// ============================================================================
// Object Capability & Permission Checks (tagRefObjCommon + 0x8C)
// ============================================================================

bool CGObj::CanTrade() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return (m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags & 1) != 0;
}

bool CGObj::CanSell() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 1) & 1) != 0;
}

bool CGObj::CanStore() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 3) & 1) != 0;
}

bool CGObj::CanExchange() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 4) & 1) != 0;
}

bool CGObj::CanPK() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 14) & 1) != 0;
}

bool CGObj::CanRepair() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 5) & 1) != 0;
}

bool CGObj::CanEnchant() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 6) & 1) != 0;
}

bool CGObj::CanAlchemy() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 7) & 1) != 0;
}

bool CGObj::CanSocket() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 8) & 1) != 0;
}

bool CGObj::CanEquip() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 9) & 1) != 0;
}

bool CGObj::CanUse() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 10) & 1) != 0;
}

bool CGObj::CanThrow() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 11) & 1) != 0;
}

bool CGObj::CanReinforce() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 12) & 1) != 0;
}

bool CGObj::CanDeconstruct() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) return false;
	return ((m_pDataPermanent->m_pRefObjCommon->m_dwTypeDetailFlags >> 13) & 1) != 0;
}

// ============================================================================
// Extended Subtype Discriminators
// ============================================================================

bool CGObj::IsFellowCOS() const {
	tagTID tid = GetTID();
	return tid.IsNonPlayer() && tid.IsCOS() && ((tid.wType & 0xF800) == 0x2800);
}

bool CGObj::IsFortressSmallTower() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20 && (tid.wType & 0x780) == 0x380 && (tid.wType & 0xF800) == 0x800;
}

bool CGObj::IsFortressBigTower() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20 && (tid.wType & 0x780) == 0x380 && (tid.wType & 0xF800) == 0x1000;
}

bool CGObj::IsFortressCommandTower() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20 && (tid.wType & 0x780) == 0x380 && (tid.wType & 0xF800) == 0x1800;
}

bool CGObj::IsFortressShieldStructure() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20 && (tid.wType & 0x780) == 0x680 && (tid.wType & 0xF800) == 0x2000;
}

bool CGObj::IsOuterGateObject() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20;
}

bool CGObj::IsInnerGateObject() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x20;
}

bool CGObj::IsGuildWarFlagObject() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x60 && (tid.wType & 0x780) == 0x500 && (tid.wType & 0xF800) == 0x2000;
}

bool CGObj::IsBattleCampFlagObject() const {
	tagTID tid = GetTID();
	return (tid.wType & 2) == 0 && (tid.wType & 0x1C) == 0x0C && (tid.wType & 0x60) == 0x60;
}

// ============================================================================
// Global Object Manager Functions
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00485D90] (41 bytes)
 * ObjMgr_FindByID
 */
CGObjChar* ObjMgr_FindByID(uint32_t dwObjectID) {
	if (!g_pGame) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	return g_pGame->FindObjectByID(dwObjectID);
}

/**
 * [RECONSTRUCTED - 0x00485DF0] (35 bytes)
 * ObjMgr_FindByUniqueID
 */
CGObj* ObjMgr_FindByUniqueID(uint32_t dwUniqueID) {
	if (!g_pGame) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	(void)dwUniqueID;
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00485E20] (34 bytes)
 * ObjMgr_FindByName
 */
CGObj* ObjMgr_FindByName(const char* pszName) {
	if (!g_pGame) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	(void)pszName;
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00485E50] (28 bytes)
 * ObjMgr_DestroyObject
 */
bool ObjMgr_DestroyObject(CGObj* pObj) {
	if (!g_pGame || !pObj) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	return true;
}

// ============================================================================
// CGSkillObject Implementation
// ============================================================================

CGSkillObject::CGSkillObject() {
	m_listNode.pOwner = this;
	m_listNode.bInList = 0;
	m_listNode.pPrev = nullptr;
	m_listNode.pNext = nullptr;
}

CGSkillObject::~CGSkillObject() = default;

uint8_t CGSkillObject::GetLifeState() const {
	return 1;
}

void CGSkillObject::OnTick(float fDeltaSec) {
	(void)fDeltaSec;
}

// ============================================================================
// CGEventDaemon Implementation
// ============================================================================

CGEventDaemon::CGEventDaemon() {
	m_listNode.pOwner = this;
	m_listNode.bInList = 0;
	m_listNode.pPrev = nullptr;
	m_listNode.pNext = nullptr;
}

CGEventDaemon::~CGEventDaemon() = default;

uint8_t CGEventDaemon::GetLifeState() const {
	return 1;
}

void CGEventDaemon::OnTick(float fDeltaSec) {
	(void)fDeltaSec;
}

// ============================================================================
// CGObjStruct Implementation
// ============================================================================

CGObjStruct::CGObjStruct() {
	m_listNode.pOwner = this;
	m_listNode.bInList = 0;
	m_listNode.pPrev = nullptr;
	m_listNode.pNext = nullptr;
}

CGObjStruct::~CGObjStruct() = default;

uint8_t CGObjStruct::GetLifeState() const {
	return 1;
}

void CGObjStruct::OnTick(float fDeltaSec) {
	(void)fDeltaSec;
}
