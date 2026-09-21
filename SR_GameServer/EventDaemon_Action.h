/**
 * ============================================================================
 * Silkroad Online - Event Daemon Action Dispatcher
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\EventDaemon_Action.h
 *
 * Implements:
 *   - CEventDaemon_Action: existing reconstruction; 0x00B02160 is file-path data
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_EVENTDAEMON_ACTION_H_
#define _SR_GAMESERVER_EVENTDAEMON_ACTION_H_

#include <cstdint>
#include <string>

class CEventDaemon_Action {
public:
	CEventDaemon_Action();
	virtual ~CEventDaemon_Action();

	// [UNIMPLEMENTED - 0x00B02160 is a source-path string, not a function]
	// Triggers automated server-wide events and scheduled scripts
	bool ExecuteEventAction(uint32_t dwActionID, const char* pszParam);
};

#endif // _SR_GAMESERVER_EVENTDAEMON_ACTION_H_
