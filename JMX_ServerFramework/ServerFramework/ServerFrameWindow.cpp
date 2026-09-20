/**
 * ============================================================================
 * Joymax ServerFramework - Server Frame Window & View Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerFrameWindow.cpp
 *
 * Implements:
 *   - CServerChildWindowBase
 *   - CServerArchitectureView @ 0x0095D750 / 0x0095DA40
 *   - CServerMoniterView @ 0x009615B0
 *   - CServerFrameWindow @ 0x00938080 / 0x00938790
 * ============================================================================
 */

#include "ServerFrameWindow.h"
#include "ServerMoniterView.h"
#include "ServerConfig.h"
#include "ServerMain.h"
#include <cstdio>
#include <cstdarg>

namespace ServerFramework {

// Native global frame window instance @ 0x00C82794
CServerFrameWindow* g_pServerFrameWindow = nullptr;

// Native global main window handle @ 0x00C82790
HWND g_hServerMainWnd = nullptr;

// Native global frame window handle
HWND g_hWndServerFrame = nullptr;

// ============================================================================
// GDI Helper Routines (Native @ 0x00951F80 - 0x00952520)
// ============================================================================

// Silkroad Debug 10-Color Text Palette @ 0x00C63B88
struct tagTextColorEntry {
	COLORREF crText;
	COLORREF crBk;
	int32_t  nBkMode;
};

static const tagTextColorEntry s_colorTable[10] = {
	{ RGB(0,   0,   0),   RGB(255, 255, 255), 1 }, // ^0: Black
	{ RGB(180, 0,   180), RGB(255, 255, 255), 1 }, // ^1: Magenta
	{ RGB(255, 0,   0),   RGB(255, 255, 255), 1 }, // ^2: Red
	{ RGB(128, 128, 128), RGB(255, 255, 255), 1 }, // ^3: Gray
	{ RGB(0,   0,   255), RGB(255, 255, 255), 1 }, // ^4: Blue
	{ RGB(255, 255, 255), RGB(255, 255, 255), 1 }, // ^5: White
	{ RGB(255, 0,   255), RGB(255, 255, 255), 1 }, // ^6: Cyan/Pink
	{ RGB(255, 255, 0),   RGB(255, 255, 255), 1 }, // ^7: Yellow
	{ RGB(0,   128, 0),   RGB(255, 255, 255), 1 }, // ^8: Dark Green
	{ RGB(134, 128, 5),   RGB(255, 255, 255), 1 }, // ^9: Brown
};

/**
 * [RECONSTRUCTED - 0x00951F80]
 * GDI_SelectPen
 */
HPEN GDI_SelectPen(HDC hdc, int nWidth, COLORREF crColor, int nStyle) {
#ifdef _WIN32
	HPEN hPen = CreatePen(nStyle, nWidth, crColor);
	return (HPEN)SelectObject(hdc, hPen);
#else
	(void)hdc; (void)nWidth; (void)crColor; (void)nStyle;
	return nullptr;
#endif
}

/**
 * [RECONSTRUCTED - 0x00951FD0]
 * GDI_SelectBrush
 */
HBRUSH GDI_SelectBrush(HDC hdc, COLORREF crColor) {
#ifdef _WIN32
	HBRUSH hBrush = CreateSolidBrush(crColor);
	return (HBRUSH)SelectObject(hdc, hBrush);
#else
	(void)hdc; (void)crColor;
	return nullptr;
#endif
}

/**
 * [RECONSTRUCTED - 0x00951FB0]
 * GDI_RestoreObject
 */
void GDI_RestoreObject(HDC hdc, HGDIOBJ hNewObj, HGDIOBJ hOldObj) {
#ifdef _WIN32
	if (hOldObj) {
		SelectObject(hdc, hOldObj);
	}
	if (hNewObj) {
		DeleteObject(hNewObj);
	}
#else
	(void)hdc; (void)hNewObj; (void)hOldObj;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952310]
 * GDI_DrawRectangle
 * Native implementation @ 0x00952310 (174 bytes)
 *
 * Selects solid brush and cosmetic pen, calls Rectangle, and restores GDI state.
 */
BOOL GDI_DrawRectangle(HDC hdc, const RECT* prc, COLORREF crFill, COLORREF crBorder) {
#ifdef _WIN32
	if (!hdc || !prc) return FALSE;
	HBRUSH hBrush = ::CreateSolidBrush(crFill);
	HGDIOBJ hOldBrush = ::SelectObject(hdc, hBrush);
	HPEN hPen = ::CreatePen(PS_SOLID, 1, crBorder);
	HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);

