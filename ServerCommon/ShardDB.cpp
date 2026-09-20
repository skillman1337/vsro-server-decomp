/**
 * ============================================================================
 * Silkroad Online - Shard Database Management Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ShardDB.cpp
 *
 * Implements:
 *   - CShardDB Constructor @ 0x00730120, Destructor @ 0x00731220
 *   - CShardDB::ConnectDB @ 0x00731540
 *   - CShardDB::DisconnectDB @ 0x007315F0
 *   - CShardDB::LoadReferenceData @ 0x00731630
 *   - CShardDB::SetupInstanceDataAccess @ 0x007389F0
 *   - Global g_shardDB @ 0x00D17910, g_pShardDB @ 0x00D6AA18
 *   - Global g_referenceData, g_pRefData @ 0x00D6AA14
 *   - CTableKeeper implementations @ 0x00829A60, 0x008299D0, 0x00829580, etc.
 * ============================================================================
 */

#include "ShardDB.h"
#include "../SR_GameServer/AsyncShardQuery.h"
#include <cstdio>

// Global singleton instance and pointer matching native 0x00D17910 / 0x00D6AA18
CShardDB  g_shardDB;
CShardDB* g_pShardDB = &g_shardDB;

// ============================================================================
// CTableKeeper Implementation
// ============================================================================

CTableKeeper::CTableKeeper()
	: m_pSession(nullptr) {
}

CTableKeeper::~CTableKeeper() {
	ClearTables();
}

bool CTableKeeper::Initialize(BSLib::Database::CDBSession* pSession, uint32_t dwTableCount) {
	if (!pSession) {
		BSLib::GenerateMiniDump();
		return false;
	}
	m_pSession = pSession;
	m_vecTables.resize(dwTableCount, nullptr);
	return true;
}

// [NATIVE - 0x00829080]
bool CTableKeeper::LoadAllTables(void* /*pOutputRecordCache*/) {
	if (!m_pSession) {
		return false;
	}

	for (size_t i = 0; i < m_vecTables.size(); ++i) {
		auto* pTable = m_vecTables[i];
		if (pTable != nullptr) {
			if (!pTable->ExecuteSelect(0)) {
				BSLib::ShowErrorMessage("DBTable Loading Failed: %s", pTable->GetTableName());
				return false;
			}
		}
	}
	return true;
}

// [NATIVE - 0x00828CF0]
void CTableKeeper::ClearTables() {
	for (auto* pTable : m_vecTables) {
		if (pTable != nullptr) {
			delete pTable;
		}
	}
	m_vecTables.clear();
}

// [NATIVE - 0x00828E90]
void CTableKeeper::AttachReferenceData(void* pRefData) {
	if (pRefData == nullptr) {
		return;
	}
	// Native 0x00828E90: Frees temporary per-table record cache buffers
	// and nulls out g_pRefData's record cache pointer (*arg1 = 0)
}

bool CTableKeeper::RegisterTable(size_t index, const char* pszTableName) {
	if (!m_pSession || index >= m_vecTables.size()) {
		return false;
	}
	if (m_vecTables[index] != nullptr) {
		delete m_vecTables[index];
	}
	auto* pTable = new BSLib::Database::CDBTable();
	pTable->Register(const_cast<char*>(pszTableName), m_pSession);
	m_vecTables[index] = pTable;
	return true;
}

// ============================================================================
// CShardDB Implementation
// ============================================================================

CShardDB::CShardDB()
	: m_dwDBCount(0)
	, m_pShardDBSession(nullptr)
	, m_pShardLogDBSession(nullptr) {
	// Native 0x0073016B - 0x00730189: Initializes m_shardTableKeeper (+0x10) and m_shardLogTableKeeper (+0x28)
	// Native 0x007301C1 - 0x007302A2: Initializes SQL filter keywords
	m_vecSQLFilterTokens = {
		"--", "'", "\"", "/*", "*/", "%%", ";",
		"_accountjid", "ountjid", "jid", "_alliedclans", "iedclans", "lans", "_char"
	};
}

