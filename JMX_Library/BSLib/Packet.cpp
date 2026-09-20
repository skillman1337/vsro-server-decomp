/**
 * ============================================================================
 * Joymax BSLib / BSNet - Network Packet Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Packet.cpp
 *
 * Implements the core network IPC/Protocol packet methods.
 * [NOTE: Native 0x00956060, 0x00956080, 0x009560B0, 0x009562A0, 0x00956430, 0x00956440
 *  were previously mislabeled as CPacket methods; they are proven to be
 *  ServerFramework::CMassiveMsg in MassiveMsg.cpp]
 *   - Stream readers        @ 0x00521750 / 0x0048BA70 / 0x00403F60
 * ============================================================================
 */

#include "Packet.h"
#include "BSLog.h"
#include <cstring>
#include <cstdio>

namespace BSLib {

/*
================
CPacket::Allocate
================
*/
CPacket* CPacket::Allocate(int32_t nType) {
	CPacket* pPacket = new CPacket();
	pPacket->m_nType   = nType;
	pPacket->m_bActive = 1;
	pPacket->m_byFlags = 0;
	pPacket->m_wOpcode = 0;
	return pPacket;
}

/*
================
CPacket::SetOpcode
Native 0x00956430
================
*/
void CPacket::SetOpcode(uint16_t wOpcode) {
	m_wOpcode = wOpcode;
}

uint16_t CPacket::GetOpcode() const {
	return m_wOpcode;
}

/*
================
CPacket::Write
Native 0x009560B0
================
*/
void CPacket::Write(const void* pData, size_t nLength) {
	if (pData != nullptr && nLength > 0) {
		const uint8_t* pBytes = static_cast<const uint8_t*>(pData);
		m_streamBuffer.insert(m_streamBuffer.end(), pBytes, pBytes + nLength);
	}
}

void CPacket::WriteUint8(uint8_t val) {
	Write(&val, sizeof(val));
}

void CPacket::WriteUint16(uint16_t val) {
	Write(&val, sizeof(val));
}

void CPacket::WriteUint32(uint32_t val) {
	Write(&val, sizeof(val));
}

void CPacket::SetReadDirection(uint32_t dwDir) {
	m_dwReadDirection = dwDir;
}

uint32_t CPacket::GetReadDirection() const {
	return m_dwReadDirection;
}

/*
================
CPacket::Send
Native 0x009562A0
================
*/
int32_t CPacket::Send(uint32_t dwSessionID, int32_t bKeepPacket) {
	(void)bKeepPacket;
	BSLib::Log_Printf(0, "[CPacket::Send] Opcode 0x%04X (%zu bytes) queued to Session 0x%08X",
		m_wOpcode, m_streamBuffer.size(), dwSessionID);
	return 1;
}

/*
================
CPacket::Flush
Native 0x00956440
================
*/
void CPacket::Flush() {
}

/*
================
CPacket::Release
Native 0x00956080
================
*/
int32_t CPacket::Release() {
	m_bActive = 0;
	delete this;
	return 1;
}

/*
================
CPacket::Read
================
*/
bool CPacket::Read(void* pDest, size_t nLength) {
	if (!pDest || nLength == 0) return false;
	if (m_nReadOffset + nLength > m_streamBuffer.size()) return false;
	std::memcpy(pDest, m_streamBuffer.data() + m_nReadOffset, nLength);
	m_nReadOffset += nLength;
	return true;
}

/*
================
CPacket::ReadUint8
================
*/
bool CPacket::ReadUint8(uint8_t* pVal) {
	return Read(pVal, sizeof(uint8_t));
}

/*
================
CPacket::ReadUint8_Forward
================
*/
bool CPacket::ReadUint8_Forward(uint8_t* pVal) {
	return Read(pVal, sizeof(uint8_t));
}

/*
================
CPacket::ReadUint16
================
*/
bool CPacket::ReadUint16(uint16_t* pVal) {
	return Read(pVal, sizeof(uint16_t));
}

/*
================
CPacket::ReadUint16_Forward
================
*/
bool CPacket::ReadUint16_Forward(uint16_t* pVal) {
	return Read(pVal, sizeof(uint16_t));
}

/*
================
CPacket::ReadUint32
================
*/
bool CPacket::ReadUint32(uint32_t* pVal) {
	return Read(pVal, sizeof(uint32_t));
}

/*
================
CPacket::ReadInt32_Forward
================
*/
bool CPacket::ReadInt32_Forward(int32_t* pVal) {
	return Read(pVal, sizeof(int32_t));
}

/*
================
CPacket::ReadFloat_Forward
================
*/
bool CPacket::ReadFloat_Forward(float* pVal) {
	return Read(pVal, sizeof(float));
}

/*
================
CPacket::GetRemainingBytes
================
*/
size_t CPacket::GetRemainingBytes() const {
	if (m_nReadOffset >= m_streamBuffer.size()) return 0;
	return m_streamBuffer.size() - m_nReadOffset;
}

/*
================
CPacket::ReadUint8_Reverse
================
*/
bool CPacket::ReadUint8_Reverse(uint8_t* pVal) {
	if (!pVal || m_streamBuffer.empty()) return false;
	if (m_nTailOffset >= m_streamBuffer.size()) return false;
	size_t idx = m_streamBuffer.size() - 1 - m_nTailOffset;
	*pVal = m_streamBuffer[idx];
	m_nTailOffset += sizeof(uint8_t);
	return true;
}

/*
================
CPacket::ReadUint16_Reverse
================
*/
bool CPacket::ReadUint16_Reverse(uint16_t* pVal) {
	if (!pVal || m_streamBuffer.size() < sizeof(uint16_t)) return false;
	if (m_nTailOffset + sizeof(uint16_t) > m_streamBuffer.size()) return false;
	size_t idx = m_streamBuffer.size() - sizeof(uint16_t) - m_nTailOffset;
	std::memcpy(pVal, m_streamBuffer.data() + idx, sizeof(uint16_t));
	m_nTailOffset += sizeof(uint16_t);
	return true;
}

/*
================
CPacket::ReadInt32_Reverse
================
*/
bool CPacket::ReadInt32_Reverse(int32_t* pVal) {
	if (!pVal || m_streamBuffer.size() < sizeof(int32_t)) return false;
	if (m_nTailOffset + sizeof(int32_t) > m_streamBuffer.size()) return false;
	size_t idx = m_streamBuffer.size() - sizeof(int32_t) - m_nTailOffset;
	std::memcpy(pVal, m_streamBuffer.data() + idx, sizeof(int32_t));
	m_nTailOffset += sizeof(int32_t);
	return true;
}

/*
================
CPacket::ReadFloat_Reverse
================
*/
bool CPacket::ReadFloat_Reverse(float* pVal) {
	if (!pVal || m_streamBuffer.size() < sizeof(float)) return false;
	if (m_nTailOffset + sizeof(float) > m_streamBuffer.size()) return false;
	size_t idx = m_streamBuffer.size() - sizeof(float) - m_nTailOffset;
	std::memcpy(pVal, m_streamBuffer.data() + idx, sizeof(float));
	m_nTailOffset += sizeof(float);
	return true;
}

/*
================
CPacket::ReadBytes_Reverse
================
*/
bool CPacket::ReadBytes_Reverse(void* pDest, size_t nLength) {
	if (!pDest || nLength == 0) return false;
	if (m_nTailOffset + nLength > m_streamBuffer.size()) return false;
	size_t idx = m_streamBuffer.size() - nLength - m_nTailOffset;
	std::memcpy(pDest, m_streamBuffer.data() + idx, nLength);
	m_nTailOffset += nLength;
	return true;
}

/*
================
CPacket::ResetReadCursors
================
*/
void CPacket::ResetReadCursors() {
	m_nReadOffset = 0;
	m_nTailOffset = 0;
	m_dwReadDirection = 1;
}

} // namespace BSLib