	BOOL bRes = ::Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);

	::SelectObject(hdc, hOldPen);
	::DeleteObject(hPen);
	::SelectObject(hdc, hOldBrush);
	::DeleteObject(hBrush);
	return bRes;
#else
	(void)hdc; (void)prc; (void)crFill; (void)crBorder;
	return TRUE;
#endif
}

/**
 * [RECONSTRUCTED - 0x009523F0]
 * GDI_DrawLine
 * Native implementation @ 0x009523F0 (132 bytes)
 *
 * Selects cosmetic pen, issues MoveToEx + LineTo, and restores GDI state.
 */
BOOL GDI_DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color, int fnPenStyle) {
#ifdef _WIN32
	if (!hdc) return FALSE;
	HPEN hPen = ::CreatePen(fnPenStyle, 1, color);
	HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);

	::MoveToEx(hdc, x1, y1, nullptr);
	BOOL bRes = ::LineTo(hdc, x2, y2);

	::SelectObject(hdc, hOldPen);
	::DeleteObject(hPen);
	return bRes;
#else
	(void)hdc; (void)x1; (void)y1; (void)x2; (void)y2; (void)color; (void)fnPenStyle;
	return TRUE;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952480]
 * GDI_DrawFilledEllipse
 */
BOOL GDI_DrawFilledEllipse(HDC hdc, int nLeft, int nTop, int nWidth, int nHeight, COLORREF crFill, COLORREF crBorder) {
#ifdef _WIN32
	HBRUSH hBrush = CreateSolidBrush(crFill);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	HPEN hPen = CreatePen(PS_SOLID, 1, crBorder);
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

	BOOL bResult = Ellipse(hdc, nLeft, nTop, nLeft + nWidth, nTop + nHeight);

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);

	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);

	return bResult;
#else
	(void)hdc; (void)nLeft; (void)nTop; (void)nWidth; (void)nHeight; (void)crFill; (void)crBorder;
	return 1;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952110]
 * GDI_DrawColorCodedText
 */
uint32_t GDI_DrawColorCodedText(HDC hdc, int x, int y, uint32_t dwFlags, const char* pszFormat, ...) {
#ifdef _WIN32
	char szBuffer[4096];
	va_list args;
	va_start(args, pszFormat);
	int nLen = vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);

	if (nLen <= 0) {
		return 0;
	}

	int curX = x;
	const char* p = szBuffer;
	char szSegment[4096];
	int segLen = 0;

	while (*p) {
		if (*p == '^' && *(p + 1) >= '0' && *(p + 1) <= '9') {
			if (segLen > 0) {
				szSegment[segLen] = '\0';
				TextOutA(hdc, curX, y, szSegment, segLen);
				SIZE sz = { 0, 0 };
				GetTextExtentPoint32A(hdc, szSegment, segLen, &sz);
				curX += sz.cx;
				segLen = 0;
			}
			int nColorIdx = (*(p + 1) - '0') % 10;
			SetTextColor(hdc, s_colorTable[nColorIdx].crText);
			p += 2;
		} else {
			szSegment[segLen++] = *p++;
		}
	}

	if (segLen > 0) {
		szSegment[segLen] = '\0';
		TextOutA(hdc, curX, y, szSegment, segLen);
		SIZE sz = { 0, 0 };
		GetTextExtentPoint32A(hdc, szSegment, segLen, &sz);
		curX += sz.cx;
	}

	(void)dwFlags;
	return static_cast<uint32_t>(curX - x);
