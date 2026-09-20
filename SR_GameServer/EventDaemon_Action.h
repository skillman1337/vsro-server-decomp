/**
 * ============================================================================
 * Silkroad Online - Event Daemon Action Dispatcher
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\EventDaemon_Action.h
 *
 * Implements:
 *   - CEventDaemon_Action @ 0x00B02160
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

	// [RECONSTRUCTED - Native 0x00B02160]
	// Triggers automated server-wide events and scheduled scripts
	bool ExecuteEventAction(uint32_t dwActionID, const char* pszParam);
};

#endif // _SR_GAMESERVER_EVENTDAEMON_ACTION_H_
