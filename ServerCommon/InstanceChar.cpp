/**
 * ============================================================================
 * Silkroad Online - Character and Entity Database Instance Records
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceChar.cpp
 *
 * Implements:
 *   - CDBRecord lifecycle and virtual methods
 *   - CInstanceObj lifecycle
 *   - CInstanceChar lifecycle and virtual methods
 *   - CInstancePC lifecycle and virtual methods
 *   - CInstanceCOS lifecycle and virtual methods
 * ============================================================================
 */

#include "InstanceChar.h"
#include "InstanceItem.h"
#include "ReferenceData.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"

uint32_t g_bItemDBWriteAllowed = 1;

// ============================================================================
// CDBRecord Implementation
// ============================================================================

CDBRecord::CDBRecord()
	: m_dwRecordID(0)
	, m_dwStateFlags(0)
	, m_pOwnerTable(nullptr)
	, m_dwReserved(0) {
}

CDBRecord::~CDBRecord() = default;

const char* CDBRecord::GetTableName() const {
	return "CDBRecord";
}

void* CDBRecord::GetTableDesc() const {
	return nullptr;
}

bool CDBRecord::CheckCondition() const {
	return true;
}

bool CDBRecord::Serialize(void* pStream) {
	(void)pStream;
	return true;
}

uint32_t CDBRecord::GetRecordOffset() const {
	return 0;
}

bool CDBRecord::BindColumn(void* pCol) {
	(void)pCol;
	return true;
}

// ============================================================================
// CInstanceObj Implementation
// ============================================================================

CInstanceObj::CInstanceObj()
	: m_pRefObjCommon(nullptr) {
}

CInstanceObj::~CInstanceObj() = default;

// ============================================================================
// CInstanceChar Implementation
// ============================================================================

CInstanceChar::CInstanceChar() = default;

CInstanceChar::~CInstanceChar() = default;

/**
 * [NATIVE - 0x009BF500]
 * Slot 8 (+0x20): UniversalNoOpStub_1Arg
 */
void CInstanceChar::NoOpStub() {
}

/**
 * [RECONSTRUCTED - 0x0042E850]
 * Slot 9 (+0x24): ResolveRefObjCommon
 *
 * Verifies or initializes m_pRefObjCommon from g_pRefData using m_dwRecordID.
 */
bool CInstanceChar::ResolveRefObjCommon() {
	if (m_pRefObjCommon != nullptr) {
		return true;
	}
	if (!g_pRefData) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x008412B0]
 * Slot 10 (+0x28): SerializeStats
 */
bool CInstanceChar::SerializeStats(void* pStream, int mode) {
	(void)pStream;
	(void)mode;
	return true;
}

/**
 * Slot 12 (+0x30): GetHwanLevel
 */
uint8_t CInstanceChar::GetHwanLevel() const {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00449340]
 * Slot 13 (+0x34): GetExp
 * Returns reward experience from reference common object (+0x250).
 */
uint32_t CInstanceChar::GetExp() const {
	if (m_pRefObjCommon != nullptr) {
		return m_pRefObjCommon->m_dwRewardExp;
	}
	return 0;
}

/**
 * Slot 14 (+0x38): GetCurrentHP
 */
uint32_t CInstanceChar::GetCurrentHP() const {
	return 0;
}

/**
 * Slot 15 (+0x3C): GetCurrentMP
 */
uint32_t CInstanceChar::GetCurrentMP() const {
	return 0;
}

// ============================================================================
// CInstancePC Implementation
// ============================================================================

CInstancePC::CInstancePC()
	: m_byHwanLevel(0)
	, m_dwHP(0)
	, m_dwMP(0) {
}

uint32_t g_bPCDBWriteAllowed = 1;

CInstancePC::~CInstancePC() = default;

/**
 * [NATIVE - 0x00827080]
 * Slot 2 (+0x08): GetTableName
 */
const char* CInstancePC::GetTableName() const {
	return "CInstancePC";
}

/**
 * [NATIVE - 0x00825E40]
 * Slot 11 (+0x2C): GetCharName
 */
const char* CInstancePC::GetCharName() const {
	return m_strCharName.c_str();
}

/**
 * [NATIVE - 0x00825E30]
 * Slot 12 (+0x30): GetHwanLevel
 */
uint8_t CInstancePC::GetHwanLevel() const {
	return m_byHwanLevel;
}

/**
 * [NATIVE - 0x00601C80]
 * Slot 14 (+0x38): GetCurrentHP
 */
uint32_t CInstancePC::GetCurrentHP() const {
	return m_dwHP;
}

/**
 * [NATIVE - 0x00825E20]
 * Slot 15 (+0x3C): GetCurrentMP
 */
uint32_t CInstancePC::GetCurrentMP() const {
	return m_dwMP;
}

// ============================================================================
// CInstanceCOS Implementation
// ============================================================================

CInstanceCOS::CInstanceCOS() = default;

CInstanceCOS::~CInstanceCOS() = default;

/**
 * Slot 2 (+0x08): GetTableName
 */
const char* CInstanceCOS::GetTableName() const {
	return "CInstanceCOS";
}

/**
 * [RECONSTRUCTED - 0x00449350]
 * Slot 11 (+0x2C): GetCharName
 */
const char* CInstanceCOS::GetCharName() const {
	if (!m_strCustomCOSName.empty()) {
		return m_strCustomCOSName.c_str();
	}
	if (m_pRefObjCommon != nullptr) {
		return m_pRefObjCommon->m_strCodeName.c_str();
	}
	return "Pet";
}
