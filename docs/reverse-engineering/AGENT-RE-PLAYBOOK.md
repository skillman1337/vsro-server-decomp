# Reverse-engineering worker playbook: first hurdles

Version 1 ? 2026-09-21. Supplements the task packet and applicable AGENTS.md;
does not widen write permissions, waive queue/proof gates, or authorize shared BN edits.
Use this for candidate research. Production promotion belongs to the coordinator.

## 1. Default: make a bounded decision and keep working

Do not ask permission for a choice already settled by machine evidence or this
playbook. Record the evidence, choose the least assumptive faithful representation,
and continue independent work. Unknown semantics need not block known mechanics.
An unresolved dependency must never be hidden by a successful no-op.

| Hurdle | Default action | Escalate only when |
| --- | --- | --- |
| Field purpose unknown, bytes known | Use an offset-based research name; document reads/writes | Purpose affects required lifecycle or representation and cannot be bounded |
| Strange sentinel or no-match result | Preserve it in the native candidate | Conflicting machine evidence prevents a contract |
| Virtual call | Trace receiver, table, slot and target; preserve alternatives | An unresolved target changes required behavior |
| Object setup needed for emulation | Construct documented memory fixtures; run original target bytes | Required environment cannot be modeled without replacing behavior under test |
| Host representation lacks identity/ownership | Document the precise prerequisite; continue native work | Integration requires a canonical design decision beyond assigned scope |
| Pseudocode question mark | Inspect ASM/LLIL and ABI first | A demonstrated stack/control-flow conflict remains after bounded investigation |
| CRT/STL child expands the graph | Establish the utility contract needed by the caller | Its effects, exceptions or ownership materially remain unknown |
| Test discrepancy | Retain failure; reopen evidence and version the candidate | A genuine ambiguity survives static review |

First inspect complete assigned functions, referenced callees/tables, existing
canonical declarations and relevant callers. Batch remaining questions in the
receipt with evidence; do not stop the whole packet on the first uncertain name.
Do not silently expand a four-function pilot into a whole-server reconstruction.

## 2. Keep four kinds of claims separate

1. Machine fact: instruction addresses, widths, offsets, comparisons and effects.
2. ABI contract: receiver/register inputs, stack arguments, cleanup and defined return bits.
3. Semantic interpretation: what an identifier, field or operation means.
4. Host adaptation: how C++/Go represents the established behavior.

A load of two words proves a 64-bit bit pattern. It does not by itself prove database
uniqueness, persistence, allocator origin, lifetime or an owning class.
Name an unproven field `field20_24` or a similarly neutral bounded projection.
A query-specific name such as `searchIdentity` is acceptable when its limited role
is explicit. Do not promote it to `Serial64` without producer/schema/lifecycle evidence.

Equal values at different offsets do not establish aliasing or interchangeable
identity. Trace initialization, setters, copying, clearing and consumers before
merging fields. Record unknown padding. Reuse canonical types; do not define another
body for a retail type to make a candidate compile. A research projection must stay
in its authorized lane and cannot become canonical ownership by declaration.

## 3. ABI before attractive pseudocode

For each function and call edge record:
- Consumed ECX/EDX/other register inputs, including custom conventions.
- Stack argument order, widths and aggregate layout.
- Callee cleanup and caller cleanup; ordinary RET N alone is insufficient.
- Defined return bits and registers: AL, EAX, EDX:EAX, x87, or none.
- Preserved registers, tailcalls, exceptional exits and stack realignment.

A function defining only AL has an 8-bit result; upper EAX is not a promised result.
Use an 8-bit semantic return where callers support it. For machine comparisons,
compare defined result bits plus required preserved state. Exact byte proof has its
own stronger ABI/compiler obligations. Do not manufacture upper-bit guarantees,
unused fastcall arguments, noreturn attributes or ESP overrides to satisfy a test.

Question marks may represent indirect tailcalls or jump tables, not stack damage.
Verify prototypes, per-site cleanup, stack probes and saved registers before proposing
a repair. Do not change the shared BN database; submit evidence-bound annotations.

## 4. Preserve native edge behavior

The candidate is not an API redesign. Keep asymmetric failure values, half-open
ranges, byte truncation, wraparound, signedness, null handling, branch ordering,
side-effect ordering and unusual fallthrough results.

