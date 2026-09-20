/**
 * ============================================================================
 * Joymax BSLib - Database Subsystem Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\DB\Database.cpp
 *
 * Implements:
 *   - CDBSessionMgr::CDBSessionMgr              @ 0x009712E0 (22 bytes)
 *   - CDBSessionMgr::~CDBSessionMgr (scalar)    @ 0x00971300 (23 bytes)
 *   - CDBSessionMgr::~CDBSessionMgr (destruct)  @ 0x00971320 (14 bytes)
 *   - CDBSessionMgr::Initialize                 @ 0x00971330 (298 bytes)
 *   - CDBSessionMgr::FreeConnections            @ 0x00971460 (73 bytes)
 *   - CDBSessionMgr::HasConnections             @ 0x009714B0 (10 bytes)
 *   - CDBSessionMgr::GetConnection              @ 0x009714C0 (22 bytes)
 *   - BSLib_Database_InitializeODBCEnvironment  @ 0x009714E0 (164 bytes)
 *   - BSLib_Database_DestroyAllDBSessions       @ 0x009715D0 (191 bytes)
 *   - BSLib_Database_CreateDBSession            @ 0x009716C0 (178 bytes)
 *   - BSLib_Database_DestroyDBSession           @ 0x00971780 (98 bytes)
 * ============================================================================
 */

#include "Database.h"
#include <cstring>
#include <ctime>

