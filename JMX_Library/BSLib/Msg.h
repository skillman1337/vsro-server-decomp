/**
 * ============================================================================
 * Joymax BSLib - Network Stream Message Buffer (CMsg)
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Msg.h
 *
 * Implements the native network packet stream buffer managed by CNetEngine
 * and CChunkAllocatorMT_For_CMsg:
 *   - Native Size: 4220 bytes (0x107C) proven @ 0x0096FFE0
 *   - Constructor @ 0x00471610 (CMsg_Constructor)
 *   - Reset       @ 0x00471500 (CMsg_Reset)
 *   - InitBuffer  @ 0x00471580 (CMsg_InitBuffer)
 *   - Write       @ 0x00404090 (CMsg_Write)
 *   - ReadBytes   @ 0x00404C20 (CMsg_ReadBytes)
 *   - ReadString  @ 0x00404B60 (CMsg_ReadString)
 *   - Capacity    @ 0x00432890 (CMsg_GetRemainingCapacity)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_MSG_H_
#define _JMX_LIBRARY_BSLIB_MSG_H_

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include "BSException.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#pragma pack(push, 1)

/**
 * [RECONSTRUCTED - Native 0x107C bytes]
 * CMsg - Core Joymax Network Packet Stream Buffer
 */
class CMsg {
public:
	CMsg();
	~CMsg();

	// [RECONSTRUCTED - Native 0x00471500] (109 bytes)
	// Resets header fields, read/write offsets back to 6, and frees dynamically grown heap buffer
	void Reset();

	// [RECONSTRUCTED - Native 0x00471580] (58 bytes)
	// Sets up active buffer pointer, internal packet header pointers, and capacity
	void InitBuffer(void* pBuffer, uint32_t dwCapacity);

	// [RECONSTRUCTED - Native 0x00404090] (130 bytes)
	// Writes data to the stream buffer, advances write offset, and updates payload length in packet header
	int16_t Write(const void* pData, uint16_t wLength);

	// [RECONSTRUCTED - Native 0x00404C20] (68 bytes)
	// Reads data from the stream buffer and advances the read offset
	void* ReadBytes(void* pDest, uint16_t wLength);

	template<typename T>
	bool Read(T* pDest, uint16_t wLength = sizeof(T)) {
		return ReadBytes(pDest, wLength) != nullptr;
	}

	// [RECONSTRUCTED - Native 0x00404B60]
	// Reads 16-bit length-prefixed ASCII string from stream
	std::string ReadString();

	// [RECONSTRUCTED - Native 0x00432890] (38 bytes)
	// Returns remaining buffer capacity (m_dwCapacity - m_wWriteOffset)
	uint32_t GetRemainingCapacity() const;

	uint16_t GetOpcode() const { return (m_pwOpcode != nullptr) ? *m_pwOpcode : 0; }
	void     SetOpcode(uint16_t wOpcode) { if (m_pwOpcode != nullptr) *m_pwOpcode = wOpcode; }
	uint16_t GetRemainingBytes() const { return (m_wWriteOffset > m_wReadOffset) ? (m_wWriteOffset - m_wReadOffset) : 0; }

	// [RECONSTRUCTED - inlined at every consumer, e.g. 0x0040AA8B, 0x004ACC54, 0x004B2260]
	// Marks the whole payload as read: m_wReadOffset (+0x103C) = m_wWriteOffset (+0x103E).
	void     Consume() { m_wReadOffset = m_wWriteOffset; }

	// [RECONSTRUCTED - forward readers 0x00521750 / 0x0048BA70 / 0x00403F60 / 0x00404C20,
	//                  reverse readers 0x005217A0 / 0x00409910 / 0x005216C0,
	//                  selected on +0x1048 by the inlined callers, e.g. 0x00404E50]
	// Forward copies from +0x103C and advances it. Reverse pops from the write end (+0x103E)
	// and rewrites the header payload length. Both throw CMsgException on underflow; in the
	// game server that exception unwinds to CGameScheduler_Tick's catch(...) (0x00413BFB).
	void ReadRaw(void* pDest, uint16_t wLength);

	template<typename T>
	CMsg& operator>>(T& value) {
		ReadRaw(&value, static_cast<uint16_t>(sizeof(T)));
		return *this;
	}