Do not automatically translate sentinel-return functions to optional/bool APIs.
A separate host wrapper may be proposed only if it preserves every distinction its
callers need and retains the original contract. If zero can mean both an unsupported
kind and a valid slot, an optional cannot recover the lost distinction by itself.

Audit full exits, including the value left by the last loop iteration. A comment
saying 'find' is not proof that failure returns null. Diagnostic paths need their own
contract; do not assume a minidump/log call terminates execution.

## 5. Resolve dispatch without inventing it

For a virtual edge, retain receiver provenance, adjustment/secondary-base effects,
actual table base, slot index/offset, entry VA, target VA and raw pointer bytes.
Check table boundaries, RTTI and nearby references. An address inside .text alone
is not vtable evidence. A static table entry proves that table's target, not that
all possible receivers use it. Do not infer missing subclasses from a convenient
three-table sample. Mark the verified subtype domain and unresolved alternatives.

Use direct graph edges and proven finite dispatch for dependency closure. Keep
analysis-proposed targets separate. Exclude intra-function jumps from child-function
counts. Give shared dependencies one owner, with a reusable contract; do not have
multiple workers independently redefine the same class or utility.

## 6. Native oracle fixtures versus behavioral mocks

Preferred method:
1. Pin and hash-check the original PE; map code and data at documented addresses.
2. Construct synthetic objects/records in separate test memory using proven layouts.
3. Point their vptrs at original mapped retail tables whenever possible.
4. Populate the record fields read by the actual native accessor.
5. Execute original caller, dispatch and callee bytes unchanged.

Synthetic input memory is a fixture, not a replacement implementation. It establishes
bounded behavior for those input states, not native constructor/lifetime correctness.
A synthetic vtable pointing to the original accessor may be used for an explicitly
bounded call-contract test, but does not establish retail vtable provenance.
A newly written accessor stub is a behavioral mock: it cannot qualify that dependency
or justify an end-to-end native comparison. Keep such unit tests separately labeled.

Record code/data hashes, mappings, receiver tables, executed targets, arguments,
return masks, ESP cleanup, required preserved registers and memory effects. Initialize
state deterministically for each case; poison and vary unconstrained values where
useful. Fail on unexpected calls, unmapped access, incomplete execution or instruction
budget exhaustion. Declare every replaced external boundary and the resulting limits.

Derive and freeze the contract and case matrix from static machine evidence before
comparison. Include boundary, no-match, duplicate-match, aliasing and identity-half
cases. Test diagnostic domains separately from valid-container domains. If a run
contradicts the model, retain it, revise from static evidence with a recorded version,
and rerun qualification. Never edit expected results just to match the candidate.

Mutations must discriminate behavior: wrong boundary, sentinel, argument order,
identity half, dispatch target, ownership effect or cleanup. Record exactly which
check rejects each mutation. A finite passing suite is not whole-component equivalence.

## 7. Go/C++ integration requires representable semantics

Pointer identity, runtime GID, reference-data ID, persistent item identity and slot
number are different concepts. Do not substitute one for another without proof.
Go value equality or an address of a copied struct is not native object identity.
A map key/slot is not automatically stable when items move, split, merge or reconnect.

Before implementing an identity-based operation, inspect creation, copy/move,
serialization, save/load, deletion, split/merge and reconnect. If the necessary identity
is absent, provide a prerequisite design, not a fabricated implementation. Specify
field width, assignment authority, uniqueness domain, lifetime, migration policy,
transaction/rollback effects and affected call sites; leave unproven choices open.

A candidate generic callback adapter may help demonstrate known mechanics, but label
it as an unintegrated boundary. It does not close the production prerequisite.
Do not add a cross-project schema/factory redesign in a bounded read-only pilot.
Continue the native candidates/tests and document the precise integration dependency.

Keep .h declarations separate from .cpp implementations. Mark partial/stub functions
immediately above their definitions. Refusal is preferable to a false success but is
still an unimplemented path, not native parity.

## 8. Specific rulings for the storage-search pilot

These rulings are scoped to the supplied evidence and must be byte-checked by the worker.

1. Slot +380 -> 48F6C0: model the returned record20/24 bit pattern with a neutral
   accessor/projection. The supplied loads establish mechanics, not durable serial
   semantics. Keep recordC0/C4 (48F6D0) separate. No coordinator approval is needed
   to reconstruct the search while naming the unknown conservatively.
