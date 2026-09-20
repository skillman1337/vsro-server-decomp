/**
 * ============================================================================
 * Silkroad Online - Timed Job Database Instance Record Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceTimedJob.cpp
 *
 * Implements:
 *   - ITimedJobCommonData methods:
 *       ITimedJobCommonData_constructor       @ 0x0073C040
 *       ITimedJobCommonData_Clear             @ 0x00824380
 *   - CInstanceTimedJob methods:
 *       CInstanceTimedJob_constructor         @ 0x0073AC20
 *       CInstanceTimedJob_ScalarVectorDtor    @ 0x007E6850
 *       CInstanceTimedJob_GetTableName        @ 0x00850B30
 *       CInstanceTimedJob_GetTableDesc        @ 0x00826B40
 *       CInstanceTimedJob_GetColumnDesc       @ 0x00826B60
 *       CInstanceTimedJob_Clear               @ 0x00824090
 *       CInstanceTimedJob Setters (15 funcs)  @ 0x008240E0 - 0x00824360
 *       CInstanceTimedJob Getters (15 funcs)  @ 0x008237E0 - 0x00871BA0
 * ============================================================================
 */

#include "InstanceTimedJob.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstring>
#include <sstream>

bool CInstanceTimedJob::BuildDirtyUpdateQuery(std::string& output) {
	output.clear();
	if (!(g_bTimedJobDBWriteAllowed & 1)) return false;
	const uint32_t groups = m_dwStateFlags & 0xC;
	if (!groups) return true;
	std::ostringstream sql;
	sql << "UPDATE _TIMEDJOB SET ";
	if (groups & 4) sql << "TimeToKeep = " << int32_t(GetTimeToKeep());
	if (groups & 8) {
		if (groups & 4) sql << ", ";
		const uint32_t data[] = {GetData1(), GetData2(), GetData3(), GetData4(),
			GetData5(), GetData6(), GetData7(), GetData8()};
		for (unsigned i = 0; i < 8; ++i) {
			if (i) sql << ", ";
			sql << "Data" << i + 1 << " = " << int32_t(data[i]);
		}
		sql << ", Serial64 = " << GetSerial64() << ", JID = " << int32_t(GetJID());
	}
	sql << " WHERE ID = " << int32_t(GetID()) << " AND CharID = " << int32_t(GetCharID());
	output = sql.str();
	// Native 9742D0 consumes dirty groups when formatting, not at SQL completion.
	m_dwStateFlags = 0;
	return true;
}

// Global database write permission / policy flag (Native @ 0x00D219FC)
// Initialized to 5 at 0x00850BA6 in CInstanceTimedJob_InitTableBinding
uint32_t g_bTimedJobDBWriteAllowed = 5;

// ============================================================================
// ITimedJobCommonData Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x0073C040]
 * ITimedJobCommonData_constructor
 */
ITimedJobCommonData::ITimedJobCommonData()
	: CDBRecord() {
}

ITimedJobCommonData::~ITimedJobCommonData() = default;

/**
 * [RECONSTRUCTED - 0x0042EFB0]
 * Slot 4 (+0x10): CheckCondition
 */
bool ITimedJobCommonData::CheckCondition() const {
	return true;
}

/**
 * [RECONSTRUCTED - 0x0042EE50]
 * Slot 6 (+0x18): GetRecordOffset
 */
uint32_t ITimedJobCommonData::GetRecordOffset() const {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0042EDE0]
 * Slot 7 (+0x1C): BindColumn
 */
bool ITimedJobCommonData::BindColumn(void* /*pCol*/) {
	return true;
}

