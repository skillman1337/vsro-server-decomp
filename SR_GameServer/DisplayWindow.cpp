/**
 * ============================================================================
 * Silkroad Online - Game Server Display Window Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\DisplayWindow.cpp
 *
 * Implements:
 *   - CDisplayWindow::CDisplayWindow() @ 0x005641A0 (3,056 bytes)
 *   - CDisplayWindow::~CDisplayWindow() @ 0x00565110 (261 bytes)
 *   - CDisplayWindow::Create() @ 0x00565220 / 0x00565410
 *   - Layer render callbacks:
 *     - CDisplayWindow_RenderTerrain @ 0x00566B10
 *     - CDisplayWindow_RenderCharacter @ 0x00567190
 *     - CDisplayWindow_RenderItem @ 0x009BF500
 *     - CDisplayWindow_RenderMainStatus @ 0x00566AA0
 *     - CDisplayWindow_RenderDrawAI @ 0x00567A20
 * ============================================================================
 */

#include "DisplayWindow.h"
#include "GameAI.h"
#include "GObjChar.h"
#include "../JMX_ServerFramework/ServerFramework/ServerMain.h"
#include "../JMX_ServerFramework/ServerFramework/ServerFrameWindow.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Native global singleton pointer @ 0x00D6A980
CDisplayWindow* CDisplayWindow::s_pInstance = nullptr;

CDisplayWindow* CDisplayWindow::GetInstance() {
	return s_pInstance;
}

/**
 * [RECONSTRUCTED - 0x005641A0]
 * CDisplayWindow::CDisplayWindow
 * Native implementation @ 0x005641A0 (3,056 bytes)
 *
 * Instantiates the game server 2D radar display window, registers 5 display layers
 * ("Terrain", "Character", "Item", "MainStatus", "DrawAI"), and loads viewport
 * configuration overrides from server.cfg.
 */
CDisplayWindow::CDisplayWindow()
	: ServerFramework::CServerChildWindowBase()
	, m_hMemDC(nullptr)
	, m_hBitmap(nullptr)
	, m_hOldBitmap(nullptr)
	, m_nClientWidth(0)
	, m_nClientHeight(0)
	, m_csLock()
	, m_dwDrawInterval(100)
	, m_dwLastTick(0)
	, m_dwLastRenderDuration(0)
	, m_dwLastDrawTick(0)
	, m_nZoomStep(0)
	, m_fZoom(1.0f)
	, m_bDragging(false)
	, m_fScaleX(0.0f)
	, m_fScaleY(0.0f)
	, m_bDrawPlayer(true)
	, m_bDrawMonster(true)
	, m_bDrawNPC(true)
	, m_bDrawCOS(true)
	, m_bDrawCharName(true)
	, m_bDrawGID(false)
	, m_bDrawNavigation(false)
	, m_bDrawState(true) {

	m_strWindowName = "GameServerDisplay";

	// Native 0x005641D8: Assert singleton instance is unique
	if (s_pInstance != nullptr) {
		BSLib::GenerateMiniDump();
	}
	s_pInstance = this;

	// Native 0x00564245 - 0x00564570: Allocate and register 5 standard display layers
	// 1. "Terrain" (Layer type 2, enabled by default, render callback 0x00566B10)
	RegisterLayer("Terrain", 2, 1, &CDisplayWindow::RenderTerrain);

	// 2. "Character" (Layer type 2, disabled by default, render callback 0x00567190)
	RegisterLayer("Character", 2, 0, &CDisplayWindow::RenderCharacter);

	// 3. "Item" (Layer type 2, disabled by default, render callback 0x009BF500)
	RegisterLayer("Item", 2, 0, &CDisplayWindow::RenderItem);

	// 4. "MainStatus" (Layer type 2, enabled by default, render callback 0x00566AA0)
	RegisterLayer("MainStatus", 2, 1, &CDisplayWindow::RenderMainStatus);

	// 5. "DrawAI" (Layer type 5, disabled by default, render callback 0x00567A20)
	RegisterLayer("DrawAI", 5, 0, &CDisplayWindow::RenderDrawAI);

	// Native 0x005645A0 - 0x00564880: Parse layer options from server.cfg ("DrawOption_<LayerName>")
	for (tagDisplayLayer* pLayer : m_listLayers) {
		if (!pLayer) continue;

		char szKey[260];
		std::snprintf(szKey, sizeof(szKey), "DrawOption_%s", pLayer->szName);
		const char* szVal = ServerFramework::ServerFramework_GetConfigString(szKey);
		if (szVal != nullptr) {
			pLayer->bEnabled = (std::atoi(szVal) == 1) ? 1 : 0;
		}

		// Also check display mode option ("DrawOption_<LayerName>_Type")
		std::snprintf(szKey, sizeof(szKey), "DrawOption_%s_Type", pLayer->szName);
		const char* szTypeVal = ServerFramework::ServerFramework_GetConfigString(szKey);
		if (szTypeVal != nullptr) {
			uint8_t bySelected = static_cast<uint8_t>(std::atoi(szTypeVal));
			if (bySelected < pLayer->byType) {
				pLayer->bSelected = bySelected;
			}
		}
	}

	// Native 0x00564890: Parse "DrawInterval" (timer refresh rate)
	const char* szInterval = ServerFramework::ServerFramework_GetConfigString("DrawInterval");
	if (szInterval != nullptr) {
		m_dwDrawInterval = static_cast<uint32_t>(std::atoi(szInterval));
	}

	// Native 0x005648C0 - 0x00564A20: Parse Character drawing options from server.cfg
	const char* szPlayer = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_Player");
	if (szPlayer != nullptr) {
		m_bDrawPlayer = (std::atoi(szPlayer) == 1);
	}

	const char* szMonster = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_Monster");
	if (szMonster != nullptr) {
		m_bDrawMonster = (std::atoi(szMonster) == 1);
	}

	const char* szNPC = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_NPC");
	if (szNPC != nullptr) {
		m_bDrawNPC = (std::atoi(szNPC) == 1);
	}

	const char* szCOS = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_COS");
	if (szCOS != nullptr) {
		m_bDrawCOS = (std::atoi(szCOS) == 1);
	}

	const char* szName = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_DrawCharName");
	if (szName != nullptr) {
		m_bDrawCharName = (std::atoi(szName) == 1);
	}

	const char* szGID = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_DrawGID");
	if (szGID != nullptr) {
		m_bDrawGID = (std::atoi(szGID) == 1);
	}

	const char* szNav = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_DrawNavigation");
	if (szNav != nullptr) {
		m_bDrawNavigation = (std::atoi(szNav) == 1);
	}

	const char* szState = ServerFramework::ServerFramework_GetConfigString("CharDrawOption_DrawState");
	if (szState != nullptr) {
		m_bDrawState = (std::atoi(szState) == 1);
	}
}

