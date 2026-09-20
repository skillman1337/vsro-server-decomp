/**
 * ============================================================================
 * Silkroad Online - Skill & Mastery Database Instance Records
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceSkill.h
 *
 * Implements:
 *   - CInstanceSkill        (Native RTTI: .?AVCInstanceSkill@@, VTable @ 0x00B0D368)
 *   - CInstanceSkillMastery (Native RTTI: .?AVCInstanceSkillMastery@@, VTable @ 0x00B0D390)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_INSTANCESKILL_H_
#define _SERVERCOMMON_INSTANCESKILL_H_

#include "InstanceChar.h"
#include <cstdint>

// Global database write permission / policy flags
extern uint32_t g_bSkillDBWriteAllowed;        // Native @ 0x00D20544
extern uint32_t g_bSkillMasteryDBWriteAllowed; // Native @ 0x00D2064C

/**
 * [RECONSTRUCTED - Native 0x00B0D368]
 * CInstanceSkill
 *
 * Shard database instance entity representing a row in table _CharSkill:
 *   +0x00: VTable pointer (0x00B0D368)
 *   +0x04: uint32_t m_dwRecordID   (from CDBRecord)
 *   +0x08: uint32_t m_dwStateFlags (from CDBRecord; dirty bit: 0x4)
 *   +0x0C: void*    m_pOwnerTable  (from CDBRecord)
 *   +0x10: uint32_t m_dwReserved   (from CDBRecord)
 *   +0x14: uint8_t  pad14[4]
 *   +0x18: uint32_t m_dwCharID     (Character Global ID)
 *   +0x1C: uint32_t m_dwSkillID    (Skill ID)
 *   +0x20: uint8_t  m_byEnable     (Skill enable/active flag)
 */
class CInstanceSkill : public CDBRecord {
public:
	CInstanceSkill();
	virtual ~CInstanceSkill() override;

	// CDBRecord virtual overrides
	virtual const char* GetTableName() const override;  // Slot 0 (+0x00) @ 0x008465A0
	virtual void*       GetTableDesc() const override;  // Slot 2 (+0x08) @ 0x00826F30
	virtual void*       GetColumnDesc() const;          // Slot 3 (+0x0C) @ 0x00826F50
	virtual void        Clear();                        // Slot 5 (+0x14) @ 0x00824F50

	// Setters and Getters
	uint32_t GetCharID() const { return m_dwCharID; }
	void SetCharID(uint32_t dwCharID);

	uint32_t GetSkillID() const { return m_dwSkillID; }
	void SetSkillID(uint32_t dwSkillID);

	uint8_t GetEnable() const { return m_byEnable; }
	void SetEnable(uint8_t byEnable);

public:
	uint8_t  m_pad14[4];
	uint32_t m_dwCharID;   // +0x18: Character ID
	uint32_t m_dwSkillID;  // +0x1C: Skill ID
	uint8_t  m_byEnable;   // +0x20: Enabled flag
	uint8_t  m_pad21[3];
};

/**
 * [RECONSTRUCTED - Native 0x00B0D390]
 * CInstanceSkillMastery
 *
 * Shard database instance entity representing a row in table _CharSkillMastery:
 *   +0x00: VTable pointer (0x00B0D390)
 *   +0x04: uint32_t m_dwRecordID   (from CDBRecord)
 *   +0x08: uint32_t m_dwStateFlags (from CDBRecord; dirty bit: 0x4)
 *   +0x0C: void*    m_pOwnerTable  (from CDBRecord)
 *   +0x10: uint32_t m_dwReserved   (from CDBRecord)
 *   +0x14: uint8_t  pad14[4]
 *   +0x18: uint32_t m_dwCharID     (Character Global ID)
 *   +0x1C: uint32_t m_dwMasteryID  (Mastery ID)
 *   +0x20: uint8_t  m_byLevel      (Mastery Level / Rank)
 */
class CInstanceSkillMastery : public CDBRecord {
public:
	CInstanceSkillMastery();
	virtual ~CInstanceSkillMastery() override;

	// CDBRecord virtual overrides
	virtual const char* GetTableName() const override;  // Slot 0 (+0x00) @ 0x00846B00
	virtual void*       GetTableDesc() const override;  // Slot 2 (+0x08) @ 0x00826F00
	virtual void*       GetColumnDesc() const;          // Slot 3 (+0x0C) @ 0x00826F20
	virtual void        Clear();                        // Slot 5 (+0x14) @ 0x00824EE0

	// Setters and Getters
	uint32_t GetCharID() const { return m_dwCharID; }
	void SetCharID(uint32_t dwCharID);

	uint32_t GetMasteryID() const { return m_dwMasteryID; }
	void SetMasteryID(uint32_t dwMasteryID);

	uint8_t GetLevel() const { return m_byLevel; }
	void SetLevel(uint8_t byLevel);

public:
	uint8_t  m_pad14[4];
	uint32_t m_dwCharID;    // +0x18: Character ID
	uint32_t m_dwMasteryID; // +0x1C: Mastery ID
	uint8_t  m_byLevel;     // +0x20: Mastery level / rank
	uint8_t  m_pad21[3];
};

#endif // _SERVERCOMMON_INSTANCESKILL_H_
