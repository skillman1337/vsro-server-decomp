/**
 * ============================================================================
 * Silkroad Online - Timed Job Database Instance Record
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceTimedJob.h
 *
 * Implements:
 *   - ITimedJobCommonData (Native RTTI: .?AUITimedJobCommonData@@, VTable @ 0x00B0D5EC)
 *   - CInstanceTimedJob   (Native RTTI: .?AVCInstanceTimedJob@@, VTable @ 0x00B0D68C)
 *   - Native struct size: 0x60 / 96 bytes (proven by ehvec dtor @ 0x007E6850)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_INSTANCETIMEDJOB_H_
#define _SERVERCOMMON_INSTANCETIMEDJOB_H_

#include "InstanceChar.h"
#include <cstdint>

// Global database write permission / policy flag (Native @ 0x00D219FC, default = 5)
extern uint32_t g_bTimedJobDBWriteAllowed;

/**
 * [RECONSTRUCTED - Native 0x00B0D5EC]
 * ITimedJobCommonData
 *
 * Abstract database instance interface for active and persistent timed jobs:
 *   - Inherits from CDBRecord (which inherits from CBase)
 *   - Contains 15 pure virtual setters (Slots 9 - 23)
 *   - Contains 15 pure virtual getters (Slots 24 - 38)
 *   - Table reset / clear method (Slot 5 @ 0x00824380)
 */
class ITimedJobCommonData : public CDBRecord {
public:
	// Native 0x0073C040: Constructor
	ITimedJobCommonData();

	// Native 0x0073C110: Destructor
	virtual ~ITimedJobCommonData() override;

	// CDBRecord virtual methods
	virtual const char* GetTableName() const override = 0;   // Slot 0 (+0x00)
	virtual void*       GetTableDesc() const override = 0;   // Slot 2 (+0x08)
	virtual void*       GetColumnDesc() const = 0;           // Slot 3 (+0x0C)
	virtual bool        CheckCondition() const override;     // Slot 4 (+0x10) @ 0x0042EFB0
	virtual int32_t     Clear();                             // Slot 5 (+0x14) @ 0x00824380
	virtual uint32_t    GetRecordOffset() const override;    // Slot 6 (+0x18) @ 0x0042EE50
	virtual bool        BindColumn(void* pCol) override;     // Slot 7 (+0x1C) @ 0x0042EDE0

	// 15 Virtual Setters (Pure virtual in ITimedJobCommonData, Slots 9 - 23)
	virtual int32_t SetID(uint32_t dwID) = 0;                                  // Slot 9  (+0x24)
	virtual int32_t SetCharID(uint32_t dwCharID) = 0;                          // Slot 10 (+0x28)
	virtual int32_t SetCategory(uint8_t byCategory) = 0;                       // Slot 11 (+0x2C)
	virtual int32_t SetJobID(uint32_t dwJobID) = 0;                            // Slot 12 (+0x30)
	virtual int32_t SetTimeToKeep(uint32_t dwTimeToKeep) = 0;                  // Slot 13 (+0x34)
	virtual int32_t SetData1(uint32_t dwData1) = 0;                            // Slot 14 (+0x38)
	virtual int32_t SetData2(uint32_t dwData2) = 0;                            // Slot 15 (+0x3C)
	virtual int32_t SetData3(uint32_t dwData3) = 0;                            // Slot 16 (+0x40)
	virtual int32_t SetData4(uint32_t dwData4) = 0;                            // Slot 17 (+0x44)
	virtual int32_t SetData5(uint32_t dwData5) = 0;                            // Slot 18 (+0x48)
	virtual int32_t SetData6(uint32_t dwData6) = 0;                            // Slot 19 (+0x4C)
	virtual int32_t SetData7(uint32_t dwData7) = 0;                            // Slot 20 (+0x50)
	virtual int32_t SetData8(uint32_t dwData8) = 0;                            // Slot 21 (+0x54)
	virtual int32_t SetSerial64(uint32_t dwLow, uint32_t dwHigh) = 0;          // Slot 22 (+0x58)
	virtual int32_t SetJID(uint32_t dwJID) = 0;                                // Slot 23 (+0x5C)

