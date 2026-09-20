/**
 * ============================================================================
 * Silkroad Online - Joymax BSLib Exception Handling Runtime
 * Original Source: MSVC CRT throw.cpp / BSLib Internal Exception Hierarchy
 *
 * Implements:
 *   - CRT_CxxThrowException @ 0x009E187B (70 bytes)
 *   - g_CxxExceptionRecordTemplate @ 0x00ADA49C (32 bytes)
 * ============================================================================
 */

#include "BSException.h"
#include <cstring>

/**
 * [RECONSTRUCTED - Native 0x00ADA49C] (32 bytes / 8 DWORDs)
 * Static prototype exception record copied into the stack frame by _CxxThrowException.
 */
static const EXCEPTION_RECORD g_CxxExceptionRecordTemplate = {
	static_cast<DWORD>(MSVC_EH_EXCEPTION_NUMBER), // +0x00: 0xE06D7363 ('.msc')
	EXCEPTION_NONCONTINUABLE,                     // +0x04: 0x00000001 (Non-continuable)
	nullptr,                                      // +0x08: Nested ExceptionRecord = nullptr
	nullptr,                                      // +0x0C: ExceptionAddress = nullptr
	3,                                            // +0x10: NumberParameters = 3
	{
		MSVC_EH_MAGIC_NUMBER1,                    // +0x14: 0x19930520 (MSVC 1993 EH specification)
		0,                                        // +0x18: ExceptionInformation[1] (pExceptionObject)
		0                                         // +0x1C: ExceptionInformation[2] (pThrowInfo)
	}
};

/**
 * [RECONSTRUCTED - Native 0x009E187B] (70 bytes)
 * CRT_CxxThrowException
 *
 * Exact machine sequence:
 *   0x009E187B: push ebp / mov ebp, esp / sub esp, 0x20
 *   0x009E1886: push 8 / pop ecx / mov esi, 0x00ADA49C / lea edi, [ebp-0x20] / rep movsd
 *   0x009E1893: mov [ebp-0x08], eax (pExceptionObject)
 *   0x009E189C: mov [ebp-0x04], eax (pThrowInfo)
 *   0x009E18A2: test byte [pThrowInfo], 8 -> if set: mov [ebp-0x0C], 0x01994000
 *   0x009E18BB: call dword [0x00AD9288] (RaiseException)
 */
extern "C" [[noreturn]] void __stdcall CRT_CxxThrowException(
	void*                 pExceptionObject,
	const _s__ThrowInfo*  pThrowInfo
) {
	EXCEPTION_RECORD thisException;
	std::memcpy(&thisException, &g_CxxExceptionRecordTemplate, sizeof(EXCEPTION_RECORD));

	thisException.ExceptionInformation[1] = reinterpret_cast<ULONG_PTR>(pExceptionObject);
	thisException.ExceptionInformation[2] = reinterpret_cast<ULONG_PTR>(pThrowInfo);

	if (pThrowInfo != nullptr) {
		// Native 0x009E18A2: test byte [eax], 0x8
		if (pThrowInfo->attributes & 0x08) {
			// Native 0x009E18A7: mov dword [ebp-0xc], 0x1994000
			thisException.ExceptionInformation[0] = MSVC_EH_MAGIC_NUMBER2;
		}
	}

	// Native 0x009E18AE - 0x009E18BB: Call imported RaiseException
	::RaiseException(
		thisException.ExceptionCode,
		thisException.ExceptionFlags,
		thisException.NumberParameters,
		thisException.ExceptionInformation
	);

#if defined(__GNUC__) || defined(__clang__)
	__builtin_unreachable();
#endif
}

namespace BSLib {

CInternalException::CInternalException(uint32_t dwCode) noexcept
	: m_dwCode(dwCode) {
}

CInternalException::~CInternalException() = default;

const char* CInternalException::what() const noexcept {
	return "CInternalException: Joymax BSLib internal error";
}

CMsgException::CMsgException() noexcept
	: CInternalException(1)
	, m_dwSubCode(1) {
}

CMsgException::~CMsgException() = default;

const char* CMsgException::what() const noexcept {
	return "CMsgException: Stream buffer underflow or invalid packet payload";
}

} // namespace BSLib
