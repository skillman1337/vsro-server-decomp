/**
 * ============================================================================
 * Silkroad Online - Skill Cast Lifecycle & Action Dispatch Subsystem
 *
 * Implements:
 *   - SkillActionHandler               (Native @ 0x00589B50)
 *   - Skill Action Dispatch Table      (Native @ 0x00C63C7C)
 *   - CastLifecycle_ProcessPersistent  (Native @ 0x005830B0, 9247 bytes)
 *   - SkillEffect_RetireContributionsAndLinks   (Native @ 0x005829D0, 1749 bytes)
 *   - CastLifecycle_ProcessInstant     (Native @ 0x00586700, 2424 bytes)
 *   - CastLifecycle_ProcessProjectile  (Native @ 0x005857B0, 2365 bytes)
 *   - CastLifecycle_ProcessContinuous  (Native @ 0x00587260, 80 bytes)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SKILLCAST_H_
#define _SR_GAMESERVER_SKILLCAST_H_

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include "Formulae.h"

class CGObjChar;
struct tagActiveSkillInstance;
namespace Skill { struct sSkillPreEngageData; }
struct tagSkillExecutionContext;

// Cast Lifecycle event codes matching native switch @ 0x005830E9
enum ECastEvent : uint32_t {
	CAST_EVENT_BEGIN           = 0,
	CAST_EVENT_TICK            = 2,
	CAST_EVENT_WRITE_MIGRATION = 4,
	CAST_EVENT_READ_MIGRATION  = 5,
	CAST_EVENT_CANCEL          = 6
};

// Cast Lifecycle outcomes returned by action handlers
enum ECastOutcome : int32_t {
	CAST_OUTCOME_KEEP           = 0, // Continue active execution, keep instance
	CAST_OUTCOME_SUCCESS        = 1, // Phase or migration processed successfully
	CAST_OUTCOME_RELEASE        = 2, // Action completed or rejected; release instance
	CAST_OUTCOME_RELEASE_NOTIFY = 3  // Cancelled; broadcast retirement and release
};

// Persistent cast state modes (pInstance->m_dwMode)
enum EPersistentMode : uint32_t {
	PERSISTENT_MODE_BEGIN    = 0, // Waiting for cast delay / initial distribution
	PERSISTENT_MODE_ACTIVATE = 1, // Activation, applying state changes & modifiers
	PERSISTENT_MODE_TICK     = 2  // Active ticking buff / aura / periodic pulse
};

#include "SkillManager.h"

// Area effect / aura link (Efr2 parameter)
struct tagAreaLink {
	uint32_t              m_dwSourceActorID;
	float                 m_fRadius;
	std::set<uint32_t>    m_memberIDs; // Native +14: unique recipient GIDs
};

// Function pointer signature for skill action handlers (table @ 0x00C63C7C)
// Native dispatch at 0x00589C0F pushes exactly 4 arguments: (dwEntryMode, pCaster, pInstance, pExtra)
typedef int32_t (*PFN_CAST_ACTION_HANDLER)(
	uint32_t dwEntryMode,
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance,
	void* pExtra
);

namespace SkillCast {
	// Master skill action dispatcher indexing table g_aSkillActionHandlers @ 0x00C63C7C
	int32_t SkillActionHandler(
		CGObjChar* pCaster,
		tagActiveSkillInstance* pInstance,
		uint32_t dwEvent,
		void* pExtra = nullptr
	);
}

// [RECONSTRUCTED - Native 0x00589B50] (270 bytes)
// Master skill action dispatcher indexing table g_aSkillActionHandlers @ 0x00C63C7C
inline int32_t SkillActionHandler(
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance,
	uint32_t dwEvent,
	void* pExtra = nullptr
) {
	return SkillCast::SkillActionHandler(pCaster, pInstance, dwEvent, pExtra);
}

// [RECONSTRUCTED - Native 0x005830B0] (9247 bytes)
// Category 3: Persistent & Buff Cast Handler
int32_t CastLifecycle_ProcessPersistent(
	uint32_t dwEntryMode,
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance,
	void* pExtra = nullptr
);

// [RECONSTRUCTED - Native 0x005829D0] (1749 bytes)
// Cleans up area links and companion links upon skill cancellation or expiration (returns 3)
int32_t SkillEffect_RetireContributionsAndLinks(
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance
);

// Subroutines of CastLifecycle_ProcessPersistent:
int32_t CastLifecycle_BeginPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance);
int32_t CastLifecycle_ActivatePersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance);
int32_t CastLifecycle_TickPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance);
int32_t CastLifecycle_DistributeTargets(CGObjChar* pCaster, tagActiveSkillInstance* pInstance, bool bMainDistribution);
int32_t CastLifecycle_SpawnPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance);

// Action category handlers matching native table at 0x00C63C7C:
int32_t CastLifecycle_ProcessInstant(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra = nullptr);     // [0] @ 0x00586700
int32_t CastLifecycle_ProcessProjectile(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra = nullptr);  // [1] @ 0x005857B0
int32_t CastLifecycle_ProcessContinuous(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra = nullptr);  // [4] @ 0x00587260

#endif // _SR_GAMESERVER_SKILLCAST_H_
