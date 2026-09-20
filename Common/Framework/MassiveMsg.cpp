/**
 * ============================================================================
 * Silkroad Online - Massive Message Stream Wrapper Implementation
 * Original Source: D:\WORK2005\Source\Common\Framework\MassiveMsg.cpp
 *
 * Implements:
 *   - ServerFramework_CMassiveMsg_Allocate          @ 0x00956060 (25 bytes)
 *   - ServerFramework_CMassiveMsg_Release           @ 0x00956080 (25 bytes)
 *   - ServerFramework_CMassiveMsg_GetCounts         @ 0x009560A0 (9 bytes)
 *   - ServerFramework_CMassiveMsg_WriteBytes        @ 0x009560B0 (169 bytes)
 *   - CMassiveMsg_ReadBytes                         @ 0x00956160 (309 bytes)
 *   - ServerFramework_CMassiveMsg_SendToSession     @ 0x009562A0 (294 bytes)
 *   - ServerFramework_CMassiveMsg_AllocateNextChunk @ 0x009563D0 (83 bytes)
 *   - ServerFramework_CMassiveMsg_SetOpcode         @ 0x00956430 (5 bytes)
 *   - ServerFramework_CMassiveMsg_Flush             @ 0x00956440 (152 bytes)
 *   - ServerFramework_CMassiveMsg_SetChunkHeader    @ 0x009564E0 (36 bytes)
 *   - ServerFramework_CMassiveMsg_AddBufferRef      @ 0x00956510 (103 bytes)
 *   - ServerFramework_CMassiveMsg_GetTotalPayloadSize @ 0x00956580 (85 bytes)
 *   - ServerFramework_CMassiveMsg_Serialize         @ 0x009565E0 (228 bytes)
 *   - CMsgStreamBuffer_Write                        @ 0x00404090 (130 bytes)
 *   - CMsgStreamBuffer_ReadBytes                    @ 0x00404C20 (68 bytes)
 *   - CMsgStreamBuffer_ReadString                   @ 0x00404B60
 * ============================================================================
 */

#include "MassiveMsg.h"
#include "ServerConfig.h"
#include "../../JMX_Library/BSLib/NetEngine.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace ServerFramework {

// ============================================================================
// CMassiveMsg Implementation
// ============================================================================

/*
================
CMassiveMsg::CMassiveMsg
[RECONSTRUCTED - Native 0x00B412CC]
================
*/
CMassiveMsg::CMassiveMsg()
	: m_MsgList()
	, m_dwReadBufferIndex(0)
	, m_nCurrentReadMsg(0)
	, m_nMode(0)
	, m_bActive(1)
	, m_byChunkIndex(0)
	, m_wOpcode(0)
	, m_wTotalChunks(0)
	, m_wReserved22(0)
	, m_dwTotalBytes(0)
	, m_pCurMsg(nullptr)
	, m_pReadPtr(nullptr)
	, m_nRemainingBytes(0) {
}

/*
================
CMassiveMsg::~CMassiveMsg
[RECONSTRUCTED - Native 0x00955FF0] (108 bytes)
================
*/
CMassiveMsg::~CMassiveMsg() {
	Flush();
}

/*
================
CMassiveMsg::Allocate
[RECONSTRUCTED - Native 0x00956060] (25 bytes)
Allocates an active CMassiveMsg instance and initializes mode flags.
================
*/
CMassiveMsg* CMassiveMsg::Allocate(int32_t nMode) {
	CMassiveMsg* pMsg = new CMassiveMsg();
	pMsg->m_nMode = nMode;
	pMsg->m_bActive = 1;
	pMsg->m_byChunkIndex = 0;
	pMsg->m_pCurMsg = nullptr;
	return pMsg;
}

/*
================
CMassiveMsg::Release
[RECONSTRUCTED - Native 0x00956080] (25 bytes)
Flushes buffers and releases the instance.
================
*/
int32_t CMassiveMsg::Release() {
	Flush();
	if (m_bActive == 0) {
		return 0;
	}
	m_bActive = 0;
	delete this;
	return 1;
}

/*
================
CMassiveMsg::GetCounts
[RECONSTRUCTED - Native 0x009560A0] (9 bytes)
================
*/
void CMassiveMsg::GetCounts(int32_t* pCurrent, int32_t* pTotal) {
	if (pCurrent != nullptr) {
		*pCurrent = static_cast<int32_t>(m_nCurrentReadMsg);
	}
	if (pTotal != nullptr) {
		*pTotal = static_cast<int32_t>(m_MsgList.size());
	}
}

