/*
===========================================================================
Silkroad Online - Command Source & Network Command Source
Original Source: D:\WORK2005\Source\Common\Framework\CmdSource.h
Native RTTI:
  - .?AVCCmdSource@@ @ 0x00ADF3C4
  - .?AVCCmdSrcNet@@ @ 0x00ADF400
  - .?AV?$CQue@PAVCMsg@@@@ @ 0x00ADF3F8
===========================================================================
*/

#ifndef _COMMON_FRAMEWORK_CMDSOURCE_H_
#define _COMMON_FRAMEWORK_CMDSOURCE_H_

#include <cstdint>
#include <deque>
#include <cstring>
#include "Msg.h"

class CGObjChar;

/*
===========================================================================
CQue<T>
Native VTable @ 0x00ADF3F8, size 0x1C
Constructor @ 0x0040AAD0, Pop @ 0x0040AB30, Push @ 0x0040ABC0, destructor @ 0x0040AF80

CORRECTION (Claude): the limit is not enforced by Push. 0x0040ABC0 always appends; the
owner checks m_dwMaxCount first (CCmdSource::PostCommand 0x0040B720).
===========================================================================
*/
template <typename T>
class CQue {
public:
	/*
	================
	CQue::CQue
	[RECONSTRUCTED - 0x0040AAD0]
	================
	*/
	CQue()
		: m_dwMaxCount( 0xFFFFFFFF ) {
	}

	/*
	================
	CQue::~CQue
	[RECONSTRUCTED - 0x0040AF80]
	================
	*/
	virtual ~CQue() {
	}

	/*
	================
	CQue::Pop
	[RECONSTRUCTED - 0x0040AB30] (regparm eax = this, edi = ppOut)
	Returns 1 and the front element, or 0 and a null element when empty.
	================
	*/
	int32_t Pop( T& outItem ) {
		if ( m_deque.empty() ) {
			outItem = nullptr;
			return 0;
		}
		outItem = m_deque.front();
		m_deque.pop_front();
		return 1;
	}

	/*
	================
	CQue::Push
	[RECONSTRUCTED - 0x0040ABC0]
	================
	*/
	void Push( const T& item ) {
		m_deque.push_back( item );
	}

	size_t Size() const {
		return m_deque.size();
	}

public:
	uint32_t      m_dwMaxCount; // +0x04: 0xFFFFFFFF = unbounded
	std::deque<T> m_deque;      // +0x08: element count at +0x18
};

/*
===========================================================================
CCmdSource
Native VTable @ 0x00ADF3C4 (12 slots), size 0x24
Base class of CCmdSrcNet (player network input) and AI::CTactics (monster AI input).

CORRECTION (Claude): +0x04 is the controlled object, not a target id, and there is no
owner pointer at +0x24 (that offset already belongs to CCmdSrcNet). The object is written
by slot 2 (0x0040A940) and by CCmdSrcNet::Init (0x0040B680), and Flush (0x0040AA50) hands
every drained message to it through CGObjChar slot 358.
===========================================================================
*/
class CCmdSource {
public:
	// [RECONSTRUCTED - 0x0040A960]
	CCmdSource();

	// [RECONSTRUCTED - slot 0 @ 0x0040A9C0 -> 0x0040A9E0]
	virtual ~CCmdSource();

	// [RECONSTRUCTED - slot 1 @ 0x0040B720]
	// Returns 0 when the queue already holds m_dwMaxCount messages.
	virtual int32_t PostCommand( CMsg* pMsg );

	// [RECONSTRUCTED - slot 2 @ 0x0040A940]
	virtual int32_t SetGObj( CGObjChar* pGObj );

	// [RECONSTRUCTED - slot 3 @ 0x009BF500]
	virtual void Slot3( void* pUnused );

	// [RECONSTRUCTED - slot 4 @ 0x0040AA50]
	virtual void Flush();

	// [RECONSTRUCTED - slot 5 @ 0x009DD3AD purecall]
	virtual CMsg* AllocMsg( uint16_t wOpcode ) = 0;

