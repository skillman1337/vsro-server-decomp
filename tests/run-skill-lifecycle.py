"""Link lifecycle probes against the coherent run-visibility.py build."""
from pathlib import Path
import os
import sys

root = Path(__file__).resolve().parents[1]
os.chdir(root)
sys.path.insert(0, str(root))
from tools.silent_process import COMMON_INCLUDES, COMMON_LIBS, find_compiler, run_silent

cxx = Path(find_compiler())
os.environ['PATH'] = str(cxx.parent) + os.pathsep + os.environ['PATH']
objects = sorted((root / 'build/visibility-audit/objects').glob('*.o'))
objects = [str(p) for p in objects if p.name != 'SR_GameServer_SR_GameServer.cpp.o']
incs = COMMON_INCLUDES
cases = [('timed-job-lifecycle', sys.argv[1]), ('character-periodic-jobs', sys.argv[2])]
cases.append(('timed-job-record', None))
cases.append(('timed-job-restore', None))
cases.append(('death-retirement', None))
cases.append(('persistent-callback-order', None))
cases.append(('persistent-migration', None))
cases.append(('real-modifiers', None))
cases.append(('hit-resource-gates', None))
cases.append(('event-registration', None))
cases.append(('actor-async-integration', None))
cases.append(('equipment-durability', '../../rebuild/apps/server/docs/evidence/native-equipment-durability-20260921.txt'))
cases.append(('inventory-slots', '../../rebuild/apps/server/docs/evidence/native-inventory-slots-20260921.txt'))
cases.append(('actor-async-jobs', '../../rebuild/apps/server/docs/evidence/native-actor-jobs-20260921.txt'))
cases.append(('event-arguments', '../../rebuild/apps/server/docs/evidence/native-event-arguments-20260921.txt'))
cases.append(('event-dispatch', '../../rebuild/apps/server/docs/evidence/native-event-dispatch-20260921.txt'))
cases.append(('resource-offsets', '../../rebuild/apps/server/docs/evidence/native-resource-offsets-20260920.txt'))
cases.append(('deferred-instructions', '../../rebuild/apps/server/docs/evidence/native-deferred-instructions-20260921.txt'))
cases.append(('berserk-points', '../../rebuild/apps/server/docs/evidence/native-berserk-points-20260920.txt'))
cases.append(('prepared-costs', '../../rebuild/apps/server/docs/evidence/native-prepared-costs-20260920.txt'))
if len(sys.argv) > 3:
    cases.append(('parameter-retirement', sys.argv[3]))
for name, fixture in cases:
    exe = root / ('build/visibility-audit/' + name + '.exe')
    run_silent([str(cxx), '-std=c++20', *['-I' + i for i in incs],
                'tests/' + name + '.cc', *objects, *COMMON_LIBS, '-o', str(exe)], check=True)
    run_silent([str(exe), *([fixture] if fixture else [])], check=True)
