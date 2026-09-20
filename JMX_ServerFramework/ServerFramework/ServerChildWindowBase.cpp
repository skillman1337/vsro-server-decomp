/**
 * ============================================================================
 * Joymax ServerFramework - Server Child Window Base Implementation
 * Original Source: D:\WORK2005\Source\JMX_ServerFramework\ServerFramework\ServerChildWindowBase.cpp
 *
 * Implements:
 *   - CServerChildWindowBase::CServerChildWindowBase @ 0x00564D50
 *   - CServerChildWindowBase::~CServerChildWindowBase @ 0x00564DC0
 *   - CServerChildWindowBase::ReleaseGDI @ 0x0066B100
 *   - CServerChildWindowBase::GetWindowName @ 0x005640B0
 * ============================================================================
 */

#include "ServerChildWindowBase.h"

namespace ServerFramework {

/*
===============================================================================
CServerChildWindowBase::CServerChildWindowBase [RECONSTRUCTED - Native 0x00564D50]

Machine Disassembly (104 bytes @ 0x00564D50):
  00564D72  8B74241C              mov     esi, [esp+0x1C]       ; esi = this
  00564D76  56                    push    esi                   ; push this
  00564D77  E834DA3E00            call    CWindowBase::Ctor     ; 0x009527B0
  00564D82  C70650A5AF00          mov     dword [esi], 0x00AFA550 ; vftable = CServerChildWindowBase::vftable
  00564D88  C786880000000F000000  mov     dword [esi+0x88], 15  ; m_strWindowName._Myres = 15
  00564D92  898684000000          mov     dword [esi+0x84], 0   ; m_strWindowName._Mysize = 0
  00564D98  884674                mov     byte [esi+0x74], 0    ; m_strWindowName._Bx._Buf[0] = '\0'
  00564DA3  8BC6                  mov     eax, esi              ; return this
  00564DB5  C20400                retn    4
===============================================================================
*/
CServerChildWindowBase::CServerChildWindowBase()
	: CWindowBase()
	, m_strWindowName() {
}

/*
===============================================================================
CServerChildWindowBase::~CServerChildWindowBase [RECONSTRUCTED - Native 0x00564DC0]

Machine Disassembly (124 bytes @ 0x00564DC0):
  00564DE1  8B742418              mov     esi, [esp+0x18]       ; esi = this
  00564DED  83BE8800000010        cmp     dword [esi+0x88], 16  ; if (_Myres >= 16)
  00564DF4  720C                  jb      0x00564E02
  00564DF6  8B4674                mov     eax, [esi+0x74]       ; eax = _Bx._Ptr
  00564DF9  50                    push    eax
  00564DFA  E83E824700            call    operator delete       ; Free dynamic string buffer
  00564DFF  83C404                add     esp, 4
  00564E02  C786880000000F000000  mov     dword [esi+0x88], 15  ; Reset string capacity
  00564E0C  C7868400000000000000  mov     dword [esi+0x84], 0   ; Reset string length
  00564E16  C6467400              mov     byte [esi+0x74], 0    ; Reset string buffer
  00564E22  8BCE                  mov     ecx, esi
  00564E24  E887DA3E00            call    CWindowBase::Dtor     ; 0x009528B0
  00564E39  C20400                retn    4
===============================================================================
*/
CServerChildWindowBase::~CServerChildWindowBase() {
}

/*
===============================================================================
CServerChildWindowBase::ReleaseGDI [RECONSTRUCTED - Native 0x0066B100]

Slot 2 (+0x08) of CServerChildWindowBase::vftable @ 0x00AFA558.
Default empty implementation in base class; overridden by derived view classes
to release GDI memory device contexts, bitmaps, brushes, and pens.
===============================================================================
*/
void CServerChildWindowBase::ReleaseGDI() {
	// Native 0x0066B100: retn
}

/*
===============================================================================
CServerChildWindowBase::GetWindowName [RECONSTRUCTED - Native 0x005640B0]

Slot 3 (+0x0C) of CServerChildWindowBase::vftable @ 0x00AFA55C.
Machine Disassembly (17 bytes @ 0x005640B0):
  005640B0  83B98800000010        cmp     dword [ecx+0x88], 16  ; if (_Myres >= 16)
  005640B7  7204                  jb      0x005640BD
  005640B9  8B4174                mov     eax, [ecx+0x74]       ; return _Bx._Ptr
  005640BC  C3                    retn
  005640BD  8D4174                lea     eax, [ecx+0x74]       ; return &_Bx._Buf[0]
  005640C0  C3                    retn
===============================================================================
*/
const char* CServerChildWindowBase::GetWindowName() const {
	return m_strWindowName.c_str();
}

void CServerChildWindowBase::SetWindowName(const std::string& strName) {
	m_strWindowName = strName;
}

} // namespace ServerFramework