#else
	(void)hdc; (void)x; (void)y; (void)dwFlags; (void)pszFormat;
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952680]
 * ServerFramework_WindowProc
 * Native implementation @ 0x00952680 (208 bytes)
 *
 * Filters messages via ServerFramework_PreTranslateMessage (0x00952750),
 * intercepts frame menu & system commands, and dispatches to CServerFrameWindow.
 */
LRESULT CALLBACK ServerFramework_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
	// Native 0x00952680: PreTranslateMessage filter
	int32_t nPre = ServerFramework_PreTranslateMessage(uMsg, wParam, lParam);
	if (nPre != -1) {
		return nPre;
	}

	if (g_pServerFrameWindow && g_pServerFrameWindow->GetSafeHwnd() == hWnd) {
		switch (uMsg) {
		case WM_COMMAND: {
			WORD wID = LOWORD(wParam);
			if (wID == 0xBB9) {
				g_pServerFrameWindow->OnSelectArchitectureView();
				return 0;
			} else if (wID == 0xBBA) {
				g_pServerFrameWindow->OnSelectChildView();
				return 0;
			} else if (wID == 100) {
				g_pServerFrameWindow->OnMenuExit();
				return 0;
			} else if (wID == 101) {
				g_pServerFrameWindow->OnHelpAbout();
				return 0;
			}
			g_pServerFrameWindow->OnMenuCommand(static_cast<UINT>(wID));
			return 0;
		}
		case WM_SYSCOMMAND:
			g_pServerFrameWindow->OnSysCommand(static_cast<UINT>(wParam), lParam);
			break;
		case WM_ERASEBKGND:
			return g_pServerFrameWindow->OnEraseBkgnd(reinterpret_cast<HDC>(wParam));
		case WM_NOTIFY:
			return g_pServerFrameWindow->OnNotify(static_cast<int32_t>(wParam), reinterpret_cast<NMHDR*>(lParam));
		case WM_CLOSE:
			::DestroyWindow(hWnd);
			return 0;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		default:
			break;
		}
	}

	return ::DefWindowProcA(hWnd, uMsg, wParam, lParam);
#else
	(void)hWnd; (void)uMsg; (void)wParam; (void)lParam;
	return 0;
#endif
}

// ============================================================================
// CServerFrameWindow
// ============================================================================

CServerFrameWindow::CServerFrameWindow()
	: CWin32SizeRuler()
	, m_hLogListView(nullptr)
	, m_hFont(nullptr)
	, m_dwReserved(0)
	, m_listViews()
	, m_listChildWindows()
	, m_hWndTopPane(nullptr)
	, m_hWndMidPane(nullptr)
	, m_hWndBottomPane(nullptr)
	, m_pMonitorView(nullptr)
	, m_pArchitectureView(nullptr)
	, m_dwActiveViewID(0xBB9) {
}

CServerFrameWindow::~CServerFrameWindow() {
	DestroyWindow();
}

/**
 * [RECONSTRUCTED - 0x00938080]
 * CServerFrameWindow::Create
 * Native implementation @ 0x00938080 (1284 bytes)
 */
