/**
 * ============================================================================
 * Silkroad Online - Entity Parameter & Dynamic Stat Keeper Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GParamKeeper.cpp
 *
 * Implements:
 *   - CParamElement methods:
 *       CParamElement_Init                     @ 0x004B2F40
 *       CParamElement_Cleanup                  @ 0x004B2F80
 *       CParamElement_AddModifier              @ 0x004B3000
 *       CParamElement_PropagateDependent       @ 0x004B3130
 *       CParamElement_RecalculateValue         @ 0x004B3210
 *       CParamElement_GetFloatValue            @ 0x004B33A0
 *   - CGParamKeeper methods:
 *       CGParamKeeper_constructor              @ 0x004B33E0
 *       CGParamKeeper_ResetDirty               @ 0x004B3450
 *       CGParamKeeper_Cleanup                  @ 0x004B3460
 *       CGParamKeeper_SetParamFloat            @ 0x004B3510
 *       CGParamKeeper_GetParamFloat            @ 0x004B3740
 *       CGParamKeeper_EnsureParamInitialized   @ 0x004B3850
 * ============================================================================
 */

#include "GParamKeeper.h"
#include "GObjChar.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

// Portable source-keyed buckets. This is not a VC8 container-layout proof.
using ModifierBucket = std::map<uintptr_t, float>;

// ============================================================================
// CParamElement Implementation
// ============================================================================

CParamElement::CParamElement()
	: m_dwID(0)
	, m_dwPad04(0)
	, m_wFlags(0)
	, m_bDirty(0)
	, m_pad1B(0)
	, m_fCalculatedValue(0.0f)
	, m_fMinValue(0.0f)
	, m_fMaxValue(1000000.0f)
	, m_fBaseValue(0.0f)
	, m_fInitialValue(0.0f) {
	for (int i = 0; i < 4; ++i) {
		m_apModifiers[i] = nullptr;
	}
}

CParamElement::~CParamElement() {
	Cleanup();
}

/**
 * [RECONSTRUCTED - 0x004B2F40]
 * CParamElement_Init
 * Machine operations:
 *   *(arg3 + 0x20) = arg4 (m_fMinValue)
 *   *(arg3 + 0x1A) = 0    (m_bDirty = 0)
 *   *(arg3 + 0x18) = arg2 (m_wFlags)
 *   *(arg3 + 0x24) = arg5 (m_fMaxValue)
 *   *(arg3 + 0x1C) = arg6 (m_fCalculatedValue)
 *   *(arg3 + 0x28) = arg6 (m_fBaseValue)
 *   *(arg3 + 0x2C) = arg7 (m_fInitialValue)
 *   Zeroes 4 modifier pointers at +0x08..+0x17
 */
void CParamElement::Init(int16_t wFlags, float fMin, float fMax, float fBase, float fInitial) {
	m_fMinValue        = fMin;
	m_bDirty           = 0;
	m_wFlags           = wFlags;
	m_fMaxValue        = fMax;
	m_fCalculatedValue = fBase;
	m_fBaseValue       = fBase;
	m_fInitialValue    = fInitial;

	for (int i = 0; i < 4; ++i) {
		m_apModifiers[i] = nullptr;
	}
}

/**
 * [RECONSTRUCTED - 0x004B2F80]
 * CParamElement_Cleanup
 * Deletes all source-keyed modifier buckets and clears dependent associations.
 */
void CParamElement::Cleanup() {
	for (int i = 0; i < 4; ++i) {
		if (m_apModifiers[i] != nullptr) {
			auto* pMod = static_cast<ModifierBucket*>(m_apModifiers[i]);
			delete pMod;
			m_apModifiers[i] = nullptr;
		}
	}
	m_mapDependent.clear();
	m_bDirty = 0;
}

/**
 * [RECONSTRUCTED - 0x004B33A0]
 * CParamElement_GetFloatValue
 *
 * If dirty flag is set (+0x1A), triggers full recalculation before returning cached value (+0x1C).
 */
float CParamElement::GetFloatValue() {
	if (m_bDirty) {
		RecalculateValue();
	}
	return m_fCalculatedValue;
}

/**
 * [RECONSTRUCTED - 0x004B3210]
 * CParamElement_RecalculateValue
 *
 * Re-evaluates parameter value across the 4 modifier tiers:
 *   Tier 0: Flat Base Addition (val0)
 *   Tier 1: Sum of percentage deltas
 *   Tier 2: Product of (1 + value/100), restarting when accumulator is zero
 *   Tier 3: Product of value/100, starting from 100
 *
 * Clamps result between m_fMinValue and m_fMaxValue.
 * Native Joymax assertion verified at line 301 (0x12D):
 *   "CLAMP() ==> min(%.3f) exceeded max(%.3f) value), File: %s, Line: %d"
 */