2. 4B9FF0 unsupported-kind AL0 versus 4BA040 miss ALFF: preserve the asymmetry.
   Do not replace it with optional or normalize both failures. Document byte return.
3. Emulation: use actual native tables and actual 48F6C0 with synthetic item/record
   memory. Do not replace the accessor with a hand-written 'equivalent' stub for
   native qualification. This qualifies the bounded dispatch path, not all item life.
4. Go: both pointer-based slots 15/16 AND 64-bit slots 17/18 require representation
   checks. Do not promise FindItemPointer if Go stores copied item values without
   stable object identity. Deliver supported candidates only; otherwise document
   each exact prerequisite and finish the native part of the packet.
5. Do not repeat the proposed labels 'kinds 2,3,4 = chest/guild/cube'. Prior factory
   evidence at 4A6A50 maps 2 COS, 3 chest, 4 guild chest, and 6 magic-cube storage;
   verify via static objects and RTTI before adopting names. Numeric branch behavior
   remains provable independently of those names. Kind5 is avatar in that mapping.

## 9. Escalation packet: one compact record per genuine blocker

Use:
- Claim needed and why it blocks a required result.
- Exact VA/call site/field, binary hash and machine evidence.
- Observed facts versus hypotheses.
- Work already performed and falsified hypotheses.
- Narrowest unresolved question and candidate resolutions.
- Independent work completed; artifacts and tests retained.

Do not ask the coordinator whether to preserve proven native behavior, whether to
avoid fake identity, or whether a documented fixture is allowed: this playbook answers
those questions. Escalate conflicting evidence, missing authority, unresolved material
dispatch and shared canonical changes outside the packet.

## 10. Handoff and acceptance

Supply a function/claim matrix: extracted, ABI-established, semantic confidence,
candidate path, compiled, native-compared domain, mutations and unresolved dependencies.
Reuse the task's evidence/annotations/receipt schema. Never add dozens of mandatory
narrative files or rerun unrelated broad suites per leaf.

The reviewer should be able to reproduce each claim from its addresses/hashes and
commands. Keep all failures. Zero sub_ names, a clean compile, a saved BNDB or an
agreement between two LLMs is not proof. Apply the repository's exact-source and
promotion gates where relevant; this guide does not weaken them.


## 11. Mandatory acceptance checks learned from pilot review (revision 2)

These checks apply generally, not just to storage searches. They are acceptance
conditions; a narrative stating that they passed is insufficient.

### Machine-check the evidence manifest

- Verify `entryVa == tableVa + slotOffset` and `slotOffset == slotIndex * pointerWidth`.
  Read the target from the computed address in the pinned original PE and compare.
  A correct target at a separately supplied address cannot validate a wrong base.
- Validate every function span/hash and mark padding versus instruction intervals.
  Generate addresses and bytes from tools; do not transcribe them from memory.
- Include every dependency claimed as verified, including small accessors, with its
  own evidence and table provenance. A name in prose is not a dependency contract.
- Declare the reference base for every argument offset: entry ESP, frame EBP, or
  call-site ESP. Prefer normalized entry-ESP offsets and retain the original operand.
  Never mix EBP+8 with entry-ESP+4 under the same unqualified stackOffset key.
- Generate status and hash tables from current artifacts. Do not hand-maintain a
  stronger receipt than the machine-readable acceptance result.

### One case corpus, two executions, one comparison

- Define one versioned corpus with stable case IDs consumed by native and candidate
  runners. Emit normalized actual results, defined return bits, memory effects and
  native ABI observations. Compare those records automatically.
- Separate hand-written suites checking similar expected values are useful smoke
  tests, but are NOT the differential comparison deliverable.
- Include each branch boundary and caller wrapper independently. Cases must make a
  wrong branch observably different: distinct object tokens, guard values, callback
  traces or effects. Putting the same returned pointer at both candidate locations
  cannot establish which location was searched.
- Cover low-only and high-only identity matches with that item INSIDE the visited
  range, followed by a distinguishable result. A matching item outside the search
  range proves nothing about the comparison.
- Record actual ESP cleanup and callee-preserved registers, not comments claiming
  they were checked. Start cases with deterministic independent state and poison
  unconstrained registers. Assert unexpected native targets and memory writes.
- Include empty inputs, equivalent-boundary cases, signed extremes, byte truncation,
  duplicates, malformed-domain boundaries and dispatch alternatives as applicable.
  Explicitly exclude any untested domain from the acceptance claim.