/**
 * [RECONSTRUCTED - 0x00565110]
 * CDisplayWindow::~CDisplayWindow
 * Native non-deleting destructor @ 0x00565110 (261 bytes)
 *
 * Sequence proven against exact native assembly and MSVC UnwindMap:
 *   0x0056513D: Resets vtable to CDisplayWindow::vftable (0x00AFA5A4)
 *   0x0056514C: Member +0x1C0: std_map_string_DisplayLayer_destructor
 *   0x0056515D: Member +0x1B4: std_list_clear_nodes & delete head (m_listDisplayLayers)
 *   0x0056517E: Member +0x1A8: std_list_clear_nodes & delete head (m_listEffects)
 *   0x0056519D: Member +0x19C: std_map_uint32_string_destructor (m_mapStatusStrings)
 *   0x005651AE: Member +0x190: std_list_DisplayEntity_clear & delete head
 *   0x005651CD: Member +0x0F4: CWin32Control_EditBox_destructor (m_editBox)
 *   0x005651DE: Member +0x0A0: CCriticalSectionBS::~CCriticalSectionBS (m_csLock)
 *   0x005651ED: Global @ 0x00D6A980: mov dword ptr [g_pDisplayWindow], 0
 *   0x005651FB: Base class: CServerChildWindowBase_destructor(this)
 */
CDisplayWindow::~CDisplayWindow() {
	// Free heap-allocated display layer structures created in constructor
	for (tagDisplayLayer* pLayer : m_listLayers) {
		delete pLayer;
	}
	m_listLayers.clear();
	m_mapLayers.clear();

	// Free floating text effects
	for (tagDisplayEffect* pEffect : m_listEffects) {
		delete pEffect;
	}
	m_listEffects.clear();

	ReleaseGDI();

	// Native 0x005651ED: Unregister global singleton pointer
	if (s_pInstance == this) {
		s_pInstance = nullptr;
	}
}

// [RECONSTRUCTED - Native 0x00565410]
void CDisplayWindow::ReleaseGDI() {
#ifdef _WIN32
	if (m_hMemDC != nullptr) {
		::DeleteDC(m_hMemDC);
		m_hMemDC = nullptr;
	}
	if (m_hOldBitmap != nullptr) {
		::DeleteObject(m_hOldBitmap);
		m_hOldBitmap = nullptr;
	}
	if (m_hBitmap != nullptr) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
#endif
}

tagDisplayLayer* CDisplayWindow::RegisterLayer(const char* szName, uint8_t byType, uint8_t bEnabled, PFN_RENDER_LAYER pfnRender) {
	if (!szName) return nullptr;

	auto* pLayer = new tagDisplayLayer();
	std::memset(pLayer, 0, sizeof(tagDisplayLayer));
	std::strncpy(pLayer->szName, szName, sizeof(pLayer->szName) - 1);
	pLayer->byType = byType;
	pLayer->bEnabled = bEnabled;
	pLayer->bSelected = 0;
	pLayer->pfnRender = pfnRender;
	pLayer->pUserData = nullptr;

	m_listLayers.push_back(pLayer);
	m_mapLayers[szName] = pLayer;
	return pLayer;
}

tagDisplayLayer* CDisplayWindow::FindLayer(const char* szName) {
	if (!szName) return nullptr;
	auto it = m_mapLayers.find(szName);
	if (it != m_mapLayers.end()) {
		return it->second;
	}
	return nullptr;
}

void CDisplayWindow::SetLayerEnabled(const char* szName, bool bEnabled) {
	tagDisplayLayer* pLayer = FindLayer(szName);
	if (pLayer != nullptr) {
		pLayer->bEnabled = bEnabled ? 1 : 0;
	}
}

/**
 * [NATIVE - 0x005657E0]
 * CDisplayWindow::OnMouseWheel
 * Native implementation @ 0x005657E0 (542 bytes)
 *
 * Handles mouse wheel zoom and viewport focal-point recalibration:
 *   - Verifies window visibility
 *   - Acquires critical section lock (+0xA0)
 *   - Computes viewport center in world coordinates
 *   - Adjusts zoom step (clamped between -10 and +30)
 *   - Recalculates viewport metrics
 *   - Keeps the world center under the cursor stable
 */
int32_t CDisplayWindow::OnMouseWheel(WPARAM wParam, LPARAM /*lParam*/) {
#ifdef _WIN32
	if (m_hWnd != nullptr && !IsWindowVisible(static_cast<HWND>(m_hWnd))) {
		return 0;
	}
#endif

	m_csLock.Lock();

	// Calculate current world center
	float fHalfWidth = static_cast<float>(m_nClientWidth) * 0.5f;
	float fHalfHeight = static_cast<float>(m_nClientHeight) * 0.5f;

	float fCenterX = (m_fZoomScale > 0.0001f) ? (fHalfWidth / m_fZoomScale + m_fOriginWorldX) : m_fOriginWorldX;
	float fCenterY = (m_fZoomScale > 0.0001f) ? (fHalfHeight / m_fZoomScale + m_fOriginWorldY) : m_fOriginWorldY;

	// Decompose relative to sector size (1920.0f native units)
	constexpr float SECTOR_SIZE = 1920.0f;
	uint8_t bySecX = static_cast<uint8_t>(fCenterX / SECTOR_SIZE);
	uint8_t bySecY = static_cast<uint8_t>(fCenterY / SECTOR_SIZE);

	float fSecBaseX = static_cast<float>(bySecX) * SECTOR_SIZE;
	float fSecBaseY = static_cast<float>(bySecY) * SECTOR_SIZE;

	float fRelX = fCenterX - fSecBaseX;
	float fRelY = fCenterY - fSecBaseY;

	// Extract wheel delta
	int16_t zDelta = static_cast<int16_t>((wParam >> 16) & 0xFFFF);
	if (zDelta > 0) {
		m_nZoomStep--;
	} else if (zDelta < 0) {
		m_nZoomStep++;
	}

	// Clamp zoom step between -10 (0xFFFFFFF6) and +30 (0x1E)
	if (m_nZoomStep < -10) {
		m_nZoomStep = -10;
	} else if (m_nZoomStep > 30) {
		m_nZoomStep = 30;
	}

	UpdateViewportMetrics();

	// Recalibrate world origin to maintain focal point
	if (m_fZoomScale > 0.0001f) {
		m_fOriginWorldX = (fRelX + fSecBaseX) - (fHalfWidth / m_fZoomScale);
		m_fOriginWorldY = (fRelY + fSecBaseY) - (fHalfHeight / m_fZoomScale);
	}

	m_csLock.Unlock();
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00566050]
 * CDisplayWindow::WorldToScreen
 * Native implementation @ 0x00566050 (102 bytes)
 *
 * Converts world coordinates (worldX, worldY) to screen pixel coordinates.
 * Note: In Silkroad's world canvas, Y is inverted (screen Y = height - relY - 1.0f).
 *
 * Machine Disassembly:
 *   00566050  8b442404             mov     eax, [esp+4]          ; eax = this
 *   00566054  d9442410             fld     dword [esp+0x10]      ; fWorldY
 *   00566058  8b4c2408             mov     ecx, [esp+8]          ; pOutScreen
 *   0056605c  da6070               fisub   dword [eax+0x170]     ; fWorldY - m_fOriginWorldY
 *   0056605f  d88884010000         fmul    dword [eax+0x184]     ; relY * m_fZoomScale
 *   00566065  db809c000000         fild    dword [eax+0x9c]      ; m_nClientHeight
 *   0056606b  dec9                 fsubrp  st1, st0              ; m_nClientHeight - relY
 *   0056606d  d9e8                 fld1                          ; 1.0f
 *   0056606f  dec9                 fsubrp  st1, st0              ; m_nClientHeight - relY - 1.0f
 *   00566071  d944240c             fld     dword [esp+0xc]       ; fWorldX
 *   00566075  da606c               fisub   dword [eax+0x16c]     ; fWorldX - m_fOriginWorldX
 *   00566078  d88884010000         fmul    dword [eax+0x184]     ; relX * m_fZoomScale
 *   0056607e  e8bd5af9ff           call    CRT_ftol              ; pOutScreen->x = (LONG)relX
 *   00566083  8901                 mov     [ecx], eax
 *   00566085  e8b65af9ff           call    CRT_ftol              ; pOutScreen->y = (LONG)screenY
 *   0056608a  894104               mov     [ecx+4], eax
 *   0056608d  c21000               retn    0x10
 */
