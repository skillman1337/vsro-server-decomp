/**
 * ============================================================================
 * Joymax ServerFramework - Server Frame Window & View Architecture
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerFrameWindow.h
 *
 * Reconstructed from:
 *   - .?AVCServerFrameWindow@ServerFramework@@ (VTable @ 0x00B40764)
 *   - .?AVCServerArchitectureView@ServerFramework@@ (VTable @ 0x00B41724)
 *   - .?AVCServerMoniterView@ServerFramework@@
 *   - .?AVCServerChildWindowBase@ServerFramework@@
 *   - Global g_pServerFrameWindow @ 0x00C82794
 *   - CServerFrameWindow_Create @ 0x00938080
 *   - CServerFrameWindow_Destroy @ 0x00938790
 *   - CServerArchitectureView_Create @ 0x0095DA40
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERFRAMEWINDOW_H_
#define _JMX_SERVERFRAMEWORK_SERVERFRAMEWINDOW_H_

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "ServerConfig.h"
#include "Win32SizeRuler.h"
#include "ServerChildWindowBase.h"
#include "ServerArchitectureView.h"
#include "ServerMoniterView.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#else
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMENU;
typedef void* HFONT;
typedef void* HICON;
typedef void* HDC;
typedef void* HGDIOBJ;
typedef void* HPEN;
typedef void* HBRUSH;
typedef uint32_t COLORREF;
#define RGB(r,g,b) ((COLORREF)(((uint8_t)(r)|((uint16_t)((uint8_t)(g))<<8))|(((uint32_t)((uint8_t)(b)))<<16)))
#define PS_SOLID 0
#define TRANSPARENT 1
#endif

namespace ServerFramework {

// Native global main window handle @ 0x00C82790
extern HWND g_hServerMainWnd;

// Native global frame window handle @ 0x00C827FC
extern HWND g_hWndServerFrame;

// [RECONSTRUCTED - 0x00951F80]
// GDI_SelectPen: creates cosmetic pen and selects into DC
HPEN GDI_SelectPen(HDC hdc, int nWidth, COLORREF crColor, int nStyle = 0);

// [RECONSTRUCTED - 0x00951FD0]
// GDI_SelectBrush: creates solid brush and selects into DC
HBRUSH GDI_SelectBrush(HDC hdc, COLORREF crColor);

// [RECONSTRUCTED - 0x00951FB0]
// GDI_RestoreObject: restores previous GDI object and deletes new object
void GDI_RestoreObject(HDC hdc, HGDIOBJ hNewObj, HGDIOBJ hOldObj);

// [RECONSTRUCTED - 0x00952310]
// GDI_DrawRectangle: draws a filled rectangle with colored border
BOOL GDI_DrawRectangle(HDC hdc, const RECT* prc, COLORREF crFill, COLORREF crBorder);

// [RECONSTRUCTED - 0x009523F0]
// GDI_DrawLine: draws a line between (x1, y1) and (x2, y2)
BOOL GDI_DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color, int fnPenStyle = 0);

// [RECONSTRUCTED - 0x00952480]
// GDI_DrawFilledEllipse: draws filled colored ellipse with specified border
BOOL GDI_DrawFilledEllipse(HDC hdc, int nLeft, int nTop, int nWidth, int nHeight, COLORREF crFill, COLORREF crBorder);

// [RECONSTRUCTED - 0x00952110]
// GDI_DrawColorCodedText: renders formatted string with ^0-^9 Silkroad color codes
uint32_t GDI_DrawColorCodedText(HDC hdc, int x, int y, uint32_t dwFlags, const char* pszFormat, ...);

// [RECONSTRUCTED - 0x00952680]
// Global Window Procedure for main frame window & registered dialogs
LRESULT CALLBACK ServerFramework_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Forward declarations: CServerArchitectureView and CServerMoniterView are defined in their respective headers
class CServerArchitectureView;
class CServerMoniterView;

/**
 * [RECONSTRUCTED - 0x00938080 / 0x00938790]
 * CServerFrameWindow
 * Native VTable @ 0x00B40764 (size 0xC8 = 200 bytes)
 * Inherits CWin32SizeRuler -> CWindowBase
 */
class CServerFrameWindow : public CWin32SizeRuler {
public:
	using CWin32SizeRuler::Create;

	CServerFrameWindow();
	virtual ~CServerFrameWindow() override;

	// Slot 9 (+0x24) @ 0x00938080
	virtual bool Create();

	// Slot 10 (+0x28) @ 0x00938790
	virtual bool DestroyWindow();

	// Slot 11 (+0x2C) @ 0x00938860
	virtual void OnHelpAbout();

	// Slot 12 (+0x30) @ 0x00938880
	virtual void OnMenuExit();

	// Slot 13 (+0x34) @ 0x009388A0
	virtual void OnTogglePerfMonitorView();

	// Slot 14 (+0x38) @ 0x00938950
	virtual LRESULT OnNotify(int32_t idCtrl, NMHDR* pNmhdr);

	// Slot 15 (+0x3C) @ 0x009856E0
	virtual BOOL OnEraseBkgnd(HDC hdc);

	// Slot 16 (+0x40) @ 0x009391F0
	virtual void OnSysCommand(UINT nID, LPARAM lParam);

	// Slot 17 (+0x44) @ 0x00938930
	virtual void OnMenuCommand(UINT nID);

	// Slot 18 (+0x48) @ 0x00939170
	virtual void OnSelectArchitectureView();

	// Slot 19 (+0x4C) @ 0x009391A0
	virtual void OnSelectChildView();

	// Slot 20 (+0x50) @ 0x009391D0
	virtual void OnCustomAppMessage(WPARAM wParam, LPARAM lParam);

	// Native @ 0x00938590: RecalcLayout
	bool RecalcLayout();

	// Native @ 0x00938700: SwitchActiveView
	bool SwitchActiveView(CWindowBase* pNewView);

	// Native @ 0x00938640: AddChildWindow(CServerChildWindowBase* pChild)
	bool AddChildWindow(CServerChildWindowBase* pChild);

	// Native @ 0x00937E70: AppendLogItem
	void AppendLogItem(uint32_t dwChannel, const char* szMessage);

	HWND GetLogListView() const;
	CServerArchitectureView* GetArchitectureView() const;
	CServerMoniterView* GetMonitorView() const;
	const std::list<CWindowBase*>& GetChildWindows() const;

private:
	HWND                     m_hLogListView      = nullptr; // +0x8C
	HFONT                    m_hFont             = nullptr; // +0x90
	uint32_t                 m_dwReserved        = 0;       // +0x94
	std::list<CWindowBase*>  m_listViews;                   // +0x98
	std::list<CWindowBase*>  m_listChildWindows;             // +0xA4
	HWND                     m_hWndTopPane       = nullptr; // +0xB0 (Monitor view)
	HWND                     m_hWndMidPane       = nullptr; // +0xB4 (Active view: Architecture or child)
	HWND                     m_hWndBottomPane    = nullptr; // +0xB8 (Log list view)
	CServerMoniterView*      m_pMonitorView      = nullptr; // +0xBC
	CServerArchitectureView* m_pArchitectureView = nullptr; // +0xC0
	uint32_t                 m_dwActiveViewID    = 0;       // +0xC4
};

// Native global pointer @ 0x00C82794
extern CServerFrameWindow* g_pServerFrameWindow;

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERFRAMEWINDOW_H_
