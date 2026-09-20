/**
 * ============================================================================
 * Joymax ServerFramework - Win32 Size Ruler Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\Win32SizeRuler.cpp
 *
 * Implements CWin32SizeRuler:
 *   - Native VTable @ 0x00B4163C (size 36 bytes = 9 slots)
 *   - Native Constructor @ 0x0095CBE0 (114 bytes)
 *   - Native Destructor @ 0x0095CC60 / 0x0095CC80 (115 bytes)
 *   - Native Create @ 0x0095CD00 (203 bytes)
 *   - Native SetPaneRatio @ 0x0095CDD0 (70 bytes)
 *   - Native HitTestSplitter @ 0x0095CE20 (173 bytes)
 *   - Native RecalcLayout @ 0x0095CED0 (259 bytes)
 *   - Native OnSize @ 0x0095CFE0 (32 bytes)
 *   - Native OnLButtonDown @ 0x0095D000 (70 bytes)
 *   - Native OnLButtonUp @ 0x0095D050 (62 bytes)
 *   - Native OnMouseMove @ 0x0095D090 (497 bytes)
 *   - Native OnPaint @ 0x0095D290 (292 bytes)
 *   - Native OnSetCursor @ 0x0095D3C0 (25 bytes)
 * ============================================================================
 */

#include "Win32SizeRuler.h"
#include "ServerFrameWindow.h"
#include <cmath>