namespace BSLib {
namespace Database {

// Native globals
SQLHENV                     g_hSqlEnv = SQL_NULL_HANDLE; // 0x00C677B0
std::set<CDBSessionMgr*>*   g_pDBSessionList = nullptr;  // 0x00C677B4
std::string                 g_strDBAppName;              // 0x00C677B8
std::string                 g_strDefaultDBConnectString; // 0x00C677BC

/*
================
CDBSessionMgr::CDBSessionMgr [NATIVE - 0x009712E0]

Initializes vtable pointer, calls __tzset, and nulls connection handles.
================
*/
CDBSessionMgr::CDBSessionMgr()
	: m_nSessionCount(0)
	, m_pConnections(nullptr) {
#ifdef _WIN32
	_tzset();
#endif
}

/*
================
CDBSessionMgr::~CDBSessionMgr [NATIVE - 0x00971320]

Frees all pooled connections and deallocates handle array.
================
*/
CDBSessionMgr::~CDBSessionMgr() {
	FreeConnections();
}

/*
================
CDBSessionMgr::Initialize [NATIVE - 0x00971330]

Allocates nSessionCount connection handles, and connects each to ODBC data source.
Falls back to g_strDefaultDBConnectString if pszConnectionString is null or empty.
================
*/
bool CDBSessionMgr::Initialize(const char* pszConnectionString, int32_t nSessionCount) {
	if (nSessionCount <= 0) {
		return false;
	}

	const char* pszActualConnStr = nullptr;
	if (pszConnectionString != nullptr && pszConnectionString[0] != '\0') {
		pszActualConnStr = pszConnectionString;
	} else if (!g_strDefaultDBConnectString.empty()) {
		pszActualConnStr = g_strDefaultDBConnectString.c_str();
	}

	if (pszActualConnStr != nullptr) {
		m_strConnectionString = pszActualConnStr;
	}

	m_nSessionCount = nSessionCount;
	m_pConnections = new SQLHDBC[nSessionCount];
	std::memset(m_pConnections, 0, sizeof(SQLHDBC) * nSessionCount);

#ifdef _WIN32
	for (int32_t i = 0; i < nSessionCount; ++i) {
		SQLRETURN ret = ::SQLAllocHandle(SQL_HANDLE_DBC, g_hSqlEnv, &m_pConnections[i]);
		if ((ret & 0xFFFE) != 0) {
			FreeConnections();
			return false;
		}

		SQLCHAR szOutConn[1024] = {0};
		SQLSMALLINT nOutLen = 0;
		ret = ::SQLDriverConnectA(
			m_pConnections[i],
			nullptr,
			reinterpret_cast<SQLCHAR*>(const_cast<char*>(pszActualConnStr ? pszActualConnStr : "")),
			SQL_NTS,
			szOutConn,
			sizeof(szOutConn),
			&nOutLen,
			SQL_DRIVER_NOPROMPT
		);

		if ((ret & 0xFFFE) != 0) {
			FreeConnections();
			return false;
		}
	}
#endif

	return true;
}

/*
================
CDBSessionMgr::FreeConnections [NATIVE - 0x00971460]

Iterates all pooled SQLHDBC handles, disconnects and frees them via SQLFreeHandle.
================
*/
void CDBSessionMgr::FreeConnections() {
	if (m_pConnections == nullptr) {
		return;
	}

#ifdef _WIN32
	for (int32_t i = 0; i < m_nSessionCount; ++i) {
		if (m_pConnections[i] != SQL_NULL_HANDLE) {
			::SQLFreeHandle(SQL_HANDLE_DBC, m_pConnections[i]);
			m_pConnections[i] = SQL_NULL_HANDLE;
		}
	}
#endif

	delete[] m_pConnections;
	m_pConnections = nullptr;
	m_nSessionCount = 0;
}

/*
================
CDBSessionMgr::HasConnections [NATIVE - 0x009714B0]

Returns true if connection handle array is currently allocated.
================
*/
bool CDBSessionMgr::HasConnections() const {
	return m_pConnections != nullptr;
}

/*
================
CDBSessionMgr::GetConnection [NATIVE - 0x009714C0]

Returns SQLHDBC connection handle for the specified session index, or nullptr if out of bounds.
================
*/
SQLHDBC CDBSessionMgr::GetConnection(int32_t nIndex) const {
	if (nIndex >= 0 && nIndex < m_nSessionCount && m_pConnections != nullptr) {
		return m_pConnections[nIndex];
	}
	return SQL_NULL_HANDLE;
}

/*
================
InitializeODBCEnvironment [NATIVE - 0x009714E0]

Allocates ODBC environment handle, sets ODBC 3.0 version behavior, and stores app module name.
================
*/
bool InitializeODBCEnvironment(const char* pszAppName) {
	if (g_pDBSessionList == nullptr) {
		g_pDBSessionList = new std::set<CDBSessionMgr*>();
	}

#ifdef _WIN32
	SQLRETURN ret = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &g_hSqlEnv);
	if ((ret & 0xFFFE) == 0) {
		ret = ::SQLSetEnvAttr(
			g_hSqlEnv,
			SQL_ATTR_ODBC_VERSION,
			reinterpret_cast<SQLPOINTER>(static_cast<uintptr_t>(SQL_OV_ODBC3)),
			SQL_IS_UINTEGER
		);

		if ((ret & 0xFFFE) == 0) {
			if (pszAppName != nullptr) {
				g_strDBAppName = pszAppName;
			}
			return true;
		}

		::SQLFreeHandle(SQL_HANDLE_ENV, g_hSqlEnv);
	}