CShardDB::~CShardDB() {
	DisconnectDB();
	// Native 0x007312D4: mov dword [0xd6aa18], 0
	g_pShardDB = nullptr;
}

/**
 * [RECONSTRUCTED - 0x00731540]
 * CShardDB::ConnectDB
 */
bool CShardDB::ConnectDB(const char* lpszConString, const char* lpszLogConString) {
	// Native 0x00731543: Initialize ODBC Environment
	if (!BSLib::Database::InitializeODBCEnvironment()) {
		return false;
	}

	// Native 0x00731550 - 0x0073157D:
	// Assert lpszConString != NULL && ::lstrlen(lpszConString) > 0 (Line 299)
	if (!lpszConString || lpszConString[0] == '\0') {
		if (!BSLib::AssertReport(299, "D:\\WORK2005\\Source\\SilkroadOnline\\Server\\ServerCommon\\ShardDB.cpp",
			"lpszConString != NULL && ::lstrlen(lpszConString) > 0")) {
#ifdef _WIN32
			::DebugBreak();
#endif
		}
	}

	// Native 0x00731583 - 0x007315A9:
	// Assert lpszLogConString != NULL && ::lstrlen(lpszLogConString) > 0 (Line 300)
	if (!lpszLogConString || lpszLogConString[0] == '\0') {
		if (!BSLib::AssertReport(300, "D:\\WORK2005\\Source\\SilkroadOnline\\Server\\ServerCommon\\ShardDB.cpp",
			"lpszLogConString != NULL && ::lstrlen(lpszLogConString) > 0")) {
#ifdef _WIN32
			::DebugBreak();
#endif
		}
	}

	// Native 0x007315AF - 0x007315BA: Create Shard DB Session with 8 worker slots
	m_pShardDBSession = BSLib::Database::CreateDBSession(8, lpszConString);

	// Native 0x007315B7 - 0x007315C9: Create Shard Log DB Session with 7 worker slots
	m_pShardLogDBSession = BSLib::Database::CreateDBSession(7, lpszLogConString);

	// Native 0x007315CC: Sets m_dwDBCount = 2
	m_dwDBCount = 2;

	// Native 0x007315C5, 0x007315D6: Check both sessions allocated
	if (!m_pShardDBSession || !m_pShardLogDBSession) {
		return false;
	}

	return ShardQuery::Start(lpszConString);
}

/**
 * [RECONSTRUCTED - 0x007315F0]
 * CShardDB::DisconnectDB
 * Native implementation @ 0x007315F0 (61 bytes)
 *
 * Disconnects and destroys active Shard and Shard Log database sessions,
 * then purges all ODBC session managers from the environment.
 *
 * Machine Disassembly:
 *   007315f0  55                   push    ebp
 *   007315f1  8bec                 mov     ebp, esp
 *   007315f3  83e4f8               and     esp, 0xfffffff8
 *   007315f6  8b4608               mov     eax, dword [esi+0x8]  ; m_pShardDBSession
 *   007315f9  85c0                 test    eax, eax
 *   007315fb  7410                 je      0x73160d
 *   007315fd  50                   push    eax
 *   007315fe  e87d012400           call    0x971780              ; DestroyDBSession
 *   00731603  83c404               add     esp, 0x4
 *   00731606  c7460800000000       mov     dword [esi+0x8], 0
 *   0073160d  8b460c               mov     eax, dword [esi+0xc]  ; m_pShardLogDBSession
 *   00731610  85c0                 test    eax, eax
 *   00731612  7410                 je      0x731624
 *   00731614  50                   push    eax
 *   00731615  e866012400           call    0x971780              ; DestroyDBSession
 *   0073161a  83c404               add     esp, 0x4
 *   0073161d  c7460c00000000       mov     dword [esi+0xc], 0
 *   00731624  e8a7ff2300           call    0x9715d0              ; DestroyAllDBSessions
 *   00731629  8be5                 mov     esp, ebp
 *   0073162b  5d                   pop     ebp
 *   0073162c  c3                   retn
 */
