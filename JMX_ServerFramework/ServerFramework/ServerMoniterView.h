/**
 * ============================================================================
 * Silkroad Online - ServerFramework GUI Performance Monitor View
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerMoniterView.h
 *
 * Implements the Win32 performance monitor graph window and graph series:
 *   - Native CMoniterGraphData @ 0x00B4187C (Size 0x68 / 104 bytes)
 *   - Native CServerMoniterView @ 0x00B41884 (Size 0x158 / 344 bytes)
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERMONITERVIEW_H_
#define _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERMONITERVIEW_H_

#include "ServerChildWindowBase.h"

#include <cstdint>
#include <string>
#include <map>
#include <list>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ServerFramework {

/**
 * [RECONSTRUCTED - 0x009611C0]
 * GetModuleCategoryName
 * Maps counter category byte (dwCounterId >> 24) to category label:
 *   1 -> "OSModule"
 *   2 -> "NetEngine"
 *   3 -> "ServerBody"
 *   Other -> "Unknown"
 */
const char* GetModuleCategoryName(uint32_t dwCounterId);

/**
 * [RECONSTRUCTED - 0x009611E0 / 0x00961290]
 * ServerFramework::CMoniterGraphData
 * Native VTable @ 0x00B4187C (RTTI: .?AVCMoniterGraphData@ServerFramework@@)
 * Size: 0x68 (104 bytes)
 */
class CMoniterGraphData {
public:
	// Native 0x009611E0: Constructor
	CMoniterGraphData();

	// Native 0x00961270: Scalar deleting destructor (VTable Slot 0)
	// Native 0x00961290: Destructor
	virtual ~CMoniterGraphData();

	// Native 0x00961300: Initialize series metadata
	void Initialize(const char* szName, COLORREF crColor, void* pCounter);

	// Native 0x00961370: Reset history
	void Reset();

	// Native 0x00961380: Sample current value from bound counter pointer
	void Sample();

	// Native 0x00961410: Set visibility flag
	void SetVisible(bool bVisible);

	// Native 0x00961420: Draw series history line graph
	void DrawHistory(HDC hdc, float fMinY, float fMaxY, int left, int top, int right, int bottom);

public:
	// +0x00: VTable pointer (0x00B4187C)
	char               m_szName[64];         // +0x04: Counter label (0x40 bytes)
	std::list<float>   m_listHistory;        // +0x44: Sample history list (head node + size = 12 bytes)
	float              m_fCurrentValue;      // +0x50: Latest metric sample
	float              m_fMinValue;          // +0x54: Minimum sample value (init FLT_MAX)
	float              m_fMaxValue;          // +0x58: Maximum sample value (init -FLT_MAX)
	void*              m_pCounter;           // +0x5C: Bound performance counter pointer (value at +8, count at +0xC)
	bool               m_bVisible;           // +0x60: Toggle display in graph
	uint8_t            m_pad61[3];           // +0x61: Alignment padding
	COLORREF           m_crLineColor;        // +0x64: Line color (assigned from 12-color palette)
};

/**
 * [RECONSTRUCTED - 0x009615B0 / 0x00961790]
 * ServerFramework::CServerMoniterView
 * Native VTable @ 0x00B41884 (RTTI: .?AVCServerMoniterView@ServerFramework@@)
 * Inherits CServerChildWindowBase -> CWindowBase
 * Total Size: 0x158 (344 bytes)
 */
class CServerMoniterView : public CServerChildWindowBase {
public:
	// Native 0x009615B0: Constructor
	CServerMoniterView();

	// Native 0x00961720: Scalar deleting destructor (VTable Slot 0)
	// Native 0x00961790: Destructor
	virtual ~CServerMoniterView() override;

	// Native VTable Slot 2 @ 0x00961920: Cleanup GDI resources and plot map
	virtual void ReleaseGDI() override;

	// Native VTable Slot 4 @ 0x00961800: Create monitor view window
	virtual bool Create(HINSTANCE hInst, HWND hWndParent, int x, int y, int cx, int cy, uint32_t dwControlId);

	// Native VTable Slot 5 @ 0x00961A40: Add performance counter graph series
	virtual bool AddGraphData(uint16_t wCounterId, const char* szName, void* pCounter);

	// Native VTable Slot 6 @ 0x00961BA0: Remove performance counter graph series
	virtual bool RemoveGraphData(uint16_t wCounterId);

	// Native 0x00961C30: Find graph data by counter ID
	CMoniterGraphData* FindGraphData(uint16_t wCounterId);

	// Native 0x00962180: Coordinate conversion helper (metric value -> client Y pixel)
	int ValueToPixelY(float fValue);

	// Native 0x00961C90: Update performance data, autoscale Y axis, and trigger backbuffer recreation
	void OnUpdateData();

	// Message Handlers
	void OnPaint();                                                 // Native 0x009626C0 (WM_PAINT)
	void OnSize(UINT nType, int cx, int cy);                        // Native 0x00962800 (WM_SIZE)
	BOOL OnSetCursor(HWND hWnd, UINT nHitTest, UINT message);       // Native 0x00962820 (WM_SETCURSOR)
	void OnTimer(UINT_PTR nIDEvent);                                // Native 0x00962840 (WM_TIMER)
	void OnRButtonUp(UINT nFlags, int x, int y);                    // Native 0x00962860 (WM_RBUTTONUP)

protected:
	// Internal rendering helpers
	void CreateBackbuffer();                                        // Native 0x00961E20
	void DrawGraph(HDC hdc, int left, int top, int right, int bottom); // Native 0x009621E0
	void DrawGraphWithLegend(HDC hdc, int nLeftOffset);             // Native 0x00962290

public:
	// Fields proven against native offsets:
	HDC                                      m_hMemDC;           // +0x8C: Backbuffer memory DC
	HBITMAP                                  m_hBitmap;          // +0x90: Backbuffer DDB bitmap
	HDC                                      m_hBgDC;            // +0x94: Background static grid memory DC
	HBITMAP                                  m_hBgBitmap;        // +0x98: Background bitmap
	COLORREF                                 m_crBackground;     // +0x9C: Background color (0x0017261C)
	COLORREF                                 m_crGrid;           // +0xA0: Grid line color (0x006AA67A)
	COLORREF                                 m_crAxis;           // +0xA4: Axis marking color (0x006AA67A)
	COLORREF                                 m_crText;           // +0xA8: Text color (0x00FFFFFF)
	float                                    m_fMinY;            // +0xAC: Minimum Y axis value (0.0f)
	float                                    m_fMaxY;            // +0xB0: Maximum Y axis value (120.0f)
	uint32_t                                 m_nGridDivisions;   // +0xB4: Grid divisions count (50 / 0x32)
	float                                    m_fUpdateInterval;  // +0xB8: Refresh timer interval in seconds (1.0f)
	char                                     m_szYAxisLabel[64]; // +0xBC: Y-axis label ("value")
	char                                     m_szXAxisLabel[64]; // +0xFC: X-axis label ("time")
	bool                                     m_bShowLegend;      // +0x13C: Toggle legend display (default true)
	uint8_t                                  m_pad13D[3];        // +0x13D: Alignment padding
	HCURSOR                                  m_hCursor;          // +0x140: Window cursor (IDC_ARROW)
	std::map<uint16_t, CMoniterGraphData*>   m_mapGraphData;     // +0x144: Graph data map (12 bytes)
	uint32_t                                 m_dwReserved150;    // +0x150: Reserved field for throttling (0)
	uint32_t                                 m_nNextColorIndex;  // +0x154: Palette color index (0 to 11)
};

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERFRAMEWORK_SERVERMONITERVIEW_H_
