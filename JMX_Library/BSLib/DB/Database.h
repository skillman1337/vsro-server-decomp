/**
 * ============================================================================
 * Joymax BSLib - Database Subsystem
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\DB\Database.h
 *
 * Implements:
 *   - CDBSessionMgr @ 0x00B4278C (Size 0x0C / 12 bytes)
 *   - BSLib_Database_InitializeODBCEnvironment @ 0x009714E0
 *   - BSLib_Database_CreateDBSession @ 0x009716C0
 *   - BSLib_Database_DestroyDBSession @ 0x00971780
 *   - BSLib_Database_DestroyAllDBSessions @ 0x009715D0
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_DB_DATABASE_H_
#define _JMX_LIBRARY_BSLIB_DB_DATABASE_H_

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <set>

#ifdef _WIN32
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#else
typedef void* SQLHENV;
typedef void* SQLHDBC;
typedef void* SQLHSTMT;
typedef void* SQLHANDLE;
#define SQL_NULL_HANDLE nullptr
#define SQL_HANDLE_ENV 1
#define SQL_HANDLE_DBC 2
#define SQL_ATTR_ODBC_VERSION 200
#define SQL_OV_ODBC3 3
#define SQL_IS_UINTEGER (-6)
#define SQL_NTS (-3)
#define SQL_DRIVER_NOPROMPT 0
#endif

namespace BSLib {
namespace Database {

/**
 * [RECONSTRUCTED - 0x00B4278C]
 * CDBSessionMgr
 * Native VTable @ 0x00B4278C (1 virtual method: scalar deleting destructor)
 * Size: 0x0C (12 bytes)
 */
class CDBSessionMgr {
public:
	// [NATIVE - 0x009712E0] Constructor
	CDBSessionMgr();

	// [NATIVE - 0x00971300 / 0x00971320] Virtual destructor
	virtual ~CDBSessionMgr();

	// [NATIVE - 0x00971330] Initializes connection pool and connects each session
	bool Initialize(const char* pszConnectionString, int32_t nSessionCount);

	// [NATIVE - 0x00971460] Disconnects and frees all active connections
	void FreeConnections();

	// [NATIVE - 0x009714B0] Returns true if connection array is allocated
	bool HasConnections() const;

	// [NATIVE - 0x009714C0] Returns connection handle for the given session index
	SQLHDBC GetConnection(int32_t nIndex) const;

	// Compatibility accessors
	int32_t GetSessionCount() const { return m_nSessionCount; }
	int32_t GetMaxSessions() const { return m_nSessionCount; }
	const std::string& GetConnectionString() const { return m_strConnectionString; }

public:
	// +0x00: vfptr (0x00B4278C)
	int32_t     m_nSessionCount = 0;             // +0x04: Number of pooled ODBC connections
	SQLHDBC*    m_pConnections = nullptr;        // +0x08: Array of connection handles
	std::string m_strConnectionString;           // Connection string cache
};

// Typedef alias for backward compatibility across server components
typedef CDBSessionMgr CDBSession;

/**
 * [RECONSTRUCTED - 0x00B42784]
 * CDBTable
 * Native VTable @ 0x00B42784 (1 virtual method: scalar deleting destructor)
 * Size: 0x14 (20 bytes)
 */
class CDBTable {
public:
	// [NATIVE - 0x00971800] Constructor
	CDBTable();

	// [NATIVE - 0x00971820 / 0x00971840 / 0x00971850] Virtual destructor
	virtual ~CDBTable();

	// [NATIVE - 0x00971880] Register record descriptor with database session
	bool Register(void* pRecordDesc, CDBSessionMgr* pSession);

	// [NATIVE - 0x009725C0] Executes SELECT query to load records from database
	bool ExecuteSelect(int32_t nParam = 0);

	// Accessors
	CDBSessionMgr* GetSession() const { return m_pSession; }
	void* GetRecordDesc() const { return m_pRecordDesc; }
	const char* GetTableName() const;

public:
	// +0x00: vfptr (0x00B42784)
	CDBSessionMgr* m_pSession = nullptr;       // +0x04: Bound database session
	void*          m_pRecordDesc = nullptr;    // +0x08: Record descriptor metadata
	void*          m_pAllocator = nullptr;     // +0x0C: Dynamic record allocator
	uint32_t       m_dwReserved10 = 0;         // +0x10: Reserved / state flags
};

// Global environment handle and session set (Native @ 0x00C677B0, 0x00C677B4)
extern SQLHENV                     g_hSqlEnv;
extern std::set<CDBSessionMgr*>*   g_pDBSessionList;
extern std::string                 g_strDBAppName;
extern std::string                 g_strDefaultDBConnectString;

// [NATIVE - 0x009717F0]
// Helper function to safely retrieve ODBC connection handle from a session manager
SQLHDBC GetConnection(CDBSessionMgr* pSession, int32_t nIndex);

// [NATIVE - 0x009714E0]
// Initializes ODBC environment handle, sets ODBC 3.0 version, and registers app module
bool InitializeODBCEnvironment(const char* pszAppName = nullptr);

// [NATIVE - 0x009716C0]
// Allocates CDBSessionMgr instance, initializes pool, and registers with g_pDBSessionList
CDBSessionMgr* CreateDBSession(int32_t nMaxSessions, const char* pszConnectionString = nullptr);

// [NATIVE - 0x00971780]
// Erases session from g_pDBSessionList, frees all connections, and deletes instance
void DestroyDBSession(CDBSessionMgr* pSession);

// [NATIVE - 0x009715D0]
// Frees and deletes all registered database sessions and releases ODBC environment handle
void DestroyAllDBSessions();

} // namespace Database
} // namespace BSLib

#endif // _JMX_LIBRARY_BSLIB_DB_DATABASE_H_