namespace ServerFramework {

/**
 * [RECONSTRUCTED - 0x0095CBE0]
 * CWin32SizeRuler constructor
 * Native implementation @ 0x0095CBE0 (114 bytes)
 */
CWin32SizeRuler::CWin32SizeRuler()
	: CWindowBase()
	, m_listPanes()
	, m_pHitPane(nullptr)
	, m_nDragX(0)
	, m_nDragY(0)
	, m_hCursor(nullptr) {
}

/**
 * [RECONSTRUCTED - 0x0095CC60 / 0x0095CC80]
 * CWin32SizeRuler destructor
 * Native implementation @ 0x0095CC60 (30 bytes) / 0x0095CC80 (115 bytes)
 */
CWin32SizeRuler::~CWin32SizeRuler() {
	m_listPanes.clear();
	m_pHitPane = nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095CD00]
 * CWin32SizeRuler::Create
 * Native implementation @ 0x0095CD00 (203 bytes)
 *
 * Machine Disassembly:
 *   0095cd00  55                   push    ebp
 *   0095cd01  8bec                 mov     ebp, esp
 *   0095cd03  83e4f8               and     esp, 0xfffffff8
 *   0095cd06  51                   push    ecx
 *   0095cd07  8b551c               mov     edx, [ebp+0x1c]       ; hInstance
 *   0095cd0a  56                   push    esi
 *   0095cd0b  83ec10               sub     esp, 0x10
 *   0095cd0e  8bc4                 mov     eax, esp
 *   0095cd10  8bf1                 mov     esi, ecx              ; this
 *   0095cd12  8b4d18               mov     ecx, [ebp+0x18]       ; hWndParent
 *   0095cd15  8908                 mov     [eax], ecx
 *   0095cd17  8b4d20               mov     ecx, [ebp+0x20]       ; x1 (left)
 *   0095cd1a  895004               mov     [eax+4], edx
 *   0095cd1d  8b5524               mov     edx, [ebp+0x24]       ; y1 (top)
 *   0095cd20  894808               mov     [eax+8], ecx
 *   0095cd23  8b4d28               mov     ecx, [ebp+0x28]       ; x2 (right)
 *   0095cd26  89500c               mov     [eax+0xc], edx
 *   0095cd29  8b4530               mov     eax, [ebp+0x30]       ; y2 (bottom)
 *   0095cd2c  8b5514               mov     edx, [ebp+0x14]       ; dwStyle
 *   0095cd2f  50                   push    eax
 *   0095cd30  8b4510               mov     eax, [ebp+0x10]       ; lpWindowName
 *   0095cd33  51                   push    ecx
 *   0095cd34  8b4d0c               mov     ecx, [ebp+0xc]        ; lpClassName
 *   0095cd37  52                   push    edx
 *   0095cd38  8b5508               mov     edx, [ebp+8]          ; dwExStyle
 *   0095cd3b  50                   push    eax
 *   0095cd3c  8b452c               mov     eax, [ebp+0x2c]       ; hMenu
 *   0095cd3f  51                   push    ecx
 *   0095cd40  52                   push    edx
 *   0095cd41  56                   push    esi
 *   0095cd42  e8695cfefe           call    CWindowBase_CreateWindowEx
 *   0095cd47  84c0                 test    al, al
 *   0095cd49  7507                 jne     0x0095cd52
 *   0095cd4b  5e                   pop     esi
 *   0095cd4c  8be5                 mov     esp, ebp
 *   0095cd4e  5d                   pop     ebp
 *   0095cd4f  c22c00               retn    0x2c                  ; return false
 *   0095cd52  68c0d59500           push    CWin32SizeRuler_thunk_WM_SIZE (0x0095D5C0)
 *   0095cd57  6a05                 push    5                     ; WM_SIZE
 *   0095cd59  8bc6                 mov     eax, esi
 *   0095cd5b  e8c05efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cd60  68e0d59500           push    CWin32SizeRuler_thunk_WM_LBUTTONDOWN (0x0095D5E0)
 *   0095cd65  6801020000           push    0x201                 ; WM_LBUTTONDOWN
 *   0095cd6a  8bc6                 mov     eax, esi
 *   0095cd6c  e8af5efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cd71  6810d69500           push    CWin32SizeRuler_thunk_WM_MOUSEMOVE (0x0095D610)
 *   0095cd76  6800020000           push    0x200                 ; WM_MOUSEMOVE
 *   0095cd7b  8bc6                 mov     eax, esi
 *   0095cd7d  e89e5efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cd82  68f0d59500           push    CWin32SizeRuler_thunk_WM_LBUTTONUP (0x0095D5F0)
 *   0095cd87  6802020000           push    0x202                 ; WM_LBUTTONUP
 *   0095cd8c  8bc6                 mov     eax, esi
 *   0095cd8e  e88d5efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cd93  6800d69500           push    CWin32SizeRuler_thunk_WM_PAINT (0x0095D600)
 *   0095cd98  6a0f                 push    0xf                   ; WM_PAINT
 *   0095cd9a  8bc6                 mov     eax, esi
 *   0095cd9c  e87f5efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cda1  68d0d59500           push    CWin32SizeRuler_thunk_WM_SETCURSOR (0x0095D5D0)
 *   0095cda6  6a20                 push    0x20                  ; WM_SETCURSOR
 *   0095cda8  8bc6                 mov     eax, esi
 *   0095cdaa  e8715efefe           call    CWindowBase_RegisterMessageHandler
 *   0095cdaf  68007f0000           push    0x7f00                ; IDC_ARROW
 *   0095cdb4  6a00                 push    0                     ; NULL
 *   0095cdb6  ff157893ad00         call    dword [LoadCursorA]
 *   0095cdbc  898688000000         mov     [esi+0x88], eax       ; m_hCursor = LoadCursorA(NULL, IDC_ARROW)
 *   0095cdc2  b001                 mov     al, 1                 ; return true
 *   0095cdc4  5e                   pop     esi
 *   0095cdc5  8be5                 mov     esp, ebp
 *   0095cdc7  5d                   pop     ebp
 *   0095cdc8  c22c00               retn    0x2c
 */
bool CWin32SizeRuler::Create(DWORD dwExStyle, const char* lpClassName, const char* lpWindowName,
                            DWORD dwStyle, int x1, int y1, int x2, int y2,
                            HWND hWndParent, HMENU hMenu, HINSTANCE hInstance) {
	if (!CWindowBase::Create(dwExStyle, lpClassName, lpWindowName, dwStyle,
	                        x1, y1, x2, y2, hWndParent, hMenu, hInstance)) {
		return false;
	}

#ifdef _WIN32
	// Register the 6 native size-ruler message handlers (thunks 0x0095D5C0 - 0x0095D610)
	RegisterMessageHandler(WM_SIZE, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnSize(wParam, lParam);
	});
	RegisterMessageHandler(WM_LBUTTONDOWN, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnLButtonDown(wParam, lParam);
	});
	RegisterMessageHandler(WM_MOUSEMOVE, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnMouseMove(wParam, lParam);
	});
	RegisterMessageHandler(WM_LBUTTONUP, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnLButtonUp(wParam, lParam);
	});
	RegisterMessageHandler(WM_PAINT, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnPaint(wParam, lParam);
	});
	RegisterMessageHandler(WM_SETCURSOR, [this](WPARAM wParam, LPARAM lParam) -> LRESULT {
		return OnSetCursor(wParam, lParam);
	});

	// Load standard arrow cursor into m_hCursor (offset +0x88)
	m_hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
