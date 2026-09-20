/**
 * ============================================================================
 * Silkroad Online - Joymax BSLib Exception Handling Runtime
 * Original Source: MSVC CRT throw.cpp / BSLib Internal Exception Hierarchy
 *
 * Implements:
 *   - CRT_CxxThrowException @ 0x009E187B (70 bytes)
 *   - g_CxxExceptionRecordTemplate @ 0x00ADA49C (32 bytes)
 *   - CInternalException @ RTTI .?AVCInternalException@@
 *   - CMsgException @ 0x004042F0 / RTTI .?AVCMsgException@@
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_BSEXCEPTION_H_
#define _JMX_LIBRARY_BSLIB_BSEXCEPTION_H_

#include <cstdint>
#include <exception>
#include <windows.h>

// MSVC C++ Exception Constants (Proven from 0x00ADA49C & 0x009E187B)
#ifndef MSVC_EH_EXCEPTION_NUMBER
#define MSVC_EH_EXCEPTION_NUMBER 0xE06D7363 // 'msc' | 0xE0000000
#endif

#ifndef MSVC_EH_MAGIC_NUMBER1
#define MSVC_EH_MAGIC_NUMBER1    0x19930520 // May 20, 1993: MSVC 1.0 EH specification
#endif

#ifndef MSVC_EH_MAGIC_NUMBER2
#define MSVC_EH_MAGIC_NUMBER2    0x01994000 // 1994: MSVC EH with RTTI / pure attributes
#endif

#pragma pack(push, 4)

/**
 * [RECONSTRUCTED - Native MSVC EH Descriptor]
 * _PMFN: Pointer to member function descriptor
 */
struct _s__PMFN {
	int32_t mdisp; // Member displacement
	int32_t pdisp; // Vfptr displacement
	int32_t vdisp; // Displacement within vtable
};

/**
 * [RECONSTRUCTED - Native MSVC EH Descriptor @ 0x00B88448 / 0x00B88464]
 * _s__CatchableType
 * Describes a type that can catch this exception (e.g., CMsgException or CInternalException).
 */
struct _s__CatchableType {
	uint32_t properties;            // +0x00: Bit 0x01: IsSimpleType, Bit 0x02: ByReferenceOnly
	void*    pTypeDescriptor;       // +0x04: Pointer to TypeDescriptor (RTTI)
	_s__PMFN thisDisplacement;      // +0x08 - +0x13: This displacement adjustment
	uint32_t sizeOrOffset;          // +0x14: Size of exception object (12 for CMsgException, 8 for CInternalException)
	void*    copyFunction;          // +0x18: Pointer to copy constructor
};

/**
 * [RECONSTRUCTED - Native MSVC EH Descriptor @ 0x00B88480]
 * _s__CatchableTypeArray
 * Array of catchable types in inheritance order.
 */
struct _s__CatchableTypeArray {
	int32_t                 nCatchableTypes; // +0x00: Number of catchable types
	const _s__CatchableType* arrayOfCatchableTypes[1]; // +0x04: Pointers to _s__CatchableType
};

/**
 * [RECONSTRUCTED - Native MSVC EH Descriptor @ 0x00B8848C]
 * _s__ThrowInfo
 * Information describing a thrown C++ exception passed to _CxxThrowException.
 */
struct _s__ThrowInfo {
	uint32_t                      attributes;          // +0x00: Bit 0x08 = TI_IsPure (upgrades to EH_MAGIC_NUMBER2)
	void*                         pmfnUnwind;          // +0x04: Exception object destructor pointer
	void*                         pForwardCompat;      // +0x08: Forward compatibility handler
	const _s__CatchableTypeArray* pCatchableTypeArray; // +0x0C: Array of catchable types
};

#pragma pack(pop)

namespace BSLib {

/**
 * [RECONSTRUCTED - Native RTTI .?AVCInternalException@@]
 * CInternalException: Base internal exception for all Joymax BSLib engine errors.
 * Size: 8 bytes (0x08)
 */
class CInternalException : public std::exception {
public:
	uint32_t m_dwCode; // +0x04: Internal exception error code

	CInternalException(uint32_t dwCode = 0) noexcept;
	virtual ~CInternalException() override;
	virtual const char* what() const noexcept override;
};

/**
 * [RECONSTRUCTED - Native 0x004042F0 / RTTI .?AVCMsgException@@]
 * CMsgException: Thrown on packet stream buffer underflow or invalid payload.
 * Size: 12 bytes (0x0C)
 */
class CMsgException : public CInternalException {
public:
	uint32_t m_dwSubCode; // +0x08: Specific stream underflow flag (1 = underflow)

	CMsgException() noexcept;
	virtual ~CMsgException() override;
	virtual const char* what() const noexcept override;
};

} // namespace BSLib

typedef BSLib::CInternalException CInternalException;
typedef BSLib::CMsgException CMsgException;

/**
 * [RECONSTRUCTED - Native 0x009E187B] (70 bytes)
 * CRT_CxxThrowException / _CxxThrowException
 *
 * Low-level CRT runtime exception dispatcher.
 * Copies the pre-baked EXCEPTION_RECORD template from 0x00ADA49C, configures
 * the exception parameters, and raises a non-continuable SEH exception (0xE06D7363).
 *
 * Note: In application C++ code, developers should write:
 *       `throw CMsgException();`
 * rather than calling CRT_CxxThrowException directly.
 */
extern "C" [[noreturn]] void __stdcall CRT_CxxThrowException(
	void*                 pExceptionObject,
	const _s__ThrowInfo*  pThrowInfo
);

#endif // _JMX_LIBRARY_BSLIB_BSEXCEPTION_H_