BOOL CDisplayWindow::WorldToScreen(POINT* pOutScreen, float fWorldX, float fWorldY) const {
	if (!pOutScreen) return FALSE;
	float fRelX = (fWorldX - m_fOriginWorldX) * m_fZoomScale;
	float fRelY = (fWorldY - m_fOriginWorldY) * m_fZoomScale;
	float fScreenY = static_cast<float>(m_nClientHeight) - fRelY - 1.0f;
	pOutScreen->x = static_cast<LONG>(fRelX);
	pOutScreen->y = static_cast<LONG>(fScreenY);
	return (pOutScreen->x >= 0 && pOutScreen->x < static_cast<LONG>(m_nClientWidth) &&
	        pOutScreen->y >= 0 && pOutScreen->y < static_cast<LONG>(m_nClientHeight));
}

/**
 * [RECONSTRUCTED - 0x00952530]
 * GDI_DrawHollowEllipse
 * Native implementation @ 0x00952530 (145 bytes)
 */
BOOL CDisplayWindow::GDI_DrawHollowEllipse(HDC hdc, int x, int y, int width, int height, COLORREF color) {
#ifdef _WIN32
	if (!hdc) return FALSE;
	HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetStockObject(HOLLOW_BRUSH));
	HPEN hPen = ::CreatePen(PS_SOLID, 1, color);
	HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);

	BOOL bRes = ::Ellipse(hdc, x, y, x + width, y + height);

	::SelectObject(hdc, hOldPen);
	::DeleteObject(hPen);
	::SelectObject(hdc, hOldBrush);
	return bRes;
#else
	(void)hdc; (void)x; (void)y; (void)width; (void)height; (void)color;
	return TRUE;
#endif
}

/**
 * [RECONSTRUCTED - 0x009523F0]
 * GDI_DrawLine
 * Native implementation @ 0x009523F0 (132 bytes)
 */
BOOL CDisplayWindow::GDI_DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color, int fnPenStyle) {
	return ServerFramework::GDI_DrawLine(hdc, x1, y1, x2, y2, color, fnPenStyle);
}

/**
 * [RECONSTRUCTED - 0x00952480]
 * GDI_DrawFilledEllipse
 * Native implementation @ 0x00952480 (161 bytes)
 */
BOOL CDisplayWindow::GDI_DrawFilledEllipse(HDC hdc, int x, int y, int width, int height, COLORREF brushColor, COLORREF penColor) {
	return ServerFramework::GDI_DrawFilledEllipse(hdc, x, y, width, height, brushColor, penColor);
}

/**
 * [RECONSTRUCTED - 0x00565220 / 0x00565410]
 * CDisplayWindow::Create
 */
bool CDisplayWindow::Create(HWND hWndParent) {
#ifdef _WIN32
	HINSTANCE hInstance = GetModuleHandleA(nullptr);

	// Register window class "GameServerDisplayWindow"
	WNDCLASSA wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = DefWindowProcA;
	wc.hInstance     = hInstance;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = "GameServerDisplayWindow";
	RegisterClassA(&wc);

	m_hWnd = CreateWindowExA(WS_EX_CLIENTEDGE, "GameServerDisplayWindow", nullptr,
	                         WS_CHILD | WS_VISIBLE,
	                         0, 0, 400, 400, hWndParent,
	                         reinterpret_cast<HMENU>(static_cast<uintptr_t>(0x200)),
	                         hInstance, nullptr);
	if (!m_hWnd) {
		std::printf("[CDisplayWindow] CreateWindowExA failed: error=%lu\n", GetLastError());
		return false;
	}
#endif

	std::printf("[CDisplayWindow] Created GameServerDisplayWindow (parent=%p)\n", (void*)hWndParent);
	return true;
}

// ============================================================================
// Layer Render Callbacks
// ============================================================================

/**
 * [NATIVE - 0x00566B10]
 * CDisplayWindow::RenderTerrain
 * Native implementation @ 0x00566B10 (1,646 bytes)
 *
 * Renders 2D heightmap, region boundaries, and sector cell grid lines:
 *   - Iterates visible active regions
 *   - Computes region bounding boxes (1920.0f x 1920.0f units per region)
 *   - Projects bounds to screen pixels via WorldToScreen
 *   - Performs viewport culling against client width/height
 *   - Draws region outline:
 *       RGB(128, 128, 128) if inactive, RGB(255, 255, 255) if active
 *   - If m_fZoom >= 100.0f and detail enabled:
 *       Draws 6x6 cell grid inside region (each cell is 320.0f x 320.0f units)
 *   - Displays Region ID hex string (e.g. "0x7000") in the top-left corner
 */
