/**
 * ============================================================================
 * Silkroad Online - Reference World Instance & Teleport Registry
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\RefInstanceGenerator.cpp
 *
 * Implements:
 *   - tagRefTeleport lifecycle
 *   - CRefInstanceWorldRegion (Table 0x3E)
 *   - CRefInstanceWorldStartPos (Table 0x3F)
 *   - CReferenceData::BuildTeleportCodeNameIndex @ 0x006BEF30
 *   - CReferenceData::GetTeleportByCodeName @ 0x006BF080
 *   - CReferenceData::BuildSkillCodeNameIndex @ 0x006F68F0
 * ============================================================================
 */

#include "RefInstanceGenerator.h"
#include "ReferenceData.h"
#include "../SR_GameServer/Formulae.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstring>

// ============================================================================
// tagRefTeleport
// ============================================================================

tagRefTeleport::tagRefTeleport()
	: m_dwID(0)
	, m_strCodeName()
	, m_wRegionID(0)
	, m_sPosX(0)
	, m_sPosY(0)
	, m_sPosZ(0)
	, m_pRefObjCommon(nullptr)
	, m_byType(0)
	, m_wGateNo(0) {
	std::memset(m_pad35, 0, sizeof(m_pad35));
}

tagRefTeleport::~tagRefTeleport() {
	m_pRefObjCommon = nullptr;
}

// ============================================================================
// CRefInstanceWorldRegion (Native VTable @ 0x00B0C750 / Size 0x20)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00735470]
 * CRefInstanceWorldRegion Constructor
 */
CRefInstanceWorldRegion::CRefInstanceWorldRegion()
	: CDBRecord()
	, m_dwWorldID(1)
	, m_wRegionID(0) {
}

/**
 * [RECONSTRUCTED - 0x007354D0 / 0x007E4210]
 * CRefInstanceWorldRegion Destructor
 */
CRefInstanceWorldRegion::~CRefInstanceWorldRegion() {
}

/**
 * [RECONSTRUCTED - 0x00827DD0]
 * CRefInstanceWorldRegion::GetTableName
 */
const char* CRefInstanceWorldRegion::GetTableName() const {
	return "_RefInstanceWorldRegion";
}

// ============================================================================
// CRefInstanceWorldStartPos (Native VTable @ 0x00B0C778 / Size 0x30)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00735520]
 * CRefInstanceWorldStartPos Constructor
 */
CRefInstanceWorldStartPos::CRefInstanceWorldStartPos()
	: CDBRecord()
	, m_dwWorldID(1)
	, m_wRegionID(0)
	, m_fPosX(0.0f)
	, m_fPosY(0.0f)
	, m_fPosZ(0.0f)
	, m_fRadius(10.0f) {
}

/**
 * [RECONSTRUCTED - 0x00735580 / 0x007E4270]
 * CRefInstanceWorldStartPos Destructor
 */
CRefInstanceWorldStartPos::~CRefInstanceWorldStartPos() {
}

/**
 * [RECONSTRUCTED - 0x00827DA0]
 * CRefInstanceWorldStartPos::GetTableName
 */
const char* CRefInstanceWorldStartPos::GetTableName() const {
	return "_RefInstanceWorldStartPos";
}

// ============================================================================
// CReferenceData - Teleport & Skill CodeName Indices
// ============================================================================

/**
 * [RECONSTRUCTED - 0x006BEF30]
 * CReferenceData::BuildTeleportCodeNameIndex
 *
 * Native source line 2526 (0x9DE):
 *   Assertion: "m_RefTeleportByCodeName.end() == m_RefTeleportByCodeName.find(pRefTeleport->m_strCodeName)"
 *
 * Iterates the loaded _RefTeleport list (this + 0xBC) and builds the code-name
 * lookup map (this + 0xC4) ensuring no duplicate building keys exist.
 */
bool CReferenceData::BuildTeleportCodeNameIndex() {
	m_RefTeleportByCodeName.clear();

	for (tagRefTeleport* pRefTeleport : m_vecRefTeleport) {
		if (pRefTeleport == nullptr) {
			continue;
		}

		auto it = m_RefTeleportByCodeName.find(pRefTeleport->m_strCodeName);
		if (it != m_RefTeleportByCodeName.end()) {
			// Native 0x006BEFED: AssertReport at line 2526 (0x9DE)
			if (!BSLib::AssertReport(
					2526,
					"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\ServerCommon\\RefInstanceGenerator.cpp",
					"m_RefTeleportByCodeName.end() == m_RefTeleportByCodeName.find(pRefTeleport->m_strCodeName)")) {
				ServerFramework::ServerFramework_GenerateMiniDump();
			}
			continue;
		}

		m_RefTeleportByCodeName[pRefTeleport->m_strCodeName] = pRefTeleport;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x006BF080]
 * CReferenceData::GetTeleportByCodeName
 *
 * Fast O(log N) lookup in m_RefTeleportByCodeName (this + 0xC4).
 * Returns nullptr if building code name is not registered.
 */
tagRefTeleport* CReferenceData::GetTeleportByCodeName(const std::string& strCodeName) {
	auto it = m_RefTeleportByCodeName.find(strCodeName);
	if (it == m_RefTeleportByCodeName.end()) {
		return nullptr;
	}
	return it->second;
}

/**
 * [RECONSTRUCTED - 0x006F68F0]
 * CReferenceData::BuildSkillCodeNameIndex
 *
 * Native source line 2383 (0x94F) in RefInstanceGenerator.h:
 *   Assertion: "m_RefSkillByCodename.end() == m_RefSkillByCodename.find(pRefSkill->m_Basic_Code.c_str())"
 *
 * Iterates m_mapRefSkill (this + 0x888) and populates m_RefSkillByCodename (this + 0x894).
 */
bool CReferenceData::BuildSkillCodeNameIndex() {
	m_RefSkillByCodename.clear();

	for (const auto& pair : m_mapRefSkill) {
		tagRefSkill* pRefSkill = reinterpret_cast<tagRefSkill*>(pair.second);
		if (pRefSkill == nullptr) {
			continue;
		}

		auto it = m_RefSkillByCodename.find(pRefSkill->m_Basic_Code);
		if (it != m_RefSkillByCodename.end()) {
			// Native 0x006F6A36: AssertReport at line 2383 (0x94F)
			if (!BSLib::AssertReport(
					2383,
					"d:\\work2005\\source\\silkroadonline\\server\\sr_gameserver\\../ServerCommon/RefInstanceGenerator.h",
					"m_RefSkillByCodename.end() == m_RefSkillByCodename.find(pRefSkill->m_Basic_Code.c_str())")) {
				ServerFramework::ServerFramework_GenerateMiniDump();
			}
			continue;
		}

		m_RefSkillByCodename[pRefSkill->m_Basic_Code] = pRefSkill;
	}

	return true;
}
