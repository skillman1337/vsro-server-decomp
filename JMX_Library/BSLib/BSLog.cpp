/**
 * ============================================================================
 * Joymax BSLib - Logging & Error Dialog Subsystem Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\BSLog.cpp
 *
 * Implements:
 *   - BSLog_ShowErrorMessage @ 0x00963930
 *   - BSLib_AssertReport     @ 0x00964760
 *   - BSLib_SetDebugOptions  @ 0x00964750
 *   - GenerateMiniDump       @ 0x00964B60
 *   - InstallExceptionHandlers @ 0x00964C40
 *   - ServerFramework_LogToFile @ 0x009378C0
 *   - BSLib_GetNextToken     @ 0x00951E50
 *   - Global debug options:
 *       g_dwBSLibDebugOptionDebuggerPresent @ 0x00C63B60
 *       g_dwBSLibDebugOptionStandAlone      @ 0x00C63B64
 * ============================================================================
 */

#include "BSLog.h"
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef enum _MINIDUMP_TYPE {
	MiniDumpNormal                         = 0x00000000,
	MiniDumpWithDataSegs                   = 0x00000001,
	MiniDumpWithFullMemory                 = 0x00000002,
	MiniDumpWithHandleData                 = 0x00000004,
	MiniDumpFilterMemory                   = 0x00000008,
	MiniDumpScanMemory                     = 0x00000010,
	MiniDumpWithUnloadedModules            = 0x00000020,
	MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
	MiniDumpFilterModulePaths              = 0x00000080,
	MiniDumpWithProcessThreadData          = 0x00000100,
	MiniDumpWithPrivateReadWriteMemory     = 0x00000200,
	MiniDumpWithoutOptionalData            = 0x00000400,
	MiniDumpWithFullMemoryInfo             = 0x00000800,
	MiniDumpWithThreadInfo                 = 0x00001000,
	MiniDumpWithCodeSegs                   = 0x00002000
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
	DWORD               ThreadId;
	PEXCEPTION_POINTERS ExceptionPointers;
	BOOL                ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef BOOL (WINAPI *PFN_MiniDumpWriteDump)(
	HANDLE                            hProcess,
	DWORD                             ProcessId,
	HANDLE                            hFile,
	MINIDUMP_TYPE                     DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION   ExceptionParam,
	void*                             UserStreamParam,
	void*                             CallbackParam
);
#endif

namespace BSLib {

// BSLib Global Debug Assert Options (Native 0x00C63B60, 0x00C63B64)
uint32_t g_dwBSLibDebugOptionDebuggerPresent = 0x0112; // 0x00C63B60
uint32_t g_dwBSLibDebugOptionStandAlone      = 0x0201; // 0x00C63B64

/**
 * BSLog_ShowErrorMessage
 * Native implementation @ 0x00963930
 */
int ShowErrorMessage(const char* pszFormat, ...) {
	char szBuffer[1024];
	va_list args;
	va_start(args, pszFormat);
	std::vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);

	std::fprintf(stderr, "[BSLog ERROR] %s\n", szBuffer);

#ifdef _WIN32
	// Native 0x009639B9: MessageBoxA(nullptr, szBuffer, "BSObj Plugin", MB_OK)
	MessageBoxA(nullptr, szBuffer, "BSObj Plugin", MB_OK);
#endif

	return 0;
}

#ifdef _WIN32
// DbgHelp Function Pointer Typedefs
typedef BOOL (WINAPI *PFN_SymInitialize)(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess);
typedef DWORD (WINAPI *PFN_SymSetOptions)(DWORD SymOptions);
typedef DWORD (WINAPI *PFN_SymGetOptions)(VOID);

// Globals @ 0x00C82800 - 0x00C83838
static PFN_MiniDumpWriteDump        g_pfnMiniDumpWriteDump        = nullptr; // 0x00C82800
static void*                        g_pfnStackWalk                = nullptr; // 0x00C82804
static void*                        g_pfnSymFunctionTableAccess   = nullptr; // 0x00C82808
static void*                        g_pfnSymGetModuleBase         = nullptr; // 0x00C8280C
static PFN_SymInitialize            g_pfnSymInitialize            = nullptr; // 0x00C82810
static PFN_SymSetOptions            g_pfnSymSetOptions            = nullptr; // 0x00C82814
static PFN_SymGetOptions            g_pfnSymGetOptions            = nullptr; // 0x00C82818
static void*                        g_pfnUnDecorateSymbolName     = nullptr; // 0x00C8281C
static void*                        g_pfnSymGetLineFromAddr       = nullptr; // 0x00C82820
static void*                        g_pfnSymGetSymFromAddr        = nullptr; // 0x00C82824
static void*                        g_pfnSymGetModuleInfo         = nullptr; // 0x00C82828
static HMODULE                      g_hDbgHelpModule              = nullptr; // 0x00C8282C
static uint32_t                     g_bCrashDumpInitialized       = 0;       // 0x00C82830
[[maybe_unused]] static void*        g_pfnAssertCustomCallback     = nullptr; // 0x00C82834
[[maybe_unused]] static char         g_szCrashDumpErrorMessage[4096] = { 0 }; // 0x00C82838
static uint32_t                     g_dwCrashDumpCounter          = 0;       // 0x00C83838

/*
================
CrashDump_BuildSymbolSearchPath
[RECONSTRUCTED - Native 0x009639E0] (999 bytes)

Builds symbol search path string from _NT_SYMBOL_PATH, _NT_ALT_SYMBOL_PATH,
SYSTEMROOT, \System32, and \System.
================
*/
int32_t CrashDump_BuildSymbolSearchPath(char* pszOutPath, size_t nMaxLen) {
	if (!pszOutPath || nMaxLen == 0) {
		return static_cast<int32_t>(0x80070057); // E_INVALIDARG
	}

	pszOutPath[0] = '\0';

	auto AppendPath = [&](const char* pszPath) {
		if (pszPath && pszPath[0] != '\0') {
			size_t curLen = std::strlen(pszOutPath);
			if (curLen < nMaxLen - 1) {
				std::strncat(pszOutPath, pszPath, nMaxLen - curLen - 1);
			}
			curLen = std::strlen(pszOutPath);
			if (curLen < nMaxLen - 2) {
				std::strncat(pszOutPath, ";", nMaxLen - curLen - 1);
			}
		}
	};

	// 1. _NT_SYMBOL_PATH
	const char* pEnv = std::getenv("_NT_SYMBOL_PATH");
	if (pEnv && *pEnv) {
		AppendPath(pEnv);
	}

	// 2. _NT_ALT_SYMBOL_PATH
	pEnv = std::getenv("_NT_ALT_SYMBOL_PATH");
	if (pEnv && *pEnv) {
		AppendPath(pEnv);
	}

	// 3. SYSTEMROOT
	pEnv = std::getenv("SYSTEMROOT");
	if (pEnv && *pEnv) {
		AppendPath(pEnv);
	} else {
		char szWinDir[MAX_PATH] = { 0 };
		if (::GetWindowsDirectoryA(szWinDir, sizeof(szWinDir)) > 0) {
			AppendPath(szWinDir);

			char szSystem32[MAX_PATH];
			std::snprintf(szSystem32, sizeof(szSystem32), "%s\\System32", szWinDir);
			AppendPath(szSystem32);

			char szSystem[MAX_PATH];
			std::snprintf(szSystem, sizeof(szSystem), "%s\\System", szWinDir);
			AppendPath(szSystem);
		}
	}

	AppendPath(".");
	return 0;
}

/*
================
CrashDump_InitDbgHelp
[RECONSTRUCTED - Native 0x00963DD0] (641 bytes)

Dynamically loads dbghelp.dll and resolves 11 debugging exports.
Sets symbol options and initializes symbol handler for current process.

Authentic developer quirks:
1. Native 0x00963FC3 - 0x00963FCA: Calls CrashDump_BuildSymbolSearchPath to fill
   szSymbolPath on stack, but then immediately passes NULL (push 0) as UserSearchPath
   to SymInitialize! The entire 999-byte path builder result was discarded!
2. Native 0x00963E37: If LoadLibraryA fails, copies "cannot load dbghelp.dll" to
   g_szCrashDumpErrorMessage.
3. Native 0x00963FE2: If any of the 11 GetProcAddress calls or SymInitialize fails,
   copies "cannot load dbghelp.dll : wrong version or SymInitialize fail" to
   g_szCrashDumpErrorMessage, calls FreeLibrary(g_hDbgHelpModule), and returns false.
================
*/
bool CrashDump_InitDbgHelp() {
	if (g_bCrashDumpInitialized) {
		return true;
	}

	char szSymbolPath[MAX_PATH];
	std::memset(szSymbolPath, 0, sizeof(szSymbolPath));

	g_hDbgHelpModule = ::LoadLibraryA("dbghelp.dll");
	if (!g_hDbgHelpModule) {
		std::strncpy(g_szCrashDumpErrorMessage, "cannot load dbghelp.dll", sizeof(g_szCrashDumpErrorMessage) - 1);
		g_szCrashDumpErrorMessage[sizeof(g_szCrashDumpErrorMessage) - 1] = '\0';
		return false;
	}

	auto ResolveProc = [](HMODULE hMod, const char* name) -> void* {
		return reinterpret_cast<void*>(::GetProcAddress(hMod, name));
	};

	g_pfnMiniDumpWriteDump      = reinterpret_cast<PFN_MiniDumpWriteDump>(ResolveProc(g_hDbgHelpModule, "MiniDumpWriteDump"));
	g_pfnStackWalk              = ResolveProc(g_hDbgHelpModule, "StackWalk");
	g_pfnSymFunctionTableAccess = ResolveProc(g_hDbgHelpModule, "SymFunctionTableAccess");
	g_pfnSymGetModuleBase       = ResolveProc(g_hDbgHelpModule, "SymGetModuleBase");
	g_pfnSymInitialize          = reinterpret_cast<PFN_SymInitialize>(ResolveProc(g_hDbgHelpModule, "SymInitialize"));
	g_pfnSymSetOptions          = reinterpret_cast<PFN_SymSetOptions>(ResolveProc(g_hDbgHelpModule, "SymSetOptions"));
	g_pfnSymGetOptions          = reinterpret_cast<PFN_SymGetOptions>(ResolveProc(g_hDbgHelpModule, "SymGetOptions"));
	g_pfnUnDecorateSymbolName   = ResolveProc(g_hDbgHelpModule, "UnDecorateSymbolName");
	g_pfnSymGetLineFromAddr     = ResolveProc(g_hDbgHelpModule, "SymGetLineFromAddr");
	g_pfnSymGetSymFromAddr      = ResolveProc(g_hDbgHelpModule, "SymGetSymFromAddr");
	g_pfnSymGetModuleInfo       = ResolveProc(g_hDbgHelpModule, "SymGetModuleInfo");

	if (!g_pfnMiniDumpWriteDump || !g_pfnStackWalk || !g_pfnSymFunctionTableAccess ||
		!g_pfnSymGetModuleBase || !g_pfnSymInitialize || !g_pfnSymSetOptions ||
		!g_pfnSymGetOptions || !g_pfnUnDecorateSymbolName || !g_pfnSymGetLineFromAddr ||
		!g_pfnSymGetSymFromAddr || !g_pfnSymGetModuleInfo)
	{
		std::strncpy(g_szCrashDumpErrorMessage, "cannot load dbghelp.dll : wrong version or SymInitialize fail", sizeof(g_szCrashDumpErrorMessage) - 1);
		g_szCrashDumpErrorMessage[sizeof(g_szCrashDumpErrorMessage) - 1] = '\0';
		::FreeLibrary(g_hDbgHelpModule);
		g_hDbgHelpModule = nullptr;
		return false;
	}

	// Native 0x00963FAA - 0x00963FB9:
	// DWORD dwOpts = (SymGetOptions() & ~2) | 0x414;
	DWORD dwOpts = g_pfnSymGetOptions();
	dwOpts = (dwOpts & ~static_cast<DWORD>(2)) | static_cast<DWORD>(0x414);
	g_pfnSymSetOptions(dwOpts);

	// Native 0x00963FBF - 0x00963FC3: Calls symbol path builder on stack
	CrashDump_BuildSymbolSearchPath(szSymbolPath, sizeof(szSymbolPath));

	// Native 0x00963FC8 - 0x00963FD3: Pass NULL (push 0) as UserSearchPath
	BOOL bSymInitSuccess = g_pfnSymInitialize(::GetCurrentProcess(), nullptr, TRUE);
	if (!bSymInitSuccess) {
		std::strncpy(g_szCrashDumpErrorMessage, "cannot load dbghelp.dll : wrong version or SymInitialize fail", sizeof(g_szCrashDumpErrorMessage) - 1);
		g_szCrashDumpErrorMessage[sizeof(g_szCrashDumpErrorMessage) - 1] = '\0';
		::FreeLibrary(g_hDbgHelpModule);
		g_hDbgHelpModule = nullptr;
		return false;
	}

	g_bCrashDumpInitialized = 1;
	return true;
}

/*
================
Assert_WriteCrashDumpAndReport
[RECONSTRUCTED - Native 0x00964060] (656 bytes)

Generates timestamped minidump file:
_<module>_[%04d-%02d-%02d %02d-%02d-%02d]_%d.dmp
If dump creation fails, logs error to very_fatal_log.txt.

Authentic developer quirks:
1. Native 0x00964089: If CrashDump_InitDbgHelp fails, jumps to 0x009642C7 and returns 1!
2. Native 0x009641F0 - 0x009642C1: If CreateFileA fails (INVALID_HANDLE_VALUE), it jumps
   to CloseHandle(INVALID_HANDLE_VALUE) and returns 1!
3. Native 0x00964273 - 0x00964279: Win32 error code is converted to an HRESULT via
   (dwErr & 0xFFFF) | 0x80070000, and printed with %d, resulting in negative numbers!
================
*/
int32_t Assert_WriteCrashDumpAndReport(void* pExceptionPointers) {
	if (!CrashDump_InitDbgHelp()) {
		return 1; // Native 0x00964089: je 0x009642C7 returns 1
	}

	char szModulePath[MAX_PATH];
	::GetModuleFileNameA(nullptr, szModulePath, sizeof(szModulePath));

	char szDrive[MAX_PATH] = { 0 };
	char szDir[MAX_PATH] = { 0 };
	char szFname[MAX_PATH] = { 0 };
	_splitpath_s(szModulePath, szDrive, sizeof(szDrive), szDir, sizeof(szDir), szFname, sizeof(szFname), nullptr, 0);

	SYSTEMTIME st;
	::GetLocalTime(&st);

	g_dwCrashDumpCounter++;

	char szSuffix[128];
	std::snprintf(szSuffix, sizeof(szSuffix), "_[%04d-%02d-%02d %02d-%02d-%02d]_%u",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, g_dwCrashDumpCounter);

	std::strncat(szFname, szSuffix, sizeof(szFname) - std::strlen(szFname) - 1);

	char szDmpPath[MAX_PATH];
	_makepath_s(szDmpPath, sizeof(szDmpPath), szDrive, szDir, szFname, ".dmp");

	HANDLE hFile = ::CreateFileA(szDmpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		// Native 0x009641F3: jumps to CloseHandle(0xFFFFFFFF) and returns 1
		::CloseHandle(hFile);
		return 1;
	}

	MINIDUMP_EXCEPTION_INFORMATION expParam;
	expParam.ThreadId = ::GetCurrentThreadId();
	expParam.ExceptionPointers = static_cast<PEXCEPTION_POINTERS>(pExceptionPointers);
	expParam.ClientPointers = FALSE;

	BOOL bDumpSuccess = FALSE;
	if (g_pfnMiniDumpWriteDump) {
		bDumpSuccess = g_pfnMiniDumpWriteDump(
			::GetCurrentProcess(),
			::GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			&expParam,
			nullptr,
			nullptr
		);
	}

	if (!bDumpSuccess) {
		DWORD dwErr = ::GetLastError();
		int32_t nHResultErr = static_cast<int32_t>(dwErr);
		// Native 0x00964273 - 0x00964279: HRESULT_FROM_WIN32 conversion
		if (nHResultErr > 0) {
			nHResultErr = (nHResultErr & 0xFFFF) | static_cast<int32_t>(0x80070000);
		}

		FILE* fpFatal = nullptr;
		if (fopen_s(&fpFatal, "very_fatal_log.txt", "a") == 0 && fpFatal) {
			SYSTEMTIME stLog;
			::GetLocalTime(&stLog);
			// Native 0x009642A8: prints nHResultErr with %d
			std::fprintf(fpFatal, "[MiniDumpWriteDump] %04d-%02d-%02d %02d:%02d:%02d Cannot Write MiniDump (LastError : %d)\r\n",
				static_cast<int>(stLog.wYear), static_cast<int>(stLog.wMonth), static_cast<int>(stLog.wDay),
				static_cast<int>(stLog.wHour), static_cast<int>(stLog.wMinute), static_cast<int>(stLog.wSecond),
				nHResultErr);
			std::fclose(fpFatal);
		}
	}

	::CloseHandle(hFile);
	return 1;
}
#else
bool CrashDump_InitDbgHelp() {
	return false;
}
int32_t CrashDump_BuildSymbolSearchPath(char* /*pszOutPath*/, size_t /*nMaxLen*/) {
	return 0;
}
int32_t Assert_WriteCrashDumpAndReport(void* /*pExceptionPointers*/) {
	return 1;
}
#endif

/**
 * Fatal_AssertFailed / AssertFailed
 * Native implementation @ 0x00964750
 */
int AssertFailed(const char* file, int line) {
	std::fprintf(stderr, "[BSLib FATAL] Assertion failed at %s:%d!\n", file, line);
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00964B60]
 * GenerateMiniDump / ServerFramework_GenerateMiniDump
 * Native implementation @ 0x00964B60 (143 bytes)
 *
 * Checks BSLib debug options mask 0x200 (Standalone vs Debugger).
 * If set, triggers minidump generation.
 *
 * Authentic Developer "no fucking way" pattern:
 * In native MSVC code, it enters an SEH __try block and literally executes:
 *     throw "oops";
 * to synthesize an EXCEPTION_POINTERS record for the __except filter, which then calls
 * Assert_WriteCrashDumpAndReport!
 */
static void (*g_pfnMiniDumpHook)() = nullptr;
extern "C" void SetMiniDumpHook(void (*pfn)()) {
	g_pfnMiniDumpHook = pfn;
}

int GenerateMiniDump(const char* file, int line) {
	if (g_pfnMiniDumpHook) {
		g_pfnMiniDumpHook();
	}
#ifdef _WIN32
	// Native 0x00964B93 - 0x00964BA0:
	// IsDebuggerPresent() ? g_dwBSLibDebugOptionDebuggerPresent : g_dwBSLibDebugOptionStandAlone
	int nIndex = ::IsDebuggerPresent() ? 0 : 1;
	uint32_t dwOptions = (nIndex == 0) ? g_dwBSLibDebugOptionDebuggerPresent : g_dwBSLibDebugOptionStandAlone;

	// Native 0x00964BA0: test dword [options], 0x200
	if ((dwOptions & 0x200) == 0) {
		return 1; // Bit 0x200 not set -> silently ignore
	}

	std::fprintf(stderr, "[BSLib FATAL] GenerateMiniDump triggered at %s:%d!\n", file, line);

#if defined(_MSC_VER)
	__try {
		// Authentic Joymax MSVC implementation: throws "oops" to invoke filter!
		throw "oops";
	}
	__except (Assert_WriteCrashDumpAndReport(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
		return 1;
	}
#else
	// GCC / MinGW implementation: capture thread context and generate minidump directly
	CONTEXT ctx;
	std::memset(&ctx, 0, sizeof(ctx));
	ctx.ContextFlags = CONTEXT_FULL;
	::RtlCaptureContext(&ctx);

	EXCEPTION_RECORD rec;
	std::memset(&rec, 0, sizeof(rec));
	rec.ExceptionCode = 0xE06D7363; // MSVC C++ Exception Code
	rec.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
	rec.ExceptionAddress = reinterpret_cast<PVOID>(GenerateMiniDump);

	EXCEPTION_POINTERS ep;
	ep.ExceptionRecord = &rec;
	ep.ContextRecord = &ctx;

	Assert_WriteCrashDumpAndReport(&ep);
#endif

#else
	std::fprintf(stderr, "[BSLib FATAL] GenerateMiniDump triggered at %s:%d (non-win32 stub)!\n", file, line);
#endif
	return 1;
}

/**
 * [RECONSTRUCTED - 0x00964760]
 * BSLib_AssertReport / sub_964760
 */
bool AssertReport(uint32_t dwLine, const char* pszFile, const char* pszExpression) {
	std::fprintf(stderr, "[BSLib ASSERTION] %s:%u: Assertion '%s' failed!\n", pszFile, dwLine, pszExpression);
#ifdef _WIN32
	if (::IsDebuggerPresent()) {
		return false; // Tells caller to call DebugBreak()
	}
#endif
	return true;
}

/**
 * [RECONSTRUCTED - 0x00964750]
 * BSLib_SetDebugOptions
 * Native implementation @ 0x00964750 (12 bytes)
 */
void SetDebugOptions(uint32_t dwDebuggerOption, uint32_t /*pfnFilter*/, uint32_t dwStandAloneOption) {
	g_dwBSLibDebugOptionDebuggerPresent = dwDebuggerOption;
	g_dwBSLibDebugOptionStandAlone      = dwStandAloneOption;
}

/**
 * InstallExceptionHandlers
 * Native implementation @ 0x00964C40
 */
void InstallExceptionHandlers() {
	// Native 0x00964C40: sets up structured exception translator and unexpected/terminate handlers
}

/**
 * ServerFramework_LogToFile
 * Native implementation @ 0x009378C0
 */
bool LogToFile(const char* pszFileName, const char* pszFormat, ...) {
	char szBuffer[4096];
	va_list args;
	va_start(args, pszFormat);
	std::vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);

	FILE* fp = std::fopen(pszFileName, "a");
	if (!fp) {
		return false;
	}

#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime(&st);
	std::fprintf(fp, "%04d-%02d-%02d\t%02d:%02d:%02d\t%s\r\n",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, szBuffer);
#else
	std::time_t t = std::time(nullptr);
	std::tm* tm = std::localtime(&t);
	std::fprintf(fp, "%04d-%02d-%02d\t%02d:%02d:%02d\t%s\r\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, szBuffer);
#endif

	std::fclose(fp);
	return true;
}

/**
 * [RECONSTRUCTED - 0x00951E50]
 * BSLib_GetNextToken / GetNextToken
 * Native implementation @ 0x00951E50 (290 bytes)
 */
const char* GetNextToken(const char* p, char* pszToken, size_t nMaxLen) {
	if (pszToken == nullptr) {
		return nullptr;
	}
	pszToken[0] = '\0';
	if (p == nullptr) {
		return nullptr;
	}

	while (*p != '\0') {
		// Skip whitespace
		while (*p != '\0' && static_cast<unsigned char>(*p) <= 0x20) {
			p++;
		}
		if (*p == '\0') {
			return nullptr;
		}

		// Skip comments: // line comment
		if (p[0] == '/' && p[1] == '/') {
			while (*p != '\0' && *p != '\n') {
				p++;
			}
			continue;
		}

		// Skip comments: /* block comment */
		if (p[0] == '/' && p[1] == '*') {
			p += 2;
			while (*p != '\0') {
				if (p[0] == '*' && p[1] == '/') {
					p += 2;
					break;
				}
				p++;
			}
			continue;
		}

		break;
	}

	if (*p == '\0') {
		return nullptr;
	}

	size_t idx = 0;
	if (*p == '"') {
		p++; // Skip opening quote
		while (*p != '\0' && *p != '"' && idx + 1 < nMaxLen) {
			pszToken[idx++] = *p++;
		}
		if (*p == '"') {
			p++; // Skip closing quote
		}
	} else if (*p == '{' || *p == '}' || *p == '(' || *p == ')' || *p == '\'' || *p == ':' || *p == ',' || *p == '-') {
		pszToken[idx++] = *p++;
	} else {
		while (*p != '\0' && static_cast<unsigned char>(*p) > 0x20 &&
		       *p != '{' && *p != '}' && *p != '(' && *p != ')' &&
		       *p != '\'' && *p != ':' && *p != ',' && *p != '-' &&
		       idx + 1 < nMaxLen) {
			if (p[0] == '/' && (p[1] == '/' || p[1] == '*')) {
				break;
			}
			pszToken[idx++] = *p++;
		}
	}

	pszToken[idx] = '\0';
	return p;
}

/**
 * [RECONSTRUCTED - 0x00952080]
 * BSLib_GDI_DrawTextFormatted / GDI_DrawTextFormatted
 * Native implementation @ 0x00952080 (138 bytes)
 *
 * Formats string into 254-byte buffer (0x00D6CB48), validates bounds,
 * saves current text alignment, draws text via TextOutA, and restores
 * previous text alignment.
 */
uint32_t GDI_DrawTextFormatted(HDC hdc, int32_t nX, int32_t nY, uint32_t nTextAlign, const char* pszFormat, ...) {
#ifdef _WIN32
	if (!hdc || !pszFormat) {
		return 0;
	}

	char szBuffer[256];
	va_list args;
	va_start(args, pszFormat);
	int nLen = std::vsnprintf(szBuffer, sizeof(szBuffer) - 1, pszFormat, args);
	va_end(args);

	if (nLen < 0 || nLen > 254) {
		szBuffer[254] = '\0';
		GenerateMiniDump();
		return 0;
	}
	szBuffer[nLen] = '\0';

	UINT dwOldAlign = ::SetTextAlign(hdc, nTextAlign);
	::TextOutA(hdc, nX, nY, szBuffer, nLen);
	return ::SetTextAlign(hdc, dwOldAlign);
#else
	(void)hdc; (void)nX; (void)nY; (void)nTextAlign; (void)pszFormat;
	return 0;
#endif
}

/**
 * Log_Printf forwarder
 * Native implementation resides in ServerFramework @ 0x00936640.
 */
int Log_Printf(uint32_t dwChannel, const char* pszFormat, ...) {
	char szBuffer[8192];
	va_list args;
	va_start(args, pszFormat);
	std::vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);

	return ServerFramework::Log_Printf(dwChannel, "%s", szBuffer);
}

} // namespace BSLib
