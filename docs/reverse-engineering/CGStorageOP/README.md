# CGStorageOP native discovery and implementation status

Incomplete recursive reconstruction. The five requested dumps are present, including
`CGStorageOP-psuedo.txt` with the requested spelling. The first export is retained in
`CGStorageOP-initial-dumps.zip`. The current dumps were regenerated after annotations.

## Identity and evidence

Research server SHA256:
`bec2375e2c4c1073e3bf7761571470c430de251de74b452dbb86537348ef5290`.
The research server is not asserted to be the v1.150 client build.

`CGStorageOP-machine.json.gz` retains 763 recognized functions / 8,706 basic-block
byte ranges checked against the original PE, structured entries for all 90 base/PC
slots, RTTI hierarchy descriptors and static operator objects. No executable bytes
were patched. Recognized direct and analysis-proposed indirect targets are discovery,
not a complete proof of all runtime dispatch. Local jumps are separately classified
in `CGStorageOP-replay-manifest.json.gz`.

Table AEC6EC has COL B55EDC and CGStorageOP -> IStorage RTTI. Table AEC7A4
has COL B55E8C and CGPCInventoryOP -> CGStorageOP -> IStorage RTTI.
Static objects C66A98/C66AA0 contain these table pointers and are selected by
4A6A50. Lack of a direct code xref to the table itself does not make it unused.

`tools/export-storageop-binja.py` (relative to the C++ project root) was executed
through MCP and reproduced the 763-function export. It writes chunks through
binja.write_file; concatenate each format's numbered chunks in numeric order.
`CGStorageOP-status.json` binds current dump hashes and unresolved counts.
`CGStorageOP-persistence.json` records independent reopening of the saved original
server BNDB, checking names, signatures, comments and bounded types, then closing
that verification view. A database annotation is not an exact-source proof.

## Corrections

- 4B9AB0 is a virtual release tail dispatcher, not an unresolved stack repair by fiat.
  Verified slot B0 typing makes its tailcall readable.
- 484FD0 clones an item-backed object and its record; the prior ProcessGameEvents
  name was false. 485E70 and 489730 construct objects from records, not collision
  queries. Unknown initialization arguments remain neutral.
- 48F990 writes the owner pointer at item+14C and caches nonnull owner's GID at
  item+154. A null owner leaves the cached GID intact. The recreation's despawn/drop
  field names at those offsets are not native-layout evidence and remain a conflict.
- 48F6D0 reads a 64-bit record field at C0/C4. Its identity semantics are unresolved;
  it must not be conflated with record20/24 or runtime GID.
- AED060 is the GStorage.cpp source-path string, not a SetItem function. Source
  comments falsely claiming that native implementation were corrected.
- Base diagnostic-only virtual methods are distinguished from PC overrides. A
  native Unsupported branch is not permission to replace the derived implementation.

## Implemented and checked subset

The portable C++ storage layer has ten bounded slot primitives. The Go inventory
has four corresponding range queries and uses FirstEmpty for bag-slot discovery.
The native-byte oracle contains 2,950 cases for ten native entries, including actual
native virtual Peek/Detach in MoveToEmpty. This is bounded valid-container testing,
not equivalence of the complete storage subsystem. Previous mutation evidence is
retained under build/inventory-mutation-20260921.

C++ item quantity accesses use the canonical partial item record instead of native
byte offsets applied to a host layout. The record setter implementation lives in
InstanceItem.cpp, with its declaration in InstanceItem.h. Partial operations are
marked above their functions. SplitItem refuses rather than returning false success;
SplitStack rejects a missing clone prerequisite before decrementing source stock.
That is a safety correction, NOT an implemented native split path.

Validation: all 109 C++ translation units compiled and linked; all 21 lifecycle
probes passed, including 2,950 inventory oracle rows and durability/quantity checks.
Focused Go inventory and combat packages passed. Logs are build/storageop-build-
20260921.log and build/storageop-lifecycle-20260921.log.

## Still required

Ten base slots remain without established semantic labels; 582 reachable functions
retain sub_ names. Five functions still render indirect-transfer question marks:
CRT memmove/memcpy jump tables, CRT invalid-parameter tail handler, and wrappers
439670/439690. No demonstrated stack imbalance justifies forcing ESP at those sites.

Unfinished implementation includes persistent record copy and identity assignment,
object factory registration and cleanup, actual owner binding, split/merge transaction
lifecycle, complete serializers and item subtype dispatch, companion/linked storage,
equipment integrity/set/rental callbacks and database closure. Go persistent item
identity is a prerequisite; RefObjID is not an item-instance identity.

Neither all 45 base operations, all PC overrides, nor their recursive descendants
are implemented/qualified. Do not promote this dossier to full native parity.
