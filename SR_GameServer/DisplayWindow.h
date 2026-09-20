/**
 * ============================================================================
 * Silkroad Online - Game Server Display Window
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\DisplayWindow.h
 *
 * Implements the GUI display window for SR_GameServer:
 *   - VTable @ 0x00AFA5A4 (RTTI: .?AVCDisplayWindow@@)
 *   - Singleton: .?AV?$CSingletonT@VCDisplayWindow@@@@ (Global @ 0x00D6A980)
 *   - Window Class: "GameServerDisplayWindow"
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_DISPLAYWINDOW_H_
#define _SR_GAMESERVER_DISPLAYWINDOW_H_

#include "../JMX_ServerFramework/ServerFramework/ServerFrameWindow.h"
#include "../JMX_Library/BSLib/Synch.h"
#include <string>
#include <list>
#include <map>
#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HDC;
typedef void* HBITMAP;
#endif

class CDisplayWindow;
struct tagDisplayLayer;

// Layer rendering callback: native __thiscall member method of CDisplayWindow
typedef void (CDisplayWindow::*PFN_RENDER_LAYER)(tagDisplayLayer* pLayer);

/**
 * [RECONSTRUCTED - 0x00568190]
 * tagDisplayEffect
 * Visual floating text effect (damage numbers, status alerts)
 */
struct tagDisplayEffect {
	void*       pNext;       // +0x00
	uint8_t     bInUse;      // +0x04
	uint8_t     pad05[7];    // +0x05 - +0x0B
	std::string strText;     // +0x0C: Text to display (e.g. "-1250", "Level Up!")
	uint8_t     bySectorX;   // +0x2C: Sector X
	uint8_t     bySectorZ;   // +0x2D: Sector Z
	uint8_t     pad2E[2];    // +0x2E - +0x2F
	float       fWorldX;     // +0x30: World X coordinate
	uint8_t     pad34[4];    // +0x34 - +0x37
	float       fWorldY;     // +0x38: World Y coordinate
	float       fProgress;   // +0x3C: Normalized animation progress [0.0f, 1.0f]
};

/**
 * [RECONSTRUCTED - 0x005641A0]
 * tagDisplayLayer
 * Native allocation size: 0x50 bytes (80 bytes)
 *
 * Struct layout proven against machine bytes @ 0x005646EB - 0x005646F7:
 *   - +0x00: char szName[64]
 *   - +0x40: uint8_t bEnabled
 *   - +0x41: uint8_t bSelected
 *   - +0x42: uint8_t byType
 *   - +0x43: uint8_t pad[5]
 *   - +0x48: PFN_RENDER_LAYER pfnRender
 *   - +0x4C: void* pUserData
 */
struct tagDisplayLayer {
	char             szName[0x40];       // +0x00: Layer name (e.g. "Terrain", "Character", "Item", "MainStatus", "DrawAI")
	uint8_t          bEnabled;           // +0x40: Visibility / enabled flag
	uint8_t          bSelected;          // +0x41: Selected / display mode
	uint8_t          byType;             // +0x42: Layer type (2 for standard, 5 for AI)
	uint8_t          pad[5];             // +0x43: Padding to align function pointer
	PFN_RENDER_LAYER pfnRender;          // +0x48: Member function pointer to render callback
	void*            pUserData;          // +0x4C: Context pointer / member pointer delta
};

/**
 * [RECONSTRUCTED - 0x005641A0 / 0x00565110]
 * CDisplayWindow
 * Native VTable @ 0x00AFA5A4 (size 0x3C bytes)
 */
class CDisplayWindow : public ServerFramework::CServerChildWindowBase {
public:
	// Native @ 0x005641A0: Constructor (size 3,056 bytes)
	CDisplayWindow();

	// Native @ 0x00565110: Destructor
	virtual ~CDisplayWindow() override;

	// Native @ 0x00565220: Create(HWND hWndParent)
	bool Create(HWND hWndParent);

	// Native @ 0x00565410: ReleaseGDI (VTable Slot 2)
	virtual void ReleaseGDI() override;

	// [NATIVE - 0x005657E0]
	// CDisplayWindow::OnMouseWheel(WPARAM wParam, LPARAM lParam)
	int32_t OnMouseWheel(WPARAM wParam, LPARAM lParam);

	// Layer management helpers
	tagDisplayLayer* RegisterLayer(const char* szName, uint8_t byType, uint8_t bEnabled, PFN_RENDER_LAYER pfnRender);
	tagDisplayLayer* FindLayer(const char* szName);
	void SetLayerEnabled(const char* szName, bool bEnabled);

	// [RECONSTRUCTED - Native 0x00565C10]
	// Primary frame rendering tick called by CGame::Tick
	void RenderFrame();

	static CDisplayWindow* GetInstance();

