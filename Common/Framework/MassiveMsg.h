/**
 * ============================================================================
 * Silkroad Online - Massive Message Stream Wrapper
 * Original Source: D:\WORK2005\Source\Common\Framework\MassiveMsg.h
 *
 * Provides segmented and continuous stream buffering for massive packet payloads,
 * inter-server cluster topology synchronizations, and certification messages.
 *   - Native RTTI Descriptor @ 0x00B76A4C: ServerFramework::CMassiveMsg
 *   - Native vftable         @ 0x00B412CC
 *   - Native Methods:
 *       ServerFramework_CMassiveMsg_Allocate          @ 0x00956060
 *       ServerFramework_CMassiveMsg_Release           @ 0x00956080
 *       ServerFramework_CMassiveMsg_GetCounts         @ 0x009560A0
 *       ServerFramework_CMassiveMsg_WriteBytes        @ 0x009560B0
 *       CMassiveMsg_ReadBytes                         @ 0x00956160
 *       ServerFramework_CMassiveMsg_SendToSession     @ 0x009562A0
 *       ServerFramework_CMassiveMsg_AllocateNextChunk @ 0x009563D0
 *       ServerFramework_CMassiveMsg_SetOpcode         @ 0x00956430
 *       ServerFramework_CMassiveMsg_Flush             @ 0x00956440
 *       ServerFramework_CMassiveMsg_SetChunkHeader    @ 0x009564E0
 *       ServerFramework_CMassiveMsg_AddBufferRef      @ 0x00956510
 *       ServerFramework_CMassiveMsg_GetTotalPayloadSize @ 0x00956580
 *       ServerFramework_CMassiveMsg_Serialize         @ 0x009565E0
 *       CMsgStreamBuffer_Write                        @ 0x00404090
 *       CMsgStreamBuffer_ReadBytes                    @ 0x00404C20
 *       CMsgStreamBuffer_ReadString                   @ 0x00404B60
 *       CMsgStreamBuffer_GetRemainingCapacity         @ 0x00432890
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_MASSIVEMSG_H_
#define _JMX_SERVERFRAMEWORK_MASSIVEMSG_H_

#include "Synch.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include "Msg.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ServerFramework {

/**
 * [RECONSTRUCTED - Native 0x00B412CC / RTTI: 0x00B76A4C]
 * CMassiveMsg
 * Native Massive Message Stream Fragmenter / Reassembler
 * Size: 44 bytes (0x2C)
 */
class CMassiveMsg {
public:
	CMassiveMsg();
	virtual ~CMassiveMsg();

	// [RECONSTRUCTED - Native 0x00956060] (25 bytes)
	// Allocates or retrieves an instance from the CMassiveMsg pool
	static CMassiveMsg* Allocate(int32_t nMode = 1);

	// [RECONSTRUCTED - Native 0x00956080] (25 bytes)
	// Flushes and releases the instance back to the CMassiveMsg pool
	int32_t Release();

	// [RECONSTRUCTED - Native 0x009560A0] (9 bytes)
	// Queries current read chunk index and total chunk count
	void GetCounts(int32_t* pCurrent, int32_t* pTotal);

	// [RECONSTRUCTED - Native 0x009560B0] (169 bytes)
	// Appends arbitrary payload bytes to the stream, splitting across chunk boundaries if necessary
	void* WriteBytes(const void* pData, uint32_t nBytes);

	// [RECONSTRUCTED - Native 0x00956160] (309 bytes)
	// Reads requested bytes across segmented or continuous stream chunks into pDest
	int32_t ReadBytes(void* pDest, size_t nBytes);

	// [RECONSTRUCTED - Native 0x00404B60]
	// Reads 16-bit length-prefixed ASCII string from stream
	std::string ReadString();

	// [RECONSTRUCTED - Native 0x009562A0] (294 bytes)
	// Transmits the massive message sequence (Header packet + Chunks) to the target session socket
	int32_t SendToSession(void* pSession, int32_t bKeepAlive = 0);

