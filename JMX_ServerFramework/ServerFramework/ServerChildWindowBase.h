/**
 * ============================================================================
 * Joymax ServerFramework - Server Child Window Base Architecture
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerChildWindowBase.h
 *
 * Implements CServerChildWindowBase:
 *   - Native VTable @ 0x00AFA550 (4 virtual slots = 0x10 bytes)
 *   - Native Constructor @ 0x00564D50 (104 bytes)
 *   - Native Destructor @ 0x00564DC0 (124 bytes)
 *   - Native Scalar Deleting Destructor @ 0x00565000 (31 bytes)
 *   - Native GetWindowName @ 0x005640B0 (17 bytes)
 *   - Base class for CDisplayWindow, CServerArchitectureView, CServerMoniterView
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERCHILDWINDOWBASE_H_
#define _JMX_SERVERFRAMEWORK_SERVERCHILDWINDOWBASE_H_

#include "WindowBase.h"
#include <string>

namespace ServerFramework {

/**
 * [RECONSTRUCTED - 0x00564D50 / 0x00564DC0]
 * ServerFramework::CServerChildWindowBase
 *
 * Native VTable @ 0x00AFA550 (16 bytes = 4 slots):
 *   Slot 0 (+0x00): virtual ~CServerChildWindowBase() (Scalar deleting destructor @ 0x00565000)
 *   Slot 1 (+0x04): virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) (Inherited from CWindowBase @ 0x00952B50)
 *   Slot 2 (+0x08): virtual void ReleaseGDI() (Native stub @ 0x0066B100, overridden by derived views)
 *   Slot 3 (+0x0C): virtual const char* GetWindowName() const (Native @ 0x005640B0, returns m_strWindowName)
 *
 * Memory Layout:
 *   +0x00 - +0x6F: ServerFramework::CWindowBase (112 bytes = 0x70)
 *   +0x70 - +0x8B: std::string m_strWindowName (28 bytes = 0x1C in MSVC 7.1 / 8.0)
 * Total Native Object Size: 0x8C bytes (140 bytes)
 */
class CServerChildWindowBase : public CWindowBase {
public:
	// [RECONSTRUCTED - Native 0x00564D50]
	CServerChildWindowBase();

	// [RECONSTRUCTED - Native 0x00564DC0]
	virtual ~CServerChildWindowBase() override;

	// [RECONSTRUCTED - Native 0x0066B100]
	// Slot 2 (+0x08): Default empty virtual method for releasing GDI surfaces/bitmaps
	virtual void ReleaseGDI();

	// [RECONSTRUCTED - Native 0x005640B0]
	// Slot 3 (+0x0C): Returns window name string for menu insertion and title display
	virtual const char* GetWindowName() const;

	void SetWindowName(const std::string& strName);

public:
	std::string m_strWindowName; // +0x70: Window name / title identifier
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERCHILDWINDOWBASE_H_
