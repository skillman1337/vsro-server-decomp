// Portable ownership projection of 5A9AE0/5A9B50 and 5A9E60/5A9E80.
// Heap storage replaces native slabs. IDs remain nonzero, unique while live,
// and recyclable only after the execution context has released its children.
#include "SkillCast.h"
#include "../ServerCommon/ReferenceData.h"
#include <deque>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace {
std::mutex contextMutex;
std::deque<uint32_t> availableContexts;
uint64_t nextContext = 1;

uint32_t acquireContextID() {
    std::lock_guard lock(contextMutex);
    if (!availableContexts.empty()) {
        const auto id = availableContexts.front();
        availableContexts.pop_front();
        return id;
    }
    if (nextContext > std::numeric_limits<uint32_t>::max())
        throw std::overflow_error("skill execution identity space exhausted");
    return static_cast<uint32_t>(nextContext++);
}
}

tagSkillExecutionContext* tagSkillExecutionContext::Allocate() {
    auto* context = new tagSkillExecutionContext{};
    try { context->m_dwContextID = acquireContextID(); }
    catch (...) { delete context; throw; }
    context->m_pad0[4] = 1;
    context->m_byMode = 1;
    return context;
}

void tagSkillExecutionContext::Release(tagSkillExecutionContext*& context) {
    auto* value = context;
    context = nullptr;
    if (!value || !value->m_pad0[4]) return;
    value->m_pad0[4] = 0;
    delete value->m_pResultBatch;
    delete value->m_pPositionResult;
    delete value->m_pTravelTimer;
    delete value->m_pRepeatingCost;
    delete value->m_pPeriodicDamage;
    // 5A9A58: recipients borrow links; only source-mode contexts own them.
    if (value->m_byMode == 1) {
        delete value->m_pCastLink;
        delete value->m_pAreaLink;
    }
    if (value->m_dwContextID) {
        std::lock_guard lock(contextMutex);
        availableContexts.push_back(value->m_dwContextID);
    }
    delete value;
}

tagActiveSkillInstance* tagActiveSkillInstance::Allocate() {
    auto* instance = new tagActiveSkillInstance{};
    instance->m_bActive = 1;
    instance->m_dwRetirement = 1;
    return instance;
}

void tagActiveSkillInstance::Release(tagActiveSkillInstance*& instance) {
    auto* value = instance;
    instance = nullptr;
    if (!value || !value->m_bActive) return;
    value->m_bActive = 0;
    Skill::sSkillPreEngageData::Release(value->m_pCommand);
    tagSkillExecutionContext::Release(value->m_pExecution);
    Skill::sSkillPreEngageData::Release(value->m_pSecondaryCommand);
    tagSkillExecutionContext::Release(value->m_pSecondaryExecution);
    value->m_dwRetirement = 0;
    delete value;
}
