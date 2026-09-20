/**
 * ============================================================================
 * Silkroad Online - Character and Entity Database Instance Records
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\InstanceChar.h
 *
 * Implements:
 *   - CDBRecord (Native RTTI: .?AVCDBRecord@@)
 *   - CInstanceObj (Native RTTI: .?AVCInstanceObj@@)
 *   - CInstanceChar (Native RTTI: .?AVCInstanceChar@@, VTable @ 0x00AE3724)
 *   - CInstancePC (Native RTTI: .?AVCInstancePC@@, VTable @ 0x00B0D12C)
 *   - CInstanceCOS (Native RTTI: .?AVCInstanceCOS@@, VTable @ 0x00AE3494)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_INSTANCECHAR_H_
#define _SERVERCOMMON_INSTANCECHAR_H_

#include <cstdint>
#include <string>

struct tagRefObjCommon;
extern uint32_t g_bPCDBWriteAllowed; // Native D1FBF4, bit 0 allows record mutation.

namespace BSLib {
namespace Database {
	class CDBTable;
}
}

/**
 * Base database record container
 * Native RTTI: .?AVCDBRecord@@
 */
class CDBRecord {
public:
	CDBRecord();
	virtual ~CDBRecord();

	virtual const char* GetTableName() const;   // Slot 2 (+0x08)
	virtual void*       GetTableDesc() const;   // Slot 3 (+0x0C)
	virtual bool        CheckCondition() const; // Slot 4 (+0x10)
	virtual bool        Serialize(void* pStream); // Slot 5 (+0x14)
	virtual uint32_t    GetRecordOffset() const; // Slot 6 (+0x18)
	virtual bool        BindColumn(void* pCol);  // Slot 7 (+0x1C)

public:
	uint32_t m_dwRecordID;   // +0x04: Database unique record primary key
	uint32_t m_dwStateFlags;  // +0x08: Record persistence state flags
	void*    m_pOwnerTable;   // +0x0C: Owning CDBTable or CMemoryPool pointer
	uint32_t m_dwReserved;    // +0x10: Reserved padding
};

/**
 * Base game object instance record with reference data linkage
 * Native RTTI: .?AVCInstanceObj@@
 */
class CInstanceObj : public CDBRecord {
public:
	CInstanceObj();
	virtual ~CInstanceObj() override;

public:
	tagRefObjCommon* m_pRefObjCommon; // +0x18: Reference common object descriptor
};

/**
 * Character instance record interface
 * Native RTTI: .?AVCInstanceChar@@
 * Native VTable @ 0x00AE3724 (16 virtual slots)
 */
class CInstanceChar : public CInstanceObj {
public:
	// Native PC accessor 4DDCC0 reads permanent-record +20. This is the
	// durable character identity, distinct from runtime GID and allocator ID.
	uint32_t m_dwCharID = 0;
	CInstanceChar();
	virtual ~CInstanceChar() override;

	// Virtual interface mapping native VTable @ 0x00AE3724
	virtual void NoOpStub();                               // Slot 8  (+0x20) @ 0x009BF500
	virtual bool ResolveRefObjCommon();                    // Slot 9  (+0x24) @ 0x0042E850
	virtual bool SerializeStats(void* pStream, int mode);  // Slot 10 (+0x28) @ 0x008412B0
	virtual const char* GetCharName() const = 0;           // Slot 11 (+0x2C) Pure virtual (_purecall @ 0x009DD3AD)
	virtual uint8_t     GetHwanLevel() const;              // Slot 12 (+0x30) Pure virtual / default 0
	virtual uint32_t    GetExp() const;                    // Slot 13 (+0x34) @ 0x00449340 (Returns m_pRefObjCommon->m_dwRewardExp)
	virtual uint32_t    GetCurrentHP() const;              // Slot 14 (+0x38) Pure virtual / default 0
	virtual uint32_t    GetCurrentMP() const;              // Slot 15 (+0x3C) Pure virtual / default 0
	virtual void        SetCurrentHP(uint32_t dwHP) {}
	virtual void        SetCurrentMP(uint32_t dwMP) {}
	virtual int64_t     GetGold() const { return 0; }
	virtual void        SetGold(int64_t nGold) {}
	virtual uint32_t    GetSkillPoints() const { return 0; }
	virtual void        SetSkillPoints(uint32_t dwSP) {}