#endif
	return true;
}

/**
 * [RECONSTRUCTED - 0x0095CDD0]
 * CWin32SizeRuler::SetPaneRatio
 * Native implementation @ 0x0095CDD0 (70 bytes)
 *
 * Adds a splitter pane entry into m_listPanes.
 */
bool CWin32SizeRuler::SetPaneRatio(HWND hWndUpper, HWND hWndLower, float fRatio) {
	tagSizeRulerPane entry;
	entry.fRatio = fRatio;
	entry.hWndUpper = hWndUpper;
	entry.hWndLower = hWndLower;
	m_listPanes.push_back(entry);
	return true;
}

/**
 * [RECONSTRUCTED - 0x0095CED0]
 * CWin32SizeRuler::RecalcLayout
 * Native implementation @ 0x0095CED0 (259 bytes)
 *
 * Machine Disassembly:
 *   0095ced0  55                   push    ebp
 *   0095ced1  8bec                 mov     ebp, esp
 *   0095ced3  83e4f8               and     esp, 0xfffffff8
 *   0095ced6  83ec1c               sub     esp, 0x1c
 *   0095ced9  8b4874               mov     ecx, [eax+0x74]       ; m_listPanes._Myhead
 *   0095cedd  8b19                 mov     ebx, [ecx]            ; first node
 *   0095cee0  33f6                 xor     esi, esi              ; pPrev = nullptr
 *   ...
 *   0095cf1a  db450c               fild    dword [ebp+0xc]       ; (float)cy
 *   0095cf1d  6a01                 push    1                     ; bRepaint = TRUE
 *   0095cf1f  d80f                 fmul    dword [edi]           ; * pane->fRatio
 *   0095cf21  dc1d50cfb400         fsub    qword [0x00B45CF0]    ; - 4.0
 *   0095cf27  e814ec0900           call    CRT_ftol              ; nHeight
 *   0095cf2c  8b4f04               mov     ecx, [edi+4]          ; pane->hWndUpper
 *   0095cf37  ff156c93ad00         call    dword [MoveWindow]    ; MoveWindow(pane->hWndUpper, 0, 0, cx, nHeight, TRUE)
 *   ...
 *   0095cf56  e8e5eb0900           call    CRT_ftol              ; Y = (int)(pPrev->fRatio * cy)
 *   0095cf61  e8daeb0900           call    CRT_ftol              ; (int)(pane->fRatio * cy)
 *   0095cf6b  83e804               sub     eax, 4                ; nHeight = (int)(pane->fRatio * cy) - Y - 4
 *   0095cf6f  8b4608               mov     eax, [esi+8]          ; pPrev->hWndLower
 *   0095cf77  ff156c93ad00         call    dword [MoveWindow]    ; MoveWindow(pPrev->hWndLower, 0, Y, cx, nHeight, TRUE)
 *   ...
 *   0095cfa7  e894eb0900           call    CRT_ftol              ; Y = (int)(cy * pPrev->fRatio)
 *   0095cfb4  2bc8                 sub     ecx, eax              ; nHeight = cy - Y
 *   0095cfb9  8b4608               mov     eax, [esi+8]          ; pPrev->hWndLower
 *   0095cfbf  ff156c93ad00         call    dword [MoveWindow]    ; MoveWindow(pPrev->hWndLower, 0, Y, cx, cy - Y, TRUE)
 */
