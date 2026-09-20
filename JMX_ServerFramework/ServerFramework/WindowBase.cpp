/**
 * ============================================================================
 * Joymax ServerFramework - Window Base Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\WindowBase.cpp
 *
 * Implements:
 *   - CWindowBase::CWindowBase() @ 0x009527B0 (142 bytes)
 *   - CWindowBase::~CWindowBase() @ 0x009528B0 (110 bytes)
 *   - CWindowBase::WindowProc() @ 0x00952B50 (87 bytes)
 *   - CWindowBase::OnMessage() @ 0x00952C90 (126 bytes)
 *   - CWindowBase::OnCommand() @ 0x00952DF0 (83 bytes)
 *   - CWindowBase::RegisterMessageHandler() @ 0x00952C20 (99 bytes)
 *   - CWindowBase::RegisterCommandHandler() @ 0x00952D10 (102 bytes)
 *   - CWindowBase::Create() @ 0x009529B0 (233 bytes)
 *   - CWindowBase::Destroy() @ 0x00952AF0 (89 bytes)
 *   - Global window mapping registry @ 0x00D67A38
 * ============================================================================
 */

#include "WindowBase.h"
#include "ServerMain.h"
#include <map>

namespace ServerFramework {

// Forward declaration of global frame window proc
LRESULT CALLBACK ServerFramework_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Native global window map @ 0x00D67A38: HWND -> CWindowBase*
static std::map<HWND, CWindowBase*> s_mapWindows;

/**
 * [RECONSTRUCTED - 0x009527B0]
 * CWindowBase constructor
 * Native implementation @ 0x009527B0 (142 bytes)
 *
 * Machine Disassembly:
 *   009527b0  6aff                 push    -1
 *   009527b2  68450aa800           push    SEH_Handler
 *   009527b7  64a100000000         mov     eax, fs:[0]
 *   009527bd  50                   push    eax
 *   009527be  51                   push    ecx
 *   009527bf  53                   push    ebx
 *   009527c0  56                   push    esi
 *   009527c1  57                   push    edi
 *   009527c2  a10030ad00           mov     eax, [__security_cookie]
 *   009527c7  33c4                 xor     eax, esp
 *   009527c9  50                   push    eax
 *   009527ca  8d442414             lea     eax, [esp+0x14]
 *   009527ce  64a300000000         mov     fs:[0], eax
 *   009527d4  8b7c2424             mov     edi, [esp+0x24]       ; edi = this
 *   009527d8  c7073c12b400         mov     dword [edi], 0xb4123c ; vptr = CWindowBase::`vftable'
 *   009527de  8d7708               lea     esi, [edi+8]          ; &m_listChildren
 *   009527e1  e84a150000           call    sub_953d30            ; allocate head node
 *   009527e6  33db                 xor     ebx, ebx
 *   009527e8  894604               mov     [esi+4], eax          ; m_listChildren._Myhead = eax
 *   009527eb  895e08               mov     [esi+8], ebx          ; m_listChildren._Mysize = 0
 *   009527ee  895c241c             mov     [esp+0x1c], ebx       ; SEH state = 0
 *   009527f2  8d4720               lea     eax, [edi+0x20]       ; &m_mapMessageHandlers
 *   009527f5  50                   push    eax
 *   009527f6  e855060000           call    sub_952e50            ; init hash_map
 *   009527fb  c644241c01           mov     byte [esp+0x1c], 1    ; SEH state = 1
 *   00952800  8d4f48               lea     ecx, [edi+0x48]       ; &m_mapCommandHandlers
 *   00952803  51                   push    ecx
 *   00952804  e847060000           call    sub_952e50            ; init hash_map
 *   00952809  c644241c02           mov     byte [esp+0x1c], 2    ; SEH state = 2
 *   0095280e  895f14               mov     [edi+0x14], ebx       ; m_hWnd = NULL
 *   00952811  895f04               mov     [edi+4], ebx          ; m_dwStyleFlags = 0
 *   00952814  895f18               mov     [edi+0x18], ebx       ; m_hInstance = NULL
 *   00952817  885f1c               mov     [edi+0x1c], bl        ; m_bChild = false
 *   0095281a  e8312cf4ff           call    std_list_clear_nodes
 *   0095281f  c744241cffffffff     mov     dword [esp+0x1c], -1
 *   00952827  8bc7                 mov     eax, edi              ; return this
 *   00952829  8b4c2414             mov     ecx, [esp+0x14]
 *   0095282d  64890d00000000       mov     fs:[0], ecx
 *   00952834  59                   pop     ecx
 *   00952835  5f                   pop     edi
 *   00952836  5e                   pop     esi
 *   00952837  5b                   pop     ebx
 *   00952838  83c410               add     esp, 0x10
 *   0095283b  c20400               retn    4
 */
CWindowBase::CWindowBase()
	: m_dwStyleFlags(0)
	, m_hWnd(nullptr)
	, m_hInstance(nullptr)
	, m_bChild(false) {
}

/**
 * [RECONSTRUCTED - 0x009528B0]
 * CWindowBase destructor
 * Native implementation @ 0x009528B0 (110 bytes)
 */
CWindowBase::~CWindowBase() {
	// Free all child windows
	for (CWindowBase* pChild : m_listChildren) {
		if (pChild != nullptr) {
			delete pChild;
		}
	}
	m_listChildren.clear();
	m_mapMessageHandlers.clear();
	m_mapCommandHandlers.clear();

	if (m_hWnd != nullptr) {
		ServerFramework_UnregisterWindowHandle(m_hWnd);
		m_hWnd = nullptr;
	}
}

/**
 * [RECONSTRUCTED - 0x00952B50]
 * CWindowBase::WindowProc
 * Native implementation @ 0x00952B50 (87 bytes)
 *
 * Virtual message dispatch procedure:
 *   - If uMsg == WM_DESTROY (2): if (!m_bChild) PostQuitMessage(0)
 *   - If uMsg == WM_COMMAND (0x111): return OnCommand(wParam, lParam)
 *   - Else: return OnMessage(uMsg, wParam, lParam)
 */
LRESULT CWindowBase::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
	if (uMsg == WM_DESTROY) {
		if (!m_bChild) {
			::PostQuitMessage(0);
		}
		return 0;
	} else if (uMsg == WM_COMMAND) {
		return OnCommand(wParam, lParam);
	}
	return OnMessage(uMsg, wParam, lParam);
#else
	(void)uMsg; (void)wParam; (void)lParam;
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952C90]
 * CWindowBase::OnMessage
 * Native implementation @ 0x00952C90 (126 bytes)
 */
