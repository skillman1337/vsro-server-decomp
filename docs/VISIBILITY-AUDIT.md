# Native visibility audit ? 2026-09-19

Target: this native C++ reconstruction, not rebuild/apps/server.
Native research executable: SR_GameServer.exe, SHA256
`bec2375e2c4c1073e3bf7761571470c430de251de74b452dbb86537348ef5290`.
This research server is not established as the same build as client v1.150.
Machine evidence checked with Rizin and the active Binary Ninja server database;
HLIL with unresolved/custom-register calling conventions is only a navigation aid.

## Implemented in this pass

- CRgnTerrain's 36 owned blocks, 320-unit coordinate lookup, explicit layer allocation,
  and eight-direction cross-region block links. This host representation is not
  an ABI/byte-exact reconstruction. Region references are borrowed; the owner must
  keep all linked regions alive and finish linking before admitting objects.
- Layer counters initialize to zero, including inline layer zero.
- Out-of-range layer lookup throws instead of silently returning layer zero.
  Host exceptions replace native dump-plus-checked-container failure.
- Non-finite terrain lookup rejects before floating-to-integer conversion.
- Correct terrain runtime descriptor: native descriptor 0xADE954, size 0x158C,
  parent CRegion; host object size is different.

Evidence: constructor 0x53A850; block initialization 0x53A9D0 -> terrain vtable
0xAF7FD8 slot 1 (entry 0xAF7FDC, target 0x537B60) -> base 0x5336A0;
layer initialization 0x533190 / 0x534980; checked lookup 0x533340;
position lookup 0x53AD30; neighbour association 0x53AAB0;
block links 0x53AB90 / 0x53AC20 / 0x53ACB0; signed direction table 0xC64090.

Run `python tests/run-visibility.py` (four compiler workers). Outputs stay in
build/visibility-audit; the existing executable is not replaced. This builds all
105 existing translation units and links a separate assertion-based test binary.
Tests cover each cell, exact and immediately preceding boundaries, invalid
coordinates/layers, initialized counts, directional and diagonal cross-region
links, duplicate-free relinking, and runtime identity.

## Correction to the junior report

0x534050 is **CMsgBlock_BroadcastObjectDespawn**, not RemoveObjectFromLayer.
Its assembly allocates opcode 0x3016 at 0x53407F, writes the object's +8 ID as
four bytes at 0x534099..0x5340A1, appends recipients at 0x5340AC, commits the
packet cursor, and dispatches through network engine +0x4C. It also decrements
the layer count for players, even if the local recipient map is empty.
The Binary Ninja label and function/branch comments were corrected and saved
in snapshot 29; the symbol was checked in that snapshot's PE/symbols data.
Original annotations were exported through MCP as visibility-534050-before.json.
Executable bytes at the corrected function were compared before/after unchanged.

The C++ map-erase helper is now explicitly marked as port bookkeeping, not an
implementation of that native VA. Do not reuse the previous false VA attribution.

## Still NOT completed ? do not deploy as working visibility

1. CMap::Load does not construct/populate CWorldMap regions from authoritative
   region/world definitions. GameWorldMgr's world registry is also unpopulated.
   New terrain geometry is available but not yet called by startup. Do not infer
   every enabled navigation region belongs to every game world.
2. Native region admission, proxy handoff, object/dungeon regions and climate
   loading remain missing. Do not force those branches into terrain geometry.
3. StepMovement currently commits locations while discarding block membership
   when regions change. It needs native admission and ownership transitions,
   including mode 3 and virtual callbacks; simply clearing the pointer leaks membership.
4. Enter/leave packets are absent. 0x534340 / 0x5344B0 use neighbour-difference
   lists; 0x533E70 emits grouped 0x3017/0x3019/0x3018 updates, with different
   spawn/despawn collectors. 0x533F70 emits 0x3015 using object serializer +0x498,
   and modes 5/6 append an additional field. Serializer families need recovery.
5. wPCCount is NOT safely modeled as mapPC.size(): native spawn/despawn helpers
   update other blocks' counters, and enter/leave also change the local counter.
   Recover the complete transition before normalizing this apparent duplication.
6. CPacket::Send only logs and returns success; CNetEngine::Send is a stub.
   Sending to object GID is not native client-context/agent routing. The real
   CAgentMsg dispatcher groups agent envelopes 0x220A/0x220E/0x220F. No actual
   network delivery was validated. ResetReadCursors is not a proven replacement
   for native write-cursor commitment.

No live visibility qualification or full native parity is claimed. A clean link
and zero unnamed callees do not establish behavioral completeness.