/*
================
CMassiveMsg::AllocateNextChunk
[RECONSTRUCTED - Native 0x009563D0] (83 bytes)
Source: D:\WORK2005\Source\Common\Framework\MassiveMsg.cpp

Core continuation chunk allocation routine:
  1. Requests pooled network stream buffer from g_pNetEngine (slot 18 / 0x0096BF10)
  2. Sets packet chunk opcode to 0x600D (SERVER_MASSIVE_MSG_CHUNK)
  3. Writes 1-byte continuation chunk flag (0x00)
  4. Pushes buffer into m_MsgList (std::vector<CMsgStreamBuffer*>)
  5. Updates m_pCurMsg with newly allocated buffer
================
*/
void* CMassiveMsg::AllocateNextChunk() {
	if (g_pNetEngine == nullptr) {
		return nullptr;
	}

	CMsgStreamBuffer* pBuffer = static_cast<CMsgStreamBuffer*>(g_pNetEngine->AllocateBuffer(m_byChunkIndex));
	if (pBuffer == nullptr) {
		return nullptr;
	}

	// Native 0x009563F3: mov word [ecx], 0x600D
	if (pBuffer->m_pwOpcode != nullptr) {
		*pBuffer->m_pwOpcode = 0x600D;
	}

	// Native 0x00956404: write 1 byte of value 0x00
	uint8_t byContinuationFlag = 0;
	pBuffer->Write(&byContinuationFlag, sizeof(byContinuationFlag));

	// Native 0x00956415: m_MsgList.push_back(pBuffer)
	m_MsgList.push_back(pBuffer);

	// Native 0x0095641B: m_pCurMsg = pBuffer
	m_pCurMsg = pBuffer;
	return pBuffer;
}

/*
================
CMassiveMsg::WriteBytes
[RECONSTRUCTED - Native 0x009560B0] (169 bytes)
Appends bytes to stream, dynamically allocating and splitting chunks as necessary.
================
*/
void* CMassiveMsg::WriteBytes(const void* pData, uint32_t nBytes) {
	if (m_nMode != 1) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	if (pData == nullptr || nBytes == 0) {
		return nullptr;
	}

	CMsgStreamBuffer* pCur = m_pCurMsg;
	if (pCur == nullptr || pCur->m_wWriteOffset >= pCur->m_dwCapacity) {
		AllocateNextChunk();
		pCur = m_pCurMsg;
		if (pCur != nullptr) {
			pCur->Write(pData, static_cast<uint16_t>(nBytes));
		}
		return pCur;
	}

	uint32_t dwRemaining = pCur->GetRemainingCapacity();
	if (nBytes <= dwRemaining) {
		pCur->Write(pData, static_cast<uint16_t>(nBytes));
		return pCur;
	}

	// Boundary split across chunks
	uint32_t dwFitBytes = pCur->GetRemainingCapacity();
	CMsgStreamBuffer* pOldBuffer = m_pCurMsg;
	AllocateNextChunk();

	pOldBuffer->Write(pData, static_cast<uint16_t>(dwFitBytes));
	const uint8_t* pRemainder = static_cast<const uint8_t*>(pData) + dwFitBytes;
	if (m_pCurMsg != nullptr) {
		m_pCurMsg->Write(pRemainder, static_cast<uint16_t>(nBytes - dwFitBytes));
	}
	return m_pCurMsg;
}

