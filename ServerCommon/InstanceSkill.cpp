/**
 * ============================================================================
 * Silkroad Online - Skill & Mastery Database Instance Records Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceSkill.cpp
 *
 * Implements:
 *   - CInstanceSkill methods:
 *       CInstanceSkill_GetTableName   @ 0x008465A0
 *       CInstanceSkill_GetTableDesc   @ 0x00826F30
 *       CInstanceSkill_GetColumnDesc  @ 0x00826F50
 *       CInstanceSkill_Clear          @ 0x00824F50
 *   - CInstanceSkillMastery methods:
 *       CInstanceSkillMastery_GetTableName  @ 0x00846B00
 *       CInstanceSkillMastery_GetTableDesc  @ 0x00826F00
 *       CInstanceSkillMastery_GetColumnDesc @ 0x00826F20
 *       CInstanceSkillMastery_Clear         @ 0x00824EE0
 * ============================================================================
 */

#include "InstanceSkill.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstring>

// Global database write permission flags
uint32_t g_bSkillDBWriteAllowed = 5;        // Native @ 0x00D20544
uint32_t g_bSkillMasteryDBWriteAllowed = 5; // Native @ 0x00D2064C

// ============================================================================
// CInstanceSkill Implementation
// ============================================================================

CInstanceSkill::CInstanceSkill()
	: CDBRecord()
	, m_dwCharID(0)
	, m_dwSkillID(0)
	, m_byEnable(0) {
	std::memset(m_pad14, 0, sizeof(m_pad14));
	std::memset(m_pad21, 0, sizeof(m_pad21));
}

CInstanceSkill::~CInstanceSkill() = default;

const char* CInstanceSkill::GetTableName() const {
	return "CInstanceSkill";
}

void* CInstanceSkill::GetTableDesc() const {
	static const char szTable[] = "_CharSkill";
	return const_cast<char*>(szTable);
}

void* CInstanceSkill::GetColumnDesc() const {
	return nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x00824F50]
 * CInstanceSkill::Clear
 */
void CInstanceSkill::Clear() {
	if (m_dwCharID != 0) {
		if ((g_bSkillDBWriteAllowed & 1) == 0) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			m_dwCharID = 0;
		}
	}

	if (m_dwSkillID != 0) {
		if ((g_bSkillDBWriteAllowed & 1) == 0) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			m_dwSkillID = 0;
		}
	}

	if (m_byEnable != 0) {
		if ((g_bSkillDBWriteAllowed & 1) != 0) {
			m_dwStateFlags |= 0x04;
			m_byEnable = 0;
			m_dwStateFlags = 0;
			m_pOwnerTable = nullptr;
			return;
		}
		ServerFramework::ServerFramework_GenerateMiniDump();
	}

	m_dwStateFlags = 0;
	m_pOwnerTable = nullptr;
}

void CInstanceSkill::SetCharID(uint32_t dwCharID) {
	if (dwCharID == m_dwCharID) {
		return;
	}
	if ((g_bSkillDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwCharID = dwCharID;
}

void CInstanceSkill::SetSkillID(uint32_t dwSkillID) {
	if (dwSkillID == m_dwSkillID) {
		return;
	}
	if ((g_bSkillDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwSkillID = dwSkillID;
}

void CInstanceSkill::SetEnable(uint8_t byEnable) {
	if (byEnable == m_byEnable) {
		return;
	}
	if ((g_bSkillDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwStateFlags |= 0x04;
	m_byEnable = byEnable;
}

// ============================================================================
// CInstanceSkillMastery Implementation
// ============================================================================

CInstanceSkillMastery::CInstanceSkillMastery()
	: CDBRecord()
	, m_dwCharID(0)
	, m_dwMasteryID(0)
	, m_byLevel(0) {
	std::memset(m_pad14, 0, sizeof(m_pad14));
	std::memset(m_pad21, 0, sizeof(m_pad21));
}

CInstanceSkillMastery::~CInstanceSkillMastery() = default;

const char* CInstanceSkillMastery::GetTableName() const {
	return "CInstanceSkillMastery";
}

void* CInstanceSkillMastery::GetTableDesc() const {
	static const char szTable[] = "_CharSkillMastery";
	return const_cast<char*>(szTable);
}

void* CInstanceSkillMastery::GetColumnDesc() const {
	return nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x00824EE0]
 * CInstanceSkillMastery::Clear
 */
void CInstanceSkillMastery::Clear() {
	if (m_dwCharID != 0) {
		if ((g_bSkillMasteryDBWriteAllowed & 1) == 0) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			m_dwCharID = 0;
		}
	}

	if (m_dwMasteryID != 0) {
		if ((g_bSkillMasteryDBWriteAllowed & 1) == 0) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			m_dwMasteryID = 0;
		}
	}

	if (m_byLevel != 0) {
		if ((g_bSkillMasteryDBWriteAllowed & 1) != 0) {
			m_dwStateFlags |= 0x04;
			m_byLevel = 0;
			m_dwStateFlags = 0;
			m_pOwnerTable = nullptr;
			return;
		}
		ServerFramework::ServerFramework_GenerateMiniDump();
	}

	m_dwStateFlags = 0;
	m_pOwnerTable = nullptr;
}

void CInstanceSkillMastery::SetCharID(uint32_t dwCharID) {
	if (dwCharID == m_dwCharID) {
		return;
	}
	if ((g_bSkillMasteryDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwCharID = dwCharID;
}

void CInstanceSkillMastery::SetMasteryID(uint32_t dwMasteryID) {
	if (dwMasteryID == m_dwMasteryID) {
		return;
	}
	if ((g_bSkillMasteryDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwMasteryID = dwMasteryID;
}

void CInstanceSkillMastery::SetLevel(uint8_t byLevel) {
	if (byLevel == m_byLevel) {
		return;
	}
	if ((g_bSkillMasteryDBWriteAllowed & 1) == 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_dwStateFlags |= 0x04;
	m_byLevel = byLevel;
}
