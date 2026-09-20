/**
 * ============================================================================
 * Silkroad Online - Entity Parameter & Dynamic Stat Keeper
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GParamKeeper.h
 *
 * Implements:
 *   - class CParamElement  (Native size: 0x3C / 60 bytes @ 0x004B2F40)
 *   - class CGParamKeeper  (Native size: 0x814 / 2068 bytes @ 0x004B33E0)
 *   - Native RTTI / String @ 0x00AECDC8:
 *       "D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\GParamKeeper.cpp"
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GPARAMKEEPER_H_
#define _SR_GAMESERVER_GPARAMKEEPER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Maximum parameter slots supported by Silkroad engine (0x200 = 512)
constexpr uint16_t MAX_KEEPER_PARAMS = 512;

// Modifier category indices into CParamElement::m_apModifiers[4]
enum ParamModifierType : int32_t {
	PARAM_MOD_BASE       = 0, // Flat base addition
	PARAM_MOD_ADD        = 1, // Sum of percentage deltas
	PARAM_MOD_PERCENT    = 2, // Percentage scale (e.g. +10%)
	PARAM_MOD_MULTIPLIER = 3  // Multiplicative factor
};

/**
 * [RECONSTRUCTED - Native 0x004B2F40 / 0x004B2F80 / 0x004B3210 / 0x004B33A0]
 * CParamElement
 *
 * Encapsulates an individual calculated stat attribute with 4 modifier buckets,
 * dependent parameter cascades, dirty evaluation caching, and Joymax CLAMP() bounds.
 *
 * Memory Layout:
 *   +0x00 - +0x07: uint32_t m_dwID; uint32_t m_dwPad04;
 *   +0x08 - +0x17: void*    m_apModifiers[4];
 *   +0x18 - +0x19: int16_t  m_wFlags;
 *   +0x1A:         uint8_t  m_bDirty; (1 = needs recalculation)
 *   +0x1B:         uint8_t  m_pad1B;
 *   +0x1C - +0x1F: float    m_fCalculatedValue; (cached evaluated result)
 *   +0x20 - +0x23: float    m_fMinValue; (clamp lower bound)
 *   +0x24 - +0x27: float    m_fMaxValue; (clamp upper bound)
 *   +0x28 - +0x2B: float    m_fBaseValue; (base attribute magnitude)
 *   +0x2C - +0x2F: float    m_fInitialValue; (ignored-write sentinel)
 *   +0x30 - +0x3B: native map from target CParam pointer to modifier channel.
 * These are native offsets, not sizeof/offsetof guarantees for this portable build.
 */
class CParamElement {
public:
	// Native 0x004B2F40: In-place initialization
	CParamElement();
	~CParamElement();

	void Init(int16_t wFlags, float fMin, float fMax, float fBase, float fInitial);
	void Cleanup();

	// Native 0x004B33A0: Retrieves calculated value, evaluating if dirty
	float GetFloatValue();

	// Native 0x004B3210: Evaluates 4-slot modifier tree and enforces CLAMP()
	void RecalculateValue();

	// Native 0x004B3000: Adds modifier contribution and marks element dirty
	bool AddModifier(int32_t nModifierType, uintptr_t source, float fValue);
	// 4B31A0: erase one source from every bucket, preserving allocated empty buckets.
	uint8_t RemoveModifier(uintptr_t source);
	// 4B3060: first registration for this target wins, including the channel.
	void AddDependent(CParamElement* target, int32_t channel);

	// Native 0x004B3130: Propagates value updates to dependent parameters
	void PropagateDependentParameters();

public:
	uint32_t                      m_dwID;              // +0x00
	uint32_t                      m_dwPad04;           // +0x04
	void*                         m_apModifiers[4];    // +0x08 - +0x17
	int16_t                       m_wFlags;            // +0x18
	uint8_t                       m_bDirty;            // +0x1A
	uint8_t                       m_pad1B;             // +0x1B
	float                         m_fCalculatedValue;  // +0x1C
	float                         m_fMinValue;         // +0x20
	float                         m_fMaxValue;         // +0x24
	float                         m_fBaseValue;        // +0x28
	float                         m_fInitialValue;     // +0x2C
	// Portable representation of the native pointer-keyed target/channel map.
	std::map<CParamElement*, int32_t> m_mapDependent;
};

/**
 * [RECONSTRUCTED - Native 0x004B33E0 / 0x004B3450 / 0x004B3460 / 0x004B3510 / 0x004B3740 / 0x004B3850]
 * CGParamKeeper
 *
 * Primary entity attribute and combat stat dictionary:
 *   - Manages 512 stat slots indexed by 16-bit integer ID
 *   - Embedded at struct offset +0x1EC inside CGObjChar (0x004A6B20)
 *
 * Memory Layout:
 *   +0x00:         void*           m_pOwner;           (owning CGObjChar pointer)
 *   +0x04 - +0x803: CParamElement* m_apParams[512];    (512 pointer slots = 2048 bytes)
 *   +0x804:        uint32_t        m_dwParamCount;
 *   +0x808:        void*           m_pParamPool;
 *   +0x80C:        uint32_t        m_dwReserved;
 *   +0x810:        uint8_t         m_bDirty;
 *   +0x811 - +0x813: uint8_t       m_pad811[3];
 * Total size: 0x814 bytes (2,068 bytes)
 */
class CGParamKeeper {
public:
	// Native 0x004B33E0: Constructor
	CGParamKeeper();

	// Native 0x004B3460: Destructor & cleanup
	virtual ~CGParamKeeper();

	// Native 0x004B3740: Queries parameter float value (validates ID < 0x200)
	float GetParamFloat(uint16_t wParamID);

	// Native 0x004B3510: Modifies parameter value and sets dirty flag
	bool SetParamFloat(uint16_t wParamID, int32_t nModifierType, uintptr_t source, float fValue);

	// Native 0x004B3850: Ensures parameter descriptor is allocated and populated
	CParamElement* EnsureParamInitialized(uint16_t wParamID);

	// Native 0x004B3450: Clears modified/dirty state
	void ResetDirty();
	// 4B3660: erase one owner across all parameters, then notify once.
	void RemoveSourceModifiers(uintptr_t source);

	// Native 0x004B3460: Tears down all allocated parameter elements
	void Cleanup();

	void SetOwner(void* pOwner) { m_pOwner = pOwner; }
	void* GetOwner() const { return m_pOwner; }

public:
	void*          m_pOwner;               // +0x00: Owning CGObjChar pointer
	CParamElement* m_apParams[MAX_KEEPER_PARAMS]; // +0x04: 512 parameter element pointers
	uint32_t       m_dwParamCount;         // +0x804: Active count
	void*          m_pParamPool;           // +0x808: Chunk allocator / pool
	uint32_t       m_dwReserved;           // +0x80C
	uint8_t        m_bDirty;               // +0x810: Modification flag
	uint8_t        m_pad811[3];            // +0x811 - +0x813
	// Portable representation of the native insertion-ordered list at +0x804.
	std::vector<CParamElement*> m_parameterOrder;
};

#endif // _SR_GAMESERVER_GPARAMKEEPER_H_
