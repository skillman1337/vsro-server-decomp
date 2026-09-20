/**
 * ============================================================================
 * Silkroad Online - Shard Database Management
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ShardDB.cpp
 *
 * Implements:
 *   - CShardDB: Shard database manager
 *       - VTable @ 0x00B0DBB4
 *       - RTTI: .?AVCShardDB@@ (inherits .?AV?$CSingletonT@VCShardDB@@@@ @ offset +0x04)
 *       - Native global instance @ 0x00D17910 (g_shardDB)
 *       - Native global pointer @ 0x00D6AA18 (g_pShardDB)
 *
 * Struct Layout proven against machine bytes:
 *   - +0x00: void**                            __vftable (0x00B0DBB4)
 *   - +0x04: uint32_t                          m_dwDBCount (set to 2 in ConnectDB @ 0x007315CC)
 *   - +0x08: BSLib::Database::CDBSession*     m_pShardDBSession (8 sessions @ 0x007315BA)
 *   - +0x0C: BSLib::Database::CDBSession*     m_pShardLogDBSession (7 sessions @ 0x007315C9)
 *
 * Native Methods:
 *   - CShardDB::ConnectDB @ 0x00731540 (165 bytes)
 *   - CShardDB::DisconnectDB @ 0x007315F0 (61 bytes)
 *   - CShardDB::LoadReferenceData @ 0x00731630 (6345 bytes)
 *   - CShardDB::SetupInstanceDataAccess @ 0x007389F0 (2701 bytes)
 *   - CShardDB::~CShardDB @ 0x00731220 (205 bytes)
 *   - CShardDB::scalar_deleting_destructor @ 0x00731200 (30 bytes, slot 0 @ +0x00)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_SHARDDB_H_
#define _SERVERCOMMON_SHARDDB_H_

#include "../JMX_Library/BSLib/DB/Database.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "ReferenceData.h"

/**
 * CTableKeeper
 * Reference table loader and SQL binder
 * Native VTable @ 0x00B0FE64 (RTTI: .?AVCTableKeeper@@)
 * Size: 24 bytes (0x18)
 *
 * Struct layout proven against machine bytes:
 *   - +0x00: void**                                   __vftable (0x00B0FE64)
 *   - +0x04: BSLib::Database::CDBSession*            m_pSession (bound via sub_829580 @ +0x04)
 *   - +0x08: std::vector<BSLib::Database::CDBTable*> m_vecTables (MSVC 7.1 vector: 16 bytes)
 */
class CTableKeeper {
public:
	// Native constructor @ 0x00829A60
	CTableKeeper();

	// Native destructor @ 0x008299D0 / slot 0 @ 0x00829A40
	virtual ~CTableKeeper();

	// [NATIVE - 0x00829580]
	// CTableKeeper::Initialize(pSession, dwTableCount = 124)
	bool Initialize(BSLib::Database::CDBSession* pSession, uint32_t dwTableCount);

	// [NATIVE - 0x00829080]
	// CTableKeeper::LoadAllTables(pOutputRecordCache)
	bool LoadAllTables(void* pOutputRecordCache);

	// [NATIVE - 0x00828CF0]
	// CTableKeeper::ClearTables
	void ClearTables();

	// [NATIVE - 0x00828E90]
	// CTableKeeper::AttachReferenceData(pRefData)
	void AttachReferenceData(void* pRefData);

	// Table accessors
	BSLib::Database::CDBSession* GetSession() const { return m_pSession; }
	size_t GetTableCount() const { return m_vecTables.size(); }
	BSLib::Database::CDBTable* GetTable(size_t index) const {
		return (index < m_vecTables.size()) ? m_vecTables[index] : nullptr;
	}
	void SetTable(size_t index, BSLib::Database::CDBTable* pTable) {
		if (index < m_vecTables.size()) {
			m_vecTables[index] = pTable;
		}
	}
	bool RegisterTable(size_t index, const char* pszTableName);

private:
	BSLib::Database::CDBSession*            m_pSession;  // +0x04
	std::vector<BSLib::Database::CDBTable*> m_vecTables; // +0x08: std::vector<CDBTable*>
};

class CShardDB {
public:
	// Native constructor @ 0x00730120
	CShardDB();

	// [RECONSTRUCTED - 0x00731220 / 0x00731200]
	// Native virtual destructor (slot 0 of VTable 0x00B0DBB4)
	virtual ~CShardDB();

	/**
	 * [RECONSTRUCTED - 0x00731540]
	 * CShardDB::ConnectDB
	 * Native implementation @ 0x00731540 (165 bytes)
	 */
	bool ConnectDB(const char* lpszConString, const char* lpszLogConString);

	/**
	 * [RECONSTRUCTED - 0x007315F0]
	 * CShardDB::DisconnectDB
	 * Native implementation @ 0x007315F0 (61 bytes)
	 */
	void DisconnectDB();

	/**
	 * [RECONSTRUCTED - 0x00731630]
	 * CShardDB::LoadReferenceData
	 * Native implementation @ 0x00731630 (6345 bytes)
	 */
	bool LoadReferenceData();

	// [RECONSTRUCTED - 0x006BE970]
	// CShardDB::PostProcessReferenceData
	bool PostProcessReferenceData();

	/**
	 * [RECONSTRUCTED - 0x007389F0]
	 * CShardDB::SetupInstanceDataAccess
	 * Native implementation @ 0x007389F0 (2701 bytes)
	 *
	 * Initializes:
	 *   - Main Shard CTableKeeper (+0x10) with m_pShardDBSession for 57 tables (0x39)
	 *   - Shard Log CTableKeeper (+0x28) with m_pShardLogDBSession for 7 tables (0x07)
	 *   - Binds prepared instance tables: User, PC, Inventory, Items, COS, Guild,
	 *     GuildMember, Skills, Quests, SiegeFortress, Avatars, TimedJobs, Logs, etc.
	 */
	bool SetupInstanceDataAccess();

	BSLib::Database::CDBSession* GetShardDBSession() const;
	BSLib::Database::CDBSession* GetShardLogDBSession() const;
	uint32_t GetDBCount() const;

	CTableKeeper& GetShardTableKeeper();
	const CTableKeeper& GetShardTableKeeper() const;
	CTableKeeper& GetShardLogTableKeeper();
	const CTableKeeper& GetShardLogTableKeeper() const;

private:
	// Exact struct layout matching native binary bytes:
	uint32_t                     m_dwDBCount;           // +0x04: 2 in standard dual-DB mode
	BSLib::Database::CDBSession* m_pShardDBSession;     // +0x08: Main shard DB session
	BSLib::Database::CDBSession* m_pShardLogDBSession;  // +0x0C: Log shard DB session
	CTableKeeper                 m_shardTableKeeper;    // +0x10: 57 instance tables
	CTableKeeper                 m_shardLogTableKeeper; // +0x28: 7 log instance tables
	std::vector<std::string>     m_vecSQLFilterTokens;  // +0x40: SQL injection sanitization keywords
};

// Global singleton instance and pointer matching native 0x00D17910 / 0x00D6AA18
extern CShardDB  g_shardDB;
extern CShardDB* g_pShardDB;

#endif // _SERVERCOMMON_SHARDDB_H_
