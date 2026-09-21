"""Run through Binary Ninja MCP execute with `binja` supplied.
Exports recognized-function DFS. Analysis indirect targets are discovery hints,
not RTTI/dispatch proofs. No annotations, byte patches or save operations.
"""
def export_storageop(binja=binja):
    import json, re, struct
    bv = binja._bv
    if not bv.file.filename.lower().endswith("sr_gameserver.exe.bndb"):
        raise ValueError("Select the research-server database explicitly")
    table = 0xaec6ec
    roots = list(struct.unpack("<45I", bv.read(table, 180)))
    pending = list(reversed(roots)); seen = set(); order = []; edges = []
    while pending:
        va = pending.pop()
        if va in seen: continue
        seen.add(va)
        f = bv.get_function_at(va)
        if f is None: continue
        order.append(va)
        for block in sorted(f.basic_blocks, key=lambda b:b.start):
            for line in block.disassembly_text:
                text = str(line)
                if not re.match(r"\s*(call|jmp)\s", text): continue
                targets = list(bv.get_callees(line.address, f))
                direct = re.search(r"(?:call|jmp)\s+(?:sub_)?(?:0x)?([0-9a-f]{6,8})\b", text)
                if direct:
                    target = int(direct.group(1),16)
                    if target not in targets: targets.append(target)
                local = all(any(b.start <= t < b.end for b in f.basic_blocks) for t in targets) if targets else False
                edges.append(dict(function=va,site=line.address,asm=text,analysisTargets=targets,localTargetsOnly=local))
                if not local:
                    pending.extend(reversed([t for t in targets if bv.get_function_at(t) is not None]))
    kinds = ("psuedo", "asm", "hlil", "mlil", "llil")
    diagnostics = []; machine = []
    for start in range(0,len(order),100):
        outputs = {k:[] for k in kinds}
        for va in order[start:start+100]:
            f=bv.get_function_at(va)
            heading="\n\n=== %08X %s | %s ===\n" % (va,f.name,f.type)
            pseudo="\n".join(str(x) for x in f.language_representation("Pseudo C").get_linear_lines(f.hlil.root))
            outputs["psuedo"].append(heading+pseudo)
            outputs["asm"].append(heading+"\n".join("%08X %s"%(x.address,x) for b in sorted(f.basic_blocks,key=lambda b:b.start) for x in b.disassembly_text))
            for kind in ("hlil","mlil","llil"):
                outputs[kind].append(heading+"\n".join("%d %08X %s %s"%(x.instr_index,x.address,x.operation,x) for x in getattr(f,kind).instructions))
            diagnostics.append(dict(va=va,name=f.name,questionLines=[x for x in pseudo.splitlines() if chr(0x2753) in x],indirectBranches=[(str(a),b) for a,b in f.unresolved_indirect_branches]))
            machine.append(dict(va=va,name=f.name,signature=str(f.type),blocks=[dict(start=b.start,end=b.end,bytes=bv.read(b.start,b.end-b.start).hex()) for b in sorted(f.basic_blocks,key=lambda b:b.start)]))
        for kind in kinds:
            binja.write_file("CGStorageOP-%s-%d.txt"%(kind,start),"".join(outputs[kind]))
    binja.write_file("CGStorageOP-replay-manifest.json",json.dumps(dict(database=bv.file.filename,table=table,roots=roots,order=order,edges=edges,diagnostics=diagnostics,functions=machine),indent=2))
    print("Exported",len(order),"recognized functions; unresolved indirect dispatch remains explicit")
export_storageop()