bool CServerFrameWindow::Create() {
#ifdef _WIN32
	m_hInstance = GetModuleHandleA(nullptr);

	// Native 0x009380C2 - 0x009380CD: Register window class with g_szAppName
	if (!ServerFramework_RegisterWindowClass(m_hInstance, g_szAppName)) {
		return false;
	}

	// Native 0x009380DD - 0x009380F3: Calculate window dimensions
	// Machine proof:
	//   edi = GetSystemMetrics(SM_CXFULLSCREEN) - 0x258 (600)
	//   rect: left = edi, top = 0, right = GetSystemMetrics(SM_CXFULLSCREEN), bottom = 500 (0x1f4)
	//   width = right - left = 600
	int nScreenWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
	int nX = (nScreenWidth > 600) ? (nScreenWidth - 600) : 0;
	int nY = 0;
	int nWidth = 600;   // Proven 600px docked at the right edge
	int nHeight = 500;  // Proven 500px height

	// Native 0x009380F5: Load menu from g_hServerFrameworkRes (ID 0x65 = 101)
	HMENU hMenu = nullptr;
	if (g_hServerFrameworkRes != nullptr) {
		hMenu = ::LoadMenuA(static_cast<HMODULE>(g_hServerFrameworkRes), MAKEINTRESOURCEA(0x65));
	}

	// Native 0x0093810B - 0x00938140: Create frame window via CWin32SizeRuler::Create (Slot 2 @ +0x08)
	if (!CWin32SizeRuler::Create(0, g_szAppName, g_szAppName,
	                             WS_OVERLAPPEDWINDOW,
	                             nX, nY, nX + nWidth, nY + nHeight,
	                             nullptr, hMenu, m_hInstance)) {
		std::printf("[CServerFrameWindow] CWin32SizeRuler::Create failed: error=%lu, appName=\"%s\"\n",
			GetLastError(), g_szAppName);
		return false;
	}

	g_hServerMainWnd = m_hWnd;

	// Native 0x00938152 - 0x00938185: Register command handlers
	RegisterCommandHandler(0x9C43, []() {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnHelpAbout();
	});
	RegisterCommandHandler(0x9C41, []() {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnMenuExit();
	});
	RegisterCommandHandler(0x9C44, []() {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnTogglePerfMonitorView();
	});
	RegisterCommandHandler(0x9C48, []() {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnMenuCommand(0x9C48);
	});
	RegisterCommandHandler(0xBB9, []() {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnSelectArchitectureView();
	});

	// Native 0x0093818A - 0x009381C3: Register frame window message handlers
	// (Note: WM_SIZE, WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP, WM_PAINT, WM_SETCURSOR are registered by CWin32SizeRuler::Create)
	RegisterMessageHandler(WM_NOTIFY, [](WPARAM w, LPARAM l) -> LRESULT {
		if (g_pServerFrameWindow) return g_pServerFrameWindow->OnNotify(static_cast<int32_t>(w), reinterpret_cast<NMHDR*>(l));
		return 0;
	});
	RegisterMessageHandler(WM_ERASEBKGND, [](WPARAM w, LPARAM) -> LRESULT {
		if (g_pServerFrameWindow) return g_pServerFrameWindow->OnEraseBkgnd(reinterpret_cast<HDC>(w));
		return TRUE;
	});
	RegisterMessageHandler(0x9000, [](WPARAM w, LPARAM l) -> LRESULT {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnCustomAppMessage(w, l);
		return 0;
	});
	RegisterMessageHandler(WM_SYSCOMMAND, [](WPARAM w, LPARAM l) -> LRESULT {
		if (g_pServerFrameWindow) g_pServerFrameWindow->OnSysCommand(static_cast<UINT>(w), l);
		return 0;
	});

	// Native 0x009381D5: Set frame 1000ms timer
	::SetTimer(m_hWnd, 0, 1000, nullptr);

	// Native 0x00938210: Create SysListView32 child control for logging (ID 0x100)
	m_hLogListView = ::CreateWindowExA(WS_EX_CLIENTEDGE, "SysListView32", nullptr,
	                                  WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                  0, 0, 100, 100, m_hWnd,
	                                  reinterpret_cast<HMENU>(static_cast<uintptr_t>(0x100)),
	                                  m_hInstance, nullptr);

	// Native 0x00937E00: Insert 3 columns into log listview
	if (m_hLogListView != nullptr) {
		LV_COLUMNA col = {};
		col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		col.cx = 60;  // Native 0x00938228: 0x3C
		col.pszText = const_cast<char*>("Time");
		::SendMessageA(m_hLogListView, LVM_INSERTCOLUMNA, 0, reinterpret_cast<LPARAM>(&col));

		col.cx = 320; // Native 0x00938237: 0x140
		col.pszText = const_cast<char*>("Message");
		::SendMessageA(m_hLogListView, LVM_INSERTCOLUMNA, 1, reinterpret_cast<LPARAM>(&col));

		col.cx = 150; // Native 0x0093824A: 0x96
		col.pszText = const_cast<char*>("Channel");
		::SendMessageA(m_hLogListView, LVM_INSERTCOLUMNA, 2, reinterpret_cast<LPARAM>(&col));
	}

	// Native 0x00938360: Create CServerArchitectureView (+0xC0)
	m_pArchitectureView = new CServerArchitectureView();
	if (m_pArchitectureView != nullptr) {
		m_pArchitectureView->Create(m_hInstance, m_hWnd, 0, 0, 100, 100, 0x101);
		m_listViews.push_back(m_pArchitectureView);
		m_listChildWindows.push_back(m_pArchitectureView);
	}

	// Native 0x00938380: Create CServerMoniterView (+0xBC)
	m_pMonitorView = new CServerMoniterView();
	if (m_pMonitorView != nullptr) {
		m_pMonitorView->Create(m_hInstance, m_hWnd, 0, 0, 100, 100, 0x102);
		m_listViews.push_back(m_pMonitorView);
	}

	m_hWndTopPane = (m_pMonitorView != nullptr) ? m_pMonitorView->GetSafeHwnd() : nullptr;
	m_hWndMidPane = (m_pArchitectureView != nullptr) ? m_pArchitectureView->GetSafeHwnd() : nullptr;
	m_hWndBottomPane = m_hLogListView;

	// Native 0x009384C3 - 0x009384F2: Set vertical splitter pane ratios
	// Ratio 1: Top 20% for MonitorView, Middle 50% for ArchitectureView
	SetPaneRatio(m_hWndTopPane, m_hWndMidPane, 0.20f);
	// Ratio 2: Middle 50% for ArchitectureView, Bottom 30% for LogListView
	SetPaneRatio(m_hWndMidPane, m_hWndBottomPane, 0.70f);

	// Native 0x00938450: Add "MainView" popup menu with "ArchitectureView" (ID 0xBB9 = 3001)
	if (hMenu != nullptr) {
		HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
		if (hSubMenu != nullptr) {
			HMENU hPopup = ::CreatePopupMenu();
			if (hPopup != nullptr) {
				::AppendMenuA(hPopup, MF_STRING, 0xBB9, "ArchitectureView");
				::CheckMenuItem(hPopup, 0xBB9, MF_CHECKED);
				::AppendMenuA(hSubMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hPopup), "MainView");
			}
		}
	}

	// Initial layout recalculation
	RecalcLayout();

	// Show and update frame window
	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);