LRESULT CWindowBase::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	auto it = m_mapMessageHandlers.find(uMsg);
	if (it != m_mapMessageHandlers.end() && it->second != nullptr) {
		return it->second(wParam, lParam);
	}

#ifdef _WIN32
	if (m_bChild) {
		return 0;
	}
	return ::DefWindowProcA(m_hWnd, uMsg, wParam, lParam);
#else
	(void)uMsg; (void)wParam; (void)lParam;
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952DF0]
 * CWindowBase::OnCommand
 * Native implementation @ 0x00952DF0 (83 bytes)
 */
LRESULT CWindowBase::OnCommand(WPARAM wParam, LPARAM lParam) {
	(void)lParam;
	UINT uCmdID = static_cast<UINT>(LOWORD(wParam));
	auto it = m_mapCommandHandlers.find(uCmdID);
	if (it != m_mapCommandHandlers.end() && it->second != nullptr) {
		it->second();
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00952C20]
 * CWindowBase::RegisterMessageHandler
 * Native implementation @ 0x00952C20 (99 bytes)
 */
bool CWindowBase::RegisterMessageHandler(UINT uMsg, PFN_MESSAGE_HANDLER pfnHandler) {
	auto it = m_mapMessageHandlers.find(uMsg);
	if (it != m_mapMessageHandlers.end()) {
		// Handler already registered - trigger minidump
		ServerFramework_GenerateMiniDump();
		return false;
	}
	m_mapMessageHandlers[uMsg] = pfnHandler;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00952D10]
 * CWindowBase::RegisterCommandHandler
 * Native implementation @ 0x00952D10 (102 bytes)
 */
bool CWindowBase::RegisterCommandHandler(UINT uCmdID, PFN_COMMAND_HANDLER pfnHandler) {
	auto it = m_mapCommandHandlers.find(uCmdID);
	if (it != m_mapCommandHandlers.end()) {
		// Handler already registered - trigger minidump
		ServerFramework_GenerateMiniDump();
		return false;
	}
	m_mapCommandHandlers[uCmdID] = pfnHandler;
	return true;
}

/**
 * [RECONSTRUCTED - 0x009529B0]
 * CWindowBase::Create
 * Native implementation @ 0x009529B0 (233 bytes)
 */
bool CWindowBase::Create(DWORD dwExStyle, const char* lpClassName, const char* lpWindowName,
                         DWORD dwStyle, int x1, int y1, int x2, int y2,
                         HWND hWndParent, HMENU hMenu, HINSTANCE hInstance) {
#ifdef _WIN32
	m_hInstance = hInstance;
	int nWidth = x2 - x1;
	int nHeight = y2 - y1;
	m_hWnd = ::CreateWindowExA(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		x1, y1, nWidth, nHeight,
		hWndParent,
		hMenu,
		hInstance,
		this
	);

	if (m_hWnd != nullptr) {
		ServerFramework_RegisterWindowHandle(m_hWnd, this);
		return true;
	}
	return false;
#else
	(void)dwExStyle; (void)lpClassName; (void)lpWindowName;
	(void)dwStyle; (void)x1; (void)y1; (void)x2; (void)y2;
	(void)hWndParent; (void)hMenu; (void)hInstance;
	return false;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952AF0]
 * CWindowBase::Destroy
 * Native implementation @ 0x00952AF0 (89 bytes)
 */
BOOL CWindowBase::Destroy() {
#ifdef _WIN32
	if (m_hWnd != nullptr && ::IsWindow(m_hWnd)) {
		::DestroyWindow(m_hWnd);
		ServerFramework_UnregisterWindowHandle(m_hWnd);
		m_hWnd = nullptr;
		return TRUE;
	}
#endif
	return FALSE;
}

/**
 * [RECONSTRUCTED - 0x00952BB0]
 * CWindowBase::ShowWindow
 * Native implementation @ 0x00952BB0 (35 bytes)
 */
BOOL CWindowBase::ShowWindow(int nCmdShow) {
#ifdef _WIN32
	if (m_hWnd != nullptr && ::IsWindow(m_hWnd)) {
		return ::ShowWindow(m_hWnd, nCmdShow);
	}
#else
	(void)nCmdShow;
#endif
	return FALSE;
}

/**
 * [RECONSTRUCTED - 0x00952C00]
 * CWindowBase::GetClientRect
 * Native implementation @ 0x00952C00 (14 bytes)
 */
BOOL CWindowBase::GetClientRect(RECT* lpRect) {
#ifdef _WIN32
	return ::GetClientRect(m_hWnd, lpRect);
#else
	if (lpRect) {
		lpRect->left = 0;
		lpRect->top = 0;
		lpRect->right = 800;
		lpRect->bottom = 600;
	}
	return TRUE;
#endif
}

/**
 * [RECONSTRUCTED - 0x00952C10]
 * CWindowBase::RedrawWindow
 * Native implementation @ 0x00952C10 (16 bytes)
 */
BOOL CWindowBase::RedrawWindow(UINT flags) {
#ifdef _WIN32
	return ::RedrawWindow(m_hWnd, nullptr, nullptr, flags);
#else
	(void)flags;
	return TRUE;
#endif
}

// ============================================================================
// Global Window Handle Registry (0x00D67A38)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00953410]
 * ServerFramework_RegisterWindowHandle
 */
void ServerFramework_RegisterWindowHandle(HWND hWnd, CWindowBase* pWindow) {
	if (hWnd != nullptr && pWindow != nullptr) {
		s_mapWindows[hWnd] = pWindow;
	}
}

/**
 * [RECONSTRUCTED - 0x00953790]
 * ServerFramework_FindWindowByHandle
 */
CWindowBase* ServerFramework_FindWindowByHandle(HWND hWnd) {
	if (hWnd == nullptr) return nullptr;
	auto it = s_mapWindows.find(hWnd);
	if (it != s_mapWindows.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x009534C0]
 * ServerFramework_UnregisterWindowHandle
 */
void ServerFramework_UnregisterWindowHandle(HWND hWnd) {
	if (hWnd != nullptr) {
		s_mapWindows.erase(hWnd);
	}
}

/**
 * [RECONSTRUCTED - 0x00952940]
 * ServerFramework_RegisterWindowClass
 */
BOOL ServerFramework_RegisterWindowClass(HINSTANCE hInstance, const char* pszClassName) {
#ifdef _WIN32
	WNDCLASSA wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = ServerFramework_WindowProc;
	wc.hInstance     = hInstance;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = pszClassName;
	return ::RegisterClassA(&wc) != 0;
#else
	(void)hInstance; (void)pszClassName;
	return TRUE;
#endif
}

HWND CWindowBase::GetSafeHwnd() const {
	return m_hWnd;
}

HWND CWindowBase::GetHwnd() const {
	return m_hWnd;
}

bool CWindowBase::IsChild() const {
	return m_bChild;
}

void CWindowBase::SetChild(bool bChild) {
	m_bChild = bChild;
}

} // namespace ServerFramework