	// 15 Virtual Getters (Pure virtual in ITimedJobCommonData, Slots 24 - 38)
	virtual uint32_t GetID() const = 0;                                        // Slot 24 (+0x60)
	virtual uint32_t GetCharID() const = 0;                                    // Slot 25 (+0x64)
	virtual uint8_t  GetCategory() const = 0;                                  // Slot 26 (+0x68)
	virtual uint32_t GetJobID() const = 0;                                     // Slot 27 (+0x6C)
	virtual uint32_t GetTimeToKeep() const = 0;                                // Slot 28 (+0x70)
	virtual uint32_t GetData1() const = 0;                                     // Slot 29 (+0x74)
	virtual uint32_t GetData2() const = 0;                                     // Slot 30 (+0x78)
	virtual uint32_t GetData3() const = 0;                                     // Slot 31 (+0x7C)
	virtual uint32_t GetData4() const = 0;                                     // Slot 32 (+0x80)
	virtual uint32_t GetData5() const = 0;                                     // Slot 33 (+0x84)
	virtual uint32_t GetData6() const = 0;                                     // Slot 34 (+0x88)
	virtual uint32_t GetData7() const = 0;                                     // Slot 35 (+0x8C)
	virtual uint32_t GetData8() const = 0;                                     // Slot 36 (+0x90)
	virtual int64_t  GetSerial64() const = 0;                                  // Slot 37 (+0x94)
	virtual uint32_t GetJID() const = 0;                                       // Slot 38 (+0x98)
};

/**
 * [RECONSTRUCTED - Native 0x00B0D68C]
 * CInstanceTimedJob
 *
 * Shard database instance entity representing a row in table _TIMEDJOB:
 *   - Exact native size: 0x60 bytes (96 bytes)
 *   - Implements table binding and dirty state propagation
 *   - Formatted in {?=CALL _ADDTIMEDJOB (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%I64d,%d)}
 *
 * Exact Memory Layout:
 *   +0x00: VTable pointer (0x00B0D68C)
 *   +0x04: uint32_t m_dwRecordID   (from CDBRecord)
 *   +0x08: uint32_t m_dwStateFlags (from CDBRecord; dirty bits: 0x4 = time, 0x8 = data)
 *   +0x0C: void*    m_pOwnerTable  (from CDBRecord)
 *   +0x10: uint32_t m_dwReserved   (from CDBRecord)
 *   +0x14: uint8_t  pad14[4]
 *   +0x18: uint32_t m_dwID         (Primary Key in _TIMEDJOB)
 *   +0x1C: uint32_t m_dwCharID     (Character Global ID / Owner key)
 *   +0x20: uint8_t  m_byCategory   (Job Category)
 *   +0x21: uint8_t  pad21[3]
 *   +0x24: uint32_t m_dwJobID      (Skill/Buff/Item template ID)
 *   +0x28: uint32_t m_dwTimeToKeep (Remaining seconds / duration)
 *   +0x2C: uint32_t m_dwData1      (Sub-type / auto-buff state)
 *   +0x30: uint32_t m_dwData2
 *   +0x34: uint32_t m_dwData3
 *   +0x38: uint32_t m_dwData4
 *   +0x3C: uint32_t m_dwData5
 *   +0x40: uint32_t m_dwData6
 *   +0x44: uint32_t m_dwData7
 *   +0x48: uint32_t m_dwData8
 *   +0x4C: uint8_t  pad4C[4]       (Alignment padding to 8-byte boundary)
 *   +0x50: int64_t  m_nSerial64    (64-bit serial integer / item reference)
 *   +0x58: uint32_t m_dwJID        (User Account / Player Sequence ID)
 *   +0x5C: uint8_t  pad5C[4]       (Padding to exact 0x60 struct size)
 */
class CInstanceTimedJob : public ITimedJobCommonData {
public:
	// Native 0x0073AC20: Constructor
	CInstanceTimedJob();

	// Native 0x007E6850: Destructor
	virtual ~CInstanceTimedJob() override;

	// CDBRecord virtual overrides
	virtual const char* GetTableName() const override;  // Slot 0 (+0x00) @ 0x00850B30
	virtual void*       GetTableDesc() const override;  // Slot 2 (+0x08) @ 0x00826B40
	virtual void*       GetColumnDesc() const override; // Slot 3 (+0x0C) @ 0x00826B60
	virtual int32_t     Clear() override;               // Slot 5 (+0x14) @ 0x00824090

	// 15 Concrete Setters (Slots 9 - 23)
	virtual int32_t SetID(uint32_t dwID) override;                         // Slot 9  (+0x24) @ 0x00824360
	virtual int32_t SetCharID(uint32_t dwCharID) override;                 // Slot 10 (+0x28) @ 0x00824340
	virtual int32_t SetCategory(uint8_t byCategory) override;              // Slot 11 (+0x2C) @ 0x00824320
	virtual int32_t SetJobID(uint32_t dwJobID) override;                   // Slot 12 (+0x30) @ 0x00824300
	virtual int32_t SetTimeToKeep(uint32_t dwTimeToKeep) override;         // Slot 13 (+0x34) @ 0x008242D0
	virtual int32_t SetData1(uint32_t dwData1) override;                   // Slot 14 (+0x38) @ 0x008242A0
	virtual int32_t SetData2(uint32_t dwData2) override;                   // Slot 15 (+0x3C) @ 0x00824270
	virtual int32_t SetData3(uint32_t dwData3) override;                   // Slot 16 (+0x40) @ 0x00824240
	virtual int32_t SetData4(uint32_t dwData4) override;                   // Slot 17 (+0x44) @ 0x00824210
	virtual int32_t SetData5(uint32_t dwData5) override;                   // Slot 18 (+0x48) @ 0x008241E0
	virtual int32_t SetData6(uint32_t dwData6) override;                   // Slot 19 (+0x4C) @ 0x008241B0
	virtual int32_t SetData7(uint32_t dwData7) override;                   // Slot 20 (+0x50) @ 0x00824180
	virtual int32_t SetData8(uint32_t dwData8) override;                   // Slot 21 (+0x54) @ 0x00824150
	virtual int32_t SetSerial64(uint32_t dwLow, uint32_t dwHigh) override; // Slot 22 (+0x58) @ 0x00824110
	virtual int32_t SetJID(uint32_t dwJID) override;                       // Slot 23 (+0x5C) @ 0x008240E0

