/**
 * ============================================================================
 * Silkroad Online - SR_GameServer Application Entry
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SR_GameServer.cpp
 *
 * Implements the application entry point matching native App_WinMain @ 0x00401110.
 *
 * 00401110  mov     eax, [esp+0xc]       ; lpCmdLine
 * 00401114  mov     ecx, [esp+0x4]       ; hInstance
 * 00401118  push    eax
 * 00401119  push    ecx
 * 0040111a  mov     dword [g_pServerApp], 0xcc3880 ; &g_gameServer
 * 00401126  call    Server_Main
 * 0040112b  pop     ecx
 * 0040112c  pop     ecx
 * 0040112d  retn    0x10
 * ============================================================================
 */

#include "GameServer.h"
#include "../JMX_ServerFramework/ServerFramework/ServerMain.h"
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/**
 * App_WinMain
 * Native implementation: 0x00401110
 */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nCmdShow*/) {
	// Native 0x0040111A: g_pServerApp = &g_gameServer
	ServerFramework::g_pServerApp = &g_gameServer;

	// Native 0x00401126: return Server_Main(hInstance, lpCmdLine)
	return ServerFramework::Server_Main(hInstance, lpCmdLine);
}
#endif

/**
 * Console entry wrapper for CLI execution / testing
 */
int main(int argc, char* argv[]) {
	std::string cmdLine;
	for (int i = 1; i < argc; ++i) {
		if (i > 1) cmdLine += " ";
		cmdLine += argv[i];
	}

#ifdef _WIN32
	return WinMain(GetModuleHandleA(nullptr), nullptr, const_cast<char*>(cmdLine.c_str()), SW_SHOW);
#else
	ServerFramework::g_pServerApp = &g_gameServer;
	return ServerFramework::Server_Main(nullptr, cmdLine.c_str());
#endif
}