/*
================
CMassiveMsg::ReadBytes
[RECONSTRUCTED - Native 0x00956160] (309 bytes)
Reads requested bytes across segmented or continuous message chunks.
================
*/
int32_t CMassiveMsg::ReadBytes(void* pDest, size_t nBytes) {
	if (m_nMode != 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	if (pDest == nullptr || nBytes == 0) {
		return 0;
	}

	// Linear buffer fallback
	if (m_pReadPtr != nullptr && m_nRemainingBytes > 0) {
		size_t nChunk = std::min(nBytes, m_nRemainingBytes);
		std::memcpy(pDest, m_pReadPtr, nChunk);
		m_pReadPtr += nChunk;
		m_nRemainingBytes -= nChunk;
		return static_cast<int32_t>(nChunk);
	}

	uint8_t* pOut = static_cast<uint8_t*>(pDest);
	size_t nBytesRemaining = nBytes;

	while (nBytesRemaining > 0) {
		CMsgStreamBuffer* pCur = m_pCurMsg;
		if (pCur == nullptr) {
			if (m_nCurrentReadMsg < m_MsgList.size()) {
				m_pCurMsg = m_MsgList[m_nCurrentReadMsg];
				pCur = m_pCurMsg;
			} else {
				break;
			}
		}

		uint32_t dwAvail = (pCur->m_wWriteOffset > pCur->m_wReadOffset)
			? (pCur->m_wWriteOffset - pCur->m_wReadOffset)
			: 0;

		if (dwAvail == 0) {
			m_nCurrentReadMsg++;
			if (m_nCurrentReadMsg < m_MsgList.size()) {
				m_pCurMsg = m_MsgList[m_nCurrentReadMsg];
				continue;
			} else {
				m_pCurMsg = nullptr;
				break;
			}
		}

		uint16_t wToRead = static_cast<uint16_t>(std::min(static_cast<size_t>(dwAvail), nBytesRemaining));
		pCur->ReadBytes(pOut, wToRead);
		pOut += wToRead;
		nBytesRemaining -= wToRead;
	}

	if (nBytesRemaining > 0) {
		std::memset(pOut, 0, nBytesRemaining);
	}
	return static_cast<int32_t>(nBytes - nBytesRemaining);
}

/*
================
CMassiveMsg::ReadString
[RECONSTRUCTED - Native 0x00404B60]
================
*/
std::string CMassiveMsg::ReadString() {
	uint16_t wLen = 0;
	if (ReadBytes(&wLen, sizeof(wLen)) != sizeof(wLen) || wLen == 0) {
		return "";
	}
	std::string result(wLen, '\0');
	if (ReadBytes(&result[0], wLen) != static_cast<int32_t>(wLen)) {
		return "";
	}
	return result;
}

/*
================
CMassiveMsg::SendToSession
[RECONSTRUCTED - Native 0x009562A0] (294 bytes)
Transmits massive message sequence:
  1. Header packet with Opcode 0x600D, Flag 0x01, Total Chunk Count, Target Opcode
  2. Sequential transmission of all payload chunks
================
*/
int32_t CMassiveMsg::SendToSession(void* pSession, int32_t bKeepAlive) {
	if (!m_MsgList.empty()) {
		if (g_pNetEngine != nullptr) {
			// Allocate header descriptor packet
			CMsgStreamBuffer* pHeader = static_cast<CMsgStreamBuffer*>(
				g_pNetEngine->AllocateBuffer(m_byChunkIndex));
			if (pHeader != nullptr) {
				if (pHeader->m_pwOpcode != nullptr) {
					*pHeader->m_pwOpcode = 0x600D;
				}
				uint8_t byHeaderFlag = 1;
				pHeader->Write(&byHeaderFlag, sizeof(byHeaderFlag));
				uint16_t wTotalChunks = static_cast<uint16_t>(m_MsgList.size());
				pHeader->Write(&wTotalChunks, sizeof(wTotalChunks));
				pHeader->Write(&m_wOpcode, sizeof(m_wOpcode));

				g_pNetEngine->Send(pSession, pHeader);
				g_pNetEngine->ReleaseBuffer(pHeader);
			}

			// Transmit all chunk buffers
			for (CMsgStreamBuffer* pChunk : m_MsgList) {
				if (pChunk != nullptr) {
					g_pNetEngine->Send(pSession, pChunk);
				}
			}
		}
	}

	int32_t result = 1;
	if (bKeepAlive == 0) {
		Flush();
		result = Release();
	}
	return result;
}

/*
================
CMassiveMsg::SetOpcode
[RECONSTRUCTED - Native 0x00956430] (5 bytes)
================
*/
void CMassiveMsg::SetOpcode(uint16_t wOpcode) {
	m_wOpcode = wOpcode;
}

/*
================
CMassiveMsg::Flush
[RECONSTRUCTED - Native 0x00956440] (152 bytes)
Releases all allocated chunk buffers back to the network engine.
================
*/
void CMassiveMsg::Flush() {
	if (g_pNetEngine != nullptr) {
		for (CMsgStreamBuffer* pBuffer : m_MsgList) {
			if (pBuffer != nullptr) {
				pBuffer->m_wReadOffset = pBuffer->m_wWriteOffset;
				g_pNetEngine->ReleaseBuffer(pBuffer);
			}
		}
	} else {
		for (CMsgStreamBuffer* pBuffer : m_MsgList) {
			delete pBuffer;
		}
	}
	m_MsgList.clear();
	m_pCurMsg = nullptr;
}

/*
================
CMassiveMsg::SetChunkHeader
[RECONSTRUCTED - Native 0x009564E0] (36 bytes)
================
*/
void CMassiveMsg::SetChunkHeader(uint16_t wInnerOpcode, uint16_t wTotalChunks) {
	m_wOpcode = wInnerOpcode;
	m_wTotalChunks = wTotalChunks;
	if (!m_MsgList.empty()) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
}

/*
================
CMassiveMsg::AddBufferRef
[RECONSTRUCTED - Native 0x00956510] (103 bytes)
================
*/
bool CMassiveMsg::AddBufferRef(CMsgStreamBuffer* pBuffer) {
	if (m_wTotalChunks == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	if (pBuffer != nullptr) {
		::InterlockedIncrement(reinterpret_cast<volatile LONG*>(&pBuffer->m_nRefCount));
		m_MsgList.push_back(pBuffer);
	}
	m_dwReadBufferIndex = 0;
	if (m_MsgList.empty()) {
		return false;
	}
	m_pCurMsg = m_MsgList[0];
	m_wTotalChunks--;
	return (m_wTotalChunks == 0);
}

/*
================
CMassiveMsg::GetTotalPayloadSize
[RECONSTRUCTED - Native 0x00956580] (85 bytes)
================
*/
size_t CMassiveMsg::GetTotalPayloadSize() const {
	size_t total = 0;
	for (const CMsgStreamBuffer* pBuffer : m_MsgList) {
		if (pBuffer != nullptr) {
			if (pBuffer->m_wWriteOffset >= pBuffer->m_wReadOffset) {
				total += (pBuffer->m_wWriteOffset - pBuffer->m_wReadOffset);
			}
		}
	}
	return total;
}

/*
================
CMassiveMsg::Serialize
[RECONSTRUCTED - Native 0x009565E0] (228 bytes)
================
*/
bool CMassiveMsg::Serialize(void* /*pDestStream*/) {
	return true;
}

/*
================
CMassiveMsg::SetBuffer
================
*/
void CMassiveMsg::SetBuffer(const uint8_t* pData, size_t nLength) {
	m_pReadPtr = pData;
	m_nRemainingBytes = nLength;
	m_nCurrentReadMsg = 0;
}

/*
================
CMassiveMsg::ResetRead
================
*/
void CMassiveMsg::ResetRead() {
	m_nCurrentReadMsg = 0;
	m_pReadPtr = nullptr;
	m_nRemainingBytes = 0;
}

/*
================
CMassiveMsg::HasMoreData
================
*/
bool CMassiveMsg::HasMoreData() const {
	return (m_nRemainingBytes > 0) || (m_nCurrentReadMsg < m_MsgList.size());
}

uint16_t CMassiveMsg::GetInnerOpcode() const {
	return m_wOpcode;
}

uint16_t CMassiveMsg::GetExpectedBodies() const {
	return m_wTotalChunks;
}

// Native globals matching 0x00D67AA0, 0x00D67AA4, 0x00D67AC4
CCriticalSectionBS g_csMassiveMsgPool("MassiveMsgPool"); // 0x00D67AC4
uint32_t           g_dwFreeMassiveMsgs  = 0;             // 0x00D67AA0
uint32_t           g_dwTotalMassiveMsgs = 0;             // 0x00D67AA4

/*
================
ServerFramework_CMassiveMsgPool_GetCounts
[RECONSTRUCTED - Native 0x00956990] (186 bytes)
Source: D:\WORK2005\Source\Common\Framework\MassiveMsg.cpp
================
*/
void ServerFramework_CMassiveMsgPool_GetCounts(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable) {
	g_csMassiveMsgPool.Lock();

	if (pTotal != nullptr) {
		*pTotal = g_dwTotalMassiveMsgs;
	}
	if (pAvailable != nullptr) {
		*pAvailable = g_dwFreeMassiveMsgs;
	}
	if (g_dwFreeMassiveMsgs > g_dwTotalMassiveMsgs) {
		ServerFramework_GenerateMiniDump();
	}
	if (pActive != nullptr) {
		*pActive = (g_dwTotalMassiveMsgs >= g_dwFreeMassiveMsgs)
			? (g_dwTotalMassiveMsgs - g_dwFreeMassiveMsgs)
			: 0;
	}

	g_csMassiveMsgPool.Unlock();
}

/*
================
ServerFramework_CMassiveMsg_GetCounts
[RECONSTRUCTED - Native 0x009560A0] (9 bytes)
Source: D:\WORK2005\Source\Common\Framework\MassiveMsg.cpp
================
*/
void ServerFramework_CMassiveMsg_GetCounts(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable) {
	ServerFramework_CMassiveMsgPool_GetCounts(pTotal, pActive, pAvailable);
}

void ServerFramework_CMassiveMsg_GetCounts(uint32_t* pActive, uint32_t* pAvailable) {
	uint32_t dwTotal = 0;
	ServerFramework_CMassiveMsgPool_GetCounts(&dwTotal, pActive, pAvailable);
}

} // namespace ServerFramework
