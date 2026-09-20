/**
 * ============================================================================
 * Silkroad Online - ServerFramework GUI Performance Monitor View
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerMoniterView.cpp
 *
 * Implements performance monitoring view window and graph data series:
 *   - GetModuleCategoryName         @ 0x009611C0
 *   - CMoniterGraphData constructor @ 0x009611E0
 *   - CMoniterGraphData destructor  @ 0x00961290
 *   - CMoniterGraphData::Initialize @ 0x00961300
 *   - CMoniterGraphData::Reset      @ 0x00961370
 *   - CMoniterGraphData::Sample     @ 0x00961380
 *   - CMoniterGraphData::SetVisible @ 0x00961410
 *   - CMoniterGraphData::DrawHistory @ 0x00961420
 *   - CServerMoniterView constructor @ 0x009615B0
 *   - CServerMoniterView destructor  @ 0x00961790
 *   - CServerMoniterView::ReleaseGDI @ 0x00961920
 *   - CServerMoniterView::Create     @ 0x00961800
 *   - CServerMoniterView::AddGraphData    @ 0x00961A40
 *   - CServerMoniterView::RemoveGraphData @ 0x00961BA0
 *   - CServerMoniterView::FindGraphData   @ 0x00961C30
 *   - CServerMoniterView::OnUpdateData   @ 0x00961C90
 *   - CServerMoniterView::CreateBackbuffer @ 0x00961E20
 *   - CServerMoniterView::ValueToPixelY  @ 0x00962180
 *   - CServerMoniterView::DrawGraph      @ 0x009621E0
 *   - CServerMoniterView::DrawGraphWithLegend @ 0x00962290
 *   - CServerMoniterView::OnPaint        @ 0x009626C0
 *   - CServerMoniterView::OnSize         @ 0x00962800
 *   - CServerMoniterView::OnSetCursor    @ 0x00962820
 *   - CServerMoniterView::OnTimer        @ 0x00962840
 *   - CServerMoniterView::OnRButtonUp    @ 0x00962860
 * ============================================================================
 */

#include "ServerMoniterView.h"
#include "ServerFrameWindow.h"
#include "ServerConfig.h"
#include "../../JMX_Library/BSLib/BSLog.h"

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cfloat>

namespace ServerFramework {

// Native 12-entry palette array proven @ 0x00961A74 - 0x00961ACC
#ifdef _WIN32
static const COLORREF s_graphPalette[12] = {
	RGB(255, 255, 255), // 0x00FFFFFF - White
	RGB(255, 0, 0),     // 0x000000FF - Red
	RGB(0, 255, 0),     // 0x0000FF00 - Green
	RGB(255, 0, 255),   // 0x00FF00FF - Magenta
	RGB(255, 255, 0),   // 0x0000FFFF - Yellow
	RGB(0, 255, 255),   // 0x00FFFF00 - Cyan
	RGB(255, 128, 128), // 0x008080FF - Light Red
	RGB(128, 255, 128), // 0x0080FF80 - Light Green
	RGB(128, 128, 255), // 0x00FF8080 - Light Blue
	RGB(255, 255, 128), // 0x0080FFFF - Light Yellow
	RGB(128, 255, 255), // 0x00FFFF80 - Light Cyan
	RGB(255, 128, 255)  // 0x00FF80FF - Light Magenta
};
#endif

/**
 * [RECONSTRUCTED - 0x009611C0]
 * GetModuleCategoryName
 * Native function @ 0x009611C0 (22 bytes)
 * High byte of dwCounterId indexes into s_categories table @ 0x00C80BBC
 */
const char* GetModuleCategoryName(uint32_t dwCounterId) {
	static const char* s_categories[4] = {
		nullptr,
		"OSModule",
		"NetEngine",
		"ServerBody"
	};
	uint32_t idx = dwCounterId >> 24;
	if (idx > 3 || s_categories[idx] == nullptr) {
		return "Unknown";
	}
	return s_categories[idx];
}

// ============================================================================
// CMoniterGraphData Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x009611E0]
 * CMoniterGraphData::CMoniterGraphData
 * Native constructor @ 0x009611E0 (128 bytes)
 */
CMoniterGraphData::CMoniterGraphData()
	: m_fCurrentValue(0.0f)
	, m_fMinValue(FLT_MAX)
	, m_fMaxValue(-FLT_MAX)
	, m_pCounter(nullptr)
	, m_bVisible(true)
#ifdef _WIN32
	, m_crLineColor(RGB(255, 255, 255))
#else
	, m_crLineColor(0x00FFFFFF)
#endif
{
	std::memset(m_szName, 0, sizeof(m_szName));
	std::memset(m_pad61, 0, sizeof(m_pad61));
}

/**
 * [RECONSTRUCTED - 0x00961290]
 * CMoniterGraphData::~CMoniterGraphData
 * Native destructor @ 0x00961290 (108 bytes)
 */
CMoniterGraphData::~CMoniterGraphData() {
	m_listHistory.clear();
}

/**
 * [RECONSTRUCTED - 0x00961300]
 * CMoniterGraphData::Initialize
 * Native initialization @ 0x00961300 (107 bytes)
 */
void CMoniterGraphData::Initialize(const char* szName, COLORREF crColor, void* pCounter) {
	if (szName) {
#if defined(_WIN32) && defined(_MSC_VER)
		strncpy_s(m_szName, sizeof(m_szName), szName, _TRUNCATE);
#else
		std::strncpy(m_szName, szName, sizeof(m_szName) - 1);
		m_szName[sizeof(m_szName) - 1] = '\0';
#endif
	} else {
		m_szName[0] = '\0';
	}
	m_pCounter = pCounter;
	if (pCounter) {
		*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pCounter) + 0x0C) = 0;
	}
	m_crLineColor = crColor;
}