void CParamElement::RecalculateValue() {
	if (!m_bDirty) {
		return;
	}

	float fModifiers[4] = { 0.0f, 0.0f, 0.0f, 100.0f };
	bool bHasModifiers = false;

	for (int i = 0; i < 4; ++i) {
		if (m_apModifiers[i] != nullptr) {
			auto* bucket = static_cast<ModifierBucket*>(m_apModifiers[i]);
			for (const auto& entry : *bucket) {
				const long double value = entry.second;
				if (i <= 1) {
					fModifiers[i] = static_cast<float>(static_cast<long double>(fModifiers[i]) + value);
				} else if (i == 2) {
					const long double factor = 1.0L + value / 100.0L;
					fModifiers[i] = static_cast<float>(fModifiers[i] == 0.0f ? factor : factor * fModifiers[i]);
				} else {
					fModifiers[i] = static_cast<float>(static_cast<long double>(fModifiers[i]) * value / 100.0L);
				}
			}
			bHasModifiers = true;
		}
	}

	if (!bHasModifiers) {
		m_bDirty = 0;
		m_fCalculatedValue = m_fBaseValue;
		return;
	}

	float fBase = (m_apModifiers[0] != nullptr) ? fModifiers[0] : m_fBaseValue;
	float fResult = static_cast<float>((static_cast<long double>(fModifiers[1]) + 100.0L) * fBase / 100.0L);

	if (fModifiers[2] != 0.0f) {
		if (fResult != 0.0f) {
			fResult = fResult * fModifiers[2];
		} else {
			fResult = fModifiers[2];
		}
	}

	fResult = static_cast<float>(static_cast<long double>(fModifiers[3]) / 100.0L * fResult);

	// CORRECTION (Claude): native line 301 is the BSLib CLAMP macro (0x004B32F4..0x004B3383). On min > max it
	// logs, takes min and then still applies the max test; the old expansion skipped the max test.
	CLAMP(fResult, m_fMinValue, m_fMaxValue);
	m_fCalculatedValue = fResult;

	m_bDirty = 0;
}

/**
 * [RECONSTRUCTED - 0x004B3000]
 * CParamElement_AddModifier
 *
 * Adds or updates a modifier bucket entry, sets dirty flag, and propagates to dependents.
 */
bool CParamElement::AddModifier(int32_t nModifierType, uintptr_t source, float fValue) {
	if (nModifierType < 0 || nModifierType >= 4) {
		return false;
	}
	if (fValue == m_fInitialValue) {
		return false; // +2C is an ignored-write sentinel, not an erase request.
	}

	if (m_apModifiers[nModifierType] == nullptr) {
		m_apModifiers[nModifierType] = new ModifierBucket;
	}
	(*static_cast<ModifierBucket*>(m_apModifiers[nModifierType]))[source] = fValue;

	m_bDirty = 1;
	PropagateDependentParameters();
	return true;
}

uint8_t CParamElement::RemoveModifier(uintptr_t source) {
	uint8_t removed = 0;
	for (unsigned channel = 0; channel < 4; ++channel) {
		auto* bucket = static_cast<ModifierBucket*>(m_apModifiers[channel]);
		if (bucket && bucket->erase(source)) removed |= static_cast<uint8_t>(channel + 1);
	}
	if (removed) {
		m_bDirty = 1;
		PropagateDependentParameters();
	}
	return removed;
}

void CParamElement::AddDependent(CParamElement* target, int32_t channel) {
	if (target && channel >= 0 && channel < 4) m_mapDependent.emplace(target, channel);
}

/**
 * [RECONSTRUCTED - 0x004B3130]
 * CParamElement_PropagateDependentParameters
 *
 * Cascades value update to all registered dependent parameter nodes.
 */
void CParamElement::PropagateDependentParameters() {
	for (auto& pair : m_mapDependent) {
		if (pair.first != nullptr) {
			// Preserve the whole host address; native uses the whole x86 address.
			pair.first->AddModifier(pair.second, reinterpret_cast<uintptr_t>(this), GetFloatValue());
		}
	}
}

// ============================================================================
// CGParamKeeper Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x004B33E0]
 * CGParamKeeper_constructor
 *
 * Machine operations:
 *   Zeroes 0x800 bytes (512 pointer slots) at +0x04..+0x803 via CRT_memset.
 *   Initializes m_dwParamCount = 0, m_pParamPool = nullptr, m_bDirty = 0.
 */