	// 15 Concrete Getters (Slots 24 - 38)
	virtual uint32_t GetID() const override;                               // Slot 24 (+0x60) @ 0x00823DE0
	virtual uint32_t GetCharID() const override;                           // Slot 25 (+0x64) @ 0x00823DD0
	virtual uint8_t  GetCategory() const override;                         // Slot 26 (+0x68) @ 0x00823DC0
	virtual uint32_t GetJobID() const override;                            // Slot 27 (+0x6C) @ 0x00871BA0
	virtual uint32_t GetTimeToKeep() const override;                       // Slot 28 (+0x70) @ 0x00823DB0
	virtual uint32_t GetData1() const override;                            // Slot 29 (+0x74) @ 0x008240D0
	virtual uint32_t GetData2() const override;                            // Slot 30 (+0x78) @ 0x008237E0
	virtual uint32_t GetData3() const override;                            // Slot 31 (+0x7C) @ 0x008240C0
	virtual uint32_t GetData4() const override;                            // Slot 32 (+0x80) @ 0x008240B0
	virtual uint32_t GetData5() const override;                            // Slot 33 (+0x84) @ 0x00823DA0
	virtual uint32_t GetData6() const override;                            // Slot 34 (+0x88) @ 0x00823D90
	virtual uint32_t GetData7() const override;                            // Slot 35 (+0x8C) @ 0x00561060
	virtual uint32_t GetData8() const override;                            // Slot 36 (+0x90) @ 0x00823D80
	virtual int64_t  GetSerial64() const override;                         // Slot 37 (+0x94) @ 0x008240A0
	virtual uint32_t GetJID() const override;                              // Slot 38 (+0x98) @ 0x00823D70
	// 850B40 defines group 0 as the ID/CharID key, group 1 as immutable
	// Category/JobID, group 2 as TimeToKeep, and group 3 as Data1..8/Serial64/JID.
	// Portable SQL projection of 9742D0's all-dirty-groups path.
	bool BuildDirtyUpdateQuery(std::string& output);

	// Helper for 64-bit int setter
	int32_t SetSerial64(int64_t nSerial) {
		return SetSerial64(static_cast<uint32_t>(nSerial & 0xFFFFFFFF), static_cast<uint32_t>(nSerial >> 32));
	}

public:
	// Exact struct layout matching native binary bytes:
	uint8_t  m_pad14[4];        // +0x14 - +0x17
	uint32_t m_dwID;            // +0x18: Unique timed job row ID
	uint32_t m_dwCharID;        // +0x1C: Owner character ID
	uint8_t  m_byCategory;      // +0x20: Job category
	uint8_t  m_pad21[3];        // +0x21 - +0x23
	uint32_t m_dwJobID;         // +0x24: Skill/Buff ID
	uint32_t m_dwTimeToKeep;    // +0x28: Remaining duration
	uint32_t m_dwData1;         // +0x2C: Data 1
	uint32_t m_dwData2;         // +0x30: Data 2
	uint32_t m_dwData3;         // +0x34: Data 3
	uint32_t m_dwData4;         // +0x38: Data 4
	uint32_t m_dwData5;         // +0x3C: Data 5
	uint32_t m_dwData6;         // +0x40: Data 6
	uint32_t m_dwData7;         // +0x44: Data 7
	uint32_t m_dwData8;         // +0x48: Data 8
	uint8_t  m_pad4C[4];        // +0x4C - +0x4F: 8-byte alignment padding
	union {
		int64_t m_nSerial64;    // +0x50 - +0x57: 64-bit serial value
		struct {
			uint32_t m_dwSerialLow;
			uint32_t m_dwSerialHigh;
		};
	};
	uint32_t m_dwJID;           // +0x58: User account JID
	uint8_t  m_pad5C[4];        // +0x5C - +0x5F: Total 0x60 bytes
};

#endif // _SERVERCOMMON_INSTANCETIMEDJOB_H_