/**
 * [RECONSTRUCTED - 0x00824380]
 * Slot 5 (+0x14): Clear
 *
 * Machine Disassembly (0x00824380):
 *   00824380  push  esi
 *   00824381  mov   esi, ecx
 *   00824383  mov   eax, [esi]
 *   00824385  mov   edx, [eax+0x24] ; slot 9: SetID
 *   00824388  push  0
 *   0082438a  call  edx
 *   ... [calls slots 10-21 with 0]
 *   00824410  mov   edx, [eax+0x58] ; slot 22: SetSerial64(0, 0)
 *   00824415  push  0; push 0; call edx
 *   0082441d  mov   edx, [eax+0x5c] ; slot 23: SetJID(0)
 *   00824422  push  0; call edx
 *   00824428  mov   dword [esi+0x8], 0 ; clears dirty flags
 *   0082442f  mov   dword [esi+0xc], 0 ; clears owner/sequence
 *   00824436  pop   esi
 *   00824437  retn
 */
int32_t ITimedJobCommonData::Clear() {
	SetID(0);
	SetCharID(0);
	SetCategory(0);
	SetJobID(0);
	SetTimeToKeep(0);
	SetData1(0);
	SetData2(0);
	SetData3(0);
	SetData4(0);
	SetData5(0);
	SetData6(0);
	SetData7(0);
	SetData8(0);
	SetSerial64(0, 0);
	int32_t result = SetJID(0);

	m_dwStateFlags = 0; // +0x08
	m_pOwnerTable  = nullptr; // +0x0C

	return result;
}

// ============================================================================
// CInstanceTimedJob Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x0073AC20]
 * CInstanceTimedJob_constructor
 */
CInstanceTimedJob::CInstanceTimedJob()
	: ITimedJobCommonData()
	, m_dwID(0)
	, m_dwCharID(0)
	, m_byCategory(0)
	, m_dwJobID(0)
	, m_dwTimeToKeep(0)
	, m_dwData1(0)
	, m_dwData2(0)
	, m_dwData3(0)
	, m_dwData4(0)
	, m_dwData5(0)
	, m_dwData6(0)
	, m_dwData7(0)
	, m_dwData8(0)
	, m_nSerial64(0)
	, m_dwJID(0) {
	std::memset(m_pad14, 0, sizeof(m_pad14));
	std::memset(m_pad21, 0, sizeof(m_pad21));
	std::memset(m_pad4C, 0, sizeof(m_pad4C));
	std::memset(m_pad5C, 0, sizeof(m_pad5C));
}

/**
 * [RECONSTRUCTED - 0x007E6850]
 * CInstanceTimedJob_ScalarVectorDtor
 */
CInstanceTimedJob::~CInstanceTimedJob() = default;

/**
 * [RECONSTRUCTED - 0x00850B30]
 * Slot 0 (+0x00): GetTableName
 */
const char* CInstanceTimedJob::GetTableName() const {
	return "CInstanceTimedJob";
}

/**
 * [RECONSTRUCTED - 0x00826B40]
 * Slot 2 (+0x08): GetTableDesc
 */
void* CInstanceTimedJob::GetTableDesc() const {
	static const char szTable[] = "_TIMEDJOB";
	return const_cast<char*>(szTable);
}

/**
 * [RECONSTRUCTED - 0x00826B60]
 * Slot 3 (+0x0C): GetColumnDesc
 */
void* CInstanceTimedJob::GetColumnDesc() const {
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00824090]
 * Slot 5 (+0x14): Clear
 */
int32_t CInstanceTimedJob::Clear() {
	return ITimedJobCommonData::Clear();
}

// ----------------------------------------------------------------------------
// 15 Virtual Setters with Write Policy & Dirty Bitmask Updates
// ----------------------------------------------------------------------------

/**
 * [RECONSTRUCTED - 0x00824360]
 * Slot 9 (+0x24): SetID
 */
