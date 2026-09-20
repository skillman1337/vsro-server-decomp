/**
 * ============================================================================
 * Silkroad Online - Asynchronous Database Query Base Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery.cpp
 *
 * Implements:
 *   - CAsyncQuery::Execute @ 0x00AE2DE0
 * ============================================================================
 */

#include "AsyncQuery.h"

static uint32_t g_dwNextAsyncQueryID = 1;

CAsyncQuery::CAsyncQuery()
	: m_dwQueryID(g_dwNextAsyncQueryID++)
	, m_nResultCode(0)
{
}

CAsyncQuery::~CAsyncQuery() {
}

/*
================
CAsyncQuery::Execute
[RECONSTRUCTED - Native 0x00AE2DE0]
================
*/
bool CAsyncQuery::Execute() {
	return true;
}

void CAsyncQuery::OnComplete() {
}

uint32_t CAsyncQuery::GetQueryID() const {
	return m_dwQueryID;
}
