/**
 * ============================================================================
 * Joymax ServerFramework - Server Architecture View Window
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerArchitectureView.h
 *
 * Implements CServerArchitectureView:
 *   - Native VTable @ 0x00B41724 (10 virtual slots = 0x28 bytes)
 *   - Native Constructor @ 0x0095D750 (422 bytes)
 *   - Native Destructor @ 0x0095D940 (114 bytes)
 *   - Native Scalar Deleting Destructor @ 0x0095D900 (30 bytes)
 *   - Native ReleaseGDI @ 0x0095DC80 (212 bytes)
 *   - Native Create @ 0x0095DA40 (574 bytes)
 *   - Native BuildArchitectureGraph @ 0x0095E570 (825 bytes)
 *   - Native LayoutNodeTree @ 0x0095E910 (238 bytes)
 *   - Native LayoutOrphanNodes @ 0x0095F530 (228 bytes)
 *   - Native DrawNode @ 0x0095E110 (545 bytes)
 *   - Native DrawConnections @ 0x0095E340 (457 bytes)
 *   - Total Native Object Size: 0x184 bytes (388 bytes)
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERARCHITECTUREVIEW_H_
#define _JMX_SERVERFRAMEWORK_SERVERARCHITECTUREVIEW_H_

#include "ServerChildWindowBase.h"
#include "ServerConfig.h"
#include "ServerTopology.h"
#include <cstdint>
#include <map>
#include <list>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HDC;
typedef void* HBITMAP;
typedef void* HCURSOR;
typedef void* HICON;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HMENU;
typedef struct tagPOINT {
	int32_t x;
	int32_t y;
} POINT;
#ifndef _TAGRECT_DEFINED_
#define _TAGRECT_DEFINED_
typedef struct tagRECT {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} RECT;
#endif
typedef struct tagSIZE {
	int32_t cx;
	int32_t cy;
} SIZE;
#endif

namespace ServerFramework {

// Forward declarations
struct tagServerNode;

// Native global Architecture View window handle @ 0x00C82810 (formerly mislabeled 0x00C827FC)
extern HWND g_hWndServerArchitectureView;

/**
 * tagServerNodeUI
 * UI node in the architecture graph view representing a single server entity
 * Native size: 0x28 bytes (40 bytes)
 * Proven from Native 0x0095F980 (copy constructor) & 0x0095D6B0 (destructor)
 */
struct tagServerNodeUI {
	RECT                        rcBounds;           // +0x00: Node bounding rectangle in view coordinates (16 bytes)
	tagServerNode*              pServerNode;        // +0x10: Underlying server configuration/state node (4 bytes)
	tagServerNodeUI*            pParentNode;        // +0x14: Pointer to parent UI node in tree (Native @ 0x0095E74B)
	std::list<tagServerNodeUI*> listChildren;       // +0x18: List of direct child nodes in topology tree (12 bytes)
	int32_t                     nStatusFlags;       // +0x24: Status flags / drawing state (bit 0: warning badge) (4 bytes)
};

/**
 * tagServerLinkUI
 * Alias to native tagServerLink descriptor @ 0x0095E570 / 0x0095E340
 */
typedef tagServerLink tagServerLinkUI;