CGParamKeeper::CGParamKeeper()
	: m_pOwner(nullptr)
	, m_dwParamCount(0)
	, m_pParamPool(nullptr)
	, m_dwReserved(0)
	, m_bDirty(0) {
	std::memset(m_apParams, 0, sizeof(m_apParams));
	std::memset(m_pad811, 0, sizeof(m_pad811));
}

/**
 * [RECONSTRUCTED - 0x004B3460]
 * CGParamKeeper_destructor
 */
CGParamKeeper::~CGParamKeeper() {
	Cleanup();
}

/**
 * [RECONSTRUCTED - 0x004B3740]
 * CGParamKeeper_GetParamFloat
 *
 * Machine Operations:
 *   1. Bounds check: if (wParamID >= 0x200) -> ServerFramework_GenerateMiniDump()
 *   2. Ensures parameter element exists via EnsureParamInitialized
 *   3. Dereferences m_apParams[wParamID] -> CParamElement_GetFloatValue()
 */
float CGParamKeeper::GetParamFloat(uint16_t wParamID) {
	if (wParamID >= MAX_KEEPER_PARAMS) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0.0f;
	}

	CParamElement* pElem = EnsureParamInitialized(wParamID);
	if (!pElem) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0.0f;
	}

	return pElem->GetFloatValue();
}

/**
 * [RECONSTRUCTED - 0x004B3510]
 * CGParamKeeper_SetParamFloat
 *
 * Ensures parameter element is initialized, delegates to AddModifier, and sets keeper dirty flag (+0x810).
 */
bool CGParamKeeper::SetParamFloat(uint16_t wParamID, int32_t nModifierType, uintptr_t source, float fValue) {
	if (wParamID >= MAX_KEEPER_PARAMS) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	CParamElement* pElem = EnsureParamInitialized(wParamID);
	if (!pElem) {
		return false;
	}

	bool bResult = pElem->AddModifier(nModifierType, source, fValue);
	m_bDirty = 1; // +0x810
	return bResult;
}

/**
 * [RECONSTRUCTED - 0x004B3850]
 * CGParamKeeper_EnsureParamInitialized
 *
 * Lazily allocates CParamElement at slot wParamID if not yet instantiated.
 */
CParamElement* CGParamKeeper::EnsureParamInitialized(uint16_t wParamID) {
	if (wParamID >= MAX_KEEPER_PARAMS) {
		return nullptr;
	}

	if (m_apParams[wParamID] == nullptr) {
		// 4B3960 registers the complete sentinel-terminated native definition
		// table. Undefined IDs are errors, not parameters with invented bounds.
		struct Definition { uint16_t id; int32_t min, max, base, ignore; };
		static constexpr Definition definitions[] = {
#include "ParameterDefinitions.inc"
		};
		const Definition* definition = nullptr;
		for (const auto& d : definitions) if (d.id == wParamID) { definition = &d; break; }
		if (!definition) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			return nullptr;
		}
		auto* pElem = new CParamElement();
		pElem->m_dwID = wParamID;
		pElem->Init(static_cast<int16_t>(wParamID), static_cast<float>(definition->min),
		    static_cast<float>(definition->max), static_cast<float>(definition->base),
		    static_cast<float>(definition->ignore));
		m_apParams[wParamID] = pElem;
		m_parameterOrder.push_back(pElem);
		m_dwParamCount++;
	}

	return m_apParams[wParamID];
}

/**
 * [RECONSTRUCTED - 0x004B3450]
 * CGParamKeeper_ResetDirty
 * Sets m_bDirty = 0 (+0x810).
 */
void CGParamKeeper::ResetDirty() {
	m_bDirty = 0;
}

void CGParamKeeper::RemoveSourceModifiers(uintptr_t source) {
	bool changed = false, speedChanged = false;
	for (auto* parameter : m_parameterOrder) {
		if (!parameter || !parameter->RemoveModifier(source)) continue;
		changed = true;
		if (source != 6 && (parameter->m_wFlags == 0x17 || parameter->m_wFlags == 0x18))
			speedChanged = true;
	}
	auto* owner = static_cast<CGObjChar*>(m_pOwner);
	if (owner && speedChanged) owner->RefreshMovementSpeeds();
	if (owner && changed && owner->IsPlayer()) owner->SendParameterStats();
	m_bDirty = 1;
}

/**
 * [RECONSTRUCTED - 0x004B3460]
 * CGParamKeeper_Cleanup
 * Frees all allocated CParamElement instances and zeroes table.
 */
void CGParamKeeper::Cleanup() {
	m_parameterOrder.clear();
	for (uint16_t i = 0; i < MAX_KEEPER_PARAMS; ++i) {
		if (m_apParams[i] != nullptr) {
			delete m_apParams[i];
			m_apParams[i] = nullptr;
		}
	}
	m_dwParamCount = 0;
	m_bDirty = 0;
}