void CDisplayWindow::RenderTerrain(tagDisplayLayer* pLayer) {
	if (!pLayer) return;

#ifdef _WIN32
	HDC hdc = static_cast<HDC>(m_hMemDC);
	if (!hdc) return;

	constexpr float SECTOR_SIZE = 1920.0f;
	constexpr float CELL_SIZE = 320.0f; // 1920.0f / 6.0f

	// Render region boundary lines
	COLORREF clrRegionBorder = RGB(128, 128, 128);
	COLORREF clrCellBorder   = RGB(64, 64, 64);
	COLORREF clrText         = RGB(192, 192, 192);

	SetTextColor(hdc, clrText);
	SetBkMode(hdc, TRANSPARENT);

	// Compute visible region range from viewport origin and size
	float fMinWorldX = m_fOriginWorldX;
	float fMinWorldY = m_fOriginWorldY;
	float fMaxWorldX = (m_fZoomScale > 0.0001f) ? (m_fOriginWorldX + static_cast<float>(m_nClientWidth) / m_fZoomScale) : (m_fOriginWorldX + 1920.0f);
	float fMaxWorldY = (m_fZoomScale > 0.0001f) ? (m_fOriginWorldY + static_cast<float>(m_nClientHeight) / m_fZoomScale) : (m_fOriginWorldY + 1920.0f);

	int32_t nMinSecX = static_cast<int32_t>(fMinWorldX / SECTOR_SIZE) - 1;
	int32_t nMaxSecX = static_cast<int32_t>(fMaxWorldX / SECTOR_SIZE) + 1;
	int32_t nMinSecY = static_cast<int32_t>(fMinWorldY / SECTOR_SIZE) - 1;
	int32_t nMaxSecY = static_cast<int32_t>(fMaxWorldY / SECTOR_SIZE) + 1;

	if (nMinSecX < 0) nMinSecX = 0;
	if (nMinSecY < 0) nMinSecY = 0;
	if (nMaxSecX > 255) nMaxSecX = 255;
	if (nMaxSecY > 255) nMaxSecY = 255;

	for (int32_t sy = nMinSecY; sy <= nMaxSecY; ++sy) {
		for (int32_t sx = nMinSecX; sx <= nMaxSecX; ++sx) {
			float fWorldX1 = static_cast<float>(sx) * SECTOR_SIZE;
			float fWorldY1 = static_cast<float>(sy) * SECTOR_SIZE;
			float fWorldX2 = fWorldX1 + SECTOR_SIZE;
			float fWorldY2 = fWorldY1 + SECTOR_SIZE;

			POINT ptTopLeft = {};
			POINT ptBottomRight = {};
			WorldToScreen(&ptTopLeft, fWorldX1, fWorldY2);     // Note Y inverted
			WorldToScreen(&ptBottomRight, fWorldX2, fWorldY1);

			// Viewport culling
			if (ptBottomRight.x < 0 || ptTopLeft.x > static_cast<LONG>(m_nClientWidth) ||
			    ptBottomRight.y < 0 || ptTopLeft.y > static_cast<LONG>(m_nClientHeight)) {
				continue;
			}

			// Draw region outer rectangle
			GDI_DrawLine(hdc, ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptTopLeft.y, clrRegionBorder, PS_SOLID);
			GDI_DrawLine(hdc, ptBottomRight.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y, clrRegionBorder, PS_SOLID);
			GDI_DrawLine(hdc, ptBottomRight.x, ptBottomRight.y, ptTopLeft.x, ptBottomRight.y, clrRegionBorder, PS_SOLID);
			GDI_DrawLine(hdc, ptTopLeft.x, ptBottomRight.y, ptTopLeft.x, ptTopLeft.y, clrRegionBorder, PS_SOLID);

			// If zoomed in (m_fZoom >= 100.0f), draw 6x6 cell grid
			if (m_fZoom >= 100.0f) {
				for (int32_t c = 1; c < 6; ++c) {
					float fCellX = fWorldX1 + static_cast<float>(c) * CELL_SIZE;
					float fCellY = fWorldY1 + static_cast<float>(c) * CELL_SIZE;

					POINT ptV1, ptV2, ptH1, ptH2;
					WorldToScreen(&ptV1, fCellX, fWorldY2);
					WorldToScreen(&ptV2, fCellX, fWorldY1);
					WorldToScreen(&ptH1, fWorldX1, fCellY);
					WorldToScreen(&ptH2, fWorldX2, fCellY);

					GDI_DrawLine(hdc, ptV1.x, ptV1.y, ptV2.x, ptV2.y, clrCellBorder, PS_DOT);
					GDI_DrawLine(hdc, ptH1.x, ptH1.y, ptH2.x, ptH2.y, clrCellBorder, PS_DOT);
				}
			}

			// Region ID label in hex
			uint16_t wRegionID = static_cast<uint16_t>((sy << 8) | sx);
			char szLabel[32];
			std::snprintf(szLabel, sizeof(szLabel), "0x%04X", wRegionID);
			TextOutA(hdc, ptTopLeft.x + 4, ptTopLeft.y + 4, szLabel, static_cast<int>(std::strlen(szLabel)));
		}
	}
#endif
}

// ============================================================================
// Silkroad Debug String Tables for Character Rendering (Native 0x00C63F80 - 0x00C6405C)
// ============================================================================

static const char* s_szMotionState[4] = {
	"MOTIONSTATE_STAND", // 0x00C63F90 -> 0x00AFA1BC
	"MOTIONSTATE_SKILL", // 0x00C63F94 -> 0x00AFA1A8
	"MOTIONSTATE_WALK",  // 0x00C63F98 -> 0x00AFA194
	"MOTIONSTATE_RUN"    // 0x00C63F9C -> 0x00AFA184
};

static const char* s_szLifeState[4] = {
	"LIFESTATE_EMBRYO", // 0x00C63F80 -> 0x00AFA200
	"LIFESTATE_ALIVE",  // 0x00C63F84 -> 0x00AFA1F0
	"LIFESTATE_DEAD",   // 0x00C63F88 -> 0x00AFA1E0
	"LIFESTATE_GONE"    // 0x00C63F8C -> 0x00AFA1D0
};

static const char* s_szBattleState[2] = {
	"BATTLESTATE_IN_PEACE",  // 0x00C64058 -> 0x00AF9E18
	"BATTLESTATE_IN_BATTLE"  // 0x00C6405C -> 0x00AF9E00
};

static const char* s_szPvPState[4] = {
	"PvPSTATE_NEUTRAL",   // 0x00C6404C -> 0x00AF9E58
	"PvPSTATE_ASSAULTER", // 0x00C64050 -> 0x00AF9E44
	"PvPSTATE_MURDERER",  // 0x00C64054 -> 0x00AF9E30
	"BATTLESTATE_IN_PEACE"
};

static const char* s_szMsgProcState[4] = {
	"MSG_PROCSTATE_NORMAL",     // 0x00C64040 -> 0x00AF9EA0
	"MSG_PROCSTATE_BLOCKED",    // 0x00C64044 -> 0x00AF9E88
	"MSG_PROCSTATE_OVERLAPPED", // 0x00C64048 -> 0x00AF9E6C
	"PvPSTATE_NEUTRAL"
};

static const char* s_szInteractMode[10] = {
	"INTERACTMODE_NONE",             // 0x00C63FE4 -> 0x00AFA018
	"INTERACTMODE_HANDSHAKING",      // 0x00C63FE8 -> 0x00AF9FFC
	"INTERACTMODE_P2P",              // 0x00C63FEC -> 0x00AF9FE8
	"INTERACTMODE_MARKET_OWNER",     // 0x00C63FF0 -> 0x00AF9FCC
	"INTERACTMODE_P2N_TALK",         // 0x00C63FF4 -> 0x00AF9FB4
	"INTERACTMODE_P2N_DEAL",         // 0x00C63FF8 -> 0x00AF9F9C
	"OBJ_INTERACTMODE_OPNMKT_DEAL",  // 0x00C63FFC -> 0x00AF9F7C
	"", "", ""
};

static const char* s_szGroupMode[5] = {
	"GROUPMODE_SOLO",             // 0x00C6400C -> 0x00AF9F6C
	"GROUPMODE_PARTY_SETUP",      // 0x00C64010 -> 0x00AF9F54
	"GROUPMODE_PARTY",            // 0x00C64014 -> 0x00AF9F44
	"GROUPMODE_SOLOPARTY_SETUP",  // 0x00C64018 -> 0x00AF9F28
	"GROUPMODE_SOLOPARTY"         // 0x00C6401C -> 0x00AF9F14
};

static const char* s_szBodyMode[8] = {
	"BODYMODE_NORMAL",     // 0x00C64020 -> 0x00AF9F04
	"BODYMODE_HWAN",       // 0x00C64024 -> 0x00AF9EF4
	"BODYMODE_INVINCIBLE", // 0x00C64028 -> 0x00AF9EE0
	"BODYMODE_INVISIBLE",  // 0x00C6402C -> 0x00AF9ECC
	"BODYMODE_BERSERKER",  // 0x00C64030 -> 0x00AF9EB8
	"", "", ""
};