/**
 * [RECONSTRUCTED - 0x00961370]
 * CMoniterGraphData::Reset
 * Native reset @ 0x00961370 (12 bytes)
 */
void CMoniterGraphData::Reset() {
	m_listHistory.clear();
}

/**
 * [RECONSTRUCTED - 0x00961380]
 * CMoniterGraphData::Sample
 * Native sampling routine @ 0x00961380 (129 bytes)
 * Samples bound counter float value at +0x08, increments count at +0x0C,
 * pushes value into m_listHistory, trims history, and updates min/max.
 */
void CMoniterGraphData::Sample() {
	if (!m_pCounter) return;

	float fVal = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(m_pCounter) + 8);
	m_fCurrentValue = fVal;
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_pCounter) + 0x0C) += 1;

	m_listHistory.push_back(fVal);
	if (m_listHistory.size() > 100) {
		m_listHistory.pop_front();
	}

	if (fVal < m_fMinValue) m_fMinValue = fVal;
	if (fVal > m_fMaxValue) m_fMaxValue = fVal;
}

/**
 * [RECONSTRUCTED - 0x00961410]
 * CMoniterGraphData::SetVisible
 * Native toggle visibility @ 0x00961410 (4 bytes)
 */
void CMoniterGraphData::SetVisible(bool bVisible) {
	m_bVisible = bVisible;
}

/**
 * [RECONSTRUCTED - 0x00961420]
 * CMoniterGraphData::DrawHistory
 * Native history renderer @ 0x00961420 (392 bytes)
 *
 * Iterates m_listHistory in reverse order (newest to oldest):
 *   - Horizontal spacing is 4 pixels per sample (x -= 4)
 *   - Stops at (left + 31) to keep the left Y-axis labels clear
 *   - Draws line segments in m_crLineColor
 */
void CMoniterGraphData::DrawHistory(HDC hdc, float fMinY, float fMaxY, int left, int top, int right, int bottom) {
#ifdef _WIN32
	if (m_listHistory.empty() || !hdc) return;

	float fRange = fMaxY - fMinY;
	if (fRange <= 0.0001f) fRange = 1.0f;

	int nHeight = (bottom - top) - 16;
	int x = right - 4;
	int minX = left + 31; // 0x1F

	bool bFirst = true;
	for (auto it = m_listHistory.rbegin(); it != m_listHistory.rend(); ++it) {
		if (x <= minX) break;

		float val = *it;
		int yOffset = static_cast<int>((static_cast<float>(nHeight) * val) / fRange);
		int y = nHeight - yOffset;
		if (y < 0) y = 0;
		y += top;

		if (bFirst) {
			::SetPixel(hdc, x, y, m_crLineColor);
			::MoveToEx(hdc, x, y, nullptr);
			bFirst = false;
		} else {
			HPEN hPen = ::CreatePen(PS_SOLID, 1, m_crLineColor);
			HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
			::LineTo(hdc, x, y);
			::SelectObject(hdc, hOldPen);
			::DeleteObject(hPen);
		}
		x -= 4;
	}
#else
	(void)hdc; (void)fMinY; (void)fMaxY; (void)left; (void)top; (void)right; (void)bottom;
#endif
}