/**
 * [RECONSTRUCTED - 0x0095D750 / 0x0095DA40]
 * ServerFramework::CServerArchitectureView
 *
 * Native VTable @ 0x00B41724 (40 bytes = 10 slots):
 *   Slot 0 (+0x00): virtual ~CServerArchitectureView() (Scalar deleting destructor @ 0x0095D900)
 *   Slot 1 (+0x04): virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) (Inherited @ 0x00952B50)
 *   Slot 2 (+0x08): virtual void ReleaseGDI() (Native @ 0x0095DC80)
 *   Slot 3 (+0x0C): virtual const char* GetWindowName() const (Inherited @ 0x005640B0)
 *   Slot 4 (+0x10): virtual bool Create(...) (Native @ 0x0095DA40)
 *   Slot 5 (+0x14): virtual void BuildArchitectureGraph() (Native @ 0x0095E570)
 *   Slot 6 (+0x18): virtual RECT* LayoutNodeTree(...) (Native @ 0x0095E910)
 *   Slot 7 (+0x1C): virtual RECT* LayoutOrphanNodes(...) (Native @ 0x0095F530)
 *   Slot 8 (+0x20): virtual void DrawNode(...) (Native @ 0x0095E110)
 *   Slot 9 (+0x24): virtual void DrawConnections(...) (Native @ 0x0095E340)
 *
 * Memory Layout (Exact 388 bytes / 0x184):
 *   +0x00 - +0x8B: CServerChildWindowBase (140 bytes = 0x8C)
 *   +0x8C:         tagServerNodeUI* m_pRootNode (Root server node)
 *   +0x90 - +0x9B: std::map<uint16_t, tagServerNodeUI> m_mapServerNodes (12 bytes)
 *   +0x9C - +0xA7: std::map<uint32_t, tagServerLink*> m_mapConnections (12 bytes)
 *   +0xA8:         SIZE m_szNodeSize (cx=40, cy=40)
 *   +0xB0:         SIZE m_szPadding (cx=30, cy=30)
 *   +0xB8:         bool m_bShowCertArch (default: true) - Menu ID 2: "Show Certification Architecture"
 *   +0xB9:         bool m_bShowServerCord (default: false) - Menu ID 3: "Show Server Cord"
 *   +0xBA:         bool m_bShowModuleName (default: true) - Menu ID 1: "Show Server Module Name"
 *   +0xBB:         bool m_bShowDetail (default: false) - Menu ID 4: "Show Detail"
 *   +0xBC:         POINT m_ptOrigin (x @ +0xBC, y @ +0xC0, default: 0, 0)
 *   +0xC4:         POINT m_ptLastMouse (x @ +0xC4, y @ +0xC8)
 *   +0xCC:         bool m_bDraggingCanvas (default: false)
 *   +0xCD - +0xCF: uint8_t m_padCD[3]
 *   +0xD0:         HDC m_hMemDC
 *   +0xD4:         HBITMAP m_hMemBmp
 *   +0xD8:         HCURSOR m_hCurrentCursor
 *   +0xDC:         tagServerNodeUI* m_pSelectedNode (Selected/dragged node)
 *   +0xE0 - +0x107: HICON m_hIcons[10] (40 bytes)
 *   +0x108:        int32_t m_nZoomLevel (default: 9)
 *   +0x10C - +0x183: float m_fZoomFactors[30] (120 bytes)
 */
class CServerArchitectureView : public CServerChildWindowBase {
public:
	// [RECONSTRUCTED - 0x0095D750]
	CServerArchitectureView();

	// [RECONSTRUCTED - 0x0095D940]
	virtual ~CServerArchitectureView() override;

	// Slot 2 (+0x08): ReleaseGDI [RECONSTRUCTED - 0x0095DC80]
	virtual void ReleaseGDI() override;

	// Slot 4 (+0x10): Create [RECONSTRUCTED - 0x0095DA40]
	virtual bool Create(HINSTANCE hInst, HWND hWndParent, int x, int y, int cx, int cy, uint32_t dwControlId);

	// Slot 5 (+0x14): BuildArchitectureGraph [RECONSTRUCTED - 0x0095E570]
	virtual void BuildArchitectureGraph();

	// Slot 6 (+0x18): LayoutNodeTree [RECONSTRUCTED - 0x0095E910]
	virtual RECT* LayoutNodeTree(RECT* pOutRect, tagServerNodeUI* pNode, int32_t xLeft, int32_t yTop);

	// Slot 7 (+0x1C): LayoutOrphanNodes [RECONSTRUCTED - 0x0095F530]
	virtual RECT* LayoutOrphanNodes(RECT* pOutRect);

	// Slot 8 (+0x20): DrawNode [RECONSTRUCTED - 0x0095E110]
	virtual void DrawNode(HDC hDC, tagServerNodeUI* pNode);

	// Slot 9 (+0x24): DrawConnections [RECONSTRUCTED - 0x0095E340]
	virtual void DrawConnections(HDC hDC);

	// Helper member functions
	// [RECONSTRUCTED - 0x0095D720]
	float GetZoomFactor() const;

	// [RECONSTRUCTED - 0x0095EAC0]
	POINT ScreenToLogical(int32_t screenX, int32_t screenY) const;

	// [RECONSTRUCTED - 0x0095EB40]
	void CalcNodeScreenRect(RECT* pOutRect, int32_t left, int32_t top, int32_t right, int32_t bottom) const;
	void CalcNodeScreenRect(tagServerNodeUI* pNode, RECT* pOutRect) const;