#else
	m_hWnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(1));
	g_hServerMainWnd = m_hWnd;
#endif
	return (m_hWnd != nullptr);
}

/**
 * [RECONSTRUCTED - 0x00938590]
 * CServerFrameWindow::RecalcLayout
 * Native implementation @ 0x00938590 (169 bytes)
 */
bool CServerFrameWindow::RecalcLayout() {
#ifdef _WIN32
	if (m_listPanes.size() == 2) {
		auto it = m_listPanes.begin();
		it->hWndUpper = m_hWndTopPane;
		it->hWndLower = m_hWndMidPane;
		++it;
		it->hWndUpper = m_hWndMidPane;
		it->hWndLower = m_hWndBottomPane;
	}

	RECT rcClient = {};
	if (m_hWnd != nullptr && ::GetClientRect(m_hWnd, &rcClient)) {
		int cx = rcClient.right - rcClient.left;
		int cy = rcClient.bottom - rcClient.top;
		return CWin32SizeRuler::RecalcLayout(cx, cy);
	}
#endif
	return false;
}

/**
 * [RECONSTRUCTED - 0x00938700]
 * CServerFrameWindow::SwitchActiveView
 * Native implementation @ 0x00938700 (143 bytes)
 */
bool CServerFrameWindow::SwitchActiveView(CWindowBase* pNewView) {
	if (pNewView == nullptr) return false;

#ifdef _WIN32
	if (m_hWndMidPane != nullptr && ::IsWindow(m_hWndMidPane)) {
		::ShowWindow(m_hWndMidPane, SW_HIDE);
	}

	m_hWndMidPane = pNewView->GetSafeHwnd();
	::ShowWindow(m_hWndMidPane, SW_SHOW);

	HMENU hMainMenu = ::GetMenu(m_hWnd);
	if (hMainMenu != nullptr) {
		HMENU hSubMenu = ::GetSubMenu(::GetSubMenu(::GetSubMenu(hMainMenu, 0), 2), 2);
		if (hSubMenu != nullptr) {
			if (pNewView != m_pArchitectureView) {
				::CheckMenuItem(hSubMenu, 0xBB9, MF_UNCHECKED);
				::CheckMenuItem(hSubMenu, 0xBBA, MF_CHECKED);
			} else {
				::CheckMenuItem(hSubMenu, 0xBB9, MF_CHECKED);
				::CheckMenuItem(hSubMenu, 0xBBA, MF_UNCHECKED);
			}
		}
	}

	return RecalcLayout();
#else
	return true;
#endif
}

