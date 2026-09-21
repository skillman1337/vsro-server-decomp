import os
import glob
import sys
from multiprocessing import Pool, cpu_count

ROOT = os.path.dirname(os.path.abspath(__file__))
os.chdir(ROOT)
sys.path.insert(0, ROOT)
from tools.silent_process import COMMON_INCLUDES, COMMON_LIBS, find_compiler, run_silent

os.makedirs("build", exist_ok=True)
os.makedirs("build/obj", exist_ok=True)

cpp_files = glob.glob("**/*.cpp", recursive=True)
cpp_files = [f.replace("\\", "/") for f in cpp_files]
cpp_files = [f for f in cpp_files if not f.startswith("build/")]

cxx = find_compiler("g++")
includes = ["-I" + i for i in COMMON_INCLUDES]

def compile_file(src):
    obj_name = src.replace("/", "_").replace(".cpp", ".o")
    obj_path = f"build/obj/{obj_name}"
    cmd = [cxx, "-std=c++20", "-c", src, "-o", obj_path] + includes
    res = run_silent(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        return (src, False, res.stderr)
    return (src, True, obj_path)

if __name__ == "__main__":
    print(f"Compiling {len(cpp_files)} C++ files across {cpu_count()} CPU workers...")
    with Pool(processes=min(cpu_count(), 16)) as pool:
        results = pool.map(compile_file, cpp_files)

    failed = [r for r in results if not r[1]]
    if failed:
        print(f"Compilation failed for {len(failed)} files:")
        for src, _, err in failed:
            print(f"--- {src} ---")
            print(err)
        sys.exit(1)

    obj_files = [r[2] for r in results]
    print(f"All {len(obj_files)} objects compiled successfully. Linking build/SR_GameServer.exe...")

    link_cmd = [
        cxx, "-std=c++20", "-mconsole",
        "-o", "build/SR_GameServer.exe",
        "-Wl,--start-group"
    ] + obj_files + [
        "-Wl,--end-group",
    ] + COMMON_LIBS

    res = run_silent(link_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Linking failed:")
        print(res.stderr)
        sys.exit(res.returncode)

    print("Build succeeded: build/SR_GameServer.exe")
