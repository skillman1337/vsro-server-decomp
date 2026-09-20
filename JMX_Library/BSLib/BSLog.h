/**
 * ============================================================================
 * Joymax BSLib - Logging & Error Dialog Subsystem
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\BSLog.h
 *
 * Implements native logging facilities:
 *   - BSLog_ShowErrorMessage @ 0x00963930
 *   - Log_Printf @ 0x00936640
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_BSLOG_H_
#define _JMX_LIBRARY_BSLIB_BSLOG_H_

#include <cstdio>
#include <cstdarg>
#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HDC;
#endif

namespace ServerFramework {
int Log_Printf(uint32_t dwChannel, const char* pszFormat, ...);
}

namespace BSLib {

/**
 * BSLog_ShowErrorMessage
 * Native implementation @ 0x00963930 in BSLog.cpp
 */
int ShowErrorMessage(const char* pszFormat, ...);

/**
 * [RECONSTRUCTED - 0x00964750]
 * Fatal_AssertFailed / AssertFailed
 * Native implementation @ 0x00964750 in BSLog.cpp
 */
int AssertFailed(const char* file = __builtin_FILE(), int line = __builtin_LINE());

/**
 * [RECONSTRUCTED - 0x00963DD0]
 * CrashDump_InitDbgHelp
 * Native implementation @ 0x00963DD0 (641 bytes) in BSLog.cpp
 *
 * Dynamically loads dbghelp.dll and resolves 11 debugging exports:
 * MiniDumpWriteDump, StackWalk, SymFunctionTableAccess, SymGetModuleBase,
 * SymInitialize, SymSetOptions, SymGetOptions, UnDecorateSymbolName,
 * SymGetLineFromAddr, SymGetSymFromAddr, SymGetModuleInfo.
 */
bool CrashDump_InitDbgHelp();

/**
 * [RECONSTRUCTED - 0x009639E0]
 * CrashDump_BuildSymbolSearchPath
 * Native implementation @ 0x009639E0 (999 bytes) in BSLog.cpp
 *
 * Assembles symbol search path string from _NT_SYMBOL_PATH, _NT_ALT_SYMBOL_PATH,
 * SYSTEMROOT, \System32, and \System.
 */
int32_t CrashDump_BuildSymbolSearchPath(char* pszOutPath, size_t nMaxLen = 260);

/**
 * [RECONSTRUCTED - 0x00964060]
 * Assert_WriteCrashDumpAndReport
 * Native implementation @ 0x00964060 (656 bytes) in BSLog.cpp
 *
 * Writes crash dump file: _<module>_[%04d-%02d-%02d %02d-%02d-%02d]_%d.dmp
 * If dump fails, logs error to very_fatal_log.txt.
 */
int32_t Assert_WriteCrashDumpAndReport(void* pExceptionPointers);

/**
 * [RECONSTRUCTED - 0x00964B60]
 * GenerateMiniDump / ServerFramework_GenerateMiniDump
 * Native implementation @ 0x00964B60 (143 bytes) in BSLog.cpp
 */
int GenerateMiniDump(const char* file = __builtin_FILE(), int line = __builtin_LINE());

/**
 * [RECONSTRUCTED - 0x00964760]
 * BSLib_AssertReport / sub_964760
 * Native implementation @ 0x00964760 in BSLog.cpp
 */
bool AssertReport(uint32_t dwLine, const char* pszFile, const char* pszExpression);

// BSLib Global Debug Assert Options (Native 0x00C63B60, 0x00C63B64)
extern uint32_t g_dwBSLibDebugOptionDebuggerPresent; // 0x00C63B60
extern uint32_t g_dwBSLibDebugOptionStandAlone;      // 0x00C63B64

/**
 * [RECONSTRUCTED - 0x00964750]
 * BSLib_SetDebugOptions
 * Native implementation @ 0x00964750 (12 bytes) in BSLog.cpp
 */
void SetDebugOptions(uint32_t dwDebuggerOption, uint32_t pfnFilter, uint32_t dwStandAloneOption);

/**
 * InstallExceptionHandlers
 * Native implementation @ 0x00964C40 in BSLog.cpp
 */
void InstallExceptionHandlers();

/**
 * ServerFramework_LogToFile
 * Native implementation @ 0x009378C0 in BSLog.cpp
 */
bool LogToFile(const char* pszFileName, const char* pszFormat, ...);

/**
 * [RECONSTRUCTED - 0x00951E50]
 * BSLib_GetNextToken / GetNextToken
 * Native implementation @ 0x00951E50 (290 bytes) in BSLog.cpp
 */
const char* GetNextToken(const char* p, char* pszToken, size_t nMaxLen = 256);

/**
 * [RECONSTRUCTED - 0x00952080]
 * BSLib_GDI_DrawTextFormatted / GDI_DrawTextFormatted
 * Native implementation @ 0x00952080 (138 bytes) in BSLog.cpp
 *
 * Formats string into 254-byte buffer (0x00D6CB48), validates bounds,
 * saves current text alignment, draws text via TextOutA, and restores
 * previous text alignment.
 */
uint32_t GDI_DrawTextFormatted(HDC hdc, int32_t nX, int32_t nY, uint32_t nTextAlign, const char* pszFormat, ...);

/**
 * Log_Printf forwarder
 * Native implementation resides in ServerFramework @ 0x00936640 in BSLog.cpp
 */
int Log_Printf(uint32_t dwChannel, const char* pszFormat, ...);

} // namespace BSLib

/**
 * [RECONSTRUCTED] ASSERT
 * Every native site is `test/cmp; jcc; call 0x00964B60` followed by the next statement:
 * the report never unwinds, so code after an ASSERT keeps its own explicit check.
 */
#define ASSERT(expr) ((expr) ? (void)0 : (void)BSLib::GenerateMiniDump(__FILE__, __LINE__))

/**
 * [PARTIAL] CLAMP
 * 38 native sites share "CLAMP() ==> min(%.3f) exceeded max(%.3f) value), File: %s, Line: %d"
 * (0x00ADF4A8, channel 0x2000001, e.g. AIHive.cpp line 572 @ 0x0055F26F). Code shape from
 * 0x0055F25A..0x0055F2EE: min > max logs and takes min, which the max test then lowers.
 * Native writes through Log_WriteError 0x0096CDB0; the port routes to BSLib::Log_Printf.
 */
#define CLAMP(value, minValue, maxValue)                                                                   \
	do {                                                                                                   \
		if ((minValue) > (maxValue)) {                                                                     \
			BSLib::Log_Printf(0x2000001, "CLAMP() ==> min(%.3f) exceeded max(%.3f) value), File: %s, Line: %d", \
				static_cast<double>(minValue), static_cast<double>(maxValue), __FILE__, __LINE__);            \
			(value) = (minValue);                                                                          \
		}                                                                                                  \
		if ((value) < (minValue)) {                                                                        \
			(value) = (minValue);                                                                          \
		} else if ((value) > (maxValue)) {                                                                 \
			(value) = (maxValue);                                                                          \
		}                                                                                                  \
	} while (0)

#endif // _JMX_LIBRARY_BSLIB_BSLOG_H_