// ============================================================================
// CServerMoniterView Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x009615B0]
 * CServerMoniterView::CServerMoniterView
 * Native constructor @ 0x009615B0 (362 bytes)
 */
CServerMoniterView::CServerMoniterView()
	: CServerChildWindowBase()
	, m_hMemDC(nullptr)
	, m_hBitmap(nullptr)
	, m_hBgDC(nullptr)
	, m_hBgBitmap(nullptr)
	, m_crBackground(0x0017261C)
	, m_crGrid(0x006AA67A)
	, m_crAxis(0x006AA67A)
	, m_crText(0x00FFFFFF)
	, m_fMinY(0.0f)
	, m_fMaxY(120.0f)
	, m_nGridDivisions(50)
	, m_fUpdateInterval(1.0f)
	, m_bShowLegend(true)
	, m_hCursor(nullptr)
	, m_dwReserved150(0)
	, m_nNextColorIndex(0) {
	std::memset(m_pad13D, 0, sizeof(m_pad13D));

#if defined(_WIN32) && defined(_MSC_VER)
	strcpy_s(m_szYAxisLabel, sizeof(m_szYAxisLabel), "value");
	strcpy_s(m_szXAxisLabel, sizeof(m_szXAxisLabel), "time");
#else
	std::strncpy(m_szYAxisLabel, "value", sizeof(m_szYAxisLabel) - 1);
	m_szYAxisLabel[sizeof(m_szYAxisLabel) - 1] = '\0';
	std::strncpy(m_szXAxisLabel, "time", sizeof(m_szXAxisLabel) - 1);
	m_szXAxisLabel[sizeof(m_szXAxisLabel) - 1] = '\0';
#endif

#ifdef _WIN32
	m_hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
#endif
}

/**
 * [RECONSTRUCTED - 0x00961790]
 * CServerMoniterView::~CServerMoniterView
 * Native non-deleting destructor @ 0x00961790 (96 bytes)
 */
CServerMoniterView::~CServerMoniterView() {
	ReleaseGDI();
}

/**
 * [RECONSTRUCTED - 0x00961920]
 * CServerMoniterView::ReleaseGDI
 * Native VTable Slot 2 @ 0x00961920 (189 bytes)
 * Cleans up allocated GDI DCs, bitmaps, and deletes all heap CMoniterGraphData entries.
 */
void CServerMoniterView::ReleaseGDI() {
#ifdef _WIN32
	if (m_hMemDC) {
		::DeleteDC(m_hMemDC);
		m_hMemDC = nullptr;
	}
	if (m_hBgDC) {
		::DeleteDC(m_hBgDC);
		m_hBgDC = nullptr;
	}
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
	if (m_hBgBitmap) {
		::DeleteObject(m_hBgBitmap);
		m_hBgBitmap = nullptr;
	}
#endif

	for (auto& pair : m_mapGraphData) {
		if (pair.second) {
			pair.second->Reset();
			delete pair.second;
		}
	}
	m_mapGraphData.clear();
}

/**
 * [RECONSTRUCTED - 0x00961800]
 * CServerMoniterView::Create
 * Native VTable Slot 4 @ 0x00961800 (275 bytes)
 */
