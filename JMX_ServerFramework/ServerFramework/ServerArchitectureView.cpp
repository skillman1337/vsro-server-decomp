/**
 * ============================================================================
 * Joymax ServerFramework - Server Architecture View Window Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerArchitectureView.cpp
 *
 * Implements CServerArchitectureView:
 *   - Native Constructor @ 0x0095D750 (422 bytes)
 *   - Native Destructor @ 0x0095D940 (114 bytes)
 *   - Native ReleaseGDI @ 0x0095DC80 (212 bytes)
 *   - Native Create @ 0x0095DA40 (574 bytes)
 *   - Native GetZoomFactor @ 0x0095D720 (47 bytes)
 *   - Native ScreenToLogical @ 0x0095EAC0 (124 bytes)
 *   - Native CalcNodeScreenRect @ 0x0095EB40 (121 bytes)
 *   - Native FindNode @ 0x0095E510 (82 bytes)
 *   - Native HitTestNode @ 0x0095EA00 (178 bytes)
 *   - Native MoveNodeWithCollision @ 0x0095EBC0 (249 bytes)
 *   - Native ResolveCollision @ 0x0095ECC0 (398 bytes)
 *   - Native LineIntersectionTest @ 0x0095EE50 (430 bytes)
 *   - Native BuildArchitectureGraph @ 0x0095E570 (825 bytes)
 *   - Native LayoutNodeTree @ 0x0095E910 (238 bytes)
 *   - Native LayoutOrphanNodes @ 0x0095F530 (228 bytes)
 *   - Native DrawNode @ 0x0095E110 (545 bytes)
 *   - Native DrawConnections @ 0x0095E340 (457 bytes)
 *   - Native OnPaint @ 0x0095DD60 (932 bytes)
 *   - Native OnLButtonDown @ 0x0095F0C0 (150 bytes)
 *   - Native OnLButtonUp @ 0x0095F160 (66 bytes)
 *   - Native OnMouseMove @ 0x0095F1B0 (294 bytes)
 *   - Native OnSetCursor @ 0x0095F2E0 (21 bytes)
 *   - Native OnRButtonUp @ 0x0095F300 (511 bytes)
 *   - Native OnRefreshGraph @ 0x0095F510 (12 bytes)
 *   - Native OnRepaintGraph @ 0x0095F520 (15 bytes)
 * ============================================================================
 */

#include "ServerArchitectureView.h"
#include "ServerFrameWindow.h"
#include "ServerTopology.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <algorithm>