	// Backward-compatibility alias
	inline const char* GetName() const { return GetCharName(); }
};

// Typedef for legacy code referencing CDataPermanent
using CDataPermanent = CInstanceChar;

/**
 * Player Character (PC) permanent instance record
 * Native RTTI: .?AVCInstancePC@@
 * Native VTable @ 0x00B0D12C
 */
class CInstancePC : public CInstanceChar {
public:
	CInstancePC();
	virtual ~CInstancePC() override;

	virtual const char* GetTableName() const override;      // Slot 2  (+0x08) @ 0x00827080 -> "CInstancePC"
	virtual const char* GetCharName() const override;       // Slot 11 (+0x2C) @ 0x00825E40 -> m_strCharName.c_str()
	virtual uint8_t     GetHwanLevel() const override;      // Slot 12 (+0x30) @ 0x00825E30 -> m_byHwanLevel
	virtual uint32_t    GetCurrentHP() const override;      // Slot 14 (+0x38) @ 0x00601C80 -> m_dwHP
	virtual uint32_t    GetCurrentMP() const override;      // Slot 15 (+0x3C) @ 0x00825E20 -> m_dwMP
	virtual void        SetCurrentHP(uint32_t dwHP) override { m_dwHP = dwHP; }
	virtual void        SetCurrentMP(uint32_t dwMP) override { m_dwMP = dwMP; }
	virtual int64_t     GetGold() const override { return m_nGold; }
	virtual void        SetGold(int64_t nGold) override { m_nGold = nGold; }
	virtual uint32_t    GetSkillPoints() const override { return m_dwRemainSkillPoint; }
	virtual void        SetSkillPoints(uint32_t dwSP) override { m_dwRemainSkillPoint = dwSP; }

public:
	std::string m_strCharName; // +0x30: Character Name
	std::string m_strNickName; // +0x4C: Job / Guild Nickname
	uint8_t     m_byHwanLevel; // +0x65: Berserker / Hwan Level
	uint8_t     m_pad66[0x12]; // +0x66 - +0x77
	int64_t     m_nGold;       // +0x78 - +0x7F: Character Gold (64-bit integer, proven @ 0x004E4B60 / 0x0059F495)
	uint32_t    m_dwRemainSkillPoint; // +0x80: Current available Skill Points (SP) (proven @ 0x0059E592)
	uint16_t    m_wStatPoints = 0; // +0x84
	uint8_t     m_byBerserkPoints = 0; // +0x86, distinct from Hwan level +65
	uint8_t     m_pad87[5];    // +0x87 - +0x8B
	uint32_t    m_dwHP;        // +0x8C: Current Health Points
	uint32_t    m_dwMP;        // +0x90: Current Mana Points
};

/**
 * Creature on Summon (COS / Pet / Mount) permanent instance record
 * Native RTTI: .?AVCInstanceCOS@@
 * Native VTable @ 0x00AE3494
 */
class CInstanceCOS : public CInstanceChar {
public:
	CInstanceCOS();
	virtual ~CInstanceCOS() override;

	virtual const char* GetTableName() const override;      // Slot 2  (+0x08) -> "CInstanceCOS"
	virtual const char* GetCharName() const override;       // Slot 11 (+0x2C) @ 0x00449350

public:
	std::string m_strCustomCOSName; // +0x40: User-assigned custom pet name
};

#endif // _SERVERCOMMON_INSTANCECHAR_H_