/**
 * [RECONSTRUCTED - 0x00938640]
 * CServerFrameWindow::AddChildWindow
 * Native implementation @ 0x00938640 (182 bytes)
 */
bool CServerFrameWindow::AddChildWindow(CServerChildWindowBase* pChild) {
	if (!pChild) return false;

	m_listViews.push_back(pChild);
	m_listChildWindows.push_back(pChild);

#ifdef _WIN32
	RegisterCommandHandler(0xBBA, []() {
		if (g_pServerFrameWindow) {
			g_pServerFrameWindow->OnSelectChildView();
		}
	});

	HMENU hMainMenu = ::GetMenu(m_hWnd);
	if (hMainMenu != nullptr) {
		HMENU hSubMenu = ::GetSubMenu(::GetSubMenu(::GetSubMenu(hMainMenu, 0), 2), 2);
		if (hSubMenu != nullptr) {
			::AppendMenuA(hSubMenu, MF_BYCOMMAND, 0xBBA, pChild->GetWindowName());
		}
	}

	if (pChild->GetSafeHwnd() != nullptr) {
		::ShowWindow(pChild->GetSafeHwnd(), SW_HIDE);
	}
#endif
	return true;
}

bool CServerFrameWindow::DestroyWindow() {
	if (m_pArchitectureView != nullptr) {
		delete m_pArchitectureView;
		m_pArchitectureView = nullptr;
	}
	if (m_pMonitorView != nullptr) {
		delete m_pMonitorView;
		m_pMonitorView = nullptr;
	}

#ifdef _WIN32
	if (m_hFont != nullptr) {
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}
	if (m_hLogListView != nullptr && IsWindow(m_hLogListView)) {
		::DestroyWindow(m_hLogListView);
		m_hLogListView = nullptr;
	}
	if (m_hWnd != nullptr && IsWindow(m_hWnd)) {
		::DestroyWindow(m_hWnd);
	}
#endif
	m_hWnd = nullptr;
	g_hServerMainWnd = nullptr;
	return true;
}

// [RECONSTRUCTED - 0x00938860]
void CServerFrameWindow::OnHelpAbout() {
#ifdef _WIN32
	::MessageBoxA(nullptr, "Joymax Silkroad Server Framework v1.150", "About Server", MB_OK | MB_ICONINFORMATION);
#endif
}

// [RECONSTRUCTED - 0x00938880]
void CServerFrameWindow::OnMenuExit() {
#ifdef _WIN32
	if (m_hWnd && ::IsWindow(m_hWnd)) {
		::PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
	}
#endif
}