bool CWin32SizeRuler::RecalcLayout(int cx, int cy) {
#ifdef _WIN32
	if (m_listPanes.empty()) {
		return false;
	}

	tagSizeRulerPane* pPrev = nullptr;
	for (auto& pane : m_listPanes) {
		if (pPrev == nullptr) {
			int nHeight = static_cast<int>(static_cast<float>(cy) * pane.fRatio - 4.0f);
			if (pane.hWndUpper != nullptr) {
				::MoveWindow(pane.hWndUpper, 0, 0, cx, nHeight, TRUE);
			}
		} else {
			int nY = static_cast<int>(static_cast<float>(cy) * pPrev->fRatio);
			int nHeight = static_cast<int>(static_cast<float>(cy) * pane.fRatio) - nY - 4;
			if (pPrev->hWndLower != nullptr) {
				::MoveWindow(pPrev->hWndLower, 0, nY, cx, nHeight, TRUE);
			}
		}
		pPrev = &pane;
	}

	if (pPrev != nullptr && pPrev->hWndLower != nullptr) {
		int nY = static_cast<int>(static_cast<float>(cy) * pPrev->fRatio);
		int nHeight = cy - nY;
		::MoveWindow(pPrev->hWndLower, 0, nY, cx, nHeight, TRUE);
	}
	return true;
#else
	(void)cx; (void)cy;
	return true;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095CE20]
 * CWin32SizeRuler::HitTestSplitter
 * Native implementation @ 0x0095CE20 (173 bytes)
 *
 * Machine Disassembly:
 *   0095ce20  55                   push    ebp
 *   0095ce21  8bec                 mov     ebp, esp
 *   0095ce23  83e4f8               and     esp, 0xfffffff8
 *   0095ce26  83ec24               sub     esp, 0x24
 *   0095ce29  53                   push    ebx
 *   0095ce2a  8b5d08               mov     ebx, [ebp+8]          ; this
 *   0095ce2d  56                   push    esi
 *   0095ce2e  57                   push    edi
 *   0095ce2f  8d742420             lea     esi, [esp+0x20]       ; &rcClient
 *   0095ce33  8bc3                 mov     eax, ebx
 *   0095ce35  e8c65dffff           call    CWindowBase_GetClientRect ; GetClientRect(&rcClient)
 *   ...
 *   0095ce7b  db44240c             fild    dword [esp+0xc]       ; cy
 *   0095ce7f  d80f                 fmul    dword [edi]           ; * pane->fRatio
 *   0095ce81  d8e1                 fsub    st0, st1              ; - 2.0 (from 0x00B45F30)
 *   0095ce83  e8b8ec0900           call    CRT_ftol              ; splitterY
 *   0095ce88  2b450c               sub     eax, [ebp+0xc]        ; splitterY - y
 *   0095ce8b  99                   cdq
 *   0095ce8c  33c2                 xor     eax, edx
 *   0095ce8e  2bc2                 sub     eax, edx              ; abs(splitterY - y)
 *   0095ce90  83f804               cmp     eax, 4                ; <= 4 pixels tolerance
 *   0095ce93  7e04                 jle     0x0095ce99            ; hit!
 *   ... Hit:
 *   0095ce9e  8b4214               mov     eax, [edx+0x14]       ; m_hWnd
 *   0095cea1  50                   push    eax
 *   0095cea2  ff151493ad00         call    dword [SetCapture]    ; SetCapture(m_hWnd)
 *   0095cea8  8bc7                 mov     eax, edi              ; return &pane
 *   ... Miss (end of loop):
 *   0095ceb5  ff151093ad00         call    dword [ReleaseCapture] ; ReleaseCapture()
 *   0095cebd  33c0                 xor     eax, eax              ; return nullptr
 */
