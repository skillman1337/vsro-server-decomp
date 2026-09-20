/**
 * ============================================================================
 * Joymax BSLib / BSNet - Network Packet
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Packet.h
 *
 * Implements the core network IPC/Protocol packet structure:
 *   - Native Ctor / Allocator @ 0x00956060 (CPacket_Allocate)
 *   - Native Opcode setter    @ 0x00956430 (CPacket_SetOpcode)
 *   - Native Data writer      @ 0x009560B0 (CPacket_WriteData)
 *   - Native Session sender   @ 0x009562A0 (CPacket_SendToSession)
 *   - Native Stream flusher   @ 0x00956440 (CPacket_Flush)
 *   - Native Packet releaser  @ 0x00956080 (CPacket_Release)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_PACKET_H_
#define _JMX_LIBRARY_BSLIB_PACKET_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <exception>
#include "BSLog.h"
#include "BSException.h"

namespace BSLib {

/**
 * CPacket
 * Joymax cluster & network protocol packet matching native layout @ 0x00956060
 */
class CPacket {
public:
	// Exact struct layout proven from 0x00956060 / 0x00956430 / 0x009560B0 / 0x009562A0:
	uint32_t             m_dwReserved1 = 0; // +0x00
	uint32_t             m_dwReserved2 = 0; // +0x04
	std::vector<uint8_t> m_streamBuffer;    // +0x08, +0x0C, +0x10 (stream buffer pointers)
	uint32_t             m_dwReserved3 = 0; // +0x14
	int32_t              m_nType       = 1; // +0x18: Packet Type (1 = Cluster IPC / Service Message)
	uint8_t              m_bActive     = 1; // +0x1C: Active / In-Use flag (1 = Active)
	uint8_t              m_byFlags     = 0; // +0x1D: Packet delivery / routing flags
	uint16_t             m_wOpcode     = 0; // +0x1E: Protocol Opcode (e.g. 0x2005 Server Notify)

public:
	CPacket() = default;
	explicit CPacket(uint16_t wOpcode) : m_wOpcode(wOpcode) {}
	explicit CPacket(void* pData) {
		(void)pData;
	}
	~CPacket() = default;

	// [NOTE: Native 0x00956060 was previously mislabeled as CPacket_Allocate;
	// it is proven to be ServerFramework::CMassiveMsg::Allocate in MassiveMsg.cpp]
	static CPacket* Allocate(int32_t nType = 1);

	// Sets the packet opcode at offset +0x1E
	void SetOpcode(uint16_t wOpcode);
	uint16_t GetOpcode() const;

	// Appends arbitrary raw bytes to the packet payload stream
	void Write(const void* pData, size_t nLength);
	void WriteBytes(const void* pData, size_t nLength) { Write(pData, nLength); }
	void WriteUint8(uint8_t val);
	void WriteUint16(uint16_t val);
	void WriteUint32(uint32_t val);
	void WriteFloat(float val) { Write(&val, sizeof(float)); }
	void WriteUInt8(uint8_t val) { WriteUint8(val); }
	void WriteUInt16(uint16_t val) { WriteUint16(val); }
	void WriteUInt32(uint32_t val) { WriteUint32(val); }

	// Stream reading cursors (Forward: +0x103C, Reverse: +0x103E, Direction: +0x1048)
	size_t   m_nReadOffset     = 0;
	size_t   m_nTailOffset     = 0;
	uint32_t m_dwReadDirection = 1; // 1 = Forward (default), 0 = Reverse

	void SetReadDirection(uint32_t dwDir);
	uint32_t GetReadDirection() const;

	// Forward stream readers [RECONSTRUCTED - 0x00521750 / 0x0048BA70 / 0x00403F60 / 0x00423F20 / 0x00404C40]
	bool Read(void* pDest, size_t nLength);
	bool ReadUint8(uint8_t* pVal);
	bool ReadUint8_Forward(uint8_t* pVal);
	bool ReadUint16(uint16_t* pVal);
	bool ReadUint16_Forward(uint16_t* pVal);
	bool ReadUint32(uint32_t* pVal);
	bool ReadInt32(int32_t* pVal) { return ReadInt32_Forward(pVal); }
	bool ReadInt32(uint32_t* pVal) { return ReadUint32(pVal); }
	bool ReadInt32_Forward(int32_t* pVal);
	bool ReadFloat(float* pVal) { return ReadFloat_Forward(pVal); }
	bool ReadFloat_Forward(float* pVal);
	size_t GetRemainingBytes() const;

	// String reader and writers (16-bit length prefix matching Silkroad wire format)
	bool ReadString(std::string& strOut) {
		uint16_t wLen = 0;
		if (!ReadUint16(&wLen)) return false;
		if (wLen == 0) { strOut.clear(); return true; }
		strOut.resize(wLen);
		return Read(&strOut[0], wLen);
	}

	void WriteString(const std::string& strIn) {
		uint16_t wLen = static_cast<uint16_t>(strIn.size());
		WriteUint16(wLen);
		if (wLen > 0) {
			Write(strIn.data(), wLen);
		}
	}

	void WriteString(const char* pszIn) {
		if (pszIn == nullptr) {
			WriteUint16(0);
			return;
		}
		uint16_t wLen = static_cast<uint16_t>(std::strlen(pszIn));
		WriteUint16(wLen);
		if (wLen > 0) {
			Write(pszIn, wLen);
		}
	}

	// Reverse stream readers [RECONSTRUCTED - Native 0x005217A0 / 0x00409910 / 0x005216C0 / 0x00423F70 / 0x00403FB0]
	bool ReadUint8_Reverse(uint8_t* pVal);
	bool ReadUint16_Reverse(uint16_t* pVal);
	bool ReadInt32_Reverse(int32_t* pVal);
	bool ReadFloat_Reverse(float* pVal);
	bool ReadBytes_Reverse(void* pDest, size_t nLength);

	void ResetReadCursors();

	// [NOTE: Native 0x009562A0 is ServerFramework::CMassiveMsg::SendToSession in MassiveMsg.cpp]
	int32_t Send(uint32_t dwSessionID, int32_t bKeepPacket = 0);

	// [NOTE: Native 0x00956440 is ServerFramework::CMassiveMsg::Flush in MassiveMsg.cpp]
	void Flush();

	// [NOTE: Native 0x00956080 is ServerFramework::CMassiveMsg::Release in MassiveMsg.cpp]
	int32_t Release();
};

} // namespace BSLib

typedef BSLib::CPacket CPacket;
typedef BSLib::CMsgException CMsgException;

#endif // _JMX_LIBRARY_BSLIB_PACKET_H_
