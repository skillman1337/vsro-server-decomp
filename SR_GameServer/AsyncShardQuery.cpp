#include "AsyncShardQuery.h"
#include "../JMX_Library/BSLib/DB/Database.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
#include <exception>

namespace ShardQuery {
namespace {
struct Work { std::string sql; bool returnInteger; Completion completion; };
struct Queue {
    std::mutex mutex;
    std::condition_variable ready;
    std::condition_variable idle;
    std::deque<Work> pending;
    std::deque<std::pair<Completion, Result>> completed;
    std::thread worker;
    BSLib::Database::CDBSession* session = nullptr;
    bool accepting = false;
    bool executing = false;
};
// Explicit Stop owns shutdown, before the ODBC environment is destroyed.
Queue& queue() { static auto* value = new Queue; return *value; }

std::string diagnostic(SQLHSTMT statement) {
    SQLCHAR state[6]{}, text[1024]{};
    SQLINTEGER native = 0; SQLSMALLINT length = 0;
    if (SQL_SUCCEEDED(SQLGetDiagRecA(SQL_HANDLE_STMT, statement, 1, state,
            &native, text, sizeof(text), &length)))
        return std::string(reinterpret_cast<char*>(state)) + ": " +
            reinterpret_cast<char*>(text);
    return "ODBC operation failed without a diagnostic record";
}

Result execute(const Work& work, SQLHDBC connection) {
    Result result;
    SQLHSTMT statement = SQL_NULL_HSTMT;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, connection, &statement))) {
        result.error = "Cannot allocate shard SQL statement"; return result;
    }
    struct StatementGuard {
        SQLHSTMT value;
        ~StatementGuard() { SQLFreeHandle(SQL_HANDLE_STMT, value); }
    } guard{statement};
    SQLLEN indicator = 0;
    SQLRETURN status = SQL_SUCCESS;
    if (work.returnInteger)
        status = SQLBindParameter(statement, 1, SQL_PARAM_OUTPUT, SQL_C_SLONG,
            SQL_INTEGER, 0, 0, &result.value, sizeof(result.value), &indicator);
    if (SQL_SUCCEEDED(status))
        status = SQLExecDirectA(statement,
            reinterpret_cast<SQLCHAR*>(const_cast<char*>(work.sql.c_str())), SQL_NTS);
    if (SQL_SUCCEEDED(status) || status == SQL_NO_DATA) {
        // Procedure output parameters are valid after all result sets drain.
        while (status != SQL_NO_DATA && SQL_SUCCEEDED(status)) status = SQLMoreResults(statement);
        result.ok = status == SQL_NO_DATA && (!work.returnInteger ||
            (indicator != SQL_NULL_DATA && result.value > 0));
        if (!result.ok) result.error = work.returnInteger && status == SQL_NO_DATA
            ? "Timed-job insert returned a non-positive record identity" : diagnostic(statement);
    } else result.error = diagnostic(statement);
    return result;
}
}

bool Start(const char* connectionString) {
    if (!connectionString || !*connectionString) return false;
    auto& q = queue();
    std::lock_guard lock(q.mutex);
    if (q.accepting || q.worker.joinable()) return false;
    q.session = BSLib::Database::CreateDBSession(1, connectionString);
    if (!q.session) return false;
    q.accepting = true;
    try { q.worker = std::thread([&q] {
        for (;;) {
            Work work;
            {
                std::unique_lock lock(q.mutex);
                q.ready.wait(lock, [&q] { return !q.accepting || !q.pending.empty(); });
                if (q.pending.empty()) break;
                work = std::move(q.pending.front()); q.pending.pop_front();
                q.executing = true;
            }
            Result result;
            try { result = execute(work, q.session->GetConnection(0)); }
            catch (const std::exception& error) { result.error = error.what(); }
            catch (...) { result.error = "Unhandled shard query exception"; }
            std::lock_guard lock(q.mutex);
            q.completed.emplace_back(std::move(work.completion), std::move(result));
            q.executing = false;
            q.idle.notify_all();
        }
    }); } catch (...) {
        q.accepting = false;
        BSLib::Database::DestroyDBSession(q.session);
        q.session = nullptr;
        throw;
    }
    return true;
}

bool Submit(std::string sql, bool returnInteger, Completion completion) {
    if (sql.empty()) return false;
    auto& q = queue();
    std::lock_guard lock(q.mutex);
    if (!q.accepting) return false;
    q.pending.push_back({std::move(sql), returnInteger, std::move(completion)});
    q.ready.notify_one();
    return true;
}

void Pump() {
    std::deque<std::pair<Completion, Result>> completed;
    auto& q = queue();
    { std::lock_guard lock(q.mutex); completed.swap(q.completed); }
    for (auto& [callback, result] : completed) {
        if (!result.ok) BSLib::Log_Printf(0x2000000, "Shard SQL failed: %s", result.error.c_str());
        if (callback) callback(result);
    }
}

void Stop() {
    auto& q = queue();
    // World-thread shutdown: an insert completion can enqueue the last update
    // or delete for an owner that disconnected while SQL was in flight. Drain
    // those dependent operations before closing admission or the connection.
    for (;;) {
        {
            std::unique_lock lock(q.mutex);
            q.idle.wait(lock, [&q] { return q.pending.empty() && !q.executing; });
        }
        Pump();
        std::lock_guard lock(q.mutex);
        if (!q.pending.empty() || !q.completed.empty() || q.executing) continue;
        q.accepting = false;
        break;
    }
    q.ready.notify_all();
    if (q.worker.joinable()) q.worker.join();
    Pump();
    if (q.session) {
        BSLib::Database::DestroyDBSession(q.session);
        q.session = nullptr;
    }
}
}
