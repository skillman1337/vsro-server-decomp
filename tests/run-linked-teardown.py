from pathlib import Path
import os,subprocess,sys
root=Path(__file__).resolve().parents[1]
os.chdir(root)
cxx=Path("D:/msys64/mingw64/bin/g++.exe")
os.environ["PATH"]=str(cxx.parent)+os.pathsep+os.environ["PATH"]
# Run run-visibility.py first: this runner consumes that coherent build.
objects=sorted((root/"build/visibility-audit/objects").glob("*.o"))
objects=[str(p) for p in objects if p.name!="SR_GameServer_SR_GameServer.cpp.o"]
incs=[".","Common/Framework","JMX_Library/BSLib","JMX_Library/EngineCommon","JMX_Library/PathFindEngine","JMX_Library/NavMesh_new","JMX_ServerFramework/ServerFramework","SR_GameServer","ServerCommon"]
exe=root/"build/visibility-audit/linked-teardown.exe"
subprocess.run([str(cxx),"-std=c++20",*["-I"+i for i in incs],"tests/linked-teardown.cc",*objects,"-lws2_32","-liphlpapi","-lgdi32","-lcomctl32","-lodbc32","-o",str(exe)],check=True)
subprocess.run([str(exe),sys.argv[1]],check=True)
