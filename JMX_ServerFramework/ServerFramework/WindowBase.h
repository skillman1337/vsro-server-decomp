/**
 * ============================================================================
 * Joymax ServerFramework - Window Base Architecture
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\WindowBase.h
 *
 * Implements CWindowBase:
 *   - Native VTable @ 0x00B4123C (size 8 bytes = 2 slots)
 *   - Native Constructor @ 0x009527B0 (142 bytes)
 *   - Native Destructor @ 0x00952840 / 0x009528B0
 *   - Root base class for CWin32SizeRuler, CServerFrameWindow,
 *     CServerChildWindowBase, and CDisplayWindow
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_WINDOWBASE_H_
#define _JMX_SERVERFRAMEWORK_WINDOWBASE_H_

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMENU;
typedef uint32_t UINT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef uint32_t DWORD;
typedef int32_t BOOL;
#endif

namespace ServerFramework {

// Callback signatures matching native disassembly
typedef std::function<LRESULT(WPARAM wParam, LPARAM lParam)> PFN_MESSAGE_HANDLER;
typedef std::function<void()> PFN_COMMAND_HANDLER;

/**
 * [RECONSTRUCTED - 0x009527B0]
 * CWindowBase
 * Native size: 0x70 bytes (112 bytes)
 *
 * Memory Layout:
 *   +0x00: VTable pointer (ServerFramework::CWindowBase::`vftable' @ 0x00B4123C)
 *   +0x04: uint32_t m_dwStyleFlags (initialized to 0)
 *   +0x08: std::list<CWindowBase*> m_listChildren (12 bytes)
 *   +0x14: HWND m_hWnd (initialized to NULL)
 *   +0x18: HINSTANCE m_hInstance (initialized to NULL)
 *   +0x1C: bool m_bChild (sub-window / child flag, initialized to false)
 *   +0x20: stdext::hash_map<UINT, PFN_MESSAGE_HANDLER> m_mapMessageHandlers (0x28 bytes = 40 bytes)
 *   +0x48: stdext::hash_map<UINT, PFN_COMMAND_HANDLER> m_mapCommandHandlers (0x28 bytes = 40 bytes)
 */
class CWindowBase {
public:
	// Native @ 0x009527B0 (142 bytes)
	CWindowBase();

	// Native @ 0x00952840 / 0x009528B0
	virtual ~CWindowBase();

	// Slot 1 (+0x04) @ 0x00952B50 (87 bytes)
	virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Message & Command dispatchers
	// Native @ 0x00952C90 (126 bytes)
	LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Native @ 0x00952DF0 (83 bytes)
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);

	// Handler registration
	// Native @ 0x00952C20 (99 bytes)
	bool RegisterMessageHandler(UINT uMsg, PFN_MESSAGE_HANDLER pfnHandler);

	// Native @ 0x00952D10 (102 bytes)
	bool RegisterCommandHandler(UINT uCmdID, PFN_COMMAND_HANDLER pfnHandler);

	// Window creation & destruction
	// Native @ 0x009529B0 (233 bytes)
	bool Create(DWORD dwExStyle, const char* lpClassName, const char* lpWindowName,
	            DWORD dwStyle, int x1, int y1, int x2, int y2,
	            HWND hWndParent, HMENU hMenu, HINSTANCE hInstance);

	// Native @ 0x00952AF0 (89 bytes)
	BOOL Destroy();

	// Native @ 0x00952BB0 (35 bytes)
	BOOL ShowWindow(int nCmdShow);

	// Native @ 0x00952C00 (14 bytes)
	BOOL GetClientRect(RECT* lpRect);

	// Native @ 0x00952C10 (16 bytes)
	BOOL RedrawWindow(UINT flags = 1);

	HWND GetSafeHwnd() const;
	HWND GetHwnd() const;
	bool IsChild() const;
	void SetChild(bool bChild);

public:
	uint32_t                               m_dwStyleFlags;       // +0x04: Flags / style
	std::list<CWindowBase*>                m_listChildren;       // +0x08: Child window list
	HWND                                   m_hWnd;               // +0x14: Window handle
	HINSTANCE                              m_hInstance;          // +0x18: Module instance handle
	bool                                   m_bChild;             // +0x1C: Child window flag
	std::map<UINT, PFN_MESSAGE_HANDLER>    m_mapMessageHandlers; // +0x20: Window message dispatch map
	std::map<UINT, PFN_COMMAND_HANDLER>    m_mapCommandHandlers; // +0x48: WM_COMMAND dispatch map
};

// Global window registration functions
// Native @ 0x00952680
LRESULT CALLBACK ServerFramework_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Native @ 0x00953410
void ServerFramework_RegisterWindowHandle(HWND hWnd, CWindowBase* pWindow);

// Native @ 0x00953790
CWindowBase* ServerFramework_FindWindowByHandle(HWND hWnd);

// Native @ 0x009534C0
void ServerFramework_UnregisterWindowHandle(HWND hWnd);

// Native @ 0x00952940
BOOL ServerFramework_RegisterWindowClass(HINSTANCE hInstance, const char* pszClassName);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_WINDOWBASE_H_