	// [RECONSTRUCTED - slot 6 @ 0x00825E50]
	// The base body is the shared illegal-virtual ASSERT stub.
	virtual void FreeMsg( CMsg* pMsg );

	// [RECONSTRUCTED - slot 7 @ 0x004A66C0] (retn 0xC)
	// Command events raised by the character subsystems: 0x10 action cancelled / item-use
	// refused, 0x11 item-use posted, 4 skill refused (0x0059ADF1, non-player owners).
	virtual int32_t OnCommandEvent( uint32_t dwEvent, uintptr_t dwParam, uint32_t dwCode );

	// [RECONSTRUCTED - slot 8 @ 0x009DD3AD purecall]
	virtual void Slot8() = 0;

	// [RECONSTRUCTED - slot 9 @ 0x00559C70]
	virtual int32_t Slot9();

	// [RECONSTRUCTED - slot 10 @ 0x005ECFD0]
	virtual int32_t Slot10();

	// [RECONSTRUCTED - slot 11 @ 0x0040A950]
	virtual int32_t Slot11();

	CGObjChar* GetGObj() const { return m_pGObj; }

	// [RECONSTRUCTED - inlined by CGObjChar_PumpNetworkMsg 0x004A8B3B -> CQue::Pop 0x0040AB30]
	int32_t PopCommand( CMsg*& pOutMsg ) { return m_queMsg.Pop( pOutMsg ); }

public:
	CGObjChar*   m_pGObj;  // +0x04: controlled object
	CQue<CMsg*>  m_queMsg; // +0x08 - +0x23
};

/*
===========================================================================
tagCmdSrcNetConnection
The 0x12C-byte connection block CCmdSrcNet::Init copies to +0x28 (0x0040B6A3: rep movsd, 0x4B).
===========================================================================
*/
struct tagCmdSrcNetConnection {
	uint32_t m_dwSessionID;       // +0x00 (CCmdSrcNet +0x28)
	uint8_t  m_abyData[0x128];    // +0x04 - +0x12B
};

/*
===========================================================================
CCmdSrcNet
Native VTable @ 0x00ADF400, size 0x154 (0x004378B3: operator new(0x154))
Constructor @ 0x0040B3B0

CORRECTION (Claude): the object is 0x154 bytes, not 0x44; +0x24 is the creation tick and
+0x28..+0x153 is the connection block copied by Init.
===========================================================================
*/
class CCmdSrcNet : public CCmdSource {
public:
	// [RECONSTRUCTED - 0x0040B3B0]
	CCmdSrcNet();

	// [RECONSTRUCTED - slot 0 @ 0x0040B450 -> 0x0040B470]
	virtual ~CCmdSrcNet() override;

	// [RECONSTRUCTED - 0x0040B680] (eax = pGObj, ecx = this)
	int32_t Init( CGObjChar* pGObj, const tagCmdSrcNetConnection* pConnection );

	// [PARTIAL - slot 5 @ 0x0040B750]
	virtual CMsg* AllocMsg( uint16_t wOpcode ) override;

	// [PARTIAL - slot 6 @ 0x0040B770]
	virtual void FreeMsg( CMsg* pMsg ) override;

	// [RECONSTRUCTED - slot 8 @ 0x0040B510]
	virtual void Slot8() override;

	// [RECONSTRUCTED - slot 10 @ 0x0040B6C0]
	// Registers the session id in g_pGame's command-source registry.
	virtual int32_t Slot10() override;

	// [RECONSTRUCTED - slot 11 @ 0x0040B6F0]
	// Unregisters the session id; non-zero when it was registered.
	virtual int32_t Slot11() override;

	uint32_t GetSessionID() const { return m_connection.m_dwSessionID; }

public:
	uint32_t               m_dwCreateTick; // +0x24
	tagCmdSrcNetConnection m_connection;   // +0x28 - +0x153
};

#endif // _COMMON_FRAMEWORK_CMDSOURCE_H_