namespace ServerFramework {

// Link style lookup table matching native 0x00C80B10 (g_linkDisplayStyles, 5 states * 12 bytes)
struct tagLinkStyleEntry {
	COLORREF clrLine;
	int32_t  nWidth;
	int32_t  nPenStyle;
};

static const tagLinkStyleEntry s_aLinkStyles[5] = {
	{ RGB(129, 129, 129), 2, 1 /* PS_SOLID */ }, // State 0: Normal / Idle
	{ RGB(255, 255, 255), 0, 1 /* PS_SOLID */ }, // State 1: Connected
	{ RGB(0,   0,   255), 0, 1 /* PS_SOLID */ }, // State 2: Active / Data Transmitting (Blue)
	{ RGB(255, 0,   0),   0, 2 /* PS_DOT */ },   // State 3: Error / Disconnected (Red dotted)
	{ RGB(128, 128, 0),   0, 1 /* PS_SOLID */ }  // State 4: Standby
};

// Global window handle for server topology graph notifications (Native 0x00C827FC)
HWND g_hWndServerArchitectureView = nullptr;

// Resource IDs for the 10 server status icons (Native 0x0095DBC9 - 0x0095DC68)
static const int s_aServerIconIDs[10] = {
	112, // 0x70: m_hIcons[0] - GlobalManager
	105, // 0x69: m_hIcons[1] - MachineManager
	107, // 0x6B: m_hIcons[2] - GatewayServer
	102, // 0x66: m_hIcons[3] - DownloadServer
	108, // 0x6C: m_hIcons[4] - FarmServer
	106, // 0x6A: m_hIcons[5] - AgentServer
	104, // 0x68: m_hIcons[6] - SR_Client
	113, // 0x71: m_hIcons[7] - GameServer
	103, // 0x67: m_hIcons[8] - Unknown/Aux
	111  // 0x6F: m_hIcons[9] - Warning / Alert Badge
};

// Helper GDI Line drawing routine matching BSLib GDI_DrawLine
static void GDI_DrawLine(HDC hDC, int32_t x1, int32_t y1, int32_t x2, int32_t y2, COLORREF color, int32_t style = PS_SOLID, int32_t width = 1) {
#ifdef _WIN32
	HPEN hPen = CreatePen(style, width, color);
	HGDIOBJ hOldPen = SelectObject(hDC, hPen);
	MoveToEx(hDC, x1, y1, nullptr);
	LineTo(hDC, x2, y2);
	SelectObject(hDC, hOldPen);
	DeleteObject(hPen);
#else
	(void)hDC; (void)x1; (void)y1; (void)x2; (void)y2; (void)color; (void)style; (void)width;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095D750]
 * CServerArchitectureView constructor
 * Native implementation: 422 bytes @ 0x0095D750
 */
CServerArchitectureView::CServerArchitectureView()
	: CServerChildWindowBase()
	, m_pRootNode(nullptr)
	, m_szNodeSize{ 40, 40 }
	, m_szPadding{ 30, 30 }
	, m_bShowCertArch(true)
	, m_bShowServerCord(false)
	, m_bShowModuleName(true)
	, m_bShowDetail(false)
	, m_ptOrigin{ 0, 0 }
	, m_ptLastMouse{ 0, 0 }
	, m_bDraggingCanvas(false)
	, m_hMemDC(nullptr)
	, m_hMemBmp(nullptr)
	, m_hCurrentCursor(nullptr)
	, m_pSelectedNode(nullptr)
	, m_nZoomLevel(9) {
	std::memset(m_padCD, 0, sizeof(m_padCD));

#ifdef _WIN32
	m_hCurrentCursor = LoadCursorA(nullptr, IDC_ARROW);
#endif

	for (auto& hIcon : m_hIcons) {
		hIcon = nullptr;
	}

	for (int i = 0; i < 30; ++i) {
		m_fZoomFactors[i] = 0.1f + static_cast<float>(i) * 0.1f;
	}

	SetWindowName("ServerArchitectureView");
}

/**
 * [RECONSTRUCTED - 0x0095D940]
 * CServerArchitectureView destructor
 * Native implementation: 114 bytes @ 0x0095D940
 */
CServerArchitectureView::~CServerArchitectureView() {
	m_mapConnections.clear();
	m_mapServerNodes.clear();
}

/**
 * [RECONSTRUCTED - 0x0095DC80]
 * CServerArchitectureView::ReleaseGDI
 * Native implementation: 212 bytes @ 0x0095DC80
 */
void CServerArchitectureView::ReleaseGDI() {
#ifdef _WIN32
	for (auto& hIcon : m_hIcons) {
		if (hIcon != nullptr) {
			DeleteObject(hIcon);
			hIcon = nullptr;
		}
	}

	if (m_hMemDC != nullptr) {
		DeleteDC(static_cast<HDC>(m_hMemDC));
		m_hMemDC = nullptr;
	}

	if (m_hMemBmp != nullptr) {
		DeleteObject(static_cast<HBITMAP>(m_hMemBmp));
		m_hMemBmp = nullptr;
	}
#endif

	m_mapServerNodes.clear();
	m_mapConnections.clear();

#ifdef _WIN32
	if (m_hWnd != nullptr && IsWindow(static_cast<HWND>(m_hWnd))) {
		DestroyWindow(static_cast<HWND>(m_hWnd));
		m_hWnd = nullptr;
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x0095DA40]
 * CServerArchitectureView::Create
 * Native implementation: 574 bytes @ 0x0095DA40
 */
bool CServerArchitectureView::Create(HINSTANCE hInst, HWND hWndParent, int x, int y, int cx, int cy, uint32_t dwControlId) {
#ifdef _WIN32
	if (!hWndParent || !IsWindow(hWndParent)) {
		return false;
	}

	HINSTANCE hInstance = hInst ? hInst : GetModuleHandleA(nullptr);

	WNDCLASSA wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = DefWindowProcA;
	wc.hInstance     = hInstance;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = "ServerArchitectureView";
	RegisterClassA(&wc);

	m_hWnd = CreateWindowExA(
		WS_EX_CLIENTEDGE,
		"ServerArchitectureView",
		nullptr,
		WS_CHILD | WS_VISIBLE,
		x, y, cx, cy,
		hWndParent,
		reinterpret_cast<HMENU>(static_cast<uintptr_t>(dwControlId)),
		hInstance,
		nullptr
	);

	if (!m_hWnd) {
		return false;
	}

	// Register message handlers matching native 0x0095DAC2 - 0x0095DB5E
	RegisterMessageHandler(WM_PAINT, [this](WPARAM, LPARAM) -> LRESULT {
		OnPaint();
		return 0;
	});

	RegisterMessageHandler(WM_LBUTTONDOWN, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnLButtonDown(w, l);
	});

	RegisterMessageHandler(WM_LBUTTONUP, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnLButtonUp(w, l);
	});

	RegisterMessageHandler(WM_MOUSEMOVE, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnMouseMove(w, l);
	});

	RegisterMessageHandler(WM_ERASEBKGND, [](WPARAM, LPARAM) -> LRESULT {
		return 1;
	});

	RegisterMessageHandler(WM_SETCURSOR, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnSetCursor(w, l);
	});

	RegisterMessageHandler(WM_RBUTTONUP, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnRButtonUp(w, l);
	});

	RegisterMessageHandler(0x07E8, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnRefreshGraph(w, l);
	});

	RegisterMessageHandler(0x07E9, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnRepaintGraph(w, l);
	});

	RegisterMessageHandler(0x07EA, [this](WPARAM w, LPARAM l) -> LRESULT {
		return OnRepaintGraph(w, l);
	});

	::ShowWindow(static_cast<HWND>(m_hWnd), SW_SHOW);
	::UpdateWindow(static_cast<HWND>(m_hWnd));

	// Setup double buffer GDI DC and fullscreen bitmap (Native @ 0x0095DB6F - 0x0095DBB8)
	HDC hDC = GetDC(static_cast<HWND>(m_hWnd));
	if (hDC) {
		m_hMemDC = CreateCompatibleDC(hDC);
		int nMaxX = GetSystemMetrics(SM_CXFULLSCREEN);
		int nMaxY = GetSystemMetrics(SM_CYFULLSCREEN);
		m_hMemBmp = CreateCompatibleBitmap(hDC, nMaxX, nMaxY);
		SelectObject(static_cast<HDC>(m_hMemDC), static_cast<HBITMAP>(m_hMemBmp));
		ReleaseDC(static_cast<HWND>(m_hWnd), hDC);
	}

	// Load 10 status icons (Native @ 0x0095DBBE - 0x0095DC68)
	for (int i = 0; i < 10; ++i) {
		m_hIcons[i] = LoadIconA(hInstance, MAKEINTRESOURCEA(s_aServerIconIDs[i]));
	}

	// Publish window handle to global pointer @ 0x00C82810
	g_hWndServerArchitectureView = static_cast<HWND>(m_hWnd);
	return true;
#else
	(void)hInst; (void)hWndParent; (void)x; (void)y; (void)cx; (void)cy; (void)dwControlId;
	return true;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095D720]
 * CServerArchitectureView::GetZoomFactor
 * Native implementation: 47 bytes @ 0x0095D720
 */
