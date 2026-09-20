"""Link lifecycle probes against the coherent run-visibility.py build."""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
os.chdir(root)
cxx = Path('D:/msys64/mingw64/bin/g++.exe')
os.environ['PATH'] = str(cxx.parent) + os.pathsep + os.environ['PATH']
objects = sorted((root / 'build/visibility-audit/objects').glob('*.o'))
objects = [str(p) for p in objects if p.name != 'SR_GameServer_SR_GameServer.cpp.o']
incs = ['.', 'Common/Framework', 'JMX_Library/BSLib', 'JMX_Library/EngineCommon',
        'JMX_Library/PathFindEngine', 'JMX_Library/NavMesh_new',
        'JMX_ServerFramework/ServerFramework', 'SR_GameServer', 'ServerCommon']
cases = [('timed-job-lifecycle', sys.argv[1]), ('character-periodic-jobs', sys.argv[2])]
cases.append(('timed-job-record', None))
cases.append(('timed-job-restore', None))
cases.append(('death-retirement', None))
cases.append(('persistent-callback-order', None))
cases.append(('persistent-migration', None))
cases.append(('real-modifiers', None))
cases.append(('resource-offsets', '../../rebuild/apps/server/docs/evidence/native-resource-offsets-20260920.txt'))
cases.append(('deferred-instructions', '../../rebuild/apps/server/docs/evidence/native-deferred-instructions-20260921.txt'))
cases.append(('berserk-points', '../../rebuild/apps/server/docs/evidence/native-berserk-points-20260920.txt'))
cases.append(('prepared-costs', '../../rebuild/apps/server/docs/evidence/native-prepared-costs-20260920.txt'))
if len(sys.argv) > 3:
    cases.append(('parameter-retirement', sys.argv[3]))
for name, fixture in cases:
    exe = root / ('build/visibility-audit/' + name + '.exe')
    subprocess.run([str(cxx), '-std=c++20', *['-I' + i for i in incs],
                    'tests/' + name + '.cc', *objects, '-lws2_32', '-liphlpapi',
                    '-lgdi32', '-lcomctl32', '-lodbc32', '-o', str(exe)], check=True)
    subprocess.run([str(exe), *([fixture] if fixture else [])], check=True)
