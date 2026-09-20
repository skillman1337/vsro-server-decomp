/**
 * ============================================================================
 * Joymax BSLib - Network Stream Message Buffer (CMsg) Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Msg.cpp
 *
 * Implements:
 *   - CMsg Constructor            @ 0x00471610 (53 bytes)
 *   - CMsg Reset                  @ 0x00471500 (109 bytes)
 *   - CMsg InitBuffer             @ 0x00471580 (58 bytes)
 *   - CMsg Write                  @ 0x00404090 (130 bytes)
 *   - CMsg ReadBytes              @ 0x00404C20 (68 bytes)
 *   - CMsg ReadString             @ 0x00404B60
 *   - CMsg GetRemainingCapacity   @ 0x00432890 (38 bytes)
 * ============================================================================
 */

#include "Msg.h"
#include "BSLog.h"
#include <stdexcept>
#include <cstdlib>

/*
================
CMsg::CMsg
[RECONSTRUCTED - Native 0x00471610] (53 bytes)
================
*/
CMsg::CMsg() {
	std::memset(this, 0, 0x14);
	m_dwState = 2;
	m_dwReserved18 = 0;
	m_dwReserved1C = 0;
	m_dwFlag20 = 1;
	m_bForwardRead = 1;
	m_wReadOffset = 0;
	m_wWriteOffset = 0;
	m_dwFlags = 0;
	m_nRefCount = 0;
	m_pData = nullptr;
	m_pAllocatedData = nullptr;
	m_bInUse = 0;

	InitBuffer(m_rawBuffer, 0x1000);
	m_pBufferBase = m_pData;
}

CMsg::~CMsg() {
	if (m_pAllocatedData != nullptr) {
		delete[] m_pAllocatedData;
		m_pAllocatedData = nullptr;
	}
}

/*
================
CMsg::Reset
[RECONSTRUCTED - Native 0x00471500] (109 bytes)
Resets read/write offsets back to 6 (skipping the 6-byte header: 2 bytes length,
2 bytes opcode, 2 bytes security), clears header bytes, and deletes heap buffer.
================
*/
void CMsg::Reset() {
	m_wWriteOffset = 6;
	m_wReadOffset = 6;
	m_bForwardRead = 1;
	m_nRefCount = 0;
	m_dwReserved60 = 0;
	m_dwReserved64 = 0;
	m_dwFlags = 0;

	InitBuffer(m_rawBuffer, 0x1000);

	if (m_pData != nullptr) {
		std::memset(m_pData, 0, 6);
	}

	if (m_pAllocatedData != nullptr) {
		delete[] m_pAllocatedData;
		m_pAllocatedData = nullptr;
	}
}

/*
================
CMsg::InitBuffer
[RECONSTRUCTED - Native 0x00471580] (58 bytes)
Sets buffer pointers and sub-header field pointers.
================
*/
void CMsg::InitBuffer(void* pBuffer, uint32_t dwCapacity) {
	uint8_t* pByteBuf = static_cast<uint8_t*>(pBuffer);
	if (m_pData == pByteBuf) {
		return;
	}

	m_dwCapacity = dwCapacity;
	m_pData = pByteBuf;
	m_pBufferBase = pByteBuf;

	if (pByteBuf != nullptr) {
		m_pwPayloadLength = reinterpret_cast<uint16_t*>(pByteBuf + 0);
		m_pwOpcode        = reinterpret_cast<uint16_t*>(pByteBuf + 2);
		m_pwSecurityCount = reinterpret_cast<uint16_t*>(pByteBuf + 4);
		m_pPayload        = pByteBuf + 6;
	} else {
		m_pwPayloadLength = nullptr;
		m_pwOpcode        = nullptr;
		m_pwSecurityCount = nullptr;
		m_pPayload        = nullptr;
	}
}