int32_t CInstanceTimedJob::SetID(uint32_t dwID) {
	if (dwID == m_dwID) {
		return static_cast<int32_t>(dwID);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwID = dwID;
	return static_cast<int32_t>(dwID);
}

/**
 * [RECONSTRUCTED - 0x00824340]
 * Slot 10 (+0x28): SetCharID
 */
int32_t CInstanceTimedJob::SetCharID(uint32_t dwCharID) {
	if (dwCharID == m_dwCharID) {
		return static_cast<int32_t>(dwCharID);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwCharID = dwCharID;
	return static_cast<int32_t>(dwCharID);
}

/**
 * [RECONSTRUCTED - 0x00824320]
 * Slot 11 (+0x2C): SetCategory
 */
int32_t CInstanceTimedJob::SetCategory(uint8_t byCategory) {
	if (byCategory == m_byCategory) {
		return static_cast<int32_t>(byCategory);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_byCategory = byCategory;
	return static_cast<int32_t>(byCategory);
}

/**
 * [RECONSTRUCTED - 0x00824300]
 * Slot 12 (+0x30): SetJobID
 */
int32_t CInstanceTimedJob::SetJobID(uint32_t dwJobID) {
	if (dwJobID == m_dwJobID) {
		return static_cast<int32_t>(dwJobID);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwJobID = dwJobID;
	return static_cast<int32_t>(dwJobID);
}

/**
 * [RECONSTRUCTED - 0x008242D0]
 * Slot 13 (+0x34): SetTimeToKeep
 * Dirty Bit: 0x04
 */
int32_t CInstanceTimedJob::SetTimeToKeep(uint32_t dwTimeToKeep) {
	if (dwTimeToKeep == m_dwTimeToKeep) {
		return static_cast<int32_t>(dwTimeToKeep);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x04; // +0x08
	m_dwTimeToKeep = dwTimeToKeep; // +0x28
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x008242A0]
 * Slot 14 (+0x38): SetData1
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData1(uint32_t dwData1) {
	if (dwData1 == m_dwData1) {
		return static_cast<int32_t>(dwData1);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData1 = dwData1;     // +0x2C
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824270]
 * Slot 15 (+0x3C): SetData2
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData2(uint32_t dwData2) {
	if (dwData2 == m_dwData2) {
		return static_cast<int32_t>(dwData2);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData2 = dwData2;     // +0x30
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824240]
 * Slot 16 (+0x40): SetData3
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData3(uint32_t dwData3) {
	if (dwData3 == m_dwData3) {
		return static_cast<int32_t>(dwData3);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData3 = dwData3;     // +0x34
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824210]
 * Slot 17 (+0x44): SetData4
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData4(uint32_t dwData4) {
	if (dwData4 == m_dwData4) {
		return static_cast<int32_t>(dwData4);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData4 = dwData4;     // +0x38
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x008241E0]
 * Slot 18 (+0x48): SetData5
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData5(uint32_t dwData5) {
	if (dwData5 == m_dwData5) {
		return static_cast<int32_t>(dwData5);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData5 = dwData5;     // +0x3C
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x008241B0]
 * Slot 19 (+0x4C): SetData6
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData6(uint32_t dwData6) {
	if (dwData6 == m_dwData6) {
		return static_cast<int32_t>(dwData6);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData6 = dwData6;     // +0x40
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824180]
 * Slot 20 (+0x50): SetData7
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData7(uint32_t dwData7) {
	if (dwData7 == m_dwData7) {
		return static_cast<int32_t>(dwData7);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData7 = dwData7;     // +0x44
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824150]
 * Slot 21 (+0x54): SetData8
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetData8(uint32_t dwData8) {
	if (dwData8 == m_dwData8) {
		return static_cast<int32_t>(dwData8);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwData8 = dwData8;     // +0x48
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x00824110]
 * Slot 22 (+0x58): SetSerial64
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetSerial64(uint32_t dwLow, uint32_t dwHigh) {
	if (dwLow == m_dwSerialLow && dwHigh == m_dwSerialHigh) {
		return static_cast<int32_t>(dwLow);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwSerialLow  = dwLow;  // +0x50
	m_dwSerialHigh = dwHigh; // +0x54
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

/**
 * [RECONSTRUCTED - 0x008240E0]
 * Slot 23 (+0x5C): SetJID
 * Dirty Bit: 0x08
 */
int32_t CInstanceTimedJob::SetJID(uint32_t dwJID) {
	if (dwJID == m_dwJID) {
		return static_cast<int32_t>(dwJID);
	}
	if ((g_bTimedJobDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}
	m_dwStateFlags |= 0x08; // +0x08
	m_dwJID = dwJID;         // +0x58
	return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_pOwnerTable));
}

// ----------------------------------------------------------------------------
// 15 Virtual Getters
// ----------------------------------------------------------------------------

/**
 * [RECONSTRUCTED - 0x00823DE0]
 * Slot 24 (+0x60): GetID
 */
uint32_t CInstanceTimedJob::GetID() const {
	return m_dwID; // +0x18
}

/**
 * [RECONSTRUCTED - 0x00823DD0]
 * Slot 25 (+0x64): GetCharID
 */
uint32_t CInstanceTimedJob::GetCharID() const {
	return m_dwCharID; // +0x1C
}

/**
 * [RECONSTRUCTED - 0x00823DC0]
 * Slot 26 (+0x68): GetCategory
 */
uint8_t CInstanceTimedJob::GetCategory() const {
	return m_byCategory; // +0x20
}

/**
 * [RECONSTRUCTED - 0x00871BA0]
 * Slot 27 (+0x6C): GetJobID
 */
uint32_t CInstanceTimedJob::GetJobID() const {
	return m_dwJobID; // +0x24
}

/**
 * [RECONSTRUCTED - 0x00823DB0]
 * Slot 28 (+0x70): GetTimeToKeep
 */
uint32_t CInstanceTimedJob::GetTimeToKeep() const {
	return m_dwTimeToKeep; // +0x28
}

/**
 * [RECONSTRUCTED - 0x008240D0]
 * Slot 29 (+0x74): GetData1
 */
uint32_t CInstanceTimedJob::GetData1() const {
	return m_dwData1; // +0x2C
}

/**
 * [RECONSTRUCTED - 0x008237E0]
 * Slot 30 (+0x78): GetData2
 */
uint32_t CInstanceTimedJob::GetData2() const {
	return m_dwData2; // +0x30
}

/**
 * [RECONSTRUCTED - 0x008240C0]
 * Slot 31 (+0x7C): GetData3
 */
uint32_t CInstanceTimedJob::GetData3() const {
	return m_dwData3; // +0x34
}

/**
 * [RECONSTRUCTED - 0x008240B0]
 * Slot 32 (+0x80): GetData4
 */
uint32_t CInstanceTimedJob::GetData4() const {
	return m_dwData4; // +0x38
}

/**
 * [RECONSTRUCTED - 0x00823DA0]
 * Slot 33 (+0x84): GetData5
 */
uint32_t CInstanceTimedJob::GetData5() const {
	return m_dwData5; // +0x3C
}

/**
 * [RECONSTRUCTED - 0x00823D90]
 * Slot 34 (+0x88): GetData6
 */
uint32_t CInstanceTimedJob::GetData6() const {
	return m_dwData6; // +0x40
}

/**
 * [RECONSTRUCTED - 0x00561060]
 * Slot 35 (+0x8C): GetData7
 */
uint32_t CInstanceTimedJob::GetData7() const {
	return m_dwData7; // +0x44
}

/**
 * [RECONSTRUCTED - 0x00823D80]
 * Slot 36 (+0x90): GetData8
 */
uint32_t CInstanceTimedJob::GetData8() const {
	return m_dwData8; // +0x48
}

/**
 * [RECONSTRUCTED - 0x008240A0]
 * Slot 37 (+0x94): GetSerial64
 */
int64_t CInstanceTimedJob::GetSerial64() const {
	return m_nSerial64; // +0x50 - +0x57
}

/**
 * [RECONSTRUCTED - 0x00823D70]
 * Slot 38 (+0x98): GetJID
 */
uint32_t CInstanceTimedJob::GetJID() const {
	return m_dwJID; // +0x58
}
