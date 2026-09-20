// Exercise the production queue and SQL result draining with an explicit ODBC
// boundary. No real database is substituted by an in-memory persistence claim.
#include "../JMX_Library/BSLib/DB/Database.h"
#include <cassert>
#include <thread>
#include <vector>
#include <iostream>
#include <stdexcept>

struct Statement {
    std::string sql;
    int32_t* output = nullptr;
    SQLLEN* indicator = nullptr;
    unsigned results = 0;
};
static unsigned allocations = 0, releases = 0, connections = 0;
static std::vector<std::string> executed;
static std::thread::id worldThread;
static SQLRETURN FakeAlloc(SQLSMALLINT, SQLHANDLE, SQLHANDLE* out) {
    *out = new Statement; ++allocations; return SQL_SUCCESS;
}
static SQLRETURN FakeFree(SQLSMALLINT, SQLHANDLE handle) {
    delete static_cast<Statement*>(handle); ++releases; return SQL_SUCCESS;
}
static SQLRETURN FakeBind(SQLHSTMT h, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT,
        SQLSMALLINT, SQLULEN, SQLSMALLINT, SQLPOINTER output, SQLLEN, SQLLEN* indicator) {
    auto* s = static_cast<Statement*>(h);
    s->output = static_cast<int32_t*>(output); s->indicator = indicator;
    return SQL_SUCCESS;
}
static SQLRETURN FakeExec(SQLHSTMT h, SQLCHAR* sql, SQLINTEGER) {
    assert(std::this_thread::get_id() != worldThread);
    auto* s = static_cast<Statement*>(h);
    s->sql = reinterpret_cast<char*>(sql);
    executed.push_back(s->sql);
    if (s->sql == "throw") throw std::runtime_error("driver exception");
    return s->sql == "fail" ? SQL_ERROR : SQL_SUCCESS;
}
static SQLRETURN FakeMore(SQLHSTMT h) {
    auto* s = static_cast<Statement*>(h);
    if (++s->results < 3) return SQL_SUCCESS;
    if (s->output) {
        *s->output = s->sql == "zero" ? 0 : 41;
        *s->indicator = s->sql == "null" ? SQL_NULL_DATA : sizeof(int32_t);
    }
    return SQL_NO_DATA;
}
static SQLRETURN FakeDiag(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLCHAR*, SQLINTEGER*,
        SQLCHAR*, SQLSMALLINT, SQLSMALLINT*) { return SQL_NO_DATA; }

#define SQLAllocHandle FakeAlloc
#define SQLFreeHandle FakeFree
#define SQLBindParameter FakeBind
#define SQLExecDirectA FakeExec
#define SQLMoreResults FakeMore
#define SQLGetDiagRecA FakeDiag
#include "../SR_GameServer/AsyncShardQuery.cpp"

namespace BSLib {
int Log_Printf(uint32_t, const char*, ...) { return 0; }
namespace Database {
CDBSessionMgr::CDBSessionMgr() = default;
CDBSessionMgr::~CDBSessionMgr() = default;
SQLHDBC CDBSessionMgr::GetConnection(int32_t) const { return reinterpret_cast<SQLHDBC>(1); }
CDBSession* CreateDBSession(int32_t, const char*) { ++connections; return new CDBSession; }
void DestroyDBSession(CDBSession* session) { --connections; delete session; }
}}

int main() {
    worldThread = std::this_thread::get_id();
    assert(!ShardQuery::Submit("before-start"));
    assert(ShardQuery::Start("test-boundary"));
    assert(!ShardQuery::Start("duplicate"));
    assert(!ShardQuery::Submit(""));
    std::vector<std::string> callbacks;
    for (std::string sql : {"insert", "fail", "zero", "null", "throw"}) {
        assert(ShardQuery::Submit(sql, true, [&, sql](const ShardQuery::Result& result) {
            assert(std::this_thread::get_id() == worldThread);
            callbacks.push_back(sql);
            assert(result.ok == (sql == "insert"));
            if (result.ok) {
                assert(result.value == 41); // output only becomes valid after result-set draining
                assert(ShardQuery::Submit("delete-41", false, [&](const ShardQuery::Result& r) {
                    assert(r.ok); callbacks.push_back("delete-41");
                }));
            } else assert(!result.error.empty());
        }));
    }
    ShardQuery::Stop(); // must drain the deletion enqueued by the insert completion
    assert(callbacks == executed);
    assert(callbacks.size() == 6 && callbacks.back() == "delete-41");
    assert(allocations == releases && connections == 0);
    assert(!ShardQuery::Submit("after-stop"));
    ShardQuery::Stop();
    assert(ShardQuery::Start("restart"));
    assert(ShardQuery::Submit("restart-query"));
    ShardQuery::Stop();
    assert(allocations == releases && connections == 0);
    std::cout << "async FIFO, world-thread completion, dependent shutdown writes, driver failures and restart passed\n";
}