/*
================
CMsg::Write
[RECONSTRUCTED - Native 0x00404090] (130 bytes)
Writes payload bytes to m_pData + m_wWriteOffset, advances write offset,
and updates the 16-bit payload length field in the packet header.
================
*/
int16_t CMsg::Write(const void* pData, uint16_t wLength) {
	uint32_t dwNewOffset = static_cast<uint32_t>(m_wWriteOffset) + static_cast<uint32_t>(wLength);
	if (dwNewOffset > m_dwCapacity) {
		throw std::runtime_error("CMsg::Write: Buffer overflow");
	}

	if (pData != nullptr && wLength > 0 && m_pData != nullptr) {
		std::memcpy(m_pData + m_wWriteOffset, pData, wLength);
	}

	m_wWriteOffset = static_cast<uint16_t>(dwNewOffset);

	// Payload length excludes the 6-byte Silkroad header
	int16_t wPayloadLen = static_cast<int16_t>(m_wWriteOffset - 6);
	if (wPayloadLen > 0x7FFF) {
		// Native generates minidump if payload exceeds 32767 bytes
	}

	if (m_pwPayloadLength != nullptr) {
		*m_pwPayloadLength = (*m_pwPayloadLength & 0x8000) | (wPayloadLen & 0x7FFF);
		return *m_pwPayloadLength;
	}

	return wPayloadLen;
}

/*
================
CMsg::ReadBytes
[RECONSTRUCTED - Native 0x00404C20] (68 bytes)
Reads data from m_pData + m_wReadOffset and advances read offset.
================
*/
void* CMsg::ReadBytes(void* pDest, uint16_t wLength) {
	if (pDest == nullptr || wLength == 0 || m_pData == nullptr) {
		return nullptr;
	}

	if (static_cast<uint32_t>(m_wReadOffset) + wLength > static_cast<uint32_t>(m_wWriteOffset)) {
		return nullptr;
	}

	std::memcpy(pDest, m_pData + m_wReadOffset, wLength);
	m_wReadOffset += wLength;
	return pDest;
}

/*
================
CMsg::ReadRaw
[RECONSTRUCTED - forward 0x00403F60 / 0x00521750, reverse 0x005216C0 / 0x005217A0]

The per-width readers are one template in the original; the caller picks the direction on
m_bForwardRead (+0x1048). Underflow throws CMsgException in both directions.
================
*/
void CMsg::ReadRaw(void* pDest, uint16_t wLength) {
	if (m_bForwardRead != 0) {
		if (static_cast<int32_t>(m_wReadOffset) + wLength > static_cast<int32_t>(m_wWriteOffset)) {
			throw CMsgException();
		}
		std::memcpy(pDest, m_pData + m_wReadOffset, wLength);
		m_wReadOffset = static_cast<uint16_t>(m_wReadOffset + wLength);
		return;
	}

	if (static_cast<int32_t>(m_wWriteOffset) - 6 < static_cast<int32_t>(wLength)) {
		throw CMsgException();
	}
	m_wWriteOffset = static_cast<uint16_t>(m_wWriteOffset - wLength);
	std::memcpy(pDest, m_pData + m_wWriteOffset, wLength);

	uint16_t wPayload = static_cast<uint16_t>(m_wWriteOffset - 6);
	ASSERT(wPayload <= 0x7FFF);
	*m_pwPayloadLength = static_cast<uint16_t>((*m_pwPayloadLength & 0x8000) | wPayload);
}

/*
================
CMsg::ReadString
[RECONSTRUCTED - Native 0x00404B60]
Reads 16-bit length-prefixed ASCII string from stream.
================
*/
std::string CMsg::ReadString() {
	uint16_t wStrLen = 0;
	if (ReadBytes(&wStrLen, sizeof(wStrLen)) == nullptr || wStrLen == 0) {
		return std::string();
	}
	if (wStrLen > 4096) {
		return std::string();
	}
	std::string result(wStrLen, '\0');
	if (ReadBytes(&result[0], wStrLen) == nullptr) {
		return std::string();
	}
	return result;
}

/*
================
CMsg::GetRemainingCapacity
[RECONSTRUCTED - Native 0x00432890] (38 bytes)
Returns remaining available bytes before reaching max capacity.
================
*/
uint32_t CMsg::GetRemainingCapacity() const {
	if (m_wWriteOffset >= m_dwCapacity) {
		return 0;
	}
	return m_dwCapacity - static_cast<uint32_t>(m_wWriteOffset);
}