	// [RECONSTRUCTED - Native 0x009563D0] (83 bytes)
	// Allocates the next stream buffer chunk from g_pNetEngine, sets opcode 0x600D, and writes the chunk flag
	void* AllocateNextChunk();

	// [RECONSTRUCTED - Native 0x00956430] (5 bytes)
	// Sets the inner opcode of the massive message
	void SetOpcode(uint16_t wOpcode);

	// [RECONSTRUCTED - Native 0x00956440] (152 bytes)
	// Releases all stream buffers in m_MsgList back to g_pNetEngine
	void Flush();

	// [RECONSTRUCTED - Native 0x009564E0] (36 bytes)
	// Sets opcode and total chunk count before any body buffers are allocated
	void SetChunkHeader(uint16_t wInnerOpcode, uint16_t wTotalChunks);

	// [RECONSTRUCTED - Native 0x00956510] (103 bytes)
	// Appends incoming body buffer reference and decrements remaining expected bodies
	bool AddBufferRef(CMsgStreamBuffer* pBuffer);

	// [RECONSTRUCTED - Native 0x00956580] (85 bytes)
	// Computes total payload size across all stored chunks
	size_t GetTotalPayloadSize() const;

	// [RECONSTRUCTED - Native 0x009565E0] (228 bytes)
	// Serializes accumulated message chunks into destination stream
	bool Serialize(void* pDestStream);

	// Compatibility accessors
	void SetBuffer(const uint8_t* pData, size_t nLength);
	void ResetRead();
	bool HasMoreData() const;
	uint16_t GetInnerOpcode() const;
	uint16_t GetExpectedBodies() const;

public:
	// Exact struct layout matching native binary:
	// +0x00: vfptr (0x00B412CC)
	// +0x04: std::vector<CMsgStreamBuffer*> m_MsgList (12 bytes: _Myfirst, _Mylast, _Myend)
	std::vector<CMsgStreamBuffer*> m_MsgList;
	// +0x10: uint32_t m_dwReadBufferIndex
	uint32_t                       m_dwReadBufferIndex = 0;
	// +0x14: uint32_t m_nCurrentReadMsg
	uint32_t                       m_nCurrentReadMsg   = 0;
	// +0x18: uint32_t m_nMode (0 = Read, 1 = Write)
	uint32_t                       m_nMode             = 0;
	// +0x1C: uint8_t m_bActive (1 = active/in-use, 0 = inactive)
	uint8_t                        m_bActive           = 1;
	// +0x1D: uint8_t m_byChunkIndex (chunk index / encryption flag)
	uint8_t                        m_byChunkIndex      = 0;
	// +0x1E: uint16_t m_wOpcode
	uint16_t                       m_wOpcode           = 0;
	// +0x20: uint16_t m_wTotalChunks
	uint16_t                       m_wTotalChunks      = 0;
	// +0x22: uint16_t m_wReserved22
	uint16_t                       m_wReserved22       = 0;
	// +0x24: uint32_t m_dwTotalBytes
	uint32_t                       m_dwTotalBytes      = 0;
	// +0x28: CMsgStreamBuffer* m_pCurMsg
	CMsgStreamBuffer*              m_pCurMsg           = nullptr;

private:
	// Linear view fallback for continuous memory buffers
	const uint8_t*                 m_pReadPtr          = nullptr;
	size_t                         m_nRemainingBytes   = 0;
};

// Native globals matching 0x00D67AA0, 0x00D67AA4, 0x00D67AC4
extern CCriticalSectionBS g_csMassiveMsgPool;
extern uint32_t           g_dwFreeMassiveMsgs;
extern uint32_t           g_dwTotalMassiveMsgs;

// [RECONSTRUCTED - Native 0x00956990] (186 bytes)
void ServerFramework_CMassiveMsgPool_GetCounts(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable);

// [RECONSTRUCTED - Native 0x009560A0] (9 bytes)
void ServerFramework_CMassiveMsg_GetCounts(uint32_t* pTotal, uint32_t* pActive, uint32_t* pAvailable);
void ServerFramework_CMassiveMsg_GetCounts(uint32_t* pActive, uint32_t* pAvailable);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_MASSIVEMSG_H_
