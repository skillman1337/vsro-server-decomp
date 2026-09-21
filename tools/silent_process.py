"""Centralized subprocess runner for build and test tools.

Provides silent process execution on Windows (CREATE_NO_WINDOW and SW_HIDE)
to prevent focus-stealing console window flashes during background runs, tests,
and parallel compilations while gaming.
"""
import os
import subprocess
import sys

DEFAULT_CXX = r"D:\msys64\mingw64\bin\g++.exe"

COMMON_INCLUDES = [
    ".",
    "Common/Framework",
    "JMX_Library/BSLib",
    "JMX_Library/EngineCommon",
    "JMX_Library/PathFindEngine",
    "JMX_Library/NavMesh_new",
    "JMX_ServerFramework/ServerFramework",
    "SR_GameServer",
    "ServerCommon",
]

COMMON_LIBS = [
    "-lws2_32",
    "-liphlpapi",
    "-lgdi32",
    "-lcomctl32",
    "-lodbc32",
]


def silent_windows_kwargs():
    """Return kwargs for subprocess on Windows to completely suppress console windows."""
    kwargs = {}
    if sys.platform == "win32":
        kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0x08000000)
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE
        kwargs["startupinfo"] = startupinfo
    return kwargs


def run_silent(cmd, cwd=None, env=None, timeout=None, capture_output=False, check=False, text=True):
    """Run a subprocess silently with window suppression flags on Windows."""
    kwargs = silent_windows_kwargs()
    if capture_output:
        kwargs["stdout"] = subprocess.PIPE
        kwargs["stderr"] = subprocess.PIPE
    return subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        timeout=timeout,
        check=check,
        text=text,
        **kwargs,
    )


def find_compiler(preferred=DEFAULT_CXX):
    """Find a valid C++ compiler path."""
    if os.path.isfile(preferred):
        return preferred
    for candidate in ["g++.exe", "g++", "clang++"]:
        path = subprocess.run(["where", candidate] if sys.platform == "win32" else ["which", candidate],
                              capture_output=True, text=True, **silent_windows_kwargs())
        if path.returncode == 0 and path.stdout.strip():
            return path.stdout.strip().splitlines()[0]
    return preferred