	// [RECONSTRUCTED - 0x0095E510]
	tagServerNodeUI* FindNode(uint16_t wServerID);

	// [RECONSTRUCTED - 0x0095EA00]
	tagServerNodeUI* HitTestNode(int32_t screenX, int32_t screenY);

	// [RECONSTRUCTED - 0x0095EBC0]
	void MoveNodeWithCollision(tagServerNodeUI* pNode, POINT* pDelta);

	// [RECONSTRUCTED - 0x0095ECC0]
	void ResolveCollision(const RECT& rcOther, RECT& rcMoving, const RECT& rcOrig, int32_t deltaX, int32_t deltaY);

	// [RECONSTRUCTED - 0x0095EE50]
	// Tests 2D intersection between segments (x1, y1)-(x2, y2) and (x3, y3)-(x4, y4)
	static bool LineIntersectionTest(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	                                 int32_t x3, int32_t y3, int32_t x4, int32_t y4);

	// Message Handlers
	// [RECONSTRUCTED - 0x0095DD60] (WM_PAINT @ 0x000F)
	void OnPaint();

	// [RECONSTRUCTED - 0x0095F0C0] (WM_LBUTTONDOWN @ 0x0201)
	int32_t OnLButtonDown(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F160] (WM_LBUTTONUP @ 0x0202)
	int32_t OnLButtonUp(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F1B0] (WM_MOUSEMOVE @ 0x0200)
	int32_t OnMouseMove(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F2E0] (WM_SETCURSOR @ 0x0020)
	int32_t OnSetCursor(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F300] (WM_RBUTTONUP @ 0x0205)
	int32_t OnRButtonUp(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F510] (Custom Notification @ 0x07E8)
	int32_t OnRefreshGraph(WPARAM wParam, LPARAM lParam);

	// [RECONSTRUCTED - 0x0095F520] (Custom Notification @ 0x07E9 / 0x07EA)
	int32_t OnRepaintGraph(WPARAM wParam, LPARAM lParam);

public:
	tagServerNodeUI*                    m_pRootNode;           // +0x8C: Root server node
	std::map<uint16_t, tagServerNodeUI> m_mapServerNodes;      // +0x90: Map of ServerID -> Node UI (12 bytes)
	std::map<uint32_t, tagServerLink*>  m_mapConnections;      // +0x9C: Active connections between servers (12 bytes)
	SIZE                                m_szNodeSize;          // +0xA8: Node icon dimensions (default: 40x40)
	SIZE                                m_szPadding;           // +0xB0: Padding / spacing (default: 30x30)
	bool                                m_bShowCertArch;       // +0xB8: Show certification architecture (default: true)
	bool                                m_bShowServerCord;     // +0xB9: Show connection cords (default: false)
	bool                                m_bShowModuleName;     // +0xBA: Show module name text labels (default: true)
	bool                                m_bShowDetail;         // +0xBB: Show detail IP/port strings (default: false)
	POINT                               m_ptOrigin;            // +0xBC: Viewport pan offset (x @ +0xBC, y @ +0xC0)
	POINT                               m_ptLastMouse;         // +0xC4: Last mouse coordinate (x @ +0xC4, y @ +0xC8)
	bool                                m_bDraggingCanvas;     // +0xCC: Mouse dragging canvas / pan flag (default: false)
	uint8_t                             m_padCD[3];            // +0xCD - +0xCF: Alignment padding
	HDC                                 m_hMemDC;              // +0xD0: Offscreen GDI DC for flicker-free rendering
	HBITMAP                             m_hMemBmp;             // +0xD4: Offscreen compatible bitmap
	HCURSOR                             m_hCurrentCursor;      // +0xD8: Active cursor handle
	tagServerNodeUI*                    m_pSelectedNode;       // +0xDC: Node currently being dragged (nullptr if canvas pan)
	HICON                               m_hIcons[10];          // +0xE0: 10 Server status icons (40 bytes)
	int32_t                             m_nZoomLevel;          // +0x108: Current zoom index (0..29, default: 9)
	float                               m_fZoomFactors[30];    // +0x10C: 30 Zoom scaling multipliers (120 bytes)
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERARCHITECTUREVIEW_H_
