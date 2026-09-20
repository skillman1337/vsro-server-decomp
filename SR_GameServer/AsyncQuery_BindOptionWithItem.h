/**
 * ============================================================================
 * Silkroad Online - Asynchronous Query: Bind Option With Item
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery_BindOptionWithItem.h
 *
 * Implements:
 *   - CAsyncQuery_BindOptionWithItem @ 0x00AE37E0
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_ASYNCQUERY_BINDOPTIONWITHITEM_H_
#define _SR_GAMESERVER_ASYNCQUERY_BINDOPTIONWITHITEM_H_

#include "AsyncQuery.h"

class CAsyncQuery_BindOptionWithItem : public CAsyncQuery {
public:
	CAsyncQuery_BindOptionWithItem();
	virtual ~CAsyncQuery_BindOptionWithItem() override;

	// [RECONSTRUCTED - Native 0x00AE37E0]
	// Saves magic option / alchemy socket binding to database
	virtual bool Execute() override;
	virtual void OnComplete() override;

	void SetBindingParams(uint32_t dwItemSerial, uint32_t dwOptionID, uint32_t dwOptionValue);

private:
	uint32_t m_dwItemSerial;
	uint32_t m_dwOptionID;
	uint32_t m_dwOptionValue;
};

#endif // _SR_GAMESERVER_ASYNCQUERY_BINDOPTIONWITHITEM_H_