bool CServerMoniterView::Create(HINSTANCE hInst, HWND hWndParent, int x, int y, int cx, int cy, uint32_t dwControlId) {
#ifdef _WIN32
	WNDCLASSA wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = DefWindowProcA;
	wc.hInstance     = hInst;
	wc.hCursor       = m_hCursor ? m_hCursor : ::LoadCursorA(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszClassName = "ServerMoniterView";
	::RegisterClassA(&wc);

	m_hWnd = ::CreateWindowExA(
		WS_EX_STATICEDGE,
		"ServerMoniterView",
		nullptr,
		WS_CHILD | WS_VISIBLE,
		x, y, cx, cy,
		hWndParent,
		reinterpret_cast<HMENU>(static_cast<uintptr_t>(dwControlId)),
		hInst,
		nullptr
	);

	if (!m_hWnd) {
		return false;
	}

	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);

	// Native 0x009618CF - 0x00961906: SetTimer(m_hWnd, 100, intervalMs, NULL)
	UINT nIntervalMs = static_cast<UINT>(m_fUpdateInterval * 1000.0f);
	::SetTimer(m_hWnd, 100, nIntervalMs, nullptr);
	return true;
#else
	(void)hInst; (void)x; (void)y; (void)cx; (void)cy; (void)dwControlId;
	m_hWnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(2));
	return true;
#endif
}

/**
 * [RECONSTRUCTED - 0x00961A40]
 * CServerMoniterView::AddGraphData
 * Native VTable Slot 5 @ 0x00961A40 (336 bytes)
 */
bool CServerMoniterView::AddGraphData(uint16_t wCounterId, const char* szName, void* pCounter) {
	if (FindGraphData(wCounterId) != nullptr) {
		return false;
	}

#ifdef _WIN32
	COLORREF crColor = s_graphPalette[m_nNextColorIndex % 12];
#else
	COLORREF crColor = 0x00FFFFFF;
#endif
	m_nNextColorIndex = (m_nNextColorIndex + 1) % 12;

	CMoniterGraphData* pData = new CMoniterGraphData();
	pData->Initialize(szName, crColor, pCounter);
	m_mapGraphData[wCounterId] = pData;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00961BA0]
 * CServerMoniterView::RemoveGraphData
 * Native VTable Slot 6 @ 0x00961BA0 (120 bytes)
 */
bool CServerMoniterView::RemoveGraphData(uint16_t wCounterId) {
	auto it = m_mapGraphData.find(wCounterId);
	if (it == m_mapGraphData.end()) {
		return false;
	}

	if (it->second) {
		it->second->Reset();
		delete it->second;
	}
	m_mapGraphData.erase(it);
	return true;
}

/**
 * [RECONSTRUCTED - 0x00961C30]
 * CServerMoniterView::FindGraphData
 * Native 0x00961C30 (48 bytes)
 */
