/**
 * ============================================================================
 * Silkroad Online - Reference World Instance & Teleport Registry
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\RefInstanceGenerator.h
 *
 * Implements:
 *   - tagRefInstanceWorldRegion (Table 0x3E record payload @ +0x18)
 *   - CRefInstanceWorldRegion (Native VTable @ 0x00B0C750, RTTI .?AVCRefInstanceWorldRegion@@)
 *   - tagRefInstanceWorldStartPos (Table 0x3F record payload @ +0x18)
 *   - CRefInstanceWorldStartPos (Native VTable @ 0x00B0C778, RTTI .?AVCRefInstanceWorldStartPos@@)
 *   - tagRefTeleport (Loaded from _RefTeleport table)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_REFINSTANCEGENERATOR_H_
#define _SERVERCOMMON_REFINSTANCEGENERATOR_H_

#include "InstanceChar.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

struct tagRefObjCommon;

/**
 * [RECONSTRUCTED - 0x006ABB20 / 0x006E0B70]
 * tagRefInstanceWorldRegion
 *
 * 8-byte payload copied from raw ShardDB record into CReferenceData + 0x258:
 *   +0x00: m_dwWorldID
 *   +0x04: m_wRegionID
 */
struct tagRefInstanceWorldRegion {
	uint32_t m_dwWorldID;   // +0x00: World instance ID (e.g. 1 = standard world)
	uint32_t m_wRegionID;   // +0x04: Region coordinate ID (e.g. 25000 = Jangan)
};

/**
 * [RECONSTRUCTED - Native VTable @ 0x00B0C750 / RTTI .?AVCRefInstanceWorldRegion@@]
 * CRefInstanceWorldRegion
 *
 * Database record descriptor for _RefInstanceWorldRegion (Table 0x3E).
 * Native allocation size: 32 bytes (0x20) via CChunkAllocatorST @ 0x007CB450.
 */
class CRefInstanceWorldRegion : public CDBRecord {
public:
	// Native 0x00735470: Constructor
	CRefInstanceWorldRegion();

	// Native 0x007354D0 / 0x007E4210: Destructor
	virtual ~CRefInstanceWorldRegion() override;

	// Native Slot 2 @ 0x00827DD0: Returns table name string
	virtual const char* GetTableName() const override;

public:
	// Exact struct layout matching native binary bytes:
	// +0x00: vptr (0x00B0C750)
	// +0x04 - +0x17: CDBRecord base fields
	uint32_t m_dwWorldID;   // +0x18: World ID
	uint32_t m_wRegionID;   // +0x1C: Region ID
};

/**
 * [RECONSTRUCTED - 0x006A2DF0 / 0x006E0C10]
 * tagRefInstanceWorldStartPos
 *
 * 24-byte payload copied from raw ShardDB record into CReferenceData + 0x25C:
 *   +0x00: m_dwWorldID
 *   +0x04: m_wRegionID
 *   +0x08: m_fPosX
 *   +0x0C: m_fPosY
 *   +0x10: m_fPosZ
 *   +0x14: m_fRadius
 */
struct tagRefInstanceWorldStartPos {
	uint32_t m_dwWorldID;   // +0x00: World instance ID
	uint32_t m_wRegionID;   // +0x04: Region coordinate ID
	float    m_fPosX;       // +0x08: Spawn position X
	float    m_fPosY;       // +0x0C: Spawn position Y
	float    m_fPosZ;       // +0x10: Spawn position Z
	float    m_fRadius;     // +0x14: Spawn area boundary radius
};

/**
 * [RECONSTRUCTED - Native VTable @ 0x00B0C778 / RTTI .?AVCRefInstanceWorldStartPos@@]
 * CRefInstanceWorldStartPos
 *
 * Database record descriptor for _RefInstanceWorldStartPos (Table 0x3F).
 * Native allocation size: 48 bytes (0x30) via CChunkAllocatorST @ 0x007CB740.
 */
class CRefInstanceWorldStartPos : public CDBRecord {
public:
	// Native 0x00735520: Constructor
	CRefInstanceWorldStartPos();

	// Native 0x00735580 / 0x007E4270: Destructor
	virtual ~CRefInstanceWorldStartPos() override;

	// Native Slot 2 @ 0x00827DA0: Returns table name string
	virtual const char* GetTableName() const override;

public:
	// Exact struct layout matching native binary bytes:
	// +0x00: vptr (0x00B0C778)
	// +0x04 - +0x17: CDBRecord base fields
	uint32_t m_dwWorldID;   // +0x18: World ID
	uint32_t m_wRegionID;   // +0x1C: Region ID
	float    m_fPosX;       // +0x20: Default X coordinate
	float    m_fPosY;       // +0x24: Default Y coordinate
	float    m_fPosZ;       // +0x28: Default Z coordinate
	float    m_fRadius;     // +0x2C: Default spawn radius
};

/**
 * [RECONSTRUCTED - 0x006A7BE0 / 0x006E4220]
 * tagRefTeleport
 *
 * Teleport building template descriptor parsed from _RefTeleport (Table 0x12).
 * Allocation size: 76 bytes (0x4C) via 0x006E3570.
 */
struct tagRefTeleport {
	uint32_t         m_dwID;           // +0x00: Teleport building template ID
	std::string      m_strCodeName;    // +0x04: Building code name (e.g. "GATE_CH_JANGAN")
	uint16_t         m_wRegionID;      // +0x26: Destination Region ID
	int16_t          m_sPosX;          // +0x28: Destination Position X
	int16_t          m_sPosY;          // +0x2A: Destination Position Y
	int16_t          m_sPosZ;          // +0x2C: Destination Position Z
	tagRefObjCommon* m_pRefObjCommon;  // +0x30: Associated building reference object
	uint8_t          m_byType;         // +0x34: Teleport type / access permission flag
	uint8_t          m_pad35[0x13];    // +0x35 - +0x47
	uint16_t         m_wGateNo;        // +0x48: Portal / gate sequence index

	tagRefTeleport();
	~tagRefTeleport();
};

class CGObj;

class CRefInstanceGenerator {
public:
	CRefInstanceGenerator();
	virtual ~CRefInstanceGenerator();

	virtual CGObj* GenerateInstance(uint32_t dwRefID);
};

#endif // _SERVERCOMMON_REFINSTANCEGENERATOR_H_
