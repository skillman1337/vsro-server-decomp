from pathlib import Path
import os, sys
root=Path(__file__).resolve().parents[1]
os.chdir(root)
sys.path.insert(0, str(root))
from tools.silent_process import COMMON_INCLUDES, COMMON_LIBS, find_compiler, run_silent

cxx=Path(find_compiler())
os.environ["PATH"]=str(cxx.parent)+os.pathsep+os.environ["PATH"]
# Run run-visibility.py first: this runner consumes that coherent build.
objects=sorted((root/"build/visibility-audit/objects").glob("*.o"))
objects=[str(p) for p in objects if p.name!="SR_GameServer_SR_GameServer.cpp.o"]
incs=COMMON_INCLUDES
exe=root/"build/visibility-audit/linked-teardown.exe"
run_silent([str(cxx),"-std=c++20",*["-I"+i for i in incs],"tests/linked-teardown.cc",*objects,*COMMON_LIBS,"-o",str(exe)],check=True)
run_silent([str(exe),sys.argv[1]],check=True)
