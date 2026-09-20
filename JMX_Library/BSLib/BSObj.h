/**
 * ============================================================================
 * Joymax BSLib - BlackSea Library: Base Object & Runtime Class System
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\
 *
 * Implements base runtime reflection structures and service objects matching:
 *   - CBase @ 0x00B427DC (RTTI: .?AVCBase@@)
 *   - CServiceObject @ 0x00B427E4 (RTTI: .?AVCServiceObject@@)
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_BSOBJ_H_
#define _JMX_LIBRARY_BSLIB_BSOBJ_H_

#include <cstdint>
#include <cstddef>

/**
 * CRuntimeClass
 * Native MFC-style runtime class descriptor
 * Size: 20 bytes (0x14)
 * Proven from machine bytes @ 0x00ADD940
 */
struct CRuntimeClass {
	const char*          lpszClassName;                  // +0x00: Class name string
	uint32_t             nObjectSize;                    // +0x04: Instance size in bytes
	void*              (*pfnCreateObject)();             // +0x08: Factory constructor function
	void               (*pfnDestroyObject)(void*);       // +0x0C: Destroyer function
	const CRuntimeClass* pBaseClass;                     // +0x10: Base class descriptor

	void* CreateObject() const;
	void DestroyObject(void* pObj) const;
};

namespace BSLib {
class CTask;
}

#ifndef _CSINGLETONT_DEFINED_
#define _CSINGLETONT_DEFINED_
template <typename T>
class CSingletonT {
public:
	CSingletonT() = default;
	virtual ~CSingletonT() = default;
	static T* GetInstance() {
		static T s_instance;
		return &s_instance;
	}
};
#endif

class CBase {
public:
	virtual ~CBase() = default;
	virtual const CRuntimeClass* GetRuntimeClass() const = 0;
};

class CServiceObject : public CBase {
public:
	CServiceObject();
	virtual ~CServiceObject() override = default;

	virtual const CRuntimeClass* GetRuntimeClass() const override;

	// [RECONSTRUCTED - 0x009859C0]
	// Slot 2 @ +0x08: Initialize(CTask* pTask, uint32_t dwParam)
	virtual bool Initialize(BSLib::CTask* pTask, uint32_t dwParam);

	// [RECONSTRUCTED - 0x009871F4, 0x00402870, 0x0094CA50]
	// Slot 3 @ +0x0C: Process(int32_t* pStopFlag, int32_t nThreadIndex = 0)
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) = 0;

	// Slot 4 @ +0x10: OnMessage()
	virtual void OnMessage() = 0;

	virtual BSLib::CTask* GetTask() const;
	virtual uint32_t GetParam() const;

public:
	static const CRuntimeClass ms_classCServiceObject;

protected:
	BSLib::CTask* m_pTask;   // +0x08: Owning task instance (Native: stored at +0x08)
	uint32_t      m_dwParam; // +0x0C: Param (Native: stored at +0x0C)
	uint8_t       m_pad[0x10008]; // Large buffer / queue structure
};

class CServiceDefaultObject : public CServiceObject {
public:
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) override;
	virtual void OnMessage() override;
};

#endif // _JMX_LIBRARY_BSLIB_BSOBJ_H_