float CServerArchitectureView::GetZoomFactor() const {
	if (m_nZoomLevel < 0 || m_nZoomLevel >= 30) {
		return 1.0f;
	}
	return m_fZoomFactors[m_nZoomLevel];
}

/**
 * [RECONSTRUCTED - 0x0095EAC0]
 * CServerArchitectureView::ScreenToLogical
 * Native implementation: 124 bytes @ 0x0095EAC0
 *
 * Converts screen coordinates to logical architecture graph coordinates:
 *   ptLogical.x = (screenX - m_ptOrigin.x) / m_fZoomFactors[m_nZoomLevel]
 *   ptLogical.y = (screenY - m_ptOrigin.y) / m_fZoomFactors[m_nZoomLevel]
 */
POINT CServerArchitectureView::ScreenToLogical(int32_t screenX, int32_t screenY) const {
	POINT pt = { 0, 0 };
	float fZoom = GetZoomFactor();
	if (std::fabs(fZoom) < 0.0001f) {
		fZoom = 1.0f;
	}

	pt.x = static_cast<int32_t>(static_cast<float>(screenX - m_ptOrigin.x) / fZoom);
	pt.y = static_cast<int32_t>(static_cast<float>(screenY - m_ptOrigin.y) / fZoom);
	return pt;
}

/**
 * [RECONSTRUCTED - 0x0095EB40]
 * CServerArchitectureView::CalcNodeScreenRect
 * Native implementation: 121 bytes @ 0x0095EB40
 *
 * Translates a logical node bounding rectangle into screen coordinates,
 * preserving width and height while centering around the zoom/pan transformed center.
 */
void CServerArchitectureView::CalcNodeScreenRect(RECT* pOutRect, int32_t left, int32_t top, int32_t right, int32_t bottom) const {
	if (!pOutRect) {
		return;
	}

	int32_t width = right - left;
	int32_t height = bottom - top;
	int32_t centerX = (left + right) / 2;
	int32_t centerY = (top + bottom) / 2;

	POINT ptCenter = ScreenToLogical(centerX, centerY);

	pOutRect->left   = ptCenter.x - width / 2;
	pOutRect->top    = ptCenter.y - height / 2;
	pOutRect->right  = pOutRect->left + width;
	pOutRect->bottom = pOutRect->top + height;
}

void CServerArchitectureView::CalcNodeScreenRect(tagServerNodeUI* pNode, RECT* pOutRect) const {
	if (pNode && pOutRect) {
		CalcNodeScreenRect(pOutRect, pNode->rcBounds.left, pNode->rcBounds.top, pNode->rcBounds.right, pNode->rcBounds.bottom);
	}
}

/**
 * [RECONSTRUCTED - 0x0095E510]
 * CServerArchitectureView::FindNode
 * Native implementation: 82 bytes @ 0x0095E510
 */
