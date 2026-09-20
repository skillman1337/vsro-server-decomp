/**
 * ============================================================================
 * Silkroad Online - Asynchronous Query: Open Market
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery_OpenMarket.h
 *
 * Implements:
 *   - CAsyncQuery_OpenMarket @ 0x00AE3D70
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_ASYNCQUERY_OPENMARKET_H_
#define _SR_GAMESERVER_ASYNCQUERY_OPENMARKET_H_

#include "AsyncQuery.h"

class CAsyncQuery_OpenMarket : public CAsyncQuery {
public:
	CAsyncQuery_OpenMarket();
	virtual ~CAsyncQuery_OpenMarket() override;

	// [RECONSTRUCTED - Native 0x00AE3D70]
	// Fetches consignment/flea market listings from database
	virtual bool Execute() override;
	virtual void OnComplete() override;
};

#endif // _SR_GAMESERVER_ASYNCQUERY_OPENMARKET_H_
