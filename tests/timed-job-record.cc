#include "ServerCommon/InstanceTimedJob.h"
#include "SR_GameServer/TimedJob.h"
#include <cassert>
#include <iostream>

int main() {
    CInstanceTimedJob record;
    record.SetID(17); record.SetCharID(29);
    record.SetCategory(0); record.SetJobID(999);
    record.m_dwStateFlags = 0; // loaded clean record
    std::string sql = "stale";
    assert(record.BuildDirtyUpdateQuery(sql) && sql.empty());
    record.SetTimeToKeep(100);
    assert(record.m_dwStateFlags == 4);
    assert(record.BuildDirtyUpdateQuery(sql));
    assert(sql == "UPDATE _TIMEDJOB SET TimeToKeep = 100 WHERE ID = 17 AND CharID = 29");
    assert(record.m_dwStateFlags == 0);
    record.SetTimeToKeep(100);
    assert(record.BuildDirtyUpdateQuery(sql) && sql.empty());
    record.SetData8(0xffffffff);
    record.SetSerial64(0x76543210, 0xfedcba98);
    record.SetJID(0x80000001);
    assert(record.m_dwStateFlags == 8);
    assert(record.BuildDirtyUpdateQuery(sql));
    assert(sql == "UPDATE _TIMEDJOB SET Data1 = 0, Data2 = 0, Data3 = 0, Data4 = 0, Data5 = 0, Data6 = 0, Data7 = 0, Data8 = -1, Serial64 = -81985529216486896, JID = -2147483647 WHERE ID = 17 AND CharID = 29");
    record.SetTimeToKeep(99); record.SetData1(3);
    g_bTimedJobDBWriteAllowed = 0;
    assert(!record.BuildDirtyUpdateQuery(sql) && sql.empty() && record.m_dwStateFlags == 12);
    g_bTimedJobDBWriteAllowed = 5;
    assert(record.BuildDirtyUpdateQuery(sql));
    assert(sql.starts_with("UPDATE _TIMEDJOB SET TimeToKeep = 99, Data1 = 3,"));
    assert(record.m_dwStateFlags == 0);
    CTimedJob job;
    auto* stored = new CInstanceTimedJob;
    stored->SetID(4); stored->m_dwStateFlags = 0;
    assert(job.Initialize(nullptr, nullptr, stored, 0));
    assert(job.Checkpoint()); // no write required; no running SQL worker
    std::cout << "timed-job dirty groups, unchanged records, signed values, keys and policy passed\n";
}