tagSizeRulerPane* CWin32SizeRuler::HitTestSplitter(int y) {
#ifdef _WIN32
	RECT rcClient = {};
	GetClientRect(&rcClient);
	int cy = rcClient.bottom - rcClient.top;

	for (auto& pane : m_listPanes) {
		int splitterY = static_cast<int>(static_cast<float>(cy) * pane.fRatio - 2.0f);
		if (std::abs(splitterY - y) <= 4) {
			::SetCapture(m_hWnd);
			return &pane;
		}
	}
	::ReleaseCapture();
#else
	(void)y;
#endif
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0095D290]
 * CWin32SizeRuler::OnPaint
 * Native implementation @ 0x0095D290 (292 bytes)
 *
 * Machine Disassembly:
 *   0095d290  55                   push    ebp
 *   0095d291  8bec                 mov     ebp, esp
 *   0095d293  83e4f8               and     esp, 0xfffffff8
 *   0095d296  83ec7c               sub     esp, 0x7c
 *   0095d299  a18015c600           mov     eax, [__security_cookie]
 *   0095d29e  33c4                 xor     eax, esp
 *   0095d2a0  89442478             mov     [esp+0x78], eax
 *   0095d2a4  53                   push    ebx
 *   0095d2a5  56                   push    esi
 *   0095d2a6  57                   push    edi
 *   0095d2a7  8bd9                 mov     ebx, ecx              ; this (CWin32SizeRuler*)
 *   0095d2a9  8b4b14               mov     ecx, [ebx+0x14]       ; m_hWnd (+0x14)
 *   0095d2ac  8d442440             lea     eax, [esp+0x40]       ; &ps
 *   0095d2b0  50                   push    eax
 *   0095d2b1  51                   push    ecx
 *   0095d2b2  ff15ec93ad00         call    dword [BeginPaint]
 *   0095d2b8  89442410             mov     [esp+0x10], eax       ; hdc
 *   0095d2bc  8d742430             lea     esi, [esp+0x30]       ; &rcClient
 *   0095d2c0  8bc3                 mov     eax, ebx              ; this
 *   0095d2c2  e83959ffff           call    CWindowBase_GetClientRect
 *   0095d2c7  8b10                 mov     edx, [eax]            ; left
 *   0095d2c9  8b4804               mov     ecx, [eax+4]          ; top
 *   0095d2cc  89542414             mov     [esp+0x14], edx
 *   0095d2d0  8b5008               mov     edx, [eax+8]          ; right
 *   0095d2d3  8b400c               mov     eax, [eax+0xc]        ; bottom
 *   0095d2d6  894c2418             mov     [esp+0x18], ecx
 *   0095d2da  8b4b74               mov     ecx, [ebx+0x74]       ; m_listPanes._Myhead
 *   0095d2dd  8b39                 mov     edi, [ecx]            ; first node
 *   ... Loop:
 *   0095d31a  db44242c             fild    dword [esp+0x2c]      ; (float)cy
 *   0095d31e  d80f                 fmul    dword [edi]           ; * pane->fRatio
 *   0095d320  dc25305fb400         fsub    qword [0x00B45F30]    ; - 2.0
 *   0095d326  e815e80900           call    CRT_ftol              ; nY
 *   0095d32b  3b7b7c               cmp     edi, [ebx+0x7c]       ; pane == m_pHitPane
 *   0095d33c  680000ff00           push    0x00FF0000            ; RGB(255, 0, 0)
 *   0095d345  6a0f                 push    0xF                   ; COLOR_BTNFACE
 *   0095d347  ff158093ad00         call    dword [GetSysColor]
 *   0095d364  e88750ffff           call    GDI_DrawLine          ; GDI_DrawLine(hdc, 0, nY, cx, nY, crLine, PS_SOLID)
 *   ... End Loop:
 *   0095d387  8b5314               mov     edx, [ebx+0x14]       ; m_hWnd
 *   0095d38a  8d4c2440             lea     ecx, [esp+0x40]       ; &ps
 *   0095d38e  51                   push    ecx
 *   0095d38f  52                   push    edx
 *   0095d390  ff15b493ad00         call    dword [EndPaint]
 *   0095d3ac  c20800               retn    8
 */
LRESULT CWin32SizeRuler::OnPaint(WPARAM wParam, LPARAM lParam) {
	(void)wParam;
	(void)lParam;
#ifdef _WIN32
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);
	if (hdc) {
		RECT rcClient = {};
		GetClientRect(&rcClient);
		int cx = rcClient.right - rcClient.left;
		int cy = rcClient.bottom - rcClient.top;

		for (const auto& pane : m_listPanes) {
			int nY = static_cast<int>(static_cast<float>(cy) * pane.fRatio - 2.0f);
			COLORREF crLine = ::GetSysColor(COLOR_BTNFACE);
			if (m_pHitPane == &pane) {
				crLine = RGB(255, 0, 0);
			}
			GDI_DrawLine(hdc, 0, nY, cx, nY, crLine, PS_SOLID);
		}
		::EndPaint(m_hWnd, &ps);
	}
	return 0;
#else
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095CFE0]
 * CWin32SizeRuler::OnSize
 * Native implementation @ 0x0095CFE0 (32 bytes)
 *
 * Machine Disassembly:
 *   0095cfe0  8bc1                 mov     eax, ecx              ; this
 *   0095cfe2  83787800             cmp     dword [eax+0x78], 0   ; m_listPanes._Mysize == 0
 *   0095cfe6  7413                 je      0x0095cffb            ; return 0
 *   0095cfe8  8b542408             mov     edx, [esp+8]          ; lParam
 *   0095cfec  8bca                 mov     ecx, edx
 *   0095cfee  c1e910               shr     ecx, 16               ; cy = (UINT)HIWORD(lParam)
 *   0095cff1  0fb7d2               movzx   edx, dx               ; cx = (UINT)LOWORD(lParam)
 *   0095cff4  51                   push    ecx                   ; cy
 *   0095cff5  52                   push    edx                   ; cx
 *   0095cff6  e8d5feffff           call    CWin32SizeRuler_RecalcLayout ; RecalcLayout(cx, cy)
 *   0095cffb  33c0                 xor     eax, eax              ; return 0
 *   0095cffd  c20800               retn    8                     ; pop wParam, lParam
 */