// [RECONSTRUCTED - 0x009388A0]
void CServerFrameWindow::OnTogglePerfMonitorView() {
#ifdef _WIN32
	if (m_pMonitorView && m_pMonitorView->GetSafeHwnd()) {
		HWND hWndView = m_pMonitorView->GetSafeHwnd();
		BOOL bVisible = ::IsWindowVisible(hWndView);
		::ShowWindow(hWndView, bVisible ? SW_HIDE : SW_SHOW);
	}
#endif
}

// [RECONSTRUCTED - 0x00938950]
LRESULT CServerFrameWindow::OnNotify(int32_t idCtrl, NMHDR* pNmhdr) {
	(void)idCtrl;
	(void)pNmhdr;
	return 0;
}

// [RECONSTRUCTED - 0x009856E0]
BOOL CServerFrameWindow::OnEraseBkgnd(HDC hdc) {
	(void)hdc;
	return TRUE;
}

// [RECONSTRUCTED - 0x009391F0]
void CServerFrameWindow::OnSysCommand(UINT nID, LPARAM lParam) {
#ifdef _WIN32
	if ((nID & 0xFFF0) == SC_CLOSE) {
		::PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
		return;
	}
	::DefWindowProcA(m_hWnd, WM_SYSCOMMAND, nID, lParam);
#else
	(void)nID; (void)lParam;
#endif
}

// [RECONSTRUCTED - 0x00938930]
void CServerFrameWindow::OnMenuCommand(UINT nID) {
	(void)nID;
}

// [RECONSTRUCTED - 0x00939170]
void CServerFrameWindow::OnSelectArchitectureView() {
	if (!m_listChildWindows.empty()) {
		SwitchActiveView(m_listChildWindows.front());
	}
}

// [RECONSTRUCTED - 0x009391A0]
void CServerFrameWindow::OnSelectChildView() {
	if (m_listChildWindows.size() >= 2) {
		auto it = m_listChildWindows.begin();
		++it;
		SwitchActiveView(*it);
	}
}

// [RECONSTRUCTED - 0x009391D0]
void CServerFrameWindow::OnCustomAppMessage(WPARAM wParam, LPARAM lParam) {
	(void)wParam;
	(void)lParam;
}

// [RECONSTRUCTED - 0x00937E70]
void CServerFrameWindow::AppendLogItem(uint32_t dwChannel, const char* szMessage) {
#ifdef _WIN32
	if (!m_hLogListView || !::IsWindow(m_hLogListView) || !szMessage) {
		return;
	}

	SYSTEMTIME st;
	::GetLocalTime(&st);
	char szTime[32];
	std::snprintf(szTime, sizeof(szTime), "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

	char szChannel[16];
	std::snprintf(szChannel, sizeof(szChannel), "0x%X", dwChannel);

	LV_ITEMA item = {};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.iSubItem = 0;
	item.pszText = szTime;

	int nIndex = static_cast<int>(::SendMessageA(m_hLogListView, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&item)));
	if (nIndex >= 0) {
		item.iItem = nIndex;
		item.iSubItem = 1;
		item.pszText = const_cast<char*>(szMessage);
		::SendMessageA(m_hLogListView, LVM_SETITEMTEXTA, nIndex, reinterpret_cast<LPARAM>(&item));

		item.iSubItem = 2;
		item.pszText = szChannel;
		::SendMessageA(m_hLogListView, LVM_SETITEMTEXTA, nIndex, reinterpret_cast<LPARAM>(&item));
	}
#else
	(void)dwChannel; (void)szMessage;
#endif
}

HWND CServerFrameWindow::GetLogListView() const {
	return m_hLogListView;
}

CServerArchitectureView* CServerFrameWindow::GetArchitectureView() const {
	return m_pArchitectureView;
}

CServerMoniterView* CServerFrameWindow::GetMonitorView() const {
	return m_pMonitorView;
}

const std::list<CWindowBase*>& CServerFrameWindow::GetChildWindows() const {
	return m_listChildWindows;
}

} // namespace ServerFramework