tagServerNodeUI* CServerArchitectureView::FindNode(uint16_t wServerID) {
	auto it = m_mapServerNodes.find(wServerID);
	if (it != m_mapServerNodes.end()) {
		return &it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095EA00]
 * CServerArchitectureView::HitTestNode
 * Native implementation: 178 bytes @ 0x0095EA00
 */
tagServerNodeUI* CServerArchitectureView::HitTestNode(int32_t screenX, int32_t screenY) {
#ifdef _WIN32
	POINT pt = { screenX, screenY };
	for (auto& pair : m_mapServerNodes) {
		RECT rcScreen = {};
		CalcNodeScreenRect(&pair.second, &rcScreen);
		if (PtInRect(&rcScreen, pt)) {
			return &pair.second;
		}
	}
#else
	(void)screenX; (void)screenY;
#endif
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095EE50]
 * CServerArchitectureView::LineIntersectionTest
 * Native implementation: 430 bytes @ 0x0095EE50
 *
 * Exact 2D line segment intersection test between (x1, y1)-(x2, y2) and (x3, y3)-(x4, y4).
 * Returns true if the segments cross, false if parallel or non-overlapping.
 */
bool CServerArchitectureView::LineIntersectionTest(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                                                  int32_t x3, int32_t y3, int32_t x4, int32_t y4) {
	float dx1 = static_cast<float>(x2 - x1);
	float dy1 = static_cast<float>(y2 - y1);
	float dx2 = static_cast<float>(x4 - x3);
	float dy2 = static_cast<float>(y4 - y3);

	float det = dy2 * dx1 - dy1 * dx2;
	float det2 = det * det;

	float len1_sq = dy1 * dy1 + dx1 * dx1;
	float len2_sq = dy2 * dy2 + dx2 * dx2;
	float eps_term = len1_sq * FLT_EPSILON * len2_sq;

	// Check if segments are collinear / parallel within epsilon
	if (det2 < eps_term) {
		return false;
	}

	// Calculate intersection parameter t
	float t = (dy2 * static_cast<float>(x3 - x1) - dx2 * static_cast<float>(y3 - y1)) / det;
	float intersectX = static_cast<float>(x1) + dx1 * t;
	float intersectY = static_cast<float>(y1) + dy1 * t;

	// Verify that intersection point lies within the bounding box of both segments
	float minX1 = static_cast<float>(std::min(x1, x2));
	float maxX1 = static_cast<float>(std::max(x1, x2));
	float minX2 = static_cast<float>(std::min(x3, x4));
	float maxX2 = static_cast<float>(std::max(x3, x4));

	if (intersectX < minX1 || intersectX > maxX1 || intersectX < minX2 || intersectX > maxX2) {
		return false;
	}

	float minY1 = static_cast<float>(std::min(y1, y2));
	float maxY1 = static_cast<float>(std::max(y1, y2));
	float minY2 = static_cast<float>(std::min(y3, y4));
	float maxY2 = static_cast<float>(std::max(y3, y4));

	if (intersectY < minY1 || intersectY > maxY1 || intersectY < minY2 || intersectY > maxY2) {
		return false;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0095ECC0]
 * CServerArchitectureView::ResolveCollision
 * Native implementation: 398 bytes @ 0x0095ECC0
 *
 * Performs continuous axis-aligned bounding box collision resolution with edge snapping:
 *   - deltaX > 0: Moving right -> tests intersection with left edge of other node, snaps right edge.
 *   - deltaX < 0: Moving left  -> tests intersection with right edge of other node, snaps left edge.
 *   - deltaY > 0: Moving down  -> tests intersection with top edge of other node, snaps bottom edge.
 *   - deltaY < 0: Moving up    -> tests intersection with bottom edge of other node, snaps top edge.
 */
void CServerArchitectureView::ResolveCollision(const RECT& rcOther, RECT& rcMoving, const RECT& rcOrig, int32_t deltaX, int32_t deltaY) {
#ifdef _WIN32
	// 1. Horizontal resolution
	if (deltaX > 0) {
		bool bHit1 = LineIntersectionTest(rcMoving.right, rcMoving.top, rcOrig.right, rcOrig.top,
		                                  rcOther.left, rcOther.top, rcOther.left, rcOther.bottom);
		bool bHit2 = false;
		if (!bHit1) {
			bHit2 = LineIntersectionTest(rcMoving.right, rcMoving.bottom, rcOrig.right, rcOrig.bottom,
			                             rcOther.left, rcOther.top, rcOther.left, rcOther.bottom);
		}
		if ((bHit1 || bHit2) && rcMoving.right > rcOther.left) {
			OffsetRect(&rcMoving, rcOther.left - rcMoving.right, 0);
		}
	} else if (deltaX < 0) {
		bool bHit1 = LineIntersectionTest(rcMoving.left, rcMoving.top, rcOrig.left, rcOrig.top,
		                                  rcOther.right, rcOther.top, rcOther.right, rcOther.bottom);
		bool bHit2 = false;
		if (!bHit1) {
			bHit2 = LineIntersectionTest(rcMoving.left, rcMoving.bottom, rcOrig.left, rcOrig.bottom,
			                             rcOther.right, rcOther.top, rcOther.right, rcOther.bottom);
		}
		if ((bHit1 || bHit2) && rcMoving.left < rcOther.right) {
			OffsetRect(&rcMoving, rcOther.right - rcMoving.left, 0);
		}
	}

	// 2. Vertical resolution
	if (deltaY > 0) {
		bool bHit1 = LineIntersectionTest(rcMoving.left, rcMoving.bottom, rcOrig.left, rcOrig.bottom,
		                                  rcOther.left, rcOther.top, rcOther.right, rcOther.top);
		bool bHit2 = false;
		if (!bHit1) {
			bHit2 = LineIntersectionTest(rcMoving.right, rcMoving.bottom, rcOrig.right, rcOrig.bottom,
			                             rcOther.left, rcOther.top, rcOther.right, rcOther.top);
		}
		if ((bHit1 || bHit2) && rcMoving.bottom > rcOther.top) {
			OffsetRect(&rcMoving, 0, rcOther.top - rcMoving.bottom);
		}
	} else if (deltaY < 0) {
		bool bHit1 = LineIntersectionTest(rcMoving.left, rcMoving.top, rcOrig.left, rcOrig.top,
		                                  rcOther.left, rcOther.bottom, rcOther.right, rcOther.bottom);
		bool bHit2 = false;
		if (!bHit1) {
			bHit2 = LineIntersectionTest(rcMoving.right, rcMoving.top, rcOrig.right, rcOrig.top,
			                             rcOther.left, rcOther.bottom, rcOther.right, rcOther.bottom);
		}
		if ((bHit1 || bHit2) && rcMoving.top < rcOther.bottom) {
			OffsetRect(&rcMoving, 0, rcOther.bottom - rcMoving.top);
		}
	}
#else
	(void)rcOther; (void)rcMoving; (void)rcOrig; (void)deltaX; (void)deltaY;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095EBC0]
 * CServerArchitectureView::MoveNodeWithCollision
 * Native implementation: 249 bytes @ 0x0095EBC0
 */
void CServerArchitectureView::MoveNodeWithCollision(tagServerNodeUI* pNode, POINT* pDelta) {
	if (!pNode || !pDelta) {
		return;
	}

#ifdef _WIN32
	RECT rcNew = pNode->rcBounds;
	OffsetRect(&rcNew, pDelta->x, pDelta->y);

	for (auto& pair : m_mapServerNodes) {
		tagServerNodeUI& other = pair.second;
		if (&other != pNode) {
			RECT rcDst = {};
			if (IntersectRect(&rcDst, &rcNew, &other.rcBounds)) {
				ResolveCollision(other.rcBounds, rcNew, pNode->rcBounds, pDelta->x, pDelta->y);
			}
		}
	}

	pDelta->x = rcNew.left - pNode->rcBounds.left;
	pDelta->y = rcNew.top - pNode->rcBounds.top;
	pNode->rcBounds = rcNew;
#else
	pNode->rcBounds.left   += pDelta->x;
	pNode->rcBounds.right  += pDelta->x;
	pNode->rcBounds.top    += pDelta->y;
	pNode->rcBounds.bottom += pDelta->y;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095E570]
 * CServerArchitectureView::BuildArchitectureGraph
 * Native implementation: 825 bytes @ 0x0095E570
 *
 * Traverses active server topology from ServerFramework_GetGameServerList()
 * and ServerFramework_GetAgentServerList(), populates m_mapServerNodes and
 * m_mapConnections, builds parent-child tree hierarchy, and triggers layout.
 */
void CServerArchitectureView::BuildArchitectureGraph() {
	m_mapServerNodes.clear();
	m_mapConnections.clear();
	m_pRootNode = nullptr;

	// 1. Populate server nodes from GameServer list (0x0095E585 - 0x0095E610)
	std::list<CServerNode*>* pGameServerList = ServerFramework_GetGameServerList();
	if (pGameServerList) {
		for (CServerNode* pNode : *pGameServerList) {
			if (!pNode) continue;

			tagServerNodeUI nodeUI = {};
			nodeUI.pServerNode   = pNode;
			nodeUI.pParentNode   = nullptr;
			nodeUI.nStatusFlags  = std::rand() & 1; // Native initial jitter state
#ifdef _WIN32
			SetRect(&nodeUI.rcBounds, 0, 0, m_szNodeSize.cx, m_szNodeSize.cy);
#else
			nodeUI.rcBounds.left   = 0;
			nodeUI.rcBounds.top    = 0;
			nodeUI.rcBounds.right  = m_szNodeSize.cx;
			nodeUI.rcBounds.bottom = m_szNodeSize.cy;
#endif
			m_mapServerNodes[pNode->wServerID] = nodeUI;
		}
	}

	// 2. Populate communication connections from AgentServer list (0x0095E615 - 0x0095E668)
	std::list<CServerLink*>* pAgentServerList = ServerFramework_GetAgentServerList();
	if (pAgentServerList) {
		for (CServerLink* pLink : *pAgentServerList) {
			if (!pLink) continue;
			m_mapConnections[pLink->dwLinkID] = pLink;
		}
	}

	// 3. Build tree parent-child linkages (0x0095E6E5 - 0x0095E7E0)
	for (auto& pair : m_mapServerNodes) {
		tagServerNodeUI& childNode = pair.second;
		if (!childNode.pServerNode) continue;

		uint16_t wParentID = childNode.pServerNode->wParentServerID;
		if (wParentID != 0) {
			childNode.pParentNode = FindNode(wParentID);
		}

		// Connect child nodes to this parent
		for (auto& otherPair : m_mapServerNodes) {
			tagServerNodeUI& possibleChild = otherPair.second;
			if (possibleChild.pServerNode && possibleChild.pServerNode->wParentServerID == childNode.pServerNode->wServerID) {
				childNode.listChildren.push_back(&possibleChild);
			}
		}
	}

	// 4. Resolve root node for layout by walking up parent chain (0x0095E7EF - 0x0095E84A)
	if (g_pLocalServerInfo) {
		tagServerNodeUI* pCurrent = FindNode(g_pLocalServerInfo->wParentServerID);
		while (pCurrent != nullptr) {
			m_pRootNode = pCurrent;
			if (!pCurrent->pServerNode || pCurrent->pServerNode->wParentServerID == 0) {
				break;
			}
			pCurrent = FindNode(pCurrent->pServerNode->wParentServerID);
		}

		if (!m_pRootNode) {
			m_pRootNode = FindNode(g_pLocalServerInfo->wServerID);
		}
	}

	if (!m_pRootNode && !m_mapServerNodes.empty()) {
		m_pRootNode = &m_mapServerNodes.begin()->second;
	}

	// 5. Layout tree recursively and compute bounds (0x0095E865 - 0x0095E8A8)
	if (m_pRootNode) {
		RECT rcBounds = {};
		LayoutNodeTree(&rcBounds, m_pRootNode, 0, 0);
		LayoutOrphanNodes(&rcBounds);
		RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
	}
}

/**
 * [RECONSTRUCTED - 0x0095E910]
 * CServerArchitectureView::LayoutNodeTree
 * Native implementation: 238 bytes @ 0x0095E910
 *
 * Positions the hierarchy of nodes using recursive top-down tree placement.
 */
RECT* CServerArchitectureView::LayoutNodeTree(RECT* pOutRect, tagServerNodeUI* pNode, int32_t xLeft, int32_t yTop) {
	if (!pOutRect) {
		return nullptr;
	}

#ifdef _WIN32
	SetRect(pOutRect, 0, 0, 0, 0);
	if (!pNode) {
		return pOutRect;
	}

	SetRect(&pNode->rcBounds, xLeft, yTop, xLeft + m_szNodeSize.cx, yTop + m_szNodeSize.cy);
	CopyRect(pOutRect, &pNode->rcBounds);

	int32_t nextX = xLeft + m_szNodeSize.cx + m_szPadding.cx;

	for (tagServerNodeUI* pChild : pNode->listChildren) {
		if (pChild) {
			RECT rcChild = {};
			LayoutNodeTree(&rcChild, pChild, nextX, yTop);

			int32_t childHeight = rcChild.bottom - rcChild.top;
			yTop = rcChild.bottom + m_szPadding.cy;
			if (childHeight > m_szNodeSize.cy) {
				yTop += m_szPadding.cy;
			}

			UnionRect(pOutRect, pOutRect, &rcChild);
		}
	}
#else
	(void)pNode; (void)xLeft; (void)yTop;
	pOutRect->left = 0; pOutRect->top = 0; pOutRect->right = 0; pOutRect->bottom = 0;
#endif

	return pOutRect;
}

/**
 * [RECONSTRUCTED - 0x0095F530]
 * CServerArchitectureView::LayoutOrphanNodes
 * Native implementation: 228 bytes @ 0x0095F530
 *
 * Positions any unpositioned orphan nodes (left == 0 && top == 0) in an organized
 * vertical stack above the root node column.
 */
RECT* CServerArchitectureView::LayoutOrphanNodes(RECT* pOutRect) {
	if (!pOutRect) {
		return nullptr;
	}

#ifdef _WIN32
	SetRect(pOutRect, 0, 0, 0, 0);

	tagServerNodeUI* pRoot = nullptr;
	if (g_pLocalServerInfo) {
		pRoot = FindNode(g_pLocalServerInfo->wServerID);
	}

	int32_t xLeft = pRoot ? pRoot->rcBounds.left : 0;
	int32_t yTop  = -(m_szPadding.cy + m_szNodeSize.cy);

	// Stack unplaced nodes (left == 0 && top == 0) vertically
	for (auto& pair : m_mapServerNodes) {
		tagServerNodeUI& node = pair.second;
		if (&node != m_pRootNode && node.rcBounds.left == 0 && node.rcBounds.top == 0) {
			SetRect(&node.rcBounds, xLeft, yTop, xLeft + m_szNodeSize.cx, yTop + m_szNodeSize.cy);
			yTop -= (m_szPadding.cy + m_szNodeSize.cy);
		}
	}

	// Compute overall bounding rectangle
	for (const auto& pair : m_mapServerNodes) {
		UnionRect(pOutRect, pOutRect, &pair.second.rcBounds);
	}
#else
	pOutRect->left = 0; pOutRect->top = 0; pOutRect->right = 0; pOutRect->bottom = 0;
#endif

	return pOutRect;
}

/**
 * [RECONSTRUCTED - 0x0095E110]
 * CServerArchitectureView::DrawNode
 * Native implementation: 545 bytes @ 0x0095E110
 *
 * Renders the server node icon, status badge, and centered text label.
 */
void CServerArchitectureView::DrawNode(HDC hDC, tagServerNodeUI* pNode) {
	if (!hDC || !pNode || !pNode->pServerNode) {
		return;
	}

#ifdef _WIN32
	// 1. Calculate node screen rectangle
	RECT rcScreen = {};
	CalcNodeScreenRect(pNode, &rcScreen);

	// 2. Draw Server Type Icon
	uint32_t nState = pNode->pServerNode->nState;
	if (nState < 10 && m_hIcons[nState] != nullptr) {
		DrawIcon(static_cast<HDC>(hDC), rcScreen.left, rcScreen.top, static_cast<HICON>(m_hIcons[nState]));
	}

	// 3. Draw Warning Badge if present (Icon 9 @ rcScreen.right - 2, rcScreen.top - 15)
	if ((pNode->nStatusFlags & 1) && m_hIcons[9] != nullptr) {
		DrawIcon(static_cast<HDC>(hDC), rcScreen.right - 2, rcScreen.top - 15, static_cast<HICON>(m_hIcons[9]));
	}

	// 4. Draw Server Text Label if enabled
	if (m_bShowModuleName) {
		char szLabel[256] = {};
		if (!m_bShowDetail) {
			std::snprintf(szLabel, sizeof(szLabel), "%2d-%s",
				static_cast<int>(pNode->pServerNode->wServerID),
				pNode->pServerNode->szName);
		} else {
			char szIP[32] = {};
			uint32_t ip = pNode->pServerNode->dwIP;
			std::snprintf(szIP, sizeof(szIP), "%u.%u.%u.%u",
				(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

			std::snprintf(szLabel, sizeof(szLabel), "%2d-%s (%s)-(%u)",
				static_cast<int>(pNode->pServerNode->wServerID),
				pNode->pServerNode->szName,
				szIP,
				static_cast<unsigned>(pNode->pServerNode->wPort));
		}

		// Select font & measure text
		HFONT hFont = CreateFontA(9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
		HGDIOBJ hOldFont = SelectObject(static_cast<HDC>(hDC), hFont);

		SIZE sizeText = {};
		GetTextExtentPoint32A(static_cast<HDC>(hDC), szLabel, static_cast<int>(std::strlen(szLabel)), &sizeText);

		// Center text under icon
		int32_t nodeCenterX = (rcScreen.left + rcScreen.right) / 2;
		int32_t textLeft = nodeCenterX - sizeText.cx / 2;
		int32_t textTop  = rcScreen.top + 34;

		RECT rcBadge = { textLeft, textTop, textLeft + sizeText.cx, textTop + 16 };
		RECT rcInflated = rcBadge;
		InflateRect(&rcInflated, 4, 1);

		// Highlight local server in yellow RGB(255, 255, 0), others in white RGB(255, 255, 255)
		COLORREF clrBadge = (pNode->pServerNode == g_pLocalServerInfo) ? RGB(255, 255, 0) : RGB(255, 255, 255);
		HBRUSH hBrush = CreateSolidBrush(clrBadge);
		FillRect(static_cast<HDC>(hDC), &rcInflated, hBrush);
		DeleteObject(hBrush);

		// Frame badge with black border
		FrameRect(static_cast<HDC>(hDC), &rcInflated, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

		// Text: Transparent background, black text
		SetBkMode(static_cast<HDC>(hDC), TRANSPARENT);
		SetTextColor(static_cast<HDC>(hDC), RGB(0, 0, 0));
		DrawTextA(static_cast<HDC>(hDC), szLabel, -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SelectObject(static_cast<HDC>(hDC), hOldFont);
		DeleteObject(hFont);
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x0095E340]
 * CServerArchitectureView::DrawConnections
 * Native implementation: 457 bytes @ 0x0095E340
 *
 * Renders communication lines and link badges between active server nodes.
 */
void CServerArchitectureView::DrawConnections(HDC hDC) {
	if (!hDC) {
		return;
	}

#ifdef _WIN32
	SetBkMode(static_cast<HDC>(hDC), TRANSPARENT);

	for (const auto& pair : m_mapConnections) {
		const tagServerLink* pLink = pair.second;
		if (!pLink) {
			continue;
		}

		tagServerNodeUI* pSrc = FindNode(pLink->wServer1);
		tagServerNodeUI* pDst = FindNode(pLink->wServer2);
		if (!pSrc || !pDst) {
			continue;
		}

		// Compute node screen centers
		RECT rcSrcScreen = {};
		RECT rcDstScreen = {};
		CalcNodeScreenRect(pSrc, &rcSrcScreen);
		CalcNodeScreenRect(pDst, &rcDstScreen);

		int32_t srcCenterX = (rcSrcScreen.left + rcSrcScreen.right) / 2;
		int32_t srcCenterY = (rcSrcScreen.top + rcSrcScreen.bottom) / 2;
		int32_t dstCenterX = (rcDstScreen.left + rcDstScreen.right) / 2;
		int32_t dstCenterY = (rcDstScreen.top + rcDstScreen.bottom) / 2;

		// Select style based on link state (0..4)
		uint32_t nState = pLink->nLinkState;
		if (nState > 4) nState = 0;
		const tagLinkStyleEntry& style = s_aLinkStyles[nState];

		// If detail enabled, draw link ID badge at midpoint
		if (m_bShowDetail) {
			int32_t midX = (srcCenterX + dstCenterX) / 2;
			int32_t midY = (srcCenterY + dstCenterY) / 2;

			char szLinkText[32] = {};
			if (pLink->byPadding == 0) {
				std::snprintf(szLinkText, sizeof(szLinkText), "%02d", static_cast<int>(pLink->dwLinkID));
			} else {
				std::snprintf(szLinkText, sizeof(szLinkText), "%02dP", static_cast<int>(pLink->dwLinkID));
			}

			SetTextColor(static_cast<HDC>(hDC), style.clrLine);
			TextOutA(static_cast<HDC>(hDC), midX, midY, szLinkText, static_cast<int>(std::strlen(szLinkText)));
		}

		// Draw connecting line
		GDI_DrawLine(static_cast<HDC>(hDC), srcCenterX, srcCenterY, dstCenterX, dstCenterY, style.clrLine, style.nPenStyle, style.nWidth);
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x0095DD60]
 * CServerArchitectureView::OnPaint
 * Native implementation: 932 bytes @ 0x0095DD60
 */
void CServerArchitectureView::OnPaint() {
#ifdef _WIN32
	PAINTSTRUCT ps = {};
	HDC hDC = BeginPaint(static_cast<HWND>(m_hWnd), &ps);
	if (!hDC) return;

	RECT rcClient = {};
	GetClientRect(&rcClient);

	// Clear memory DC background
	HBRUSH hBrBkgnd = GetSysColorBrush(COLOR_BTNFACE);
	FillRect(static_cast<HDC>(m_hMemDC), &rcClient, hBrBkgnd);

	// 1. Draw connecting lines if enabled
	if (m_bShowServerCord) {
		DrawConnections(m_hMemDC);
	}

	// 2. Draw all server nodes
	for (auto& pair : m_mapServerNodes) {
		DrawNode(m_hMemDC, &pair.second);
	}

	// 3. Draw 4 red selection handles on selected node (Native 0x0095DEF8 - 0x0095DF6E)
	if (m_pSelectedNode) {
		RECT rcSelected = {};
		CalcNodeScreenRect(m_pSelectedNode, &rcSelected);

		HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
		RECT rcCorner;

		// Top-left handle (6x6)
		SetRect(&rcCorner, rcSelected.left - 3, rcSelected.top - 3, rcSelected.left + 3, rcSelected.top + 3);
		FillRect(static_cast<HDC>(m_hMemDC), &rcCorner, hRedBrush);

		// Top-right handle (6x6)
		SetRect(&rcCorner, rcSelected.right - 3, rcSelected.top - 3, rcSelected.right + 3, rcSelected.top + 3);
		FillRect(static_cast<HDC>(m_hMemDC), &rcCorner, hRedBrush);

		// Bottom-left handle (6x6)
		SetRect(&rcCorner, rcSelected.left - 3, rcSelected.bottom - 3, rcSelected.left + 3, rcSelected.bottom + 3);
		FillRect(static_cast<HDC>(m_hMemDC), &rcCorner, hRedBrush);

		// Bottom-right handle (6x6)
		SetRect(&rcCorner, rcSelected.right - 3, rcSelected.bottom - 3, rcSelected.right + 3, rcSelected.bottom + 3);
		FillRect(static_cast<HDC>(m_hMemDC), &rcCorner, hRedBrush);

		DeleteObject(hRedBrush);
	}

	// 4. BitBlt double-buffer to screen
	BitBlt(hDC, 0, 0, rcClient.right, rcClient.bottom, static_cast<HDC>(m_hMemDC), 0, 0, SRCCOPY);

	EndPaint(static_cast<HWND>(m_hWnd), &ps);
#endif
}

/**
 * [RECONSTRUCTED - 0x0095F0C0]
 * CServerArchitectureView::OnLButtonDown
 * Native implementation: 150 bytes @ 0x0095F0C0
 */
int32_t CServerArchitectureView::OnLButtonDown(WPARAM /*wParam*/, LPARAM lParam) {
#ifdef _WIN32
	int32_t x = static_cast<int16_t>(LOWORD(lParam));
	int32_t y = static_cast<int16_t>(HIWORD(lParam));

	m_ptLastMouse.x = x;
	m_ptLastMouse.y = y;

	tagServerNodeUI* pHit = HitTestNode(x, y);
	if (pHit) {
		m_pSelectedNode = pHit;
		m_hCurrentCursor = LoadCursorA(nullptr, IDC_ARROW);
	} else {
		m_pSelectedNode = nullptr;
		m_bDraggingCanvas = true;
		SetCapture(static_cast<HWND>(m_hWnd));
		m_hCurrentCursor = LoadCursorA(nullptr, IDC_SIZEALL);
	}

	SetCursor(static_cast<HCURSOR>(m_hCurrentCursor));
	RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
#else
	(void)lParam;
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095F160]
 * CServerArchitectureView::OnLButtonUp
 * Native implementation: 66 bytes @ 0x0095F160
 */
int32_t CServerArchitectureView::OnLButtonUp(WPARAM /*wParam*/, LPARAM /*lParam*/) {
#ifdef _WIN32
	ReleaseCapture();
	m_pSelectedNode = nullptr;
	m_bDraggingCanvas = false;
	m_hCurrentCursor = LoadCursorA(nullptr, IDC_ARROW);
	SetCursor(static_cast<HCURSOR>(m_hCurrentCursor));
	RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095F1B0]
 * CServerArchitectureView::OnMouseMove
 * Native implementation: 294 bytes @ 0x0095F1B0
 */
int32_t CServerArchitectureView::OnMouseMove(WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
	if (!(wParam & MK_LBUTTON)) {
		return 0;
	}

	int32_t x = static_cast<int16_t>(LOWORD(lParam));
	int32_t y = static_cast<int16_t>(HIWORD(lParam));

	int32_t deltaX = x - m_ptLastMouse.x;
	int32_t deltaY = y - m_ptLastMouse.y;

	if (m_bDraggingCanvas) {
		// Canvas pan
		float fZoom = GetZoomFactor();
		m_ptOrigin.x -= static_cast<int32_t>(static_cast<float>(deltaX) * fZoom);
		m_ptOrigin.y -= static_cast<int32_t>(static_cast<float>(deltaY) * fZoom);
		RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
	} else if (m_pSelectedNode != nullptr) {
		// Move individual node with collision avoidance
		float fZoom = GetZoomFactor();
		POINT ptDelta;
		ptDelta.x = static_cast<int32_t>(static_cast<float>(deltaX) * fZoom);
		ptDelta.y = static_cast<int32_t>(static_cast<float>(deltaY) * fZoom);
		MoveNodeWithCollision(m_pSelectedNode, &ptDelta);
		RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
	}

	m_ptLastMouse.x = x;
	m_ptLastMouse.y = y;
#else
	(void)wParam; (void)lParam;
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095F2E0]
 * CServerArchitectureView::OnSetCursor
 * Native implementation: 21 bytes @ 0x0095F2E0
 */
int32_t CServerArchitectureView::OnSetCursor(WPARAM /*wParam*/, LPARAM /*lParam*/) {
#ifdef _WIN32
	if (m_hCurrentCursor) {
		SetCursor(static_cast<HCURSOR>(m_hCurrentCursor));
	}
#endif
	return 1;
}

/**
 * [RECONSTRUCTED - 0x0095F300]
 * CServerArchitectureView::OnRButtonUp
 * Native implementation: 511 bytes @ 0x0095F300
 *
 * Popups the architecture view context menu:
 *   - "Show Server Module Name" (ID 1)
 *   - "Show Certification Architecture" (ID 2)
 *   - "Show Server Cord" (ID 3)
 *   - "Show Detail" (ID 4)
 *   - "Rearrange Items" (ID 0x100)
 *   - "Property" (ID 0x101)
 */
int32_t CServerArchitectureView::OnRButtonUp(WPARAM /*wParam*/, LPARAM /*lParam*/) {
#ifdef _WIN32
	HMENU hMenu = CreatePopupMenu();
	HMENU hSubMenu = CreatePopupMenu();

	AppendMenuA(hSubMenu, MF_STRING, 1, "Show Server Module Name");
	AppendMenuA(hSubMenu, MF_STRING, 2, "Show Certification Architecture");
	AppendMenuA(hSubMenu, MF_STRING, 3, "Show Server Cord");
	AppendMenuA(hSubMenu, MF_STRING, 4, "Show Detail");

	CheckMenuItem(hSubMenu, 1, m_bShowModuleName ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hSubMenu, 2, m_bShowCertArch ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hSubMenu, 3, m_bShowServerCord ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hSubMenu, 4, m_bShowDetail ? MF_CHECKED : MF_UNCHECKED);

	AppendMenuA(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), "View");
	AppendMenuA(hMenu, MF_STRING, 0x100, "Rearrange Items");

	POINT ptCursor = {};
	GetCursorPos(&ptCursor);

	POINT ptClient = ptCursor;
	ScreenToClient(static_cast<HWND>(m_hWnd), &ptClient);

	tagServerNodeUI* pHit = HitTestNode(ptClient.x, ptClient.y);
	if (pHit) {
		AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
		AppendMenuA(hMenu, MF_STRING, 0x101, "Property");
	}

	int nSelected = TrackPopupMenu(
		hMenu,
		TPM_RETURNCMD | TPM_RIGHTBUTTON,
		ptCursor.x, ptCursor.y,
		0,
		static_cast<HWND>(m_hWnd),
		nullptr
	);

	DestroyMenu(hSubMenu);
	DestroyMenu(hMenu);

	switch (nSelected) {
		case 1:
			m_bShowModuleName = !m_bShowModuleName;
			RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
			break;
		case 2:
			m_bShowCertArch = !m_bShowCertArch;
			RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
			break;
		case 3:
			m_bShowServerCord = !m_bShowServerCord;
			RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
			break;
		case 4:
			m_bShowDetail = !m_bShowDetail;
			RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
			break;
		case 0x100:
			// Rearrange Items (Native 0x0095F438 - 0x0095F482)
			if (m_pRootNode) {
				RECT rcBounds = {};
				LayoutNodeTree(&rcBounds, m_pRootNode, 0, 0);
				LayoutOrphanNodes(&rcBounds);
			}
			m_ptOrigin.x = 0;
			m_ptOrigin.y = 0;
			RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
			break;
		case 0x101:
			// Property dialog
			break;
		default:
			break;
	}
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095F510]
 * CServerArchitectureView::OnRefreshGraph
 * Native implementation: 12 bytes @ 0x0095F510
 */
int32_t CServerArchitectureView::OnRefreshGraph(WPARAM /*wParam*/, LPARAM /*lParam*/) {
	BuildArchitectureGraph();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095F520]
 * CServerArchitectureView::OnRepaintGraph
 * Native implementation: 15 bytes @ 0x0095F520
 */
int32_t CServerArchitectureView::OnRepaintGraph(WPARAM /*wParam*/, LPARAM /*lParam*/) {
	RedrawWindow(5 /* RDW_INVALIDATE | RDW_ERASE */);
	return 0;
}

} // namespace ServerFramework
