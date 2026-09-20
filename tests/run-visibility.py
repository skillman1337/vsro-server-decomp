from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import subprocess, os
root=Path(__file__).resolve().parents[1]
os.chdir(root)
cxx=Path("D:/msys64/mingw64/bin/g++.exe")
os.environ["PATH"]=str(cxx.parent)+os.pathsep+os.environ["PATH"]
out=root/"build/visibility-audit/objects";out.mkdir(parents=True,exist_ok=True)
incs=[".","Common/Framework","JMX_Library/BSLib","JMX_Library/EngineCommon","JMX_Library/PathFindEngine","JMX_Library/NavMesh_new","JMX_ServerFramework/ServerFramework","SR_GameServer","ServerCommon"]
flags=["-std=c++20"]+["-I"+i for i in incs]
def compile(p):
 obj=out/(str(p).replace("\\","_").replace("/","_")+".o")
 r=subprocess.run([str(cxx),*flags,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
 if r.returncode: raise RuntimeError(str(p)+"\n"+r.stderr)
 return str(obj)
files=[p for p in Path(".").rglob("*.cpp") if p.parts[0] not in ("build","tests")]
with ThreadPoolExecutor(max_workers=4) as pool: objs=list(pool.map(compile,files))
libs=["-lws2_32","-liphlpapi","-lgdi32","-lcomctl32","-lodbc32"]
subprocess.run([str(cxx),"-mconsole","-o",str(out.parent/"server.exe"),"-Wl,--start-group",*objs,"-Wl,--end-group",*libs],check=True)
print(f"Built and linked {len(files)} translation units",flush=True)
objs=[o for p,o in zip(files,objs) if p.name!="SR_GameServer.cpp"]
exe=out.parent/"visibility-test.exe"
subprocess.run([str(cxx),*flags,"tests/visibility-grid.cc",*objs,*libs,"-o",str(exe)],check=True)
subprocess.run([str(exe)],check=True)
