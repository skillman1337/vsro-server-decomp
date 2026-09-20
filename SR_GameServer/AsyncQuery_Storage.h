/**
 * ============================================================================
 * Silkroad Online - Asynchronous Query: Storage
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery_Storage.h
 *
 * Implements:
 *   - CAsyncQuery_Storage @ 0x00AE50F0
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_ASYNCQUERY_STORAGE_H_
#define _SR_GAMESERVER_ASYNCQUERY_STORAGE_H_

#include "AsyncQuery.h"

class CAsyncQuery_Storage : public CAsyncQuery {
public:
	CAsyncQuery_Storage();
	virtual ~CAsyncQuery_Storage() override;

	// [RECONSTRUCTED - Native 0x00AE50F0]
	// Persists chest/bank storage changes to database
	virtual bool Execute() override;
	virtual void OnComplete() override;
};

#endif // _SR_GAMESERVER_ASYNCQUERY_STORAGE_H_
