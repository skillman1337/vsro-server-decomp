/**
 * ============================================================================
 * Silkroad Online - Asynchronous Query: Bind Option With Item Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AsyncQuery_BindOptionWithItem.cpp
 *
 * Implements:
 *   - CAsyncQuery_BindOptionWithItem::Execute @ 0x00AE37E0
 * ============================================================================
 */

#include "AsyncQuery_BindOptionWithItem.h"

CAsyncQuery_BindOptionWithItem::CAsyncQuery_BindOptionWithItem()
	: m_dwItemSerial(0)
	, m_dwOptionID(0)
	, m_dwOptionValue(0)
{
}

CAsyncQuery_BindOptionWithItem::~CAsyncQuery_BindOptionWithItem() {
}

/*
================
CAsyncQuery_BindOptionWithItem::Execute
[RECONSTRUCTED - Native 0x00AE37E0]
================
*/
bool CAsyncQuery_BindOptionWithItem::Execute() {
	return true;
}

void CAsyncQuery_BindOptionWithItem::OnComplete() {
}

void CAsyncQuery_BindOptionWithItem::SetBindingParams(uint32_t dwItemSerial, uint32_t dwOptionID, uint32_t dwOptionValue) {
	m_dwItemSerial = dwItemSerial;
	m_dwOptionID = dwOptionID;
	m_dwOptionValue = dwOptionValue;
}