/**
 * [RECONSTRUCTED - 0x00567190]
 * CDisplayWindow::RenderCharacter
 * Native implementation @ 0x00567190 (2,187 bytes)
 *
 * Visual debugging overlay for all character entities:
 *   1. Iterates intrusive doubly linked list (g_pCharacterListHead @ 0x00C825F8)
 *   2. Filters entities by draw options:
 *        - m_bDrawPlayer   (+0x1CC) -> pwOut->IsPlayer()
 *        - m_bDrawNPC      (+0x1CD) -> pwOut->IsNPC()
 *        - m_bDrawMonster  (+0x1CE) -> pwOut->IsMonster()
 *        - m_bDrawCOS      (+0x1CF) -> pwOut->IsCOS()
 *   3. Computes world coordinates using Silkroad 1920.0f region scale:
 *        fWorldX = (float)pwOut->m_byCellX * 1920.0f + pwOut->m_fLocalPosX;
 *        fWorldZ = (float)pwOut->m_byCellZ * 1920.0f + pwOut->m_fLocalPosZ;
 *        WorldToScreen(&ptScreen, fWorldX, fWorldZ);
 *   4. Viewport clipping:
 *        if (ptScreen.x + 1 < 0 || ptScreen.x > m_nClientWidth) continue;
 *        if (ptScreen.y + 1 < 0 || ptScreen.y > m_nClientHeight) continue;
 *   5. Draws 5x5 colored circle:
 *        Player: Blue (0x00FF0000), NPC: Red (0x000000FF),
 *        Monster: Green (0x0000FF00), COS/Pet: Gray (0x00323232)
 *   6. If pLayer->bSelected >= 1:
 *        - Name & GID: "^5%s(^8%08X^5)" or "^5%s"
 *        - Navigation: "^5(^6%d^5)%.2f,%.2f,%.2f" + speed/mode/dest
 *        - State: "^%d%d/%d,%d/%d^5,%s,%s" + player mode info
 */