LRESULT CWin32SizeRuler::OnSize(WPARAM wParam, LPARAM lParam) {
	(void)wParam;
	if (!m_listPanes.empty()) {
		int cx = LOWORD(lParam);
		int cy = HIWORD(lParam);
		RecalcLayout(cx, cy);
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095D000]
 * CWin32SizeRuler::OnLButtonDown
 * Native implementation @ 0x0095D000 (70 bytes)
 *
 * Machine Disassembly:
 *   0095d000  8b442408             mov     eax, [esp+8]          ; lParam
 *   0095d004  56                   push    esi
 *   0095d005  8bf1                 mov     esi, ecx              ; this (CWin32SizeRuler*)
 *   0095d007  0fbfcf               movsx   ecx, ax               ; ecx = (short)LOWORD(lParam)
 *   0095d00a  c1e810               shr     eax, 16
 *   0095d00d  0fbff8               movsx   eax, ax               ; eax = (short)HIWORD(lParam)
 *   0095d010  50                   push    eax                   ; y
 *   0095d011  56                   push    esi                   ; this
 *   0095d012  898e80000000         mov     [esi+0x80], ecx       ; m_nDragX = ecx
 *   0095d018  898684000000         mov     [esi+0x84], eax       ; m_nDragY = eax
 *   0095d01e  e8fdfdffff           call    CWin32SizeRuler_HitTestSplitter ; HitTestSplitter(y)
 *   0095d023  85c0                 test    eax, eax
 *   0095d025  89867c000000         mov     [esi+0x7c], eax       ; m_pHitPane = hitPane
 *   0095d028  7416                 je      0x0095d040            ; if NULL, return 0
 *   0095d02a  8b5614               mov     edx, [esi+0x14]       ; m_hWnd
 *   0095d02d  52                   push    edx
 *   0095d02e  ff151493ad00         call    dword [SetCapture]    ; SetCapture(m_hWnd)
 *   0095d034  b801000000           mov     eax, 1                ; RDW_INVALIDATE
 *   0095d039  8bce                 mov     ecx, esi              ; this
 *   0095d03b  e8d05bffff           call    CWindowBase_RedrawWindow ; RedrawWindow(RDW_INVALIDATE)
 *   0095d040  33c0                 xor     eax, eax              ; return 0
 *   0095d042  5e                   pop     esi
 *   0095d043  c20800               retn    8                     ; pop wParam, lParam
 */
LRESULT CWin32SizeRuler::OnLButtonDown(WPARAM wParam, LPARAM lParam) {
	(void)wParam;
#ifdef _WIN32
	m_nDragX = static_cast<int>(static_cast<short>(LOWORD(lParam)));
	m_nDragY = static_cast<int>(static_cast<short>(HIWORD(lParam)));
	m_pHitPane = HitTestSplitter(m_nDragY);
	if (m_pHitPane != nullptr) {
		::SetCapture(m_hWnd);
		RedrawWindow(RDW_INVALIDATE);
	}
#else
	(void)lParam;
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095D050]
 * CWin32SizeRuler::OnLButtonUp
 * Native implementation @ 0x0095D050 (62 bytes)
 *
 * Machine Disassembly:
 *   0095d050  56                   push    esi
 *   0095d051  8bf1                 mov     esi, ecx              ; this (CWin32SizeRuler*)
 *   0095d053  ff151093ad00         call    dword [ReleaseCapture] ; ReleaseCapture()
 *   0095d059  837e7c00             cmp     dword [esi+0x7c], 0   ; cmp m_pHitPane, 0
 *   0095d05d  7426                 je      0x0095d085            ; if NULL, skip cursor reset & redraw
 *   0095d05f  68007f0000           push    0x7f00                ; IDC_ARROW
 *   0095d064  6a00                 push    0                     ; NULL
 *   0095d066  ff157893ad00         call    dword [LoadCursorA]   ; LoadCursorA(NULL, IDC_ARROW)
 *   0095d06c  50                   push    eax
 *   0095d06d  898688000000         mov     [esi+0x88], eax       ; m_hCursor = eax
 *   0095d073  ff157c93ad00         call    dword [SetCursor]     ; SetCursor(m_hCursor)
 *   0095d079  b801000000           mov     eax, 1                ; RDW_INVALIDATE
 *   0095d07e  8bce                 mov     ecx, esi              ; this
 *   0095d080  e88b5bffff           call    CWindowBase_RedrawWindow ; RedrawWindow(RDW_INVALIDATE)
 *   0095d085  33c0                 xor     eax, eax              ; eax = 0
 *   0095d087  89467c               mov     [esi+0x7c], eax       ; m_pHitPane = NULL
 *   0095d08a  5e                   pop     esi
 *   0095d08b  c20800               retn    8                     ; pop wParam, lParam
 */
LRESULT CWin32SizeRuler::OnLButtonUp(WPARAM wParam, LPARAM lParam) {
	(void)wParam;
	(void)lParam;
#ifdef _WIN32
	::ReleaseCapture();
	if (m_pHitPane != nullptr) {
		m_hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
		::SetCursor(m_hCursor);
		RedrawWindow(RDW_INVALIDATE);
	}
	m_pHitPane = nullptr;
#endif
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0095D090]
 * CWin32SizeRuler::OnMouseMove
 * Native implementation @ 0x0095D090 (497 bytes)
 *
 * Machine Disassembly:
 *   0095d090  55                   push    ebp
 *   0095d091  8bec                 mov     ebp, esp
 *   0095d093  83e4f8               and     esp, 0xfffffff8
 *   0095d096  83ec2c               sub     esp, 0x2c
 *   0095d099  f6450801             test    byte [ebp+8], 1       ; test (wParam & MK_LBUTTON)
 *   0095d09d  53                   push    ebx
 *   0095d09e  56                   push    esi
 *   0095d09f  57                   push    edi
 *   0095d0a0  8bf9                 mov     edi, ecx              ; this (CWin32SizeRuler*)
 *   0095d0a2  0f8499010000         je      0x0095d241            ; if no button down, goto hover cursor update
 *   0095d0a8  837f7c00             cmp     dword [edi+0x7c], 0   ; cmp this->m_pHitPane, 0
 *   0095d0ac  0f848f010000         je      0x0095d241            ; if no hit pane, goto hover cursor update
 *   0095d0b2  8d742418             lea     esi, [esp+0x18]       ; &rcClient
 *   0095d0b6  8bc7                 mov     eax, edi
 *   0095d0b8  e8435bffff           call    CWindowBase_GetClientRect ; GetClientRect(&rcClient)
 *   0095d0d3  2bc2                 sub     eax, edx              ; cy = rcClient.bottom - rcClient.top
 *   0095d0d5  0fbf4d0c             movsx   esi, word [ebp+0xc]   ; x = (short)LOWORD(lParam)
 *   0095d0df  0fbf4d0e             movsx   ecx, word [ebp+0xe]   ; y = (short)HIWORD(lParam)
 *   0095d100  d95c240c             fstp    dword [esp+0xc]       ; fRatio = (float)y / (float)cy
 *   0095d104  dc25c05fb400         fdivr   qword [0x00B45FC0]    ; 35.0 / (float)cy
 *   0095d10a  d95c2414             fstp    dword [esp+0x14]      ; fMinRatio = 35.0f / (float)cy
 *   ... [fRatio clamped to [fMinRatio, 1.0f - fMinRatio]]
 *   ... [Adjacent pane collision clamping loop: 0x0095D14C - 0x0095D1B7]
 *   0095d1fa  d919                 fstp    dword [ecx]           ; m_pHitPane->fRatio = new fRatio
 *   0095d204  898f80000000         mov     [edi+0x80], ecx       ; this->m_nDragX = x
 *   0095d20a  898784000000         mov     [edi+0x84], eax       ; this->m_nDragY = y
 *   0095d210  7464                 je      0x0095d276            ; if (!bChanged) return 0
 *   0095d21a  e8b1fdffff           call    CWin32SizeRuler_RecalcLayout ; RecalcLayout(cx, cy)
 *   0095d226  e8e55bffff           call    CWindowBase_RedrawWindow     ; RedrawWindow(RDW_INVALIDATE)
 *   ... Hover state (0x0095D241):
 *   0095d24c  e8cf5bffff           call    CWin32SizeRuler_HitTestSplitter
 *   0095d255  68857f0000           push    0x7f85                ; IDC_SIZENS
 *   0095d25c  68007f0000           push    0x7f00                ; IDC_ARROW
 *   0095d263  ff157893ad00         call    dword [LoadCursorA]   ; LoadCursorA(NULL, lpCursorName)
 *   0095d26a  898788000000         mov     [edi+0x88], eax       ; this->m_hCursor = eax
 *   0095d270  ff157c93ad00         call    dword [SetCursor]     ; SetCursor(m_hCursor)
 *   0095d27e  c20800               retn    8
 */
LRESULT CWin32SizeRuler::OnMouseMove(WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
	int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
	int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));

	// Hover state: update cursor to IDC_SIZENS if over a splitter, else IDC_ARROW
	if ((wParam & MK_LBUTTON) == 0 || m_pHitPane == nullptr) {
		tagSizeRulerPane* pPane = HitTestSplitter(y);
		LPCSTR lpCursorName = (pPane != nullptr) ? IDC_SIZENS : IDC_ARROW;
		m_hCursor = ::LoadCursorA(nullptr, lpCursorName);
		::SetCursor(m_hCursor);
		return 0;
	}

	// Active dragging state
	RECT rcClient = {};
	GetClientRect(&rcClient);
	int cx = rcClient.right - rcClient.left;
	int cy = rcClient.bottom - rcClient.top;

	if (cy > 0) {
		float fNewRatio = static_cast<float>(y) / static_cast<float>(cy);
		float fMinRatio = 35.0f / static_cast<float>(cy);
		float fMaxRatio = 1.0f - fMinRatio;

		if (fNewRatio < fMinRatio) {
			fNewRatio = fMinRatio;
		}
		if (fNewRatio > fMaxRatio) {
			fNewRatio = fMaxRatio;
		}

		// Check adjacent panes constraints
		for (const auto& pane : m_listPanes) {
			if (&pane == m_pHitPane) continue;

			if (pane.hWndLower == m_pHitPane->hWndUpper) {
				if (pane.fRatio > fNewRatio - fMinRatio) {
					fNewRatio = pane.fRatio + fMinRatio;
				}
			}
			if (pane.hWndUpper == m_pHitPane->hWndLower) {
				if (pane.fRatio < fNewRatio + fMinRatio) {
					fNewRatio = pane.fRatio - fMinRatio;
				}
			}
		}

		// Determine if changed by at least 1 pixel
		bool bChanged = false;
		if (cx > 0) {
			float fDelta = std::abs(m_pHitPane->fRatio - fNewRatio);
			if (fDelta >= (1.0f / static_cast<float>(cx))) {
				bChanged = true;
			}
		}

		// Always update ratio and coordinates
		m_pHitPane->fRatio = fNewRatio;
		m_nDragX = x;
		m_nDragY = y;

		// Only recalculate layout and redraw if moved by >= 1 pixel
		if (bChanged) {
			RecalcLayout(cx, cy);
			RedrawWindow(RDW_INVALIDATE);
		}
	}
	return 0;
#else
	(void)wParam; (void)lParam;
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x0095D3C0]
 * CWin32SizeRuler::OnSetCursor
 * Native implementation @ 0x0095D3C0 (25 bytes)
 *
 * Machine Disassembly:
 *   0095d3c0  8b442408             mov     eax, [esp+8]          ; lParam
 *   0095d3c4  8b542404             mov     edx, [esp+4]          ; wParam
 *   0095d3c8  50                   push    eax
 *   0095d3c9  8b4114               mov     eax, [ecx+0x14]       ; m_hWnd (+0x14)
 *   0095d3cc  52                   push    edx
 *   0095d3cd  6a20                 push    0x20                  ; WM_SETCURSOR
 *   0095d3cf  50                   push    eax                   ; m_hWnd
 *   0095d3d0  ff153893ad00         call    dword [DefWindowProcA]
 *   0095d3d6  c20800               retn    8
 */
LRESULT CWin32SizeRuler::OnSetCursor(WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
	return ::DefWindowProcA(m_hWnd, WM_SETCURSOR, wParam, lParam);
#else
	(void)wParam;
	(void)lParam;
	return 0;
#endif
}

} // namespace ServerFramework
