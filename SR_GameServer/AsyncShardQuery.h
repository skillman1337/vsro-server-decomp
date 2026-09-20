#pragma once
#include <cstdint>
#include <functional>
#include <string>

// Portable transport for AQ_BrutalQuery (441FD0) and AQ_TimedJob (463200).
// SQL executes on a dedicated connection; callbacks run only on the world
// thread through Pump. Enqueue success never means database success.
namespace ShardQuery {
struct Result { bool ok = false; int32_t value = 0; std::string error; };
using Completion = std::function<void(const Result&)>;
bool Start(const char* connectionString);
bool Submit(std::string sql, bool returnInteger = false, Completion completion = {});
void Pump();
void Stop();
}