	// [RECONSTRUCTED - 0x00566050]
	// Coordinate transformation: World coordinates to screen pixel coordinates
	BOOL WorldToScreen(POINT* pOutScreen, float fWorldX, float fWorldY) const;

	// GDI Drawing Helpers
	static BOOL GDI_DrawHollowEllipse(HDC hdc, int x, int y, int width, int height, COLORREF color);
	static BOOL GDI_DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color, int fnPenStyle);
	static BOOL GDI_DrawFilledEllipse(HDC hdc, int x, int y, int width, int height, COLORREF brushColor, COLORREF penColor);

	// Render callbacks (Native __thiscall member methods)
	// [NATIVE - 0x00566B10]
	void RenderTerrain(tagDisplayLayer* pLayer);
	// [RECONSTRUCTED - 0x00567190]
	void RenderCharacter(tagDisplayLayer* pLayer);
	// [NATIVE - 0x009BF500] (UniversalNoOpStub_1Arg)
	void RenderItem(tagDisplayLayer* pLayer);
	// [RECONSTRUCTED - 0x00566AA0]
	void RenderMainStatus(tagDisplayLayer* pLayer);
	// [NATIVE - 0x00567A20]
	void RenderDrawAI(tagDisplayLayer* pLayer);

	// [RECONSTRUCTED - 0x005666F0]
	// Processes queued console commands from the EditBox
	void ProcessPendingCommands();

	// [RECONSTRUCTED - 0x00566830]
	// Parses and executes a console command
	void ExecuteCommand(const std::string& strCommand);

	// [RECONSTRUCTED - 0x00565DE0]
	// Recalculates camera zoom, coordinate scales, and world bounds
	void UpdateViewportMetrics();

	// [RECONSTRUCTED - 0x00568190]
	// Updates and renders floating combat/status texts
	void RenderFloatingTexts(uint32_t dwElapsedMs);

private:
	// Native singleton pointer @ 0x00D6A980
	static CDisplayWindow* s_pInstance;

	HDC                       m_hMemDC;               // +0x8C: Memory DC for rendering
	HBITMAP                   m_hBitmap;              // +0x90: Memory bitmap
	HBITMAP                   m_hOldBitmap;           // +0x94: Previous bitmap handle
	uint32_t                  m_nClientWidth;         // +0x98: Client viewport width
	uint32_t                  m_nClientHeight;        // +0x9C: Client viewport height
	CCriticalSectionBS        m_csLock;               // +0xA0: Synchronization critical section
	uint32_t                  m_dwDrawInterval;       // +0xDC: Draw interval in ms (from "DrawInterval", default 100)
	uint32_t                  m_dwLastTick;           // +0xE0: Last frame tick timestamp
	uint32_t                  m_dwLastRenderDuration; // +0xE4: Elapsed render duration in ms
	uint32_t                  m_dwLastDrawTick;       // +0xE8: Last draw execution tick
	int32_t                   m_nZoomStep;            // +0x164: Mouse wheel zoom step
	float                     m_fZoom;                // +0x168: Viewport zoom level (default 1.0f)
	float                     m_fOriginWorldX;        // +0x16C: Camera viewport world origin X
	float                     m_fOriginWorldY;        // +0x170: Camera viewport world origin Y
	bool                      m_bDragging;            // +0x17C: Mouse drag flag
	float                     m_fZoomScale;           // +0x184: Combined world-to-screen zoom scale
	float                     m_fScaleX;              // +0x188: Viewport scale X
	float                     m_fScaleY;              // +0x18C: Viewport scale Y

	std::list<std::string>                  m_listCommands; // +0x190: Queued console commands
	std::list<tagDisplayEffect*>            m_listEffects;  // +0x1A8: Floating text effects
	std::list<tagDisplayLayer*>             m_listLayers;   // +0x1B0: Sequential render list
	std::map<std::string, tagDisplayLayer*> m_mapLayers;    // +0x1C0: Name lookup map

	// Character rendering options (+0x1CC - +0x1D3)
	bool m_bDrawPlayer;       // +0x1CC: CharDrawOption_Player (default true)
	bool m_bDrawMonster;      // +0x1CD: CharDrawOption_Monster (default true)
	bool m_bDrawNPC;          // +0x1CE: CharDrawOption_NPC (default true)
	bool m_bDrawCOS;          // +0x1CF: CharDrawOption_COS (default true)
	bool m_bDrawCharName;     // +0x1D0: CharDrawOption_DrawCharName (default true)
	bool m_bDrawGID;          // +0x1D1: CharDrawOption_DrawGID (default false)
	bool m_bDrawNavigation;   // +0x1D2: CharDrawOption_DrawNavigation (default false)
	bool m_bDrawState;        // +0x1D3: CharDrawOption_DrawState (default true)
};

#endif // _SR_GAMESERVER_DISPLAYWINDOW_H_