	// [RECONSTRUCTED - 0x00404090 CMsgStreamBuffer_Write]
	template<typename T>
	CMsg& operator<<(const T& value) {
		Write(&value, static_cast<uint16_t>(sizeof(T)));
		return *this;
	}
	void     ResetReadCursors() { m_wReadOffset = 6; }
	bool     ReadUint8(uint8_t* pVal) { return Read(pVal); }
	bool     ReadUint16(uint16_t* pVal) { return Read(pVal); }
	bool     ReadUint32(uint32_t* pVal) { return Read(pVal); }
	bool     ReadFloat(float* pVal) { return Read(pVal); }
	void     WriteUint8(uint8_t val) { Write(&val, sizeof(val)); }
	void     WriteUint16(uint16_t val) { Write(&val, sizeof(val)); }
	void     WriteUint32(uint32_t val) { Write(&val, sizeof(val)); }
	void     WriteFloat(float val) { Write(&val, sizeof(val)); }

public:
	// +0x00: Node linkage (queue / intrusive pool list)
	void*         m_pPrevNode = nullptr;         // +0x00
	void*         m_pNextNode = nullptr;         // +0x04
	void*         m_pPoolOwner = nullptr;        // +0x08
	uint32_t      m_dwNodeReserved1 = 0;         // +0x0C
	uint32_t      m_dwNodeReserved2 = 0;         // +0x10
	uint32_t      m_dwState = 2;                 // +0x14 (initialized to 2 by 0x00471610)
	uint32_t      m_dwReserved18 = 0;            // +0x18
	uint32_t      m_dwReserved1C = 0;            // +0x1C
	uint32_t      m_dwFlag20 = 1;                // +0x20 (initialized to 1 by 0x00471610)
	uint32_t      m_dwReserved24 = 0;            // +0x24
	void*         m_pBufferBase = nullptr;       // +0x28 (points to buffer)
	uint8_t       m_pad2C[8] = { 0 };            // +0x2C - +0x34

	// +0x34: Inline buffer storage of 4096 bytes (0x1000)
	uint8_t       m_rawBuffer[4096];             // +0x34 - +0x1034

	// Stream buffer control members:
	uint8_t*      m_pData = nullptr;             // +0x1034: Base buffer data pointer (defaults to &m_rawBuffer[0])
	uint8_t*      m_pAllocatedData = nullptr;    // +0x1038: Dynamically allocated buffer if resized
	uint16_t      m_wReadOffset = 6;             // +0x103C: Current read offset (starts after 6-byte header)
	uint16_t      m_wWriteOffset = 6;            // +0x103E: Current write offset (starts after 6-byte header)
	uint32_t      m_dwFlags = 0;                 // +0x1040: Stream flags
	volatile long m_nRefCount = 0;               // +0x1044: Reference counter
	// CORRECTION: +0x1048 is not an auto-delete flag. Every stream reader (e.g. 0x00404E50,
	// 0x004ACC7B, 0x005AA492) takes the forward reader when it is non-zero and the reverse
	// (stack-style) reader when it is zero. 0x00471610 and 0x00471500 both set it to 1.
	uint32_t      m_bForwardRead = 1;            // +0x1048: 1 = read forward from +0x103C, 0 = pop from +0x103E
	uint32_t      m_dwCapacity = 4096;           // +0x104C: Max capacity (0x1000)
	uint16_t*     m_pwOpcode = nullptr;          // +0x1050: Pointer to 16-bit opcode in packet header (m_pData + 2)
	uint16_t*     m_pwPayloadLength = nullptr;   // +0x1054: Pointer to 16-bit payload length (m_pData + 0)
	uint16_t*     m_pwSecurityCount = nullptr;   // +0x1058: Pointer to 16-bit security field (m_pData + 4)
	uint8_t*      m_pPayload = nullptr;          // +0x105C: Pointer to payload data start (m_pData + 6)
	uint32_t      m_dwReserved60 = 0;            // +0x1060: Reserved
	uint32_t      m_dwReserved64 = 0;            // +0x1064: Reserved
	uint32_t      m_bInUse = 0;                  // +0x1068: In-use flag
	uint8_t       m_pad106C[16] = { 0 };         // +0x106C - +0x107C (total 0x107C bytes)
};

#pragma pack(pop)

// Forward compatibility aliases for codebase
typedef CMsg CMsgStreamBuffer;
typedef CMsg CStreamBuffer;

namespace BSLib {
	typedef ::CMsg CMsg;
	typedef ::CMsgStreamBuffer CMsgStreamBuffer;
	typedef ::CStreamBuffer CStreamBuffer;
}

#endif // _JMX_LIBRARY_BSLIB_MSG_H_