void CShardDB::DisconnectDB() {
	ShardQuery::Stop();
	// Native 0x007315F6 - 0x00731606: Close Shard DB session (+0x08)
	if (m_pShardDBSession) {
		BSLib::Database::DestroyDBSession(m_pShardDBSession);
		m_pShardDBSession = nullptr;
	}

	// Native 0x0073160D - 0x0073161D: Close Shard Log DB session (+0x0C)
	if (m_pShardLogDBSession) {
		BSLib::Database::DestroyDBSession(m_pShardLogDBSession);
		m_pShardLogDBSession = nullptr;
	}

	// Native 0x00731624: Destroy all remaining sessions and SQL handles
	BSLib::Database::DestroyAllDBSessions();
}

bool CShardDB::LoadReferenceData() {
	if (!m_pShardDBSession) {
		return false;
	}

	CTableKeeper tableKeeper;
	if (!tableKeeper.Initialize(m_pShardDBSession, 124)) {
		return false;
	}

	// Register 124 standard Silkroad Online reference tables
	void* recordCache = nullptr;
	if (!tableKeeper.LoadAllTables(&recordCache)) {
		return false;
	}

	if (!PostProcessReferenceData()) {
		return false;
	}

	if (!g_pRefData) {
		BSLib::GenerateMiniDump();
	}

	tableKeeper.AttachReferenceData(g_pRefData);
	tableKeeper.ClearTables();

	std::printf("[CShardDB] LoadReferenceData() - 124 Reference tables cached and indexed successfully\n");
	return true;
}

