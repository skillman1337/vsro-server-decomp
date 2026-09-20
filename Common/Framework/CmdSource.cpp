/*
===========================================================================
Silkroad Online - Command Source & Network Command Source Implementation
Original Source: D:\WORK2005\Source\Common\Framework\CmdSource.cpp
Native RTTI:
  - .?AVCCmdSource@@ @ 0x00ADF3C4
  - .?AVCCmdSrcNet@@ @ 0x00ADF400
===========================================================================
*/

#include "CmdSource.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "GObjChar.h"
#include "Game.h"
#include <windows.h>
#include <cstring>

/*
================
CCmdSource::CCmdSource
[RECONSTRUCTED - 0x0040A960]
================
*/
CCmdSource::CCmdSource()
	: m_pGObj( nullptr )
	, m_queMsg() {
}

/*
================
CCmdSource::~CCmdSource
[RECONSTRUCTED - 0x0040A9E0]

CORRECTION (Claude): the base destructor does not flush. It clears the object pointer and
reports a non-empty queue (0x0040AA16), leaving draining to the owner.
================
*/
CCmdSource::~CCmdSource() {
	ASSERT( m_queMsg.Size() == 0 );
	m_pGObj = nullptr;
}

/*
================
CCmdSource::PostCommand
[RECONSTRUCTED - slot 1 @ 0x0040B720]
================
*/
int32_t CCmdSource::PostCommand( CMsg* pMsg ) {
	if ( m_queMsg.m_dwMaxCount != 0xFFFFFFFF && m_queMsg.Size() >= m_queMsg.m_dwMaxCount ) {
		return 0;
	}
	m_queMsg.Push( pMsg );
	return 1;
}

/*
================
CCmdSource::SetGObj
[RECONSTRUCTED - slot 2 @ 0x0040A940]
================
*/
int32_t CCmdSource::SetGObj( CGObjChar* pGObj ) {
	m_pGObj = pGObj;
	return 1;
}

/*
================
CCmdSource::Slot3
[RECONSTRUCTED - slot 3 @ 0x009BF500]
================
*/
void CCmdSource::Slot3( void* /*pUnused*/ ) {
}

/*
================
CCmdSource::Flush
[RECONSTRUCTED - slot 4 @ 0x0040AA50]

CORRECTION (Claude): drained messages are delivered to the controlled object's slot 358
before being consumed and freed; they are not just released.
================
*/
void CCmdSource::Flush() {
	CMsg* pMsg = nullptr;
	while ( m_queMsg.Pop( pMsg ) == 1 ) {
		if ( m_pGObj != nullptr ) {
			// Slot 358 receives the CMsg itself; the port still declares it with BSLib::CPacket
			// (see CGObjChar::OnClientPacket), as do the other slot-358 call sites.
			m_pGObj->OnClientPacket( reinterpret_cast<BSLib::CPacket*>( pMsg ) );
		}
		pMsg->Consume();
		FreeMsg( pMsg );
	}
}

/*
================
CCmdSource::FreeMsg
[RECONSTRUCTED - slot 6 @ 0x00825E50]
================
*/
void CCmdSource::FreeMsg( CMsg* /*pMsg*/ ) {
	ASSERT( false );
}

/*
================
CCmdSource::OnCommandEvent
[RECONSTRUCTED - slot 7 @ 0x004A66C0]
================
*/
int32_t CCmdSource::OnCommandEvent( uint32_t /*dwEvent*/, uintptr_t /*dwParam*/, uint32_t /*dwCode*/ ) {
	return 0;
}

/*
================
CCmdSource::Slot9
[RECONSTRUCTED - slot 9 @ 0x00559C70]
================
*/
int32_t CCmdSource::Slot9() {
	return 0;
}

/*
================
CCmdSource::Slot10
[RECONSTRUCTED - slot 10 @ 0x005ECFD0]
================
*/
int32_t CCmdSource::Slot10() {
	return 1;
}

/*
================
CCmdSource::Slot11
[RECONSTRUCTED - slot 11 @ 0x0040A950]
================
*/
int32_t CCmdSource::Slot11() {
	Flush();
	return 1;
}

/*
================
CCmdSrcNet::CCmdSrcNet
[RECONSTRUCTED - 0x0040B3B0]
================
*/
CCmdSrcNet::CCmdSrcNet()
	: CCmdSource()
	, m_dwCreateTick( ::GetTickCount() ) {
	std::memset( &m_connection, 0, sizeof( m_connection ) );
}

/*
================
CCmdSrcNet::~CCmdSrcNet
[RECONSTRUCTED - 0x0040B470]
================
*/
CCmdSrcNet::~CCmdSrcNet() {
	m_pGObj = nullptr;
	Flush();
	if ( m_connection.m_dwSessionID != 0 ) {
		ASSERT( g_pGame != nullptr );
		g_pGame->UnregisterCmdSrcNet( m_connection.m_dwSessionID );
	}
	std::memset( &m_connection, 0, sizeof( m_connection ) );
}

/*
================
CCmdSrcNet::Init
[RECONSTRUCTED - 0x0040B680] (52 bytes)
================
*/
int32_t CCmdSrcNet::Init( CGObjChar* pGObj, const tagCmdSrcNetConnection* pConnection ) {
	ASSERT( pGObj->IsPlayer() );
	m_pGObj = pGObj;
	std::memcpy( &m_connection, pConnection, sizeof( m_connection ) );
	return 1;
}

/*
================
CCmdSrcNet::AllocMsg
[PARTIAL - slot 5 @ 0x0040B750] (32 bytes)

Native takes the message from g_pNetEngine slot 18 ("NetEngine::MsgPool", whose chunks are
initialised by CMsg::Reset 0x00471500) and stamps the opcode. The port's CNetBufferManager
does not construct CMsg objects yet, so the message is constructed and reset here directly.
================
*/
CMsg* CCmdSrcNet::AllocMsg( uint16_t wOpcode ) {
	CMsg* pMsg = new CMsg();
	pMsg->Reset();
	*pMsg->m_pwOpcode = wOpcode;
	return pMsg;
}

/*
================
CCmdSrcNet::FreeMsg
[PARTIAL - slot 6 @ 0x0040B770] (21 bytes)

Native returns the message through g_pNetEngine slot 19; see AllocMsg.
================
*/
void CCmdSrcNet::FreeMsg( CMsg* pMsg ) {
	delete pMsg;
}

/*
================
CCmdSrcNet::Slot8
[RECONSTRUCTED - slot 8 @ 0x0040B510] (13 bytes)
Destroys the command source through its deleting destructor.
================
*/
void CCmdSrcNet::Slot8() {
	delete this;
}

/*
================
CCmdSrcNet::Slot10
[RECONSTRUCTED - slot 10 @ 0x0040B6C0] (35 bytes)
Registers this command source under its session id.
================
*/
int32_t CCmdSrcNet::Slot10() {
	ASSERT( g_pGame != nullptr );
	return g_pGame->RegisterCmdSrcNet( m_connection.m_dwSessionID, this );
}

/*
================
CCmdSrcNet::Slot11
[RECONSTRUCTED - slot 11 @ 0x0040B6F0] (42 bytes)
Unregisters the session id; non-zero when it was registered.
================
*/
int32_t CCmdSrcNet::Slot11() {
	ASSERT( g_pGame != nullptr );
	return ( g_pGame->UnregisterCmdSrcNet( m_connection.m_dwSessionID ) != 0 ) ? 1 : 0;
}