void CDisplayWindow::RenderCharacter(tagDisplayLayer* pLayer) {
	g_pCharacterListCurrent = g_pCharacterListHead;
	g_dwCharacterListIterFlags &= ~1;

	if (!g_pCharacterListCurrent) {
		return;
	}

	while (g_pCharacterListCurrent != nullptr) {
		// Native @ 0x005671B9 - 0x005671C3: Calculate containing CGObjChar from intrusive list node (sub edi, 0x178)
		CGObjChar* pChar = reinterpret_cast<CGObjChar*>(
			reinterpret_cast<uint8_t*>(g_pCharacterListCurrent) - 0x178
		);

		// Filter out entity categories based on configuration flags
		if (!m_bDrawPlayer && pChar->IsPlayer()) {
			goto next_char;
		}
		if (!m_bDrawNPC && pChar->IsNPC()) {
			goto next_char;
		}
		if (!m_bDrawMonster && pChar->IsMonster()) {
			goto next_char;
		}
		if (!m_bDrawCOS && pChar->IsCOS()) {
			goto next_char;
		}

		{
			// Native @ 0x00567233 - 0x00567299: Calculate world coordinates
			// Silkroad region grid: 1 region = 1920.0f coordinate units
			float fWorldX = static_cast<float>(pChar->m_wRegionID & 0xFF) * 1920.0f + pChar->m_fLocalPosX;
			float fWorldZ = static_cast<float>(pChar->m_wRegionID >> 8) * 1920.0f + pChar->m_fLocalPosZ;

			POINT ptScreen = { 0, 0 };
			WorldToScreen(&ptScreen, fWorldX, fWorldZ);

			// Frustum / viewport clipping (Native @ 0x0056729E - 0x005672CB)
			if (ptScreen.x + 1 < 0 || ptScreen.x > static_cast<LONG>(m_nClientWidth) ||
			    ptScreen.y + 1 < 0 || ptScreen.y > static_cast<LONG>(m_nClientHeight)) {
				goto next_char;
			}

#ifdef _WIN32
			SetBkMode(m_hMemDC, TRANSPARENT);
#endif

			// Determine marker color (Native @ 0x005672DF - 0x00567332)
			COLORREF crFill = RGB(50, 50, 50);
			if (pChar->IsPlayer()) {
				crFill = RGB(0, 0, 255); // Blue (0x00FF0000 in Win32 COLORREF)
			} else if (pChar->IsNPC()) {
				crFill = RGB(255, 0, 0); // Red (0x000000FF)
			} else if (pChar->IsMonster()) {
				crFill = RGB(0, 255, 0); // Green (0x0000FF00)
			} else if (pChar->IsCOS()) {
				crFill = RGB(50, 50, 50);
			}

			// Draw 5x5 centered marker circle (Native @ 0x00567336 - 0x00567365)
			ServerFramework::GDI_DrawFilledEllipse(
				m_hMemDC,
				static_cast<int>(ptScreen.x - 2.5f),
				static_cast<int>(ptScreen.y - 2.5f),
				5, 5,
				crFill,
				RGB(255, 255, 255)
			);

			// Check detail level (Native @ 0x00567370: cmp byte [eax+0x41], 1)
			if (pLayer && pLayer->bSelected >= 1) {
#ifdef _WIN32
				SetBkMode(m_hMemDC, TRANSPARENT);
				SetTextColor(m_hMemDC, RGB(255, 255, 255));
#endif
				int nTextYOffset = 0;

				// 1. Draw Character Name and/or Global ID (Native @ 0x0056739B - 0x0056742B)
				if (m_bDrawGID) {
					const char* pszName = m_bDrawCharName ? pChar->GetName() : "";
					ServerFramework::GDI_DrawColorCodedText(
						m_hMemDC,
						ptScreen.x,
						ptScreen.y,
						0,
						"^5%s(^8%08X^5)",
						pszName,
						pChar->m_dwGlobalID
					);
					nTextYOffset = 12;
				} else if (m_bDrawCharName) {
					ServerFramework::GDI_DrawColorCodedText(
						m_hMemDC,
						ptScreen.x,
						ptScreen.y,
						0,
						"^5%s",
						pChar->GetName()
					);
					nTextYOffset = 12;
				}

				// 2. Draw Navigation / Coordinates (Native @ 0x00567433 - 0x0056780C)
				if (m_bDrawNavigation) {
					char szNavBuffer[1024] = { 0 };
					std::snprintf(
						szNavBuffer,
						sizeof(szNavBuffer),
						"^5(^6%d^5)%.2f,%.2f,%.2f",
						pChar->m_wRegionID,
						pChar->m_fLocalPosX,
						pChar->m_fLocalPosY,
						pChar->m_fLocalPosZ
					);

					if (pChar->IsMoving()) {
						float fWalkSpeed = pChar->GetParamFloat(0x17);
						float fRunSpeed  = pChar->GetParamFloat(0x18);
						int   nWalkColor = (pChar->m_bySpeedMode != 0) ? 8 : 5;
						int   nRunColor  = (pChar->m_bySpeedMode != 1) ? 8 : 5;

						char szSpeedBuf[128];
						std::snprintf(
							szSpeedBuf,
							sizeof(szSpeedBuf),
							" ^%d%.2f,^%d%.2f",
							nRunColor, fRunSpeed,
							nWalkColor, fWalkSpeed
						);
						std::strncat(szNavBuffer, szSpeedBuf, sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);

						if (pChar->m_byMovementType == 0) {
							std::strncat(szNavBuffer, "^1 TYPE^5FC^1:", sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
							if (pChar->m_byMovementFlags & 0x01) {
								std::strncat(szNavBuffer, "^5GO_FORWARD^5:", sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
							}
							if (pChar->m_byMovementFlags & 0x02) {
								std::strncat(szNavBuffer, "^2GO_BACKWARD^5:", sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
							}
							if (pChar->m_byMovementFlags & 0x10) {
								std::strncat(szNavBuffer, "^6TURN_LEFT", sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
							}
							if (pChar->m_byMovementFlags & 0x20) {
								std::strncat(szNavBuffer, "^7TURN_RIGHT", sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
							}
						} else if (pChar->m_byMovementType == 1) {
							char szDestBuf[128];
							std::snprintf(
								szDestBuf,
								sizeof(szDestBuf),
								"^1 TYPE^5MD^1:^7(^6%d^7)%.2f,%.2f,%.2f",
								pChar->m_wDestRegionID,
								static_cast<float>(pChar->m_nDestPosX),
								static_cast<float>(pChar->m_nDestPosY),
								static_cast<float>(pChar->m_nDestPosZ)
							);
							std::strncat(szNavBuffer, szDestBuf, sizeof(szNavBuffer) - std::strlen(szNavBuffer) - 1);
						}
					}

					ServerFramework::GDI_DrawColorCodedText(
						m_hMemDC,
						ptScreen.x,
						ptScreen.y + nTextYOffset,
						0,
						"%s",
						szNavBuffer
					);
					nTextYOffset += 12;
				}

				// 3. Draw Character State (Native @ 0x00567812 - 0x005679D7)
				if (m_bDrawState) {
					char szStateBuffer[1024] = { 0 };
					const char* pszMotionState = s_szMotionState[pChar->GetMotionState() & 3];
					const char* pszLifeState   = s_szLifeState[pChar->GetLifeState() & 3];
					int nLifeColor = (pChar->GetLifeState() == 1) ? 5 : 8;

					std::snprintf(
						szStateBuffer,
						sizeof(szStateBuffer),
						"^%d%d/%d,%d/%d^5,%s,%s",
						nLifeColor,
						pChar->GetCurrentHP(),
						pChar->GetMaxHP(),
						pChar->GetCurrentMP(),
						pChar->GetMaxMP(),
						pszLifeState,
						pszMotionState
					);

					if (pChar->IsPlayer() && pChar->m_pCharData) {
						char szPlayerModeBuf[256];
						const char* pszBodyMode     = s_szBodyMode[pChar->GetBodyMode() & 7];
						const char* pszInteractMode = s_szInteractMode[pChar->m_pCharData->m_byInteractMode & 7];
						const char* pszGroupMode    = s_szGroupMode[pChar->m_pCharData->m_byGroupMode & 3];
						const char* pszMsgProcState = s_szMsgProcState[pChar->m_pCharData->m_byMsgProcState & 3];
						const char* pszPvPState     = s_szPvPState[pChar->m_pCharData->m_byPvPState & 3];
						const char* pszBattleState  = s_szBattleState[pChar->m_pCharData->m_byBattleState & 1];

						std::snprintf(
							szPlayerModeBuf,
							sizeof(szPlayerModeBuf),
							",%s,%s,%s,%s,%s,%s",
							pszBodyMode,
							pszInteractMode,
							pszGroupMode,
							pszMsgProcState,
							pszPvPState,
							pszBattleState
						);
						std::strncat(szStateBuffer, szPlayerModeBuf, sizeof(szStateBuffer) - std::strlen(szStateBuffer) - 1);
					}

					ServerFramework::GDI_DrawColorCodedText(
						m_hMemDC,
						ptScreen.x,
						ptScreen.y + nTextYOffset,
						0,
						"%s",
						szStateBuffer
					);
				}
			}
		}

next_char:
		// Safe iterator advancement (Native @ 0x005679DE - 0x00567A08)
		if (!(g_dwCharacterListIterFlags & 1)) {
			g_pCharacterListCurrent = g_pCharacterListCurrent->pNext;
		}
		g_dwCharacterListIterFlags &= ~1;
	}
}

/**
 * [NATIVE - 0x009BF500]
 * CDisplayWindow::RenderItem
 * Native implementation @ 0x009BF500 (3 bytes: retn 4 / UniversalNoOpStub_1Arg)
 * In Joymax's original release binary, item rendering on the 2D server debug monitor
 * was compiled as an empty no-op callback.
 */
void CDisplayWindow::RenderItem(tagDisplayLayer* pLayer) {
	(void)pLayer;
}

/**
 * [RECONSTRUCTED - 0x00566AA0]
 * CDisplayWindow::RenderMainStatus
 * Native implementation @ 0x00566AA0 (103 bytes)
 *
 * Renders the top-left performance & zoom status overlay:
 *   "Interval %dms, Last %dms, fZoom = %.2f"
 *
 * Machine Disassembly:
 *   00566aa0  55                   push    ebp
 *   00566aa1  8bec                 mov     ebp, esp
 *   00566aa3  83e4c0               and     esp, 0xffffffc0
 *   00566aa6  83ec3c               sub     esp, 0x3c
 *   00566aa9  56                   push    esi
 *   00566aaa  8bf1                 mov     esi, ecx              ; esi = this
 *   00566aac  8b868c000000         mov     eax, [esi+0x8c]       ; m_hMemDC
 *   00566ab2  68ffffff00           push    0xffffff              ; RGB(255, 255, 255)
 *   00566ab7  50                   push    eax
 *   00566ab8  ff151490ad00         call    SetTextColor
 *   00566abe  8b8e8c000000         mov     ecx, [esi+0x8c]       ; m_hMemDC
 *   00566ac4  6a01                 push    1                     ; TRANSPARENT = 1
 *   00566ac6  51                   push    ecx
 *   00566ac7  ff152090ad00         call    SetBkMode
 *   00566acd  d98668010000         fld     dword [esi+0x168]     ; m_fZoom
 *   00566ad3  8b96e4000000         mov     edx, [esi+0xe4]       ; m_dwLastRenderDuration
 *   00566ad9  8b86dc000000         mov     eax, [esi+0xdc]       ; m_dwDrawInterval
 *   00566adf  8bb68c000000         mov     esi, [esi+0x8c]       ; m_hMemDC (for GDI call)
 *   00566ae5  83ec08               sub     esp, 8
 *   00566ae8  dd1c24               fstp    qword [esp]           ; (double)m_fZoom
 *   00566aeb  52                   push    edx                   ; m_dwLastRenderDuration
 *   00566aec  50                   push    eax                   ; m_dwDrawInterval
 *   00566aed  6810a4af00           push    0xafa410              ; "Interval %dms, Last %dms, fZoom = %.2f"
 *   00566af2  6a00                 push    0                     ; TA_NOUPDATECP
 *   00566af4  6a00                 push    0                     ; y = 0
 *   00566af6  6a00                 push    0                     ; x = 0
 *   00566af8  e883b53e00           call    BSLib_GDI_DrawTextFormatted
 *   00566afd  83c420               add     esp, 0x20
 *   00566b00  5e                   pop     esi
 *   00566b01  8be5                 mov     esp, ebp
 *   00566b03  5d                   pop     ebp
 *   00566b04  c20400               retn    4
 */
void CDisplayWindow::RenderMainStatus(tagDisplayLayer* pLayer) {
	(void)pLayer;
#ifdef _WIN32
	if (m_hMemDC != nullptr) {
		::SetTextColor(static_cast<HDC>(m_hMemDC), RGB(255, 255, 255));
		::SetBkMode(static_cast<HDC>(m_hMemDC), TRANSPARENT);
		BSLib::GDI_DrawTextFormatted(
			static_cast<HDC>(m_hMemDC),
			0, 0, 0,
			"Interval %dms, Last %dms, fZoom = %.2f",
			m_dwDrawInterval,
			m_dwLastRenderDuration,
			static_cast<double>(m_fZoom)
		);
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x00567A20]
 * CDisplayWindow::RenderDrawAI
 * Native implementation @ 0x00567A20 (1,632 bytes)
 *
 * Renders monster AI debug visualization on the radar/map viewport:
 *   1. Patrol and path region circles (Green - 0x00FF00)
 *   2. Active AI monster radius circles (Aggro/Detection/Leash based on pLayer->bSelected):
 *      - Normal Monster: Red (0x0000FF)
 *      - Champion: Yellow (0x00FFFF)
 *      - Giant/Unique: Grey (0x9A9B9C)
 *   3. Moving navigation target waypoint vector (Dotted red line to target, white dot at target)
 *   4. Aggro/Threat target linking lines to players/entities (Dotted grey line, 0x667878)
 */
void CDisplayWindow::RenderDrawAI(tagDisplayLayer* pLayer) {
	if (!pLayer) return;

	// 1. Verify global AI manager instance @ 0x00D6A96C
	if (!g_pGameAI) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}

#ifdef _WIN32
	HDC hdc = static_cast<HDC>(m_hMemDC);
	if (!hdc) return;

	const float fSectorSize = 1920.0f; // Native float constant @ 0x00B45AD0

	// ========================================================================
	// PASS 1: Render Patrol / Path Regions (0x00567A47 - 0x00567ADC)
	// ========================================================================
	for (const auto& pair : g_pGameAI->m_mapPatrolRegions) {
		tagAIPatrolRegion* pRegion = pair.second;
		if (!pRegion) continue;

		for (const auto& hivePair : *pRegion) {
			AI::CAIHive* pHive = hivePair.second;
			if (!pHive) continue;

			// CORRECTION (Claude): native iterates CAIHive::m_vecNest ([hive+0x20]..[hive+0x24] @ 0x00567A8C) and
			// draws each CNest::m_Pos (+0x10) with radius tagRefNest::m_nRadius (+0x20) * zoom (0x00567BD2). CAIHive
			// has no entity list; the previous m_vecEntities loop had no native counterpart.
			for (AI::CNest* pNest : pHive->m_vecNest) {
				if (!pNest) continue;

				uint8_t bySectorX = static_cast<uint8_t>(pNest->m_Pos.wRegionID & 0xFF);
				uint8_t bySectorY = static_cast<uint8_t>((pNest->m_Pos.wRegionID >> 8) & 0xFF);

				float fWorldX = fSectorSize * static_cast<float>(bySectorX) + pNest->m_Pos.fPosX;
				float fWorldY = fSectorSize * static_cast<float>(bySectorY) + pNest->m_Pos.fPosZ;

				POINT ptScreen = {};
				WorldToScreen(&ptScreen, fWorldX, fWorldY);

				float fRadius = static_cast<float>(pNest->m_pRefNest->m_nRadius);
				int32_t nRadiusScreen = static_cast<int32_t>(fRadius * m_fZoomScale);
				int32_t nDiameter = nRadiusScreen * 2;

				int32_t nLeft = ptScreen.x - nRadiusScreen;
				int32_t nTop = ptScreen.y - nRadiusScreen;

				// Viewport bounds check
				if (nLeft <= static_cast<int32_t>(m_nClientWidth) && (nLeft + nDiameter) >= 0 &&
				    nTop <= static_cast<int32_t>(m_nClientHeight) && (nTop + nDiameter) >= 0) {
					GDI_DrawHollowEllipse(hdc, nLeft, nTop, nDiameter, nDiameter, RGB(0, 255, 0));
				}
			}
		}
	}

	// ========================================================================
	// PASS 2: Render Active AI Entities / Monsters (0x00567ADC - 0x00568078)
	// ========================================================================
	for (const auto& pair : g_pGameAI->m_mapActiveAI) {
		AI::CTactics* pTactics = pair.second;
		if (!pTactics) continue;

		CGObjChar* pChar = pTactics->m_pOwner;
		if (!pChar) continue;

		uint8_t bySectorX = static_cast<uint8_t>(pChar->m_wSectorID & 0xFF);
		uint8_t bySectorY = static_cast<uint8_t>((pChar->m_wSectorID >> 8) & 0xFF);

		float fWorldX = fSectorSize * static_cast<float>(bySectorX) + pChar->m_fPosX;
		float fWorldY = fSectorSize * static_cast<float>(bySectorY) + pChar->m_fPosY;

		POINT ptMonster = {};
		WorldToScreen(&ptMonster, fWorldX, fWorldY);

		// Viewport bounds check
		if (ptMonster.x <= static_cast<int32_t>(m_nClientWidth) && (ptMonster.x + 1) >= 0 &&
		    ptMonster.y <= static_cast<int32_t>(m_nClientHeight) && (ptMonster.y + 1) >= 0) {

			// Select radius based on pLayer->bSelected (+0x41)
			float fRadius = 0.0f;
			if (pLayer->bSelected == 1) {
				fRadius = pTactics->m_fSearchRadius; // Mode 1: Search / Detection Radius
			} else if (pLayer->bSelected == 2) {
				fRadius = pTactics->m_fLeashRadius;  // Mode 2: Leash / Roam Max Radius
			} else {
				fRadius = pTactics->m_fAggroRadius;  // Mode 0: Default Aggro Radius
			}

			float fRadiusScreen = fRadius * m_fZoomScale;
			COLORREF color = 0x0000FF; // Default Red (RGB(255, 0, 0))

			// Determine monster status color via virtual methods:
			// Slot 10 (+0x28): IsNPC(), Slot 75 (+0x12C): GetMonsterClass()
			if (pChar->IsNPC()) {
				uint8_t byClass = pChar->GetMonsterClass();
				if (byClass != 0) {
					if (byClass == 1) {
						color = 0x00FFFF; // Champion: Yellow (RGB(255, 255, 0))
					} else {
						color = 0x9A9B9C; // Giant/Unique/Other: Grey (RGB(156, 155, 154))
					}
				}
			}

			int32_t nDiameter = static_cast<int32_t>(fRadiusScreen * 2.0f);
			int32_t nLeft = static_cast<int32_t>(static_cast<float>(ptMonster.x) - fRadiusScreen);
			int32_t nTop = static_cast<int32_t>(static_cast<float>(ptMonster.y) - fRadiusScreen);

			GDI_DrawHollowEllipse(hdc, nLeft, nTop, nDiameter, nDiameter, color);

			// ----------------------------------------------------------------
			// Navigation Waypoint Target Vector
			// Slot 304 (+0x4C0): IsMoving()
			// ----------------------------------------------------------------
			if (pChar->IsMoving()) {
				uint8_t byTargetSectorX = static_cast<uint8_t>(pChar->m_wTargetSectorID & 0xFF);
				uint8_t byTargetSectorY = static_cast<uint8_t>((pChar->m_wTargetSectorID >> 8) & 0xFF);

				// CORRECTION (Claude): the destination is stored as three int32 (the native fild's them at
				// 0x0048C362), so it was read through a float union that never held a float.
				float fTargetWorldX = fSectorSize * static_cast<float>(byTargetSectorX)
					+ static_cast<float>(pChar->m_nDestPosX);
				float fTargetWorldY = fSectorSize * static_cast<float>(byTargetSectorY)
					+ static_cast<float>(pChar->m_nDestPosZ);

				POINT ptTarget = {};
				WorldToScreen(&ptTarget, fTargetWorldX, fTargetWorldY);

				// Dotted red line from monster to destination waypoint
				GDI_DrawLine(hdc, ptMonster.x, ptMonster.y, ptTarget.x, ptTarget.y, 0x0000FF, PS_DOT);

				// 2x2 White filled dot at destination waypoint
				GDI_DrawFilledEllipse(hdc, ptTarget.x - 1, ptTarget.y - 1, 2, 2, 0xFFFFFF, 0xFFFFFF);
			}

			// ----------------------------------------------------------------
			// Aggro / Threat Target Linking Lines
			// ----------------------------------------------------------------
			for (uint32_t dwTargetID : pTactics->m_vecAggroTargets) {
				CGObjChar* pTarget = ObjMgr_FindByID(dwTargetID);
				if (pTarget != nullptr) {
					uint8_t byTgtSecX = static_cast<uint8_t>(pTarget->m_wSectorID & 0xFF);
					uint8_t byTgtSecY = static_cast<uint8_t>((pTarget->m_wSectorID >> 8) & 0xFF);

					float fTgtX = fSectorSize * static_cast<float>(byTgtSecX) + pTarget->m_fPosX;
					float fTgtY = fSectorSize * static_cast<float>(byTgtSecY) + pTarget->m_fPosY;

					POINT ptTgtScreen = {};
					WorldToScreen(&ptTgtScreen, fTgtX, fTgtY);

					// Dotted line connecting monster to threat target
					GDI_DrawLine(hdc, ptMonster.x, ptMonster.y, ptTgtScreen.x, ptTgtScreen.y, 0x667878, PS_DOT);
				}
			}
		}
	}
#else
	(void)pLayer;
#endif
}

// [RECONSTRUCTED - 0x005666F0]
void CDisplayWindow::ProcessPendingCommands() {
	while (!m_listCommands.empty()) {
		std::string strCommand = m_listCommands.front();
		m_listCommands.pop_front();
		ExecuteCommand(strCommand);
	}
}

// [RECONSTRUCTED - 0x00566830]
void CDisplayWindow::ExecuteCommand(const std::string& strCommand) {
	if (strCommand.empty()) return;
	BSLib::Log_Printf(0x1000000, "DisplayWindow command executed: %s", strCommand.c_str());
}

// [RECONSTRUCTED - 0x00565DE0]
void CDisplayWindow::UpdateViewportMetrics() {
	if (m_nZoomStep == 0) {
		m_fZoom = 1.0f;
	} else if (m_nZoomStep < 0) {
		m_fZoom = 1.0f / static_cast<float>(1 - m_nZoomStep);
	} else {
		m_fZoom = static_cast<float>(m_nZoomStep + 1);
	}

	float fZoomFactor = m_fZoom * 200.0f;
	m_fZoomScale = fZoomFactor / 1920.0f;
}

// [RECONSTRUCTED - 0x00568190]
void CDisplayWindow::RenderFloatingTexts(uint32_t dwElapsedMs) {
#ifdef _WIN32
	if (m_hMemDC == nullptr) return;

	float fProgressDelta = static_cast<float>(dwElapsedMs) / 3000.0f;
	auto it = m_listEffects.begin();
	while (it != m_listEffects.end()) {
		tagDisplayEffect* pEffect = *it;
		if (!pEffect) {
			it = m_listEffects.erase(it);
			continue;
		}

		if (pEffect->fProgress < 1.0f) {
			POINT ptScreen = { 0, 0 };
			WorldToScreen(&ptScreen, pEffect->fWorldX, pEffect->fWorldY);

			int32_t nDrawY = ptScreen.y - static_cast<int32_t>(pEffect->fProgress * 30.0f);
			int32_t nDrawX = ptScreen.x;

			pEffect->fProgress += fProgressDelta;

			if (nDrawX >= 0 && nDrawX <= static_cast<int32_t>(m_nClientWidth) &&
				nDrawY >= 0 && nDrawY <= static_cast<int32_t>(m_nClientHeight)) {
				::SetBkMode(m_hMemDC, TRANSPARENT);
				::SetTextColor(m_hMemDC, RGB(255, 255, 0));
				::TextOutA(m_hMemDC, nDrawX, nDrawY, pEffect->strText.c_str(), static_cast<int>(pEffect->strText.length()));
			}
			++it;
		} else {
			delete pEffect;
			it = m_listEffects.erase(it);
		}
	}
#else
	(void)dwElapsedMs;
#endif
}

// [RECONSTRUCTED - Native 0x00565C10]
void CDisplayWindow::RenderFrame() {
#ifdef _WIN32
	HWND hWndParent = ::GetParent(m_hWnd);
	HWND hWndForeground = ::GetForegroundWindow();
	if (hWndForeground != hWndParent && hWndForeground != m_hWnd) {
		return;
	}

	uint32_t dwNow = ::GetTickCount();
	m_dwLastRenderDuration = dwNow - m_dwLastTick;
	m_dwLastTick = dwNow;

	uint32_t dwElapsed = dwNow - m_dwLastDrawTick;
	if (dwElapsed < m_dwDrawInterval || m_nClientWidth <= 0 || m_nClientHeight <= 0) {
		return;
	}

	// Native 0x00565C9F: Scoped synchronization lock guard
	m_csLock.Lock();

	// Native 0x00565CB8: Process any pending console commands
	ProcessPendingCommands();

	// Native 0x00565CC9: Create and select Arial 9pt font
	HFONT hFont = ::CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HGDIOBJ hOldFont = ::SelectObject(m_hMemDC, hFont);

	// Native 0x00565CE1: Select double buffer bitmap into memory DC
	::SelectObject(m_hMemDC, m_hBitmap);

	// Native 0x00565D03: Clear viewport backbuffer with black
	RECT rcClient = { 0, 0, static_cast<LONG>(m_nClientWidth), static_cast<LONG>(m_nClientHeight) };
	HBRUSH hBlackBrush = ::CreateSolidBrush(RGB(0, 0, 0));
	::FillRect(m_hMemDC, &rcClient, hBlackBrush);
	::DeleteObject(hBlackBrush);

	// Native 0x00565D0C: Update camera zoom, scales, and world bounds
	UpdateViewportMetrics();

	// Native 0x00565D11 - 0x00565D57: Render all enabled display layers in sequential order
	for (tagDisplayLayer* pLayer : m_listLayers) {
		if (pLayer != nullptr && pLayer->bEnabled != 0 && pLayer->pfnRender != nullptr) {
			(this->*(pLayer->pfnRender))(pLayer);
		}
	}

	// Native 0x00565D6D: Render floating damage and status texts
	RenderFloatingTexts(dwElapsed);

	// Native 0x00565D74: Update draw timestamp
	m_dwLastDrawTick = ::GetTickCount();

	// Native 0x00565D81: Present backbuffer and invalidate window
	::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE);

	// Native 0x00565D8F: Restore original font and delete created font
	if (hOldFont != nullptr) {
		::SelectObject(m_hMemDC, hOldFont);
	}
	if (hFont != nullptr) {
		::DeleteObject(hFont);
	}

	// Native 0x00565DB7: Release synchronization lock
	m_csLock.Unlock();
#endif
}
