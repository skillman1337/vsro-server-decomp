/**
 * ============================================================================
 * Silkroad Online - Asynchronous Database Query Base
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery.h
 *
 * Implements:
 *   - CAsyncQuery @ 0x00AE2DE0
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_ASYNCQUERY_H_
#define _SR_GAMESERVER_ASYNCQUERY_H_

#include <cstdint>
#include <string>

class CAsyncQuery {
public:
	CAsyncQuery();
	virtual ~CAsyncQuery();

	// [RECONSTRUCTED - Native 0x00AE2DE0]
	// Executes SQL command on worker thread and dispatches result to main thread
	virtual bool Execute();
	virtual void OnComplete();

	uint32_t GetQueryID() const;

protected:
	uint32_t    m_dwQueryID;
	int32_t     m_nResultCode;
	std::string m_strQuery;
};

#endif // _SR_GAMESERVER_ASYNCQUERY_H_