CMoniterGraphData* CServerMoniterView::FindGraphData(uint16_t wCounterId) {
	auto it = m_mapGraphData.find(wCounterId);
	if (it != m_mapGraphData.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x00962180]
 * CServerMoniterView::ValueToPixelY
 * Native coordinate mapping helper @ 0x00962180 (83 bytes)
 *
 * Maps a floating-point metric value to vertical pixel coordinate:
 *   y = (rc.bottom - 16) - fValue * (rc.bottom - 16) / (m_fMaxY - m_fMinY)
 */
int CServerMoniterView::ValueToPixelY(float fValue) {
#ifdef _WIN32
	if (!m_hWnd) return 0;
	RECT rc = {};
	::GetClientRect(m_hWnd, &rc);
	float fTotalH = static_cast<float>(rc.bottom - 16);
	float fRange = m_fMaxY - m_fMinY;
	if (fRange <= 0.0001f) fRange = 1.0f;
	return static_cast<int>(fTotalH - (fValue * fTotalH / fRange));
#else
	(void)fValue;
	return 0;
#endif
}

/**
 * [RECONSTRUCTED - 0x00961C90]
 * CServerMoniterView::OnUpdateData
 * Native data collection, autoscaling, and backbuffer recreation @ 0x00961C90 (395 bytes)
 */
void CServerMoniterView::OnUpdateData() {
#ifdef _WIN32
	int maxValInt = 120;
	char szLogBuffer[1024];
	szLogBuffer[0] = '\0';

	for (auto& pair : m_mapGraphData) {
		CMoniterGraphData* pData = pair.second;
		if (!pData) continue;

		pData->Sample();

		if (pData->m_fMaxValue > static_cast<float>(maxValInt)) {
			maxValInt = static_cast<int>(pData->m_fMaxValue * 9.0f * 0.125f);
		}

		if (g_bWritePerfLogToText) {
			char szEntry[256];
			std::snprintf(szEntry, sizeof(szEntry), "%s, %s: %.2f; ",
				GetModuleCategoryName(pair.first), pData->m_szName, pData->m_fCurrentValue);
			std::strncat(szLogBuffer, szEntry, sizeof(szLogBuffer) - std::strlen(szLogBuffer) - 1);
		}
	}

	if (szLogBuffer[0] != '\0') {
		BSLib::Log_Printf(0x3000000, "PERFMON >> %s", szLogBuffer);
	}

	float fNewMaxY = static_cast<float>(maxValInt);
	if (m_fMaxY != fNewMaxY) {
		DWORD now = ::GetTickCount();
		if (m_fMaxY < fNewMaxY || (now - m_dwReserved150 > 1000)) {
			m_dwReserved150 = now;
			m_fMaxY = fNewMaxY;
			m_nGridDivisions = static_cast<uint32_t>(m_fMaxY / 3.0f);
			CreateBackbuffer();
		}
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x00961E20]
 * CServerMoniterView::CreateBackbuffer
 * Native backbuffer surface and static grid renderer @ 0x00961E20 (862 bytes)
 *
 * Source citation: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerMoniterView.cpp line 439
 *
 * Allocates DDB memory DCs/bitmaps and pre-renders static background:
 *   - Background fill (m_crBackground: 0x0017261C)
 *   - Left Y-axis vertical line at (left + 35)
 *   - Bottom X-axis horizontal line at (bottom - 16)
 *   - Maximum scale label at (left + 35, 0)
 *   - Minimum scale label at (left + 35, bottom - 16)
 *   - Grid division lines (PS_DOT) and numeric value labels
 */
void CServerMoniterView::CreateBackbuffer() {
#ifdef _WIN32
	if (!m_hWnd) return;

	RECT rc = {};
	::GetClientRect(m_hWnd, &rc);
	int cx = rc.right - rc.left;
	int cy = rc.bottom - rc.top;
	if (cx <= 0 || cy <= 0) return;

	HDC hDC = ::GetDC(m_hWnd);
	if (!hDC) return;

	if (!m_hMemDC) m_hMemDC = ::CreateCompatibleDC(hDC);
	if (!m_hBgDC)  m_hBgDC  = ::CreateCompatibleDC(hDC);

	m_crBackground = ::GetNearestColor(hDC, 0x0017261C);
	m_crGrid       = ::GetNearestColor(hDC, 0x006AA67A);
	m_crText       = ::GetNearestColor(hDC, 0x00FFFFFF);
	m_crAxis       = ::GetNearestColor(hDC, 0x006AA67A);

	if (m_hBitmap)   { ::DeleteObject(m_hBitmap);   m_hBitmap = nullptr; }
	if (m_hBgBitmap) { ::DeleteObject(m_hBgBitmap); m_hBgBitmap = nullptr; }

	m_hBitmap   = ::CreateCompatibleBitmap(hDC, cx, cy);
	m_hBgBitmap = ::CreateCompatibleBitmap(hDC, cx, cy);

	::SelectObject(m_hMemDC, m_hBitmap);
	::SelectObject(m_hBgDC, m_hBgBitmap);

	// 1. Fill background rectangle
	GDI_DrawRectangle(m_hBgDC, &rc, m_crBackground, m_crBackground);

	int axisLeft = rc.left + 35;
	int axisBottom = rc.bottom - 16;

	// 2. Y-axis line from top + 4 to bottom - 16
	GDI_DrawLine(m_hBgDC, axisLeft, rc.top + 4, axisLeft, axisBottom, m_crAxis, PS_SOLID);

	// 3. X-axis line from left + 35 to right - 4
	GDI_DrawLine(m_hBgDC, axisLeft, axisBottom, rc.right - 4, axisBottom, m_crAxis, PS_SOLID);

	// 4. Create and select 8pt Arial font
	HFONT hFont = ::CreateFontA(-8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "arial");
	HGDIOBJ hOldFont = ::SelectObject(m_hBgDC, hFont);

	::SetBkMode(m_hBgDC, TRANSPARENT);
	::SetTextColor(m_hBgDC, m_crText);

	// 5. Draw top Y max label
	BSLib::GDI_DrawTextFormatted(m_hBgDC, axisLeft, 0, TA_RIGHT, "%d", static_cast<int>(m_fMaxY));

	// 6. Draw bottom Y min label
	BSLib::GDI_DrawTextFormatted(m_hBgDC, axisLeft, axisBottom, TA_RIGHT | TA_BOTTOM, "%d", static_cast<int>(m_fMinY));

	// 7. Grid division markers and labels
	if (m_nGridDivisions > 0) {
		int curVal = static_cast<int>(m_fMinY + static_cast<float>(m_nGridDivisions));
		int curY = ValueToPixelY(static_cast<float>(curVal));
		while (curY > rc.top) {
			GDI_DrawLine(m_hBgDC, axisLeft, curY, rc.right - 4, curY, m_crGrid, PS_DOT);
			BSLib::GDI_DrawTextFormatted(m_hBgDC, axisLeft, curY - 4, TA_RIGHT, "%d", curVal);
			curVal += m_nGridDivisions;
			curY = ValueToPixelY(static_cast<float>(curVal));
		}
	}

	::SelectObject(m_hBgDC, hOldFont);
	::DeleteObject(hFont);
	::ReleaseDC(m_hWnd, hDC);
#endif
}

/**
 * [RECONSTRUCTED - 0x009621E0]
 * CServerMoniterView::DrawGraph
 * Native graph lines renderer @ 0x009621E0 (175 bytes)
 */
void CServerMoniterView::DrawGraph(HDC hdc, int left, int top, int right, int bottom) {
#ifdef _WIN32
	for (auto& pair : m_mapGraphData) {
		CMoniterGraphData* pData = pair.second;
		if (!pData || !pData->m_bVisible) continue;
		pData->DrawHistory(hdc, m_fMinY, m_fMaxY, left, top, right, bottom);
	}
#else
	(void)hdc; (void)left; (void)top; (void)right; (void)bottom;
#endif
}

/**
 * [RECONSTRUCTED - 0x00962290]
 * CServerMoniterView::DrawGraphWithLegend
 * Native floating legend overlay renderer @ 0x00962290 (1052 bytes)
 *
 * Two-pass rendering:
 *   Pass 1: Measures text extent of "(%d)%s" and category string across all series,
 *           accumulating bounding boxes via UnionRect, padded with InflateRect(..., 4, 0).
 *   Pass 2: Draws background border rectangle, 10px line swatches, text labels,
 *           and active yellow indicator bullets (GDI_DrawFilledEllipse).
 */
void CServerMoniterView::DrawGraphWithLegend(HDC hdc, int nLeftOffset) {
#ifdef _WIN32
	if (!hdc || m_mapGraphData.empty()) return;

	HFONT hFont = ::CreateFontA(-8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "arial");
	HGDIOBJ hOldFont = ::SelectObject(hdc, hFont);
	::SetBkMode(hdc, TRANSPARENT);

	int startX = nLeftOffset + 10;
	int legendLeft = startX + 35;
	int legendTop = 10;
	int curY = legendTop;

	RECT rcLegend = { legendLeft, legendTop, legendLeft, legendTop };

	// Pass 1: Measure bounds
	for (auto& pair : m_mapGraphData) {
		CMoniterGraphData* pData = pair.second;
		if (!pData) continue;

		char szValName[128];
		std::snprintf(szValName, sizeof(szValName), "(%d)%s",
			static_cast<int>(pData->m_fCurrentValue), pData->m_szName);
		SIZE sizeName = {};
		::GetTextExtentPoint32A(hdc, szValName, static_cast<int>(std::strlen(szValName)), &sizeName);

		const char* szCategory = GetModuleCategoryName(pair.first);
		SIZE sizeCat = {};
		::GetTextExtentPoint32A(hdc, szCategory, static_cast<int>(std::strlen(szCategory)), &sizeCat);

		RECT rcItem = {
			legendLeft,
			curY,
			legendLeft + sizeName.cx + sizeCat.cx + 29,
			curY + 16
		};
		::UnionRect(&rcLegend, &rcLegend, &rcItem);
		curY += 16;
	}

	::InflateRect(&rcLegend, 4, 0);
	GDI_DrawRectangle(hdc, &rcLegend, m_crBackground, m_crGrid);

	// Pass 2: Draw items
	curY = 2;
	for (auto& pair : m_mapGraphData) {
		CMoniterGraphData* pData = pair.second;
		if (!pData) continue;

		// Line swatch: 10px line
		GDI_DrawLine(hdc, startX + 35, curY + 8, startX + 45, curY + 8, pData->m_crLineColor, PS_SOLID);

		// Value & Name in line color
		::SetTextColor(hdc, pData->m_crLineColor);
		char szValName[128];
		std::snprintf(szValName, sizeof(szValName), "(%d)%s",
			static_cast<int>(pData->m_fCurrentValue), pData->m_szName);
		BSLib::GDI_DrawTextFormatted(hdc, startX + 47, curY, TA_NOUPDATECP, "%s", szValName);
		SIZE sizeName = {};
		::GetTextExtentPoint32A(hdc, szValName, static_cast<int>(std::strlen(szValName)), &sizeName);

		// Category in white (m_crText)
		const char* szCategory = GetModuleCategoryName(pair.first);
		::SetTextColor(hdc, m_crText);
		BSLib::GDI_DrawTextFormatted(hdc, startX + 52 + sizeName.cx, curY, TA_NOUPDATECP, "%s", szCategory);
		SIZE sizeCat = {};
		::GetTextExtentPoint32A(hdc, szCategory, static_cast<int>(std::strlen(szCategory)), &sizeCat);

		// Visibility indicator yellow circle
		if (pData->m_bVisible) {
			GDI_DrawFilledEllipse(hdc, startX + 55 + sizeName.cx + sizeCat.cx, curY + 3, 10, 10, 0x00FFFF, 0);
		}
		curY += 16;
	}

	::SelectObject(hdc, hOldFont);
	::DeleteObject(hFont);
#else
	(void)hdc; (void)nLeftOffset;
#endif
}

/**
 * [RECONSTRUCTED - 0x009626C0]
 * CServerMoniterView::OnPaint
 * Native WM_PAINT message handler @ 0x009626C0 (311 bytes)
 *
 * Sequence proven against native assembly:
 *   1. BitBlt background grid from m_hBgDC to m_hMemDC
 *   2. If m_bShowLegend is enabled, DrawGraphWithLegend(m_hMemDC, rcClient.left)
 *   3. DrawGraph(m_hMemDC, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom)
 *   4. BitBlt composite backbuffer to client DC
 */
void CServerMoniterView::OnPaint() {
#ifdef _WIN32
	if (!m_hWnd) return;

	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hWnd, &ps);
	if (!hDC) return;

	RECT rcClient = {};
	::GetClientRect(m_hWnd, &rcClient);
	int cx = rcClient.right - rcClient.left;
	int cy = rcClient.bottom - rcClient.top;

	if (m_hMemDC && m_hBgDC) {
		::BitBlt(m_hMemDC, 0, 0, cx, cy, m_hBgDC, 0, 0, SRCCOPY);

		if (m_bShowLegend) {
			DrawGraphWithLegend(m_hMemDC, rcClient.left);
		}

		DrawGraph(m_hMemDC, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

		::BitBlt(hDC, 0, 0, cx, cy, m_hMemDC, 0, 0, SRCCOPY);
	}

	::EndPaint(m_hWnd, &ps);
#endif
}

/**
 * [RECONSTRUCTED - 0x00962800]
 * CServerMoniterView::OnSize
 * Native WM_SIZE message handler @ 0x00962800 (26 bytes)
 */
void CServerMoniterView::OnSize(UINT nType, int cx, int cy) {
	(void)nType; (void)cx; (void)cy;
#ifdef _WIN32
	CreateBackbuffer();
	if (m_hWnd) {
		::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x00962820]
 * CServerMoniterView::OnSetCursor
 * Native WM_SETCURSOR message handler @ 0x00962820 (21 bytes)
 */
BOOL CServerMoniterView::OnSetCursor(HWND hWnd, UINT nHitTest, UINT message) {
	(void)hWnd; (void)nHitTest; (void)message;
#ifdef _WIN32
	if (m_hCursor) {
		::SetCursor(m_hCursor);
		return TRUE;
	}
#endif
	return FALSE;
}

/**
 * [RECONSTRUCTED - 0x00962840]
 * CServerMoniterView::OnTimer
 * Native WM_TIMER message handler @ 0x00962840 (26 bytes)
 */
void CServerMoniterView::OnTimer(UINT_PTR nIDEvent) {
	(void)nIDEvent;
#ifdef _WIN32
	OnUpdateData();
	if (m_hWnd) {
		::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
	}
#endif
}

/**
 * [RECONSTRUCTED - 0x00962860]
 * CServerMoniterView::OnRButtonUp
 * Native WM_RBUTTONUP message handler @ 0x00962860 (852 bytes)
 *
 * Populates context menu:
 *   - "Start Moniter" (Cmd 1)
 *   - "Stop Moniter"  (Cmd 2)
 *   - "Option"        (Cmd 3)
 *   - Separator
 *   - "View" Submenu:
 *       - "Show Legend" (Cmd 4, Checked if m_bShowLegend)
 *       - Separator
 *       - "Show All Moniter" (Cmd 5)
 *       - "Hide All Moniter" (Cmd 6)
 *       - Separator
 *       - "Show %s %s" (Cmd = wCounterId, Checked if m_bVisible)
 */
void CServerMoniterView::OnRButtonUp(UINT nFlags, int x, int y) {
	(void)nFlags; (void)x; (void)y;
#ifdef _WIN32
	if (!m_hWnd) return;

	HMENU hMainMenu = ::CreatePopupMenu();
	HMENU hViewSubMenu = ::CreatePopupMenu();
	if (!hMainMenu || !hViewSubMenu) return;

	::AppendMenuA(hMainMenu, MF_STRING, 1, "Start Moniter");
	::AppendMenuA(hMainMenu, MF_STRING, 2, "Stop Moniter");
	::AppendMenuA(hMainMenu, MF_STRING, 3, "Option");
	::AppendMenuA(hMainMenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenuA(hMainMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hViewSubMenu), "View");

	::AppendMenuA(hViewSubMenu, MF_STRING, 4, "Show Legend");
	::CheckMenuItem(hViewSubMenu, 4, m_bShowLegend ? MF_CHECKED : MF_UNCHECKED);
	::AppendMenuA(hViewSubMenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenuA(hViewSubMenu, MF_STRING, 5, "Show All Moniter");
	::AppendMenuA(hViewSubMenu, MF_STRING, 6, "Hide All Moniter");
	::AppendMenuA(hViewSubMenu, MF_SEPARATOR, 0, nullptr);

	for (auto& pair : m_mapGraphData) {
		CMoniterGraphData* pData = pair.second;
		if (!pData) continue;

		char szMenuItem[256];
		std::snprintf(szMenuItem, sizeof(szMenuItem), "Show %s %s",
			GetModuleCategoryName(pair.first), pData->m_szName);

		::AppendMenuA(hViewSubMenu, MF_STRING, pair.first, szMenuItem);
		::CheckMenuItem(hViewSubMenu, pair.first, pData->m_bVisible ? MF_CHECKED : MF_UNCHECKED);
	}

	POINT pt = {};
	::GetCursorPos(&pt);

	int nCommand = ::TrackPopupMenu(hMainMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
	switch (nCommand) {
	case 4:
		m_bShowLegend = !m_bShowLegend;
		break;
	case 5:
		for (auto& pair : m_mapGraphData) {
			if (pair.second) pair.second->SetVisible(true);
		}
		break;
	case 6:
		for (auto& pair : m_mapGraphData) {
			if (pair.second) pair.second->SetVisible(false);
		}
		break;
	default:
		if (nCommand > 6) {
			CMoniterGraphData* pData = FindGraphData(static_cast<uint16_t>(nCommand));
			if (pData) {
				pData->SetVisible(!pData->m_bVisible);
			}
		}
		break;
	}

	::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
	::DestroyMenu(hViewSubMenu);
	::DestroyMenu(hMainMenu);
#endif
}

} // namespace ServerFramework