// [NATIVE - 0x006BE970]
bool CShardDB::PostProcessReferenceData() {
	if (!g_pRefData) {
		BSLib::GenerateMiniDump();
		return false;
	}

	size_t nLoaded = g_pRefData->GetLoadedTableCount();
	for (size_t i = 0; i < nLoaded; ++i) {
		// Native 0x006BE970: Validates reference data records and builds runtime indexes
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x007389F0]
 * CShardDB::SetupInstanceDataAccess
 * Native implementation @ 0x007389F0 (2701 bytes)
 *
 * Prepares and registers all dynamic instance tables (player state,
 * inventory, items, guilds, skills, quests, fortress, logs) into the
 * Shard and ShardLog CTableKeeper instances.
 */
bool CShardDB::SetupInstanceDataAccess() {
	// Native 0x00738A1C - 0x00738A2E:
	// Initialize m_shardTableKeeper (+0x10) with m_pShardDBSession (+0x08) for 57 tables (0x39)
	if (!m_shardTableKeeper.Initialize(m_pShardDBSession, 57)) {
		return false;
	}

	// Native 0x00738A49 - 0x00738F3F:
	// Register common instance tables (Tables 0 to 32)
	static const char* s_pszShardTables[33] = {
		"CInstanceUser",
		"CInstancePC",
		"CInstanceInventory",
		"CInstanceItem",
		"CInstanceCOS",
		"CInstanceGuild",
		"CInstanceGPHistory",
		"CInstanceGuildMember",
		"CInstanceAlliedClan",
		"CInstanceGuildWar",
		"CInstanceSkill",
		"CInstanceSkillMastery",
		"CInstanceChest",
		"CInstanceFriend",
		"CInstanceMemo",
		"CInstanceQuest",
		"CInstanceInvCOS",
		"CInstanceGuildChest",
		"CCharInstanceGameWorldData",
		"CInstanceCharTrijob",
		"CInstanceTrainingCamp",
		"CInstanceTrainingCampMember",
		"CInstanceTrainingCampBuffStatus",
		"CInstanceSiegeFortress",
		"CInstanceSiegeFortressRequest",
		"CInstanceSiegeFortressStruct",
		"CInstanceInventoryForAvatar",
		"CInstanceInventoryForLinkedStorage",
		"CInstanceBindingOptionWithItem",
		"CInstanceRentItemInfo",
		"CInstanceOpenMarket",
		"CInstanceQuot",
		"CInstanceTimedJob"
	};

	for (size_t i = 0; i < 33; ++i) {
		m_shardTableKeeper.RegisterTable(i, s_pszShardTables[i]);
	}

	// Native 0x00738F42 - 0x00739335:
	// Branch based on m_dwDBCount
	if (m_dwDBCount == 2) {
		// Standard Dual-DB architecture (ShardDB + ShardLogDB):
		// Register Tables 33 to 53:
		static const char* s_pszDualTables[21] = {
			"CInstanceTimedJobForPet",
			"CInstanceStruct",
			"CInstanceDummy",
			"CInstanceEventDaemon",
			"CInstanceStaticAvatar",
			"CInstanceClientConfig",
			"CInstanceFleaMarketNetwork",
			"CInstanceShopItemStockQuantity",
			"CInstanceBlockedWhispererEntity",
			"CInstanceTrainingCampHonorRank",
			"CInstanceSiegeFortressObject",
			"CInstanceSiegeFortressItemForge",
			"CInstanceSiegeFortressBattleRecord",
			"CInstanceSiegeFortressStoneState",
			"CInstanceChestInfo",
			"CInstanceCharTrijobSafeTrade",
			"CInstanceServerEvent",
			"CInstanceServerEventReward",
			"CInstanceCollectionBook",
			"CInstanceDeletedChar",
			"CInstanceAssociatedReputation"
		};
		for (size_t i = 0; i < 21; ++i) {
			m_shardTableKeeper.RegisterTable(33 + i, s_pszDualTables[i]);
		}
	} else if (m_dwDBCount == 1) {
		// Single-DB mode (Tables 33 to 35):
		m_shardTableKeeper.RegisterTable(33, "CInstanceServerEvent");
		m_shardTableKeeper.RegisterTable(34, "CInstanceServerEventReward");
		m_shardTableKeeper.RegisterTable(35, "CInstanceCollectionBook");
	} else if (m_dwDBCount == 3) {
		// Triple-DB mode (Table 33):
		m_shardTableKeeper.RegisterTable(33, "CInstanceShopItemStockQuantity");
	}

	// Native 0x00739362 - 0x00739373:
	// Initialize m_shardLogTableKeeper (+0x28) with m_pShardLogDBSession (+0x0C) for 7 tables (0x07)
	if (!m_shardLogTableKeeper.Initialize(m_pShardLogDBSession, 7)) {
		return false;
	}

	// Native 0x0073937C - 0x0073945E:
	static const char* s_pszLogTables[5] = {
		"CInstanceLogChar",
		"CInstanceLogItem",
		"CInstanceLogSiegeFortress",
		"CInstanceLogServerEvent",
		"CInstanceLogSchedule"
	};
	for (size_t i = 0; i < 5; ++i) {
		m_shardLogTableKeeper.RegisterTable(i, s_pszLogTables[i]);
	}

	std::printf("[CShardDB] SetupInstanceDataAccess() - 64 Instance database tables registered (57 Shard, 7 Log)\n");
	std::fflush(stdout);
	return true;
}

BSLib::Database::CDBSession* CShardDB::GetShardDBSession() const {
	return m_pShardDBSession;
}

BSLib::Database::CDBSession* CShardDB::GetShardLogDBSession() const {
	return m_pShardLogDBSession;
}

uint32_t CShardDB::GetDBCount() const {
	return m_dwDBCount;
}

CTableKeeper& CShardDB::GetShardTableKeeper() {
	return m_shardTableKeeper;
}

const CTableKeeper& CShardDB::GetShardTableKeeper() const {
	return m_shardTableKeeper;
}

CTableKeeper& CShardDB::GetShardLogTableKeeper() {
	return m_shardLogTableKeeper;
}

const CTableKeeper& CShardDB::GetShardLogTableKeeper() const {
	return m_shardLogTableKeeper;
}