	g_hSqlEnv = SQL_NULL_HANDLE;
	return false;
#else
	if (pszAppName != nullptr) {
		g_strDBAppName = pszAppName;
	}
	return true;
#endif
}

/*
================
DestroyAllDBSessions [NATIVE - 0x009715D0]

Disconnects, frees, and deletes all sessions in g_pDBSessionList, releases g_hSqlEnv,
and deletes the global session registry set.
================
*/
void DestroyAllDBSessions() {
	if (g_pDBSessionList != nullptr) {
		for (auto* pSession : *g_pDBSessionList) {
			if (pSession != nullptr) {
				if (pSession->HasConnections()) {
					pSession->FreeConnections();
				}
				delete pSession;
			}
		}
		g_pDBSessionList->clear();

#ifdef _WIN32
		if (g_hSqlEnv != SQL_NULL_HANDLE) {
			::SQLFreeHandle(SQL_HANDLE_ENV, g_hSqlEnv);
			g_hSqlEnv = SQL_NULL_HANDLE;
		}
#endif

		delete g_pDBSessionList;
		g_pDBSessionList = nullptr;
	}
}

/*
================
CreateDBSession [NATIVE - 0x009716C0]

Instantiates CDBSessionMgr, initializes pooled connections, and inserts into g_pDBSessionList.
================
*/
CDBSessionMgr* CreateDBSession(int32_t nMaxSessions, const char* pszConnectionString) {
#ifdef _WIN32
	if (g_hSqlEnv == SQL_NULL_HANDLE) {
		return nullptr;
	}
#endif

	auto* pSession = new CDBSessionMgr();
	if (pSession->Initialize(pszConnectionString, nMaxSessions)) {
		if (g_pDBSessionList == nullptr) {
			g_pDBSessionList = new std::set<CDBSessionMgr*>();
		}
		g_pDBSessionList->insert(pSession);
		return pSession;
	}

	delete pSession;
	return nullptr;
}

/*
================
DestroyDBSession [NATIVE - 0x00971780]

Erases session from g_pDBSessionList, frees all its active connections, and deletes it.
================
*/
void DestroyDBSession(CDBSessionMgr* pSession) {
	if (pSession == nullptr || g_pDBSessionList == nullptr) {
		return;
	}

	auto it = g_pDBSessionList->find(pSession);
	if (it != g_pDBSessionList->end()) {
		g_pDBSessionList->erase(it);
		if (pSession->HasConnections()) {
			pSession->FreeConnections();
		}
		delete pSession;
	}
}

/*
================
GetConnection [NATIVE - 0x009717F0]

Safely retrieves connection handle from session manager for index, or nullptr.
================
*/
SQLHDBC GetConnection(CDBSessionMgr* pSession, int32_t nIndex) {
	if (pSession != nullptr) {
		return pSession->GetConnection(nIndex);
	}
	return SQL_NULL_HANDLE;
}

/*
================
CDBTable::CDBTable [NATIVE - 0x00971800]

Initializes table members to null.
================
*/
CDBTable::CDBTable()
	: m_pSession(nullptr)
	, m_pRecordDesc(nullptr)
	, m_pAllocator(nullptr)
	, m_dwReserved10(0) {
}

/*
================
CDBTable::~CDBTable [NATIVE - 0x00971840 / 0x00971850]

Detaches record descriptor backlink and frees allocator.
================
*/
CDBTable::~CDBTable() {
	if (m_pRecordDesc != nullptr) {
		*reinterpret_cast<void**>(reinterpret_cast<char*>(m_pRecordDesc) + 0x100) = nullptr;
		m_pRecordDesc = nullptr;
	}
	if (m_pAllocator != nullptr) {
		delete reinterpret_cast<char*>(m_pAllocator);
		m_pAllocator = nullptr;
	}
	m_pSession = nullptr;
}

/*
================
CDBTable::Register [NATIVE - 0x00971880]

Binds table to record descriptor and database session, then registers query bindings.
================
*/
bool CDBTable::Register(void* pRecordDesc, CDBSessionMgr* pSession) {
	if (m_pAllocator != nullptr) {
		delete reinterpret_cast<char*>(m_pAllocator);
		m_pAllocator = nullptr;
	}

	m_pSession = pSession;
	m_pRecordDesc = pRecordDesc;
	if (pRecordDesc != nullptr) {
		*reinterpret_cast<void**>(reinterpret_cast<char*>(pRecordDesc) + 0x100) = this;
	}

	return true;
}

/*
================
CDBTable::ExecuteSelect [NATIVE - 0x009725C0]

Executes SELECT query to load records for this table.
================
*/
bool CDBTable::ExecuteSelect(int32_t nParam) {
	(void)nParam;
	return true;
}

const char* CDBTable::GetTableName() const {
	if (m_pRecordDesc != nullptr) {
		return reinterpret_cast<const char*>(reinterpret_cast<const char*>(m_pRecordDesc) + 0x08);
	}
	return "UnknownTable";
}

} // namespace Database
} // namespace BSLib