### Independent mutation accounting

- Each mutation needs a source diff/hash, successful compilation, case ID, and a
  structured expected-versus-actual mismatch. A nonzero process exit alone is not
  proof of detection; DLL failure, timeout and unrelated crashes are infrastructure
  failures. Verify relevant diagnostics for deliberate trap tests separately.
- Mutation descriptions must match the actual changed expression. Exercise every
  assigned function, not just a similarly named sibling. Reviewers may add holdout
  mutations; a surviving material mutation reopens the affected claim.

### Keep native memory and host models separate

- Never read sizeof(host_pointer) bytes at a native 32-bit offset. A packed mock
  containing 64-bit pointers is a host fixture, not the retail representation.
  For native memory use explicit-width addresses and a memory reader; for a host
  semantic candidate use explicit fields or a declared accessor boundary.
- Do not add null guards or safe defaults absent from native and then call the
  result equivalent. Either preserve the behavior or specify a bounded nonnull
  domain and keep host validation outside the reconstructed function.
- Cast BEFORE wrapping arithmetic: uint32_t(kind) - 2u models a 32-bit unsigned
  subtraction; uint32_t(kind - 2) may already have invoked signed C++ overflow.
  Audit pointer arithmetic on malformed containers for host undefined behavior.
- A four-method C++ virtual interface is a semantic adapter, not a retail table
  with methods at slots 15 through 18. State that clearly. Keep implementations in
  .cpp as required by the task, including adapter overrides.

### Do not turn missing semantics into a schema prescription

An unproven native identity cannot become a proposed unique persistent Serial64,
a database sequence, a named SQL column, or a network field without producer/schema/
wire evidence. Runtime pointer identity and record-value identity remain separate
prerequisites. A candidate returning pointers into a relocating value slice does not
close stable identity just because another field was added. Unsupported host design
ideas belong in hypotheses, not a supposedly activation-ready patch.

### Promotion language

Use candidate/compiled/bounded-native-compared with the tested domain. Do not use
Complete Parity or all qualification gates passed unless the actual required gates
exist and pass. Keep evidence mistakes, candidate bugs, test blind spots and missing
integration separately visible so the coordinator can route each cheaply.

## 12. Scale workers behind a shared, adversarially tested gate

The v2 pilot's valid packet passed, but its validator also accepted missing-all-vtable
and duplicated-function manifests. Passing the happy path does not validate the gate.

- The coordinator freezes the assignment and validator version. Research workers
  supply candidates/evidence/cases; they do not redefine their own acceptance policy.
- Validate exact assigned VA sets and table-slot sets, uniqueness, schema versions,
  exact byte-read lengths, and claimed section/interval properties. Counts alone
  cannot establish closure. Test missing, duplicate, substituted and malformed records.
- Require native and candidate result IDs to equal the validated corpus IDs exactly.
  Reject unknown result kinds, duplicate IDs, unknown object tokens and missing fields.
  Use a real parser or coordinator-generated validated transport, not substring JSON.
- Bind receipts to source, corpus, native binary, compiler and gate identities. Retain
  structured mutation results. Verify observed native and baseline values agree with
  the required baseline, and that the mutant produces the distinct specified result.
- Compare relevant memory effects and callback/dispatch traces, not just return values.
  For read-only routines verify permitted writes explicitly; for mutators include
  state changes, ordering and failure effects. Declare bounded exclusions honestly.
- Missing fixture callbacks or unresolved dependencies must fail setup explicitly;
  they must not silently manufacture zero/null results in reconstructed behavior.
- Keep shared dependency ownership and BN annotation application centralized. Workers
  propose conflicting names/types with evidence; they do not race on shared databases.
- Review findings should route to the shared harness, evidence, candidate, or integration
  owner. Do not request another complete research rewrite for a harness-only defect.
- A Go pointer into a replaced slice backing array can remain memory-valid while no
  longer identifying the inventory's live logical item. Distinguish identity failure
  from memory invalidity, and do not invent a persistence schema to conceal either.

Start with three disjoint research batches plus one shared-tooling task, within the
four-worker default. Use the repository's queue/worktree and batching rules. Research
parallelism is not automatic semantic promotion; integration still requires canonical
ownership, ABI closure, and applicable component gates.
