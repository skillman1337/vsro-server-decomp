/**
 * ============================================================================
 * Joymax ServerFramework - Win32 Size Ruler (Split-Pane Layout Manager)
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\Win32SizeRuler.h
 *
 * Implements CWin32SizeRuler:
 *   - Native VTable @ 0x00B4163C (size 36 bytes = 9 slots)
 *   - Native Constructor @ 0x0095CBE0 (114 bytes)
 *   - Native Destructor @ 0x0095CC60 / 0x0095CC80 (115 bytes)
 *   - Base class for CServerFrameWindow
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_WIN32SIZERULER_H_
#define _JMX_SERVERFRAMEWORK_WIN32SIZERULER_H_

#include "WindowBase.h"
#include <list>
#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HCURSOR;
#ifndef _TAGRECT_DEFINED_
#define _TAGRECT_DEFINED_
typedef struct tagRECT {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} RECT;
#endif
#endif

namespace ServerFramework {

/**
 * [RECONSTRUCTED - 0x0095D470]
 * tagSizeRulerPane: Split-pane layout descriptor
 *
 * Native node payload size: 12 bytes (0x0C bytes)
 * Allocated node size: 20 bytes (0x14 bytes = 8 bytes list links + 12 bytes data)
 * Struct layout proven against machine bytes @ 0x0095D470 - 0x0095D490:
 *   - +0x08: float fRatio      (Relative height ratio, 0.0f - 1.0f)
 *   - +0x0C: HWND  hWndUpper   (Upper pane window handle)
 *   - +0x10: HWND  hWndLower   (Lower pane window handle)
 */
struct tagSizeRulerPane {
	float fRatio    = 0.0f;    // +0x08: Relative vertical ratio (0.0f - 1.0f)
	HWND  hWndUpper = nullptr; // +0x0C: Upper pane window handle
	HWND  hWndLower = nullptr; // +0x10: Lower pane window handle
};

/**
 * [RECONSTRUCTED - 0x0095CBE0]
 * CWin32SizeRuler
 * Native size: 0x8C bytes (140 bytes)
 *
 * Memory Layout:
 *   +0x00 - +0x6F: CWindowBase (112 bytes)
 *   +0x70: std::list<tagSizeRulerPane> m_listPanes (12 bytes, head: +0x74, size: +0x78)
 *   +0x7C: tagSizeRulerPane* m_pHitPane (active dragging splitter; initialized to NULL)
 *   +0x80: int32_t m_nDragX (mouse down X)
 *   +0x84: int32_t m_nDragY (mouse down Y)
 *   +0x88: HCURSOR m_hCursor (splitter resize cursor handle)
 *
 * Virtual Method Table (0x00B4163C):
 *   Slot 0 (+0x00): Destructor @ 0x0095CC60
 *   Slot 1 (+0x04): WindowProc @ 0x00952B50 (inherited from CWindowBase)
 *   Slot 2 (+0x08): Create @ 0x0095CD00
 *   Slot 3 (+0x0C): OnPaint @ 0x0095D290
 *   Slot 4 (+0x10): OnSize @ 0x0095CFE0
 *   Slot 5 (+0x14): OnLButtonDown @ 0x0095D000
 *   Slot 6 (+0x18): OnLButtonUp @ 0x0095D050
 *   Slot 7 (+0x1C): OnMouseMove @ 0x0095D090
 *   Slot 8 (+0x20): OnSetCursor @ 0x0095D3C0
 */
class CWin32SizeRuler : public CWindowBase {
public:
	// Native @ 0x0095CBE0 (114 bytes)
	CWin32SizeRuler();

	// Native @ 0x0095CC60 / 0x0095CC80 (115 bytes)
	virtual ~CWin32SizeRuler() override;

	// Slot 2 (+0x08) @ 0x0095CD00 (203 bytes)
	virtual bool Create(DWORD dwExStyle, const char* lpClassName, const char* lpWindowName,
	                    DWORD dwStyle, int x1, int y1, int x2, int y2,
	                    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance);

	// Slot 3 (+0x0C) @ 0x0095D290 (292 bytes)
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam);

	// Slot 4 (+0x10) @ 0x0095CFE0 (32 bytes)
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam);

	// Slot 5 (+0x14) @ 0x0095D000 (70 bytes)
	virtual LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);

	// Slot 6 (+0x18) @ 0x0095D050 (62 bytes)
	virtual LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);

	// Slot 7 (+0x1C) @ 0x0095D090 (497 bytes)
	virtual LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);

	// Slot 8 (+0x20) @ 0x0095D3C0 (25 bytes)
	virtual LRESULT OnSetCursor(WPARAM wParam, LPARAM lParam);

	// Layout and hit-testing helpers
	// Native @ 0x0095CE20 (173 bytes)
	tagSizeRulerPane* HitTestSplitter(int y);

	// Native @ 0x0095CED0 (259 bytes)
	bool RecalcLayout(int cx, int cy);

	// Split pane registration
	// Native @ 0x0095CDD0 (70 bytes)
	bool SetPaneRatio(HWND hWndUpper, HWND hWndLower, float fRatio);

public:
	std::list<tagSizeRulerPane> m_listPanes;                  // +0x70: Split pane list
	tagSizeRulerPane*           m_pHitPane = nullptr;         // +0x7C: Currently dragged splitter
	int32_t                     m_nDragX   = 0;               // +0x80: Mouse drag start X
	int32_t                     m_nDragY   = 0;               // +0x84: Mouse drag start Y
	HCURSOR                     m_hCursor  = nullptr;         // +0x88: Splitter cursor handle
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_WIN32SIZERULER_H_
