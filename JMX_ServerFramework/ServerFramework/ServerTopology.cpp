/**
 * ============================================================================
 * Silkroad Online - Server Topology & Routing Management Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerFramework\ServerTopology.cpp
 *
 * Implements server cluster state propagation, topology node/link registries,
 * cluster routing cleanup, and inter-manager broadcast notifications
 * matching native 0x009396A0 - 0x00957C00:
 *   - ServerFramework_SetStateObserver          @ 0x009396A0
 *   - ServerFramework_CleanupServerRouting      @ 0x009396B0
 *   - ServerFramework_GetServerLinks            @ 0x0093A350
 *   - ServerFramework_CountServerLinks          @ 0x0093A3E0
 *   - ServerFramework_BuildClusterRouteRecursive @ 0x0093A440
 *   - ServerFramework_BuildClusterRoutes        @ 0x0093A600
 *   - ServerFramework_SetServerListsAndBuildRoutes @ 0x0093A980
 *   - ServerFramework_FindServerNode            @ 0x0093B0F0
 *   - ServerFramework_FindServerByType          @ 0x0093DA40
 *   - ServerFramework_GetOtherServerNode        @ 0x0093B140
 *   - ServerFramework_GetPeerServerNode         @ 0x0093B170
 *   - ServerFramework_FindServerLink            @ 0x0093B180
 *   - ServerFramework_FindServerNodeByName      @ 0x0093B530
 *   - ServerFramework_NotifyServerStateChange   @ 0x0093B650
 *   - ServerFramework_NotifyServerLinkStateChange @ 0x0093B760
 *   - ServerFramework_CleanupProxyRoutes        @ 0x0093DAE0
 *   - ServerFramework_BroadcastPacketToManagers @ 0x0093DDD0
 *   - ServerFramework_CleanupServerNodes        @ 0x00957BF0
 *   - ServerFramework_CleanupServerNodesInternal @ 0x00957C00
 * ============================================================================
 */

#include "ServerTopology.h"
#include "ServerMain.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/BSLib/Synch.h"
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ServerFramework {

// 10 Global Server Lists allocated dynamically during certification (Native 0x00C827A0 - 0x00C827C8)
std::list<CServerNode*>* g_pListServers_Shard          = nullptr; // 0x00C827A0
std::list<CServerNode*>* g_pListServers_GlobalManager  = nullptr; // 0x00C827A4
std::list<CServerNode*>* g_pListServers_MachineManager = nullptr; // 0x00C827A8
std::list<CServerNode*>* g_pListServers_GatewayServer  = nullptr; // 0x00C827AC
std::list<CServerNode*>* g_pListServers_DownloadServer = nullptr; // 0x00C827B0
std::list<CServerNode*>* g_pListServers_Certification  = nullptr; // 0x00C827B4
std::list<CServerNode*>* g_pListServers_GameServer     = nullptr; // 0x00C827B8
std::list<CServerLink*>* g_pListServers_AgentServer    = nullptr; // 0x00C827BC (tagServerLink* links)
std::list<CServerNode*>* g_pListServers_FarmServer     = nullptr; // 0x00C827C0
std::list<CServerNode*>* g_pListServers_Proxy          = nullptr; // 0x00C827C8
std::list<CServerNode*>* g_pListServers_ProxyAux       = nullptr; // 0x00C827CC

// Download & Gateway File Transfer Routing Containers (Native 0x00D678D4, 0x00D678E0)
std::map<uint32_t, void*>        g_mapDownloadTransferEntries; // 0x00D678D4
std::map<uint32_t, void*>        g_mapDownloadFileLinks;       // 0x00D678E0 (stdext::hash_map)

// Cluster Manager Broadcast Sets (Native 0x00D6788C, 0x00D678BC, 0x00D678C8)
std::set<uint16_t>                g_setNotifyServerIDs;       // 0x00D678BC: Set of Server IDs receiving notifications
std::set<uint32_t>                g_setDirectNotifySessions;  // 0x00D678C8: Set of direct manager session IDs
std::map<uint16_t, CServerLink*>  g_mapServerLinkIndex;       // 0x00D6788C: Server ID to Link lookup index

// 13 Global Static Topology & Routing Containers (Native 0x00D67818 - 0x00D678C8)
std::map<uint16_t, void*>              g_mapServersByType;         // 0x00D67818
std::map<uint8_t, CServerNode*>        g_mapServersByCluster;      // 0x00D67824
std::map<uint16_t, CServerNode*>       g_mapServerNodes;           // 0x00D67830
std::map<uint32_t, CServerLink*>       g_mapServerLinks;           // 0x00D6783C
std::map<uint32_t, tagServerEndpoint*> g_mapServerEndpoints;       // 0x00D67848
std::map<std::string, CServerNode*> g_mapServerNodesByName;  // 0x00D67854
std::list<void*>                 g_listManagerConnections;   // 0x00D67864
std::vector<void*>               g_vecPeerConnections;       // 0x00D67870
std::list<void*>                 g_listClusterPaths;         // 0x00D6788C / 0x00D67890
std::vector<void*>               g_vecTopologyLinks;         // 0x00D67898
std::map<uint32_t, CServerLink*> g_mapServerLinksBySession;  // 0x00D678B0

// Architecture View Window Handle (Native 0x00C827FC) is defined in ServerArchitectureView.cpp
// HWND g_hWndServerArchitectureView; (extern in ServerTopology.h)

// Cluster Server Node Manager Synchronization Guard (Native 0x00D67B40)
CCriticalSectionBS g_csServerNodeManager("ServerNodeManager");

// Cluster Server Node Registry Collections (Native 0x00D67B04, 0x00D67B28, 0x00D67B34)
std::list<tagServerNode*>                     g_listServerNodes;
std::map<uint32_t, std::list<tagServerNode*>> g_mapServerNodesByType;
std::map<uint16_t, tagServerNode*>            g_mapServerNodesByID;

// Active Cluster Dynamic Server Node Lists (Native 0x00C827E4, 0x00C827E8, 0x00C827EC)
std::list<tagServerNode*>*                    g_pListServerNodes     = nullptr;
std::list<tagServerNode*>*                    g_pListRegisteredNodes = nullptr;
std::list<tagServerNode*>*                    g_pListActiveNodes     = nullptr;

// Cached Local Server Node Type Code (Native 0x00C82798)
uint32_t                                      g_dwLocalServerType    = 0;

/**
 * [RECONSTRUCTED - 0x00939690]
 * ServerFramework_CacheLocalServerType
 * Native implementation @ 0x00939690 (16 bytes)
 *
 * Machine Bytes:
 *   00939690  a19c27c800            mov     eax, [0x00C8279C] ; g_pLocalServerInfo
 *   00939695  0fb64802              movzx   ecx, byte [eax+0x2] ; wServerType
 *   00939699  890d9827c800          mov     [0x00C82798], ecx ; g_dwLocalServerType
 *   0093969f  c3                    retn
 */
void ServerFramework_CacheLocalServerType() {
	if (g_pLocalServerInfo != nullptr) {
		g_dwLocalServerType = static_cast<uint32_t>(g_pLocalServerInfo->wServerType & 0xFF);
	}
}

/**
 * [RECONSTRUCTED - 0x009396A0]
 * ServerFramework_SetStateObserver
 * Native implementation @ 0x009396A0
 *
 * Machine Bytes: a3 c4 27 c8 00 c3 (mov [0x00C827C4], eax; retn)
 * Registers the observer callback sink (called by CServerProcessMain ctor @ 0x00948D59).
 */
void ServerFramework_SetStateObserver(IServerStateObserver* pObserver) {
	g_pServerStateObserver = pObserver;
}

/**
 * Helper template to safely destroy dynamic STL lists of allocated items
 * matching the exact MSVC loop in 0x009396BD - 0x0093975E:
 *   1. Iterates nodes: deletes item payload at node->_Myval (+0x08)
 *   2. Clears nodes (std_list_clear_nodes @ 0x00695450)
 *   3. Deletes list container (thunk_CRT_operator_delete @ 0x009DD03D)
 *   4. Zeroes global pointer
 */
template <typename T>
static inline void SafeDestroyServerList(std::list<T*>*& pList) {
	if (pList != nullptr) {
		for (auto* pItem : *pList) {
			delete pItem;
		}
		pList->clear();
		delete pList;
		pList = nullptr;
	}
}

/**
 * [RECONSTRUCTED - 0x0093DAE0]
 * ServerFramework_CleanupProxyRoutes
 * Native implementation @ 0x0093DAE0 (1,087 bytes)
 *
 * Full destruction and routing cache clearance for proxy and file download topology:
 *   Phase 1 (0x0093DAE7 - 0x0093DB90):
 *     Iterates and deletes all node payloads in g_pListServers_Proxy (0x00C827C8),
 *     clears the list node chain, frees the list container, and zeroes the pointer.
 *   Phase 2 (0x0093DB98 - 0x0093DC59):
 *     Iterates and deletes all node payloads in g_pListServers_ProxyAux (0x00C827CC),
 *     clears the list node chain, frees the list container, and zeroes the pointer.
 *   Phase 3 (0x0093DC5F - 0x0093DCAD):
 *     Recursively frees all tree nodes in g_mapDownloadTransferEntries (0x00D678D4)
 *     via std_tree_erase_nodes (0x005A3BE0), restores head sentinel node pointers,
 *     and resets container size to 0.
 *   Phase 4 (0x0093DCB0 - 0x0093DD56):
 *     Clears list elements and frees node buffers in g_mapDownloadFileLinks (0x00D678E0),
 *     resets hash bucket vector ranges via std_vector_reallocate_clear_Proxy (0x00944F90),
 *     and resets load factor and growth factor floats to 1.0f.
 *   Phase 5 (0x0093DD4B - 0x0093DD5F):
 *     Returns true (1).
 */
bool ServerFramework_CleanupProxyRoutes() {
	// Phase 1: Primary proxy servers list (Native 0x0093DAE7 - 0x0093DB90)
	SafeDestroyServerList(g_pListServers_Proxy);

	// Phase 2: Auxiliary proxy servers list (Native 0x0093DB98 - 0x0093DC59)
	SafeDestroyServerList(g_pListServers_ProxyAux);

	// Phase 3: Download transfer routing tree (Native 0x0093DC5F - 0x0093DCAD)
	g_mapDownloadTransferEntries.clear();

	// Phase 4: Download file link hash map (Native 0x0093DCB0 - 0x0093DD56)
	g_mapDownloadFileLinks.clear();

	return true;
}

/**
 * [RECONSTRUCTED - 0x0093DD70]
 * ServerFramework_FindDownloadFileLink
 * Native implementation @ 0x0093DD70 (68 bytes)
 *
 * Looks up download file transfer record in g_mapDownloadFileLinks (0x00D678E0)
 * by file ID.
 */
void* ServerFramework_FindDownloadFileLink(int32_t nFileID) {
	auto it = g_mapDownloadFileLinks.find(static_cast<uint32_t>(nFileID));
	if (it != g_mapDownloadFileLinks.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x009396B0]
 * ServerFramework_CleanupServerRouting
 * Native implementation @ 0x009396B0 (5,319 bytes)
 *
 * Toggles and clears all cluster routing tables, lists, and static topology maps on shutdown.
 */
void ServerFramework_CleanupServerRouting() {
	// 1. Purge 9 heap-allocated server list pointers (0x00C827A0 - 0x00C827C0)
	SafeDestroyServerList(g_pListServers_GlobalManager);  // 0x009396BD
	SafeDestroyServerList(g_pListServers_MachineManager); // 0x00939768
	SafeDestroyServerList(g_pListServers_GatewayServer);  // 0x0093982F
	SafeDestroyServerList(g_pListServers_DownloadServer); // 0x009398F0
	SafeDestroyServerList(g_pListServers_Certification);  // 0x009399B1
	SafeDestroyServerList(g_pListServers_Shard);          // 0x00939A72
	SafeDestroyServerList(g_pListServers_GameServer);     // 0x00939B33
	SafeDestroyServerList(g_pListServers_AgentServer);    // 0x00939BF4
	SafeDestroyServerList(g_pListServers_FarmServer);     // 0x00939CBF

	// 2. Clear static .data routing containers (0x00D67818 - 0x00D678D0)
	g_listClusterPaths.clear();         // 0x00939D80
	g_vecTopologyLinks.clear();         // 0x00939DE5
	g_listManagerConnections.clear();   // 0x00939E43
	g_vecPeerConnections.clear();       // 0x0093A017
	g_mapServerNodes.clear();           // 0x0093A07E
	g_mapServerLinks.clear();           // 0x0093A0CE
	g_mapServersByCluster.clear();      // 0x0093A11E
	g_mapServerNodesByName.clear();     // 0x0093A16E (Native 0x00D67854)
	for (auto& pair : g_mapServerEndpoints) {
		delete pair.second;
	}
	g_mapServerEndpoints.clear();       // 0x0093A19F
	g_mapServerLinksBySession.clear();  // 0x0093A1EE (Native 0x00D678B0)
	g_mapServersByType.clear();         // 0x0093A23E
	g_setNotifyServerIDs.clear();       // 0x0093A28E (Native 0x00D678BC)
	g_setDirectNotifySessions.clear();  // 0x0093A2DE (Native 0x00D678C8)

	// 3. Purge proxy route list
	ServerFramework_CleanupProxyRoutes(); // 0x0093A32E
}

/**
 * [RECONSTRUCTED - 0x0093B0A0]
 * ServerFramework_FindServerEndpoint
 * Native implementation @ 0x0093B0A0 (188 bytes)
 * Calling convention: __cdecl (dwEndpointID @ [esp+4])
 *
 * Looks up endpoint in g_mapServerEndpoints (0x00D67848) by 32-bit endpoint key.
 * Returns tagServerEndpoint* or nullptr if not found.
 *
 * Machine Bytes:
 *   0093b0a0  83ec08            sub     esp, 0x8
 *   0093b0a3  56                push    esi
 *   0093b0a4  57                push    edi
 *   0093b0a5  8d742414          lea     esi, [esp+0x14]  ; &dwEndpointID
 *   0093b0a9  8d7c2408          lea     edi, [esp+0x8]   ; &resultIter
 *   0093b0ad  e84e390000        call    std_map_uint32_find_Endpoints
 *   0093b0c6  3b054c78d600      cmp     eax, [0x00D6784C] ; cmp iter._Ptr, g_mapServerEndpoints_pHead
 *   0093b0e0  8b4010            mov     eax, [eax+0x10]  ; return pair.second
 */
tagServerEndpoint* ServerFramework_FindServerEndpoint(uint32_t dwEndpointID) {
	auto it = g_mapServerEndpoints.find(dwEndpointID);
	if (it != g_mapServerEndpoints.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093B0F0]
 * ServerFramework_FindServerNode
 * Native implementation @ 0x0093B0F0 (73 bytes)
 *
 * Looks up a CServerNode in the cluster topology map g_mapServerNodes (0x00D67830) by wServerID.
 *
 * Machine Bytes & Disassembly trace:
 *   0093b0f0  83ec08            sub     esp, 0x8
 *   0093b0f3  56                push    esi
 *   0093b0f4  57                push    edi
 *   0093b0f5  8d742414          lea     esi, [esp+0x14]  ; &wServerID
 *   0093b0f9  8d7c2408          lea     edi, [esp+0x8]   ; &resultIter
 *   0093b0fd  e8fe320000        call    std_map_uint16_find_ServerNodes
 *   0093b102  8b4c2408          mov     ecx, [esp+0x8]   ; map pointer
 *   0093b106  85c9              test    ecx, ecx
 *   0093b10a  81f93078d600      cmp     ecx, 0x00D67830  ; verify map base pointer
 *   0093b112  8b44240c          mov     eax, [esp+0xc]   ; iter._Ptr
 *   0093b116  3b053478d600      cmp     eax, [0x00D67834]; cmp iter._Ptr, g_mapServerNodes_pHead
 *   0093b11c  7508              jne     0x0093B126
 *   0093b11e  33c0              xor     eax, eax         ; not found -> return nullptr
 *   0093b130  8b4010            mov     eax, [eax+0x10]  ; [PROVEN OFFSET +0x10]: return pair.second (CServerNode*)
 *   0093b138  c3                retn
 *
 * [PARTIAL / FALLBACK NOTE]:
 * Native 0x0093B0F0 performs a direct lookup in g_mapServerNodes with no fallback.
 * The fallback to g_pLocalServerInfo is maintained during offline/early bootstrap
 * before full cluster handshake registration occurs.
 */
CServerNode* ServerFramework_FindServerNode(uint16_t wServerID) {
	auto it = g_mapServerNodes.find(wServerID);
	if (it != g_mapServerNodes.end()) {
		return it->second;
	}

	// Offline / Early-boot fallback (before node map registration)
	if (g_pLocalServerInfo && g_pLocalServerInfo->wServerID == wServerID) {
		return g_pLocalServerInfo;
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093DA40]
 * ServerFramework_FindServerByType
 * Native implementation @ 0x0093DA40 (185 bytes)
 *
 * Resolves a server node descriptor registered in g_mapServersByType (0x00D67818)
 * by its 16-bit server type identifier (e.g. shard server node).
 *
 * Calling Convention: __cdecl (wServerType passed on stack)
 *
 * Exact Machine Operations:
 *   0093da40  83ec08              sub     esp, 0x8
 *   0093da43  56                  push    esi
 *   0093da44  57                  push    edi
 *   0093da45  8d742414            lea     esi, [esp+0x14]  ; &wServerType
 *   0093da49  8d7c2408            lea     edi, [esp+0x8]   ; &result_iter
 *   0093da4d  e8de050000          call    0x0093E030       ; std_map_uint16_find_ServersByType
 *   0093da52  8b08                mov     ecx, [eax]       ; ecx = result_iter._Mycont
 *   0093da54  85c9                test    ecx, ecx
 *   0093da56  8b4004              mov     eax, [eax+0x4]   ; eax = result_iter._Ptr
 *   0093da59  742b                je      0x0093DA86       ; throw if null container
 *   0093da5b  81f91878d600        cmp     ecx, 0x00D67818  ; verify container base
 *   0093da61  7523                jne     0x0093DA86
 *   0093da63  3b051c78d600        cmp     eax, [0x00D6781C]; compare with end() sentinel
 *   0093da69  7508                jne     0x0093DA73
 *   0093da6b  33c0                xor     eax, eax         ; not found -> return nullptr
 *   0093da6d  5f                  pop     edi
 *   0093da6e  5e                  pop     esi
 *   0093da6f  83c408              add     esp, 0x8
 *   0093da72  c3                  retn
 *   0093da73  3b4104              cmp     eax, [ecx+0x4]   ; safety check before dereferencing
 *   0093da76  7505                jne     0x0093DA7D
 *   0093da78  e9a3470000          jmp     0x00942220       ; throw out_of_range
 *   0093da7d  8b4010              mov     eax, [eax+0x10]  ; return node->value (pair.second: CServerNode*)
 *   0093da80  5f                  pop     edi
 *   0093da81  5e                  pop     esi
 *   0093da82  add     esp, 0x8
 *   0093da85  c3                  retn
 *   0093da86  e945220000          jmp     0x0093FCD0       ; throw invalid_argument
 */
CServerNode* ServerFramework_FindServerByType(uint16_t wServerType) {
	auto it = g_mapServersByType.find(wServerType);
	if (it != g_mapServersByType.end()) {
		return static_cast<CServerNode*>(it->second);
	}
	return nullptr;
}

void* ServerFramework_FindServerByType(uint32_t dwServerType) {
	return ServerFramework_FindServerByType(static_cast<uint16_t>(dwServerType));
}

/**
 * [RECONSTRUCTED - 0x0093B140]
 * ServerFramework_GetOtherServerNode
 * Native implementation @ 0x0093B140 (45 bytes)
 *
 * In-register calling convention (__fastcall):
 *   ESI = pLink (CServerLink*)
 *   EDI = pNode (CServerNode*)
 *
 * Machine Bytes:
 *   0093b140  51                push    ecx
 *   0093b141  0fb707            movzx   eax, word [edi]     ; eax = pNode->wServerID (offset 0)
 *   0093b144  66394604          cmp     word [esi+0x4], ax  ; cmp pLink->wServer1, ax
 *   0093b148  740b              je      0x0093B155
 *   0093b14a  66394606          cmp     word [esi+0x6], ax  ; cmp pLink->wServer2, ax
 *   0093b14e  740b              je      0x0093B155
 *   0093b150  e80b9a0200        call    0x00964B60          ; Fatal Assertion (BSLib::AssertFailed / GenerateMiniDump)
 *   0093b155  0fb74604          movzx   eax, word [esi+0x4] ; eax = pLink->wServer1
 *   0093b159  663b07            cmp     ax, word [edi]      ; if (pLink->wServer1 == pNode->wServerID)
 *   0093b15c  7504              jne     0x0093B162
 *   0093b15e  0fb74606          movzx   eax, word [esi+0x6] ; eax = pLink->wServer2
 *   0093b162  50                push    eax                 ; push wOtherServerID
 *   0093b163  e888ffffff        call    0x0093B0F0          ; ServerFramework_FindServerNode(wOtherServerID)
 *   0093b168  83c404            add     esp, 0x4
 *   0093b16b  59                pop     ecx
 *   0093b16c  c3                retn
 */
CServerNode* ServerFramework_GetOtherServerNode(CServerLink* pLink, CServerNode* pNode) {
	if (!pLink || !pNode) {
		return nullptr;
	}

	uint16_t wNodeID = pNode->wServerID;
	if (pLink->wServer1 != wNodeID && pLink->wServer2 != wNodeID) {
		// Native 0x0093B150: Fatal topology invariant violation
		BSLib::AssertFailed(__FILE__, __LINE__);
		return nullptr;
	}

	uint16_t wOtherID = (pLink->wServer1 == wNodeID) ? pLink->wServer2 : pLink->wServer1;
	return ServerFramework_FindServerNode(wOtherID);
}

CServerNode* ServerFramework_GetOtherServerNode(CServerLink* pLink, uint16_t wServerID) {
	if (!pLink) {
		return nullptr;
	}

	if (pLink->wServer1 != wServerID && pLink->wServer2 != wServerID) {
		BSLib::AssertFailed(__FILE__, __LINE__);
		return nullptr;
	}

	uint16_t wOtherID = (pLink->wServer1 == wServerID) ? pLink->wServer2 : pLink->wServer1;
	return ServerFramework_FindServerNode(wOtherID);
}

/**
 * [RECONSTRUCTED - 0x0093B170]
 * ServerFramework_GetPeerServerNode
 * Native implementation @ 0x0093B170 (14 bytes)
 *
 * Machine Bytes:
 *   0093b170  57                push    edi
 *   0093b171  8b3d9c27c800      mov     edi, dword [0x00C8279C] ; edi = g_pLocalServerInfo
 *   0093b177  e8c4ffffff        call    0x0093B140              ; ServerFramework_GetOtherServerNode(pLink, g_pLocalServerInfo)
 *   0093b17c  5f                pop     edi
 *   0093b17d  c3                retn
 */
CServerNode* ServerFramework_GetPeerServerNode(CServerLink* pLink) {
	if (!g_pLocalServerInfo) {
		return nullptr;
	}
	return ServerFramework_GetOtherServerNode(pLink, g_pLocalServerInfo);
}

/**
 * [RECONSTRUCTED - 0x0093B4D0]
 * ServerFramework_FindClusterServer
 * Native implementation @ 0x0093B4D0 (197 bytes)
 * Calling convention: __cdecl (byClusterID @ [esp+4])
 *
 * Looks up cluster server node in g_mapServersByCluster (0x00D67824) by 8-bit cluster ID.
 * Returns CServerNode* or nullptr if not found.
 *
 * Machine Bytes:
 *   0093b4d0  83ec0c            sub     esp, 0xc
 *   0093b4d3  833dc027c80000    cmp     dword [0x00C827C0], 0   ; cmp g_pListServers_FarmServer, 0
 *   0093b4da  57                push    edi
 *   0093b4db  7505              jne     0x0093B4E2
 *   0093b4dd  e87e960200        call    ServerFramework_GenerateMiniDump (0x00964B60)
 *   0093b4e2  8d442408          lea     eax, [esp+0x8]          ; &resultIter
 *   0093b4e6  50                push    eax
 *   0093b4e7  8d7c2418          lea     edi, [esp+0x18]         ; &byClusterID
 *   0093b4eb  e8202d0000        call    std_map_uint8_find_ClusterServers (0x0093E210)
 *   0093b501  3b052878d600      cmp     eax, [0x00D67828]       ; cmp eax, g_mapServersByCluster._Myhead (end())
 *   0093b509  33c0              xor     eax, eax                ; return nullptr
 *   0093b51a  8b4010            mov     eax, [eax+0x10]         ; return pair.second (CServerNode*)
 */
CServerNode* ServerFramework_FindClusterServer(uint8_t byClusterID) {
	if (g_pListServers_FarmServer == nullptr) {
		BSLib::GenerateMiniDump();
	}
	auto it = g_mapServersByCluster.find(byClusterID);
	if (it != g_mapServersByCluster.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093B180]
 * ServerFramework_FindServerLink
 * Native implementation @ 0x0093B180 (76 bytes)
 *
 * Looks up a CServerLink in g_mapServerLinks (0x00D6783C) by composite Link ID.
 *
 * Machine Bytes:
 *   0093b180  83ec08            sub     esp, 0x8
 *   0093b183  53                push    ebx
 *   0093b184  56                push    esi
 *   0093b185  8d5c2414          lea     ebx, [esp+0x14]  ; &dwLinkID
 *   0093b189  8d442408          lea     eax, [esp+0x8]   ; &resultIter
 *   0093b18d  be3c78d600        mov     esi, 0xd6783c    ; &g_mapServerLinks
 *   0093b192  e8f96ec6ff        call    std_map_uint32_find
 *   0093b1a7  3b0d4078d600      cmp     ecx, [0xd67840]  ; is end()?
 *   0093b1c1  8b4110            mov     eax, [ecx+0x10]  ; return pair.second (CServerLink*)
 */
CServerLink* ServerFramework_FindServerLink(uint32_t dwLinkID) {
	auto it = g_mapServerLinks.find(dwLinkID);
	if (it != g_mapServerLinks.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093B1D0]
 * ServerFramework_FindServerLinkBySession
 * Native implementation @ 0x0093B1D0 (76 bytes)
 *
 * Looks up a CServerLink in g_mapServerLinksBySession (0x00D678B0) by Session ID.
 */
CServerLink* ServerFramework_FindServerLinkBySession(uint32_t dwSessionID) {
	auto it = g_mapServerLinksBySession.find(dwSessionID);
	if (it != g_mapServerLinksBySession.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093B220]
 * ServerFramework_GetGameServerList
 * Native implementation @ 0x0093B220 (6 bytes)
 *
 * Machine Bytes:
 *   0093b220  a1b827c800        mov     eax, [0x00C827B8] ; g_pListServers_GameServer
 *   0093b225  c3                retn
 */
std::list<CServerNode*>* ServerFramework_GetGameServerList() {
	return g_pListServers_GameServer;
}

/**
 * [RECONSTRUCTED - 0x0093B240]
 * ServerFramework_SetFarmServerList
 * Native implementation @ 0x0093B240 (651 bytes)
 *
 * Configures g_pListServers_FarmServer (0x00C827C0), iterates each node to bind
 * cluster map (g_mapServersByCluster @ 0x00D67824) and endpoint lookup
 * (g_mapServerEndpoints @ 0x00D67848).
 */
bool ServerFramework_SetFarmServerList(std::list<CServerNode*>* pList) {
	if (!pList || pList->empty()) {
		return false;
	}

	g_pListServers_FarmServer = pList;

	for (CServerNode* pNode : *pList) {
		if (!pNode) continue;
		// First byte at node buffer is Cluster ID
		uint8_t byClusterID = *reinterpret_cast<const uint8_t*>(pNode);
		g_mapServersByCluster[byClusterID] = pNode;

		// Following bytes contain endpoint string
		const char* szEndpoint = reinterpret_cast<const char*>(pNode) + 1;
		auto* pEndpoint = new tagServerEndpoint();
		pEndpoint->strEndpoint = szEndpoint;
		pEndpoint->dwServerID = static_cast<uint32_t>(pNode->wServerID);
		g_mapServerEndpoints[pNode->wServerID] = pEndpoint;
	}

	return true;
}

/**
 * [NATIVE - 0x00958610]
 * ServerFramework_ProcessDynamicServerNodes
 * Native implementation @ 0x00958610 (666 bytes)
 *
 * Acquires g_csServerNodeManager (0x00D67B40) and applies dynamic server node updates:
 *   - Deserializes dynamic server nodes (289 bytes per node, loop delimited by sentinel 1)
 *   - Deserializes dynamic cluster links (324 bytes per link, loop delimited by sentinel 1)
 *   - Deserializes dynamic server states (5 bytes per state, loop delimited by sentinel 1)
 *   - Updates global dynamic node lists (g_pListServerNodes, g_pListRegisteredNodes, g_pListActiveNodes)
 *   - Synchronizes topology routing maps
 */
bool ServerFramework_ProcessDynamicServerNodes(CMassiveMsg* pMsg) {
	if (!pMsg) {
		return false;
	}
	g_csServerNodeManager.Lock();

	auto* pNewNodes = new std::list<tagServerNode*>();
	auto* pNewLinks = new std::list<tagServerNode*>();
	auto* pNewStates = new std::list<tagServerNode*>();

	uint8_t bySentinel = 0;
	// 1. Read dynamic server nodes (0x121 = 289 bytes each)
	if (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1) {
		do {
			auto* pBuffer = new uint8_t[0x121]();
			pMsg->ReadBytes(pBuffer, 0x121);
			pNewNodes->push_back(reinterpret_cast<tagServerNode*>(pBuffer));
		} while (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1);
	}

	// 2. Read dynamic cluster links (0x144 = 324 bytes each)
	if (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1) {
		do {
			auto* pBuffer = new uint8_t[0x144]();
			pMsg->ReadBytes(pBuffer, 0x144);
			pNewLinks->push_back(reinterpret_cast<tagServerNode*>(pBuffer));
		} while (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1);
	}

	// 3. Read dynamic server states (5 bytes each)
	if (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1) {
		do {
			auto* pBuffer = new uint8_t[5]();
			pMsg->ReadBytes(pBuffer, 5);
			pNewStates->push_back(reinterpret_cast<tagServerNode*>(pBuffer));
		} while (pMsg->ReadBytes(&bySentinel, 1) && bySentinel == 1);
	}

	// Native sub_9580a0: integrate dynamic lists
	if (g_pListServerNodes) {
		delete g_pListServerNodes;
	}
	if (g_pListRegisteredNodes) {
		delete g_pListRegisteredNodes;
	}
	if (g_pListActiveNodes) {
		delete g_pListActiveNodes;
	}

	g_pListServerNodes = pNewNodes;
	g_pListRegisteredNodes = pNewLinks;
	g_pListActiveNodes = pNewStates;

	g_csServerNodeManager.Unlock();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0093A440]
 * ServerFramework_BuildClusterRouteRecursive
 * Native implementation @ 0x0093A440 (447 bytes)
 *
 * Recursively explores paths between server nodes across CServerLink edges,
 * tracking hop distances in distanceMap to prevent cycles.
 * Returns next hop link ID towards target, or 0 if no path.
 */
int32_t ServerFramework_BuildClusterRouteRecursive(
	uint16_t wCurrentNodeID,
	uint16_t wTargetNodeID,
	int32_t nPreviousLinkID,
	void* pRouteMap,
	std::list<CServerLink*>* pVisitedLinks
) {
	auto* pDistMap = static_cast<std::map<uint16_t, int32_t>*>(pRouteMap);
	int32_t currentDist = (*pDistMap)[wCurrentNodeID];

	std::list<CServerLink*> localLinks;
	ServerFramework_GetServerLinks(wCurrentNodeID, localLinks);

	for (CServerLink* pLink : localLinks) {
		if (!pLink || static_cast<int32_t>(pLink->dwLinkID) == nPreviousLinkID) {
			continue;
		}

		CServerNode* pOther = ServerFramework_GetOtherServerNode(pLink, wCurrentNodeID);
		if (!pOther) {
			continue;
		}

		if (pOther->wServerID == wTargetNodeID) {
			(*pDistMap)[pOther->wServerID] = currentDist + 1;
			if (pVisitedLinks) {
				pVisitedLinks->push_back(pLink);
			}
			return static_cast<int32_t>(pLink->dwLinkID);
		}

		auto it = pDistMap->find(pOther->wServerID);
		if (it == pDistMap->end() || it->second == -1) {
			(*pDistMap)[pOther->wServerID] = currentDist + 1;
			int32_t nextHop = ServerFramework_BuildClusterRouteRecursive(
				pOther->wServerID,
				wTargetNodeID,
				static_cast<int32_t>(pLink->dwLinkID),
				pRouteMap,
				pVisitedLinks
			);
			if (nextHop != 0) {
				if (pVisitedLinks) {
					pVisitedLinks->push_back(pLink);
				}
				return static_cast<int32_t>(pLink->dwLinkID);
			}
		}
	}

	return 0;
}

/**
 * [RECONSTRUCTED - 0x0093A600]
 * ServerFramework_BuildClusterRoutes
 * Native implementation @ 0x0093A600 (980 bytes)
 *
 * For every GameServer node in g_pListServers_GameServer:
 * Identifies direct CServerLink connections and shortest multi-hop paths to
 * g_pLocalServerInfo, registering routing links in g_vecTopologyLinks.
 */
int32_t ServerFramework_BuildClusterRoutes() {
	if (!g_pListServers_GameServer || !g_pLocalServerInfo) {
		return 0;
	}

	g_vecTopologyLinks.clear();

	for (CServerNode* pTarget : *g_pListServers_GameServer) {
		if (!pTarget || pTarget == g_pLocalServerInfo) {
			continue;
		}

		// First, check direct links connecting local server to target server
		std::list<CServerLink*> directLinks;
		ServerFramework_GetServerLinks(g_pLocalServerInfo->wServerID, directLinks);

		bool bDirectFound = false;
		for (CServerLink* pLink : directLinks) {
			if (!pLink) continue;
			CServerNode* pOther = ServerFramework_GetOtherServerNode(pLink, g_pLocalServerInfo);
			if (pOther == pTarget) {
				// Direct route
				g_vecTopologyLinks.push_back(pLink);
				bDirectFound = true;
				break;
			}
		}

		if (bDirectFound) {
			continue;
		}

		// Multi-hop path search
		std::map<uint16_t, int32_t> distMap;
		for (CServerNode* pNode : *g_pListServers_GameServer) {
			if (pNode) {
				distMap[pNode->wServerID] = -1;
			}
		}
		distMap[g_pLocalServerInfo->wServerID] = 0;

		std::list<CServerLink*> pathLinks;
		int32_t nextHopLinkID = ServerFramework_BuildClusterRouteRecursive(
			g_pLocalServerInfo->wServerID,
			pTarget->wServerID,
			0,
			&distMap,
			&pathLinks
		);

		if (nextHopLinkID != 0) {
			CServerLink* pLink = ServerFramework_FindServerLink(static_cast<uint32_t>(nextHopLinkID));
			if (pLink) {
				g_vecTopologyLinks.push_back(pLink);
			}
		}
	}

	return 1;
}

/**
 * [RECONSTRUCTED - 0x0093A980]
 * ServerFramework_SetServerListsAndBuildRoutes
 * Native implementation @ 0x0093A980 (2,571 bytes)
 *
 * Binds the 8 remaining server lists to global pointers, cross-links download servers to
 * gateways, registers shard endpoints, resolves GameServer relations (clusters, endpoints,
 * machine managers, gateways, certifications), indexes cluster links, generates shortest
 * routing paths, updates the architecture view GUI, and builds the notification server set.
 *
 * Calling convention: __cdecl (8 stack arguments)
 *
 * Exact Machine Instructions & Flow:
 *   0093a980  6aff              push    0xffffffff
 *   0093a982  68982dab00        push    0x00AB2D98              ; SEH handler
 *   0093a987  64a100000000      mov     eax, fs:[0x0]
 *   0093a98d  50                push    eax
 *   0093a98e  83ec34            sub     esp, 0x34
 *   0093a991  53                push    ebx
 *   0093a992  55                push    ebp
 *   0093a993  56                push    esi
 *   0093a994  57                push    edi
 *   0093a995  a18015c600        mov     eax, [0x00C61580]       ; __security_cookie
 *   0093a99a  33c4              xor     eax, esp
 *   0093a99c  50                push    eax
 *   0093a99d  8d442448          lea     eax, [esp+0x48]
 *   0093a9a1  64a300000000      mov     fs:[0x0], eax
 *   0093a9a7  8b742470          mov     esi, [esp+0x70]         ; esi = pGameServer
 *   0093a9ab  85f6              test    esi, esi
 *   0093a9ad  7505              jne     0x0093A9B4
 *   0093a9af  e8ac010200        call    0x00964B60              ; ServerFramework_GenerateMiniDump
 *   0093a9b4  837e0800          cmp     dword [esi+0x8], 0      ; pGameServer->size() > 0
 *   0093a9b8  7705              ja      0x0093A9BF
 *   0093a9ba  e8a1010200        call    0x00964B60              ; ServerFramework_GenerateMiniDump
 *   ...
 *   0093adc0  8b0da827c800      mov     ecx, [0x00C827FC]       ; g_hWndServerArchitectureView
 *   0093adcb  51                push    ecx
 *   0093adcc  ff1554109f00      call    IsWindow
 *   0093add0  a1fc27c800        mov     eax, [0x00C827FC]
 *   0093add5  6a00              push    0
 *   0093add7  6a00              push    0
 *   0093add9  68e8070000        push    0x7e8                   ; WM_USER + 0x3E8
 *   0093adde  50                push    eax
 *   0093addf  ff1550109f00      call    PostMessageA
 *   ...
 *   0093ae6a  68a412b400        push    "kek!!! why did you assign shard to common manager server? -_-+ plz check out server license table !!!"
 *   0093ae6f  e8bc8a0200        call    BSLog_ShowErrorMessage (0x00963930)
 *   ...
 *   0093af70  682012b400        push    "There is no server notify target!!! (check whether Farm has assigned shard or not...)"
 *   0093af75  e8b6890200        call    BSLog_ShowErrorMessage (0x00963930)
 *   ...
 *   0093b063  b001              mov     al, 0x1
 *   0093b078  c3                retn
 */
int32_t ServerFramework_SetServerListsAndBuildRoutes(
	std::list<CServerNode*>* pGlobalManager,
	std::list<CServerNode*>* pMachineManager,
	std::list<CServerNode*>* pGatewayServer,
	std::list<CServerNode*>* pDownloadServer,
	std::list<CServerNode*>* pCertification,
	std::list<CServerNode*>* pShard,
	std::list<CServerNode*>* pGameServer,
	std::list<CServerLink*>* pAgentServer
) {
	// Assertions matching native lines 0x0093A9AB - 0x0093A9BF
	if (!pGameServer || pGameServer->empty()) {
		BSLib::GenerateMiniDump();
		return 0;
	}

	// 1. Bind global list pointers (Native 0x0093A9C8 - 0x0093A9E8)
	g_pListServers_GlobalManager  = pGlobalManager;
	g_pListServers_Certification  = pCertification;
	g_pListServers_MachineManager = pMachineManager;
	g_pListServers_GatewayServer  = pGatewayServer;
	g_pListServers_DownloadServer = pDownloadServer;
	g_pListServers_Shard          = pShard;
	g_pListServers_GameServer     = pGameServer;
	g_pListServers_AgentServer    = pAgentServer;

	// 2. Cross-link DownloadServer to GatewayServer (Native 0x0093AA00 - 0x0093AA60)
	if (pDownloadServer && pGatewayServer) {
		for (CServerNode* pDownload : *pDownloadServer) {
			if (!pDownload) continue;
			uint8_t byDownloadID = *reinterpret_cast<const uint8_t*>(pDownload);
			for (CServerNode* pGateway : *pGatewayServer) {
				if (!pGateway) continue;
				uint8_t byGatewayID = *reinterpret_cast<const uint8_t*>(pGateway);
				if (byGatewayID == byDownloadID) {
					*reinterpret_cast<CServerNode**>(reinterpret_cast<uint8_t*>(pDownload) + 2) = pGateway;
					break;
				}
			}
		}
	}

	// 3. Register Shard endpoints into g_mapServerEndpoints (Native 0x0093AA65 - 0x0093AAB0)
	if (pShard) {
		for (CServerNode* pShardNode : *pShard) {
			if (!pShardNode) continue;
			uint32_t dwEndpointID = *reinterpret_cast<const uint32_t*>(pShardNode);
			auto* pEndpoint = new tagServerEndpoint();
			pEndpoint->dwServerID = dwEndpointID;
			g_mapServerEndpoints[dwEndpointID] = pEndpoint;
		}
	}

	// 4. Resolve local server info from the first GameServer record (Native 0x0093AAB4 - 0x0093AAC0)
	g_pLocalServerInfo = pGameServer->front();

	// 5. Initialize GameServer nodes & index them into g_mapServerNodes (Native 0x0093AAC8 - 0x0093AC90)
	for (CServerNode* pNode : *pGameServer) {
		if (!pNode) continue;
		uint8_t* pRaw = reinterpret_cast<uint8_t*>(pNode);

		// +0x0A: byClusterID -> ServerFramework_FindClusterServer (0x0093B4D0)
		uint8_t byClusterID = pRaw[0x0A];
		*reinterpret_cast<void**>(pRaw + 0x14) = ServerFramework_FindClusterServer(byClusterID);

		// +0x06: dwEndpointID -> ServerFramework_FindServerEndpoint (0x0093B0A0)
		uint32_t dwEndpointID = *reinterpret_cast<uint32_t*>(pRaw + 0x06);
		*reinterpret_cast<void**>(pRaw + 0x18) = ServerFramework_FindServerEndpoint(dwEndpointID);

		// +0x02: byMachineManagerID -> Match in g_pListServers_MachineManager -> +0x1C
		uint8_t byMachineManagerID = pRaw[0x02];
		*reinterpret_cast<void**>(pRaw + 0x1C) = nullptr;
		if (pMachineManager) {
			for (CServerNode* pMM : *pMachineManager) {
				if (pMM && *reinterpret_cast<uint8_t*>(pMM) == byMachineManagerID) {
					*reinterpret_cast<void**>(pRaw + 0x1C) = pMM;
					break;
				}
			}
		}

		// +0x03: byGatewayServerID -> Match in g_pListServers_GatewayServer -> +0x20
		uint8_t byGatewayServerID = pRaw[0x03];
		*reinterpret_cast<void**>(pRaw + 0x20) = nullptr;
		if (pGatewayServer) {
			for (CServerNode* pGW : *pGatewayServer) {
				if (pGW && *reinterpret_cast<uint8_t*>(pGW) == byGatewayServerID) {
					*reinterpret_cast<void**>(pRaw + 0x20) = pGW;
					break;
				}
			}
		}

		// +0x04: wCertificationServerID -> Match in g_pListServers_Certification -> +0x24
		uint16_t wCertificationServerID = *reinterpret_cast<uint16_t*>(pRaw + 0x04);
		*reinterpret_cast<void**>(pRaw + 0x24) = nullptr;
		if (pCertification) {
			for (CServerNode* pCert : *pCertification) {
				if (pCert && *reinterpret_cast<uint16_t*>(pCert) == wCertificationServerID) {
					*reinterpret_cast<void**>(pRaw + 0x24) = pCert;
					break;
				}
			}
		}

		// Role binding: GlobalManager -> machine manager target (Native 0x0093AC4A - 0x0093AC6C)
		if (byMachineManagerID != 0 && byGatewayServerID == 0 && wCertificationServerID == 0) {
			CServerNode* pGM = ServerFramework_FindServerNodeByName("GlobalManager");
			if (*reinterpret_cast<void**>(pRaw + 0x14) == pGM) {
				void* pMM = *reinterpret_cast<void**>(pRaw + 0x1C);
				if (pMM) {
					*reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pMM) + 0x121) = pNode->wServerID;
				}
			}
		}

		// Role binding: MachineManager -> endpoint target (Native 0x0093AC6E - 0x0093AC8A)
		CServerNode* pMMNode = ServerFramework_FindServerNodeByName("MachineManager");
		if (*reinterpret_cast<void**>(pRaw + 0x14) == pMMNode) {
			void* pEP = *reinterpret_cast<void**>(pRaw + 0x18);
			if (pEP) {
				*reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pEP) + 0x45) = pNode->wServerID;
			}
		}

		// Insert into g_mapServerNodes (Native 0x0093AC92)
		g_mapServerNodes[pNode->wServerID] = pNode;
		g_mapServerNodesByID[pNode->wServerID] = pNode;
	}

	// 6. Index links in pAgentServer (Native 0x0093ACA0 - 0x0093AD00)
	if (pAgentServer) {
		for (CServerLink* pLink : *pAgentServer) {
			if (!pLink) continue;
			pLink->m_dwSessionID = 0;
			g_mapServerLinks[pLink->dwLinkID] = pLink;
			g_mapServerLinkIndex[pLink->dwLinkID] = pLink;
		}
	}

	// 7. Calculate cluster routing graph (Native 0x0093AD10: call ServerFramework_BuildClusterRoutes)
	ServerFramework_BuildClusterRoutes();

	// 8. Index Certification server types into g_mapServersByType (Native 0x0093AD20 - 0x0093AD60)
	if (pCertification) {
		for (CServerNode* pCert : *pCertification) {
			if (pCert) {
				g_mapServersByType[pCert->wServerType] = pCert;
			}
		}
	}

	// 9. Update architecture view window (Native 0x0093ADC0 - 0x0093ADE5)
	if (g_hWndServerArchitectureView != nullptr && ::IsWindow(g_hWndServerArchitectureView)) {
		::PostMessageA(g_hWndServerArchitectureView, 0x07E8, 0, 0);
	}

	// 10. Collect local server links & build notification set (Native 0x0093ADF0 - 0x0093AF80)
	std::list<CServerLink*> localLinks;
	if (g_pLocalServerInfo) {
		ServerFramework_GetServerLinks(g_pLocalServerInfo->wServerID, localLinks);
	}

	for (CServerLink* pLink : localLinks) {
		if (!pLink) continue;
		CServerNode* pOther = ServerFramework_GetOtherServerNode(pLink, g_pLocalServerInfo);
		if (!pOther) continue;

		uint8_t byCapacityType = *reinterpret_cast<const uint8_t*>(&pOther->dwCapacity);
		uint8_t byLocalCapacityType = *reinterpret_cast<const uint8_t*>(&g_pLocalServerInfo->dwCapacity);

		if ((byCapacityType == 2 || byCapacityType == 3 || byCapacityType == 4 || byCapacityType == 5) &&
			byCapacityType == byLocalCapacityType) {
			BSLib::GenerateMiniDump();
		}

		if ((byCapacityType != 2 && byCapacityType != 3 && byCapacityType != 4 && byCapacityType != 5) ||
			pOther->pDbConfig == nullptr) {
			if (g_pLocalServerInfo->pDbConfig != nullptr && pOther->pDbConfig != nullptr) {
				continue;
			}

			if (g_setNotifyServerIDs.find(pOther->wServerID) == g_setNotifyServerIDs.end()) {
				g_setNotifyServerIDs.insert(pOther->wServerID);
				BSLib::Log_Printf(0, "Add ServerNotify : (%d) - %s\n", pOther->wServerID, pOther->szName);
			}
			continue;
		}

		BSLib::ShowErrorMessage("kek!!! why did you assign shard to common manager server? -_-+ plz check out server license table !!!");
		BSLib::Log_Printf(0x2000000, "kek!!! why did you assign shard to common manager server? -_-+ plz check out server license table !!!\n");
		return 0;
	}

	if (g_setNotifyServerIDs.empty()) {
		BSLib::ShowErrorMessage("There is no server notify target!!! (check whether Farm has assigned shard or not...)");
		BSLib::Log_Printf(0x2000000, "There is no server notify target!!! (check whether Farm has assigned shard or not...)\n");
		return 0;
	}

	return 1;
}

/**
 * [RECONSTRUCTED - 0x0093BCC0]
 * ServerFramework_CollectServerNodesByNameOrIP
 * Native implementation @ 0x0093BCC0 (278 bytes)
 *
 * Iterates g_pListServers_GameServer to find a server node matching the given
 * cluster/server name and IP address, validating that it is subordinate to g_pLocalServerInfo.
 */
CServerNode* ServerFramework_CollectServerNodesByNameOrIP(const char* szServerName, const char* szServerIP) {
	if (!szServerName || !szServerIP || !g_pListServers_GameServer || !g_pLocalServerInfo) {
		return nullptr;
	}

	for (CServerNode* pNode : *g_pListServers_GameServer) {
		if (!pNode || pNode == g_pLocalServerInfo) {
			continue;
		}

		uint8_t* pRaw = reinterpret_cast<uint8_t*>(pNode);
		uint16_t wParentID = *reinterpret_cast<uint16_t*>(pRaw + 0x0C);
		if (wParentID != g_pLocalServerInfo->wServerID) {
			continue;
		}

		// Cluster name @ *(pNode + 0x14) + 1
		void* pCluster = *reinterpret_cast<void**>(pRaw + 0x14);
		if (pCluster) {
			const char* szClusterName = reinterpret_cast<const char*>(pCluster) + 1;
			if (::CompareStringA(0x400, 0x10001, szClusterName, -1, szServerName, -1) == CSTR_EQUAL) {
				// Endpoint IP @ *(pNode + 0x18) + 0x25
				void* pEndpoint = *reinterpret_cast<void**>(pRaw + 0x18);
				if (pEndpoint) {
					const char* szEndpointIP = reinterpret_cast<const char*>(pEndpoint) + 0x25;
					if (::CompareStringA(0x400, 0x10001, szEndpointIP, -1, szServerIP, -1) == CSTR_EQUAL) {
						return pNode;
					}
				}
			}
		}
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0093B890]
 * ServerFramework_CollectConnectedNodesByLinks
 * Native implementation @ 0x0093B890 (422 bytes)
 *
 * Collects all peer server nodes and link edges directly connected to pNode via CServerLink.
 */
void ServerFramework_CollectConnectedNodesByLinks(
	CServerNode* pNode,
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
) {
	if (!pNode) {
		return;
	}

	std::list<CServerLink*> localLinks;
	ServerFramework_GetServerLinks(pNode->wServerID, localLinks);

	for (CServerLink* pLink : localLinks) {
		if (!pLink) continue;

		CServerNode* pOther = ServerFramework_GetOtherServerNode(pLink, pNode);
		if (pOther) {
			if (visitedNodes.find(pOther) == visitedNodes.end()) {
				visitedNodes.insert(pOther);
				outNodes.push_back(pOther);
			}
			if (visitedLinks.find(pLink) == visitedLinks.end()) {
				visitedLinks.insert(pLink);
			}
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0093BA40]
 * ServerFramework_CollectClusterReachableNodes
 * Native implementation @ 0x0093BA40 (273 bytes)
 *
 * Recursively explores and aggregates all reachable cluster nodes and links.
 */
void ServerFramework_CollectClusterReachableNodes(
	CServerNode* pNode,
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
) {
	if (!pNode || !g_pListServers_GameServer) {
		return;
	}

	ServerFramework_CollectConnectedNodesByLinks(pNode, outNodes, visitedNodes, visitedLinks);

	for (CServerNode* pChild : *g_pListServers_GameServer) {
		if (!pChild) continue;

		uint8_t* pRaw = reinterpret_cast<uint8_t*>(pChild);
		uint16_t wParentID = *reinterpret_cast<uint16_t*>(pRaw + 0x0C);

		if (wParentID == pNode->wServerID) {
			if (visitedNodes.find(pChild) == visitedNodes.end()) {
				visitedNodes.insert(pChild);
				outNodes.push_back(pChild);
			}
			ServerFramework_CollectClusterReachableNodes(pChild, outNodes, visitedNodes, visitedLinks);
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0093BB60]
 * ServerFramework_CollectFarmManagerTopologyNodes
 * Native implementation @ 0x0093BB60 (558 bytes)
 *
 * Discovers the FarmManager node and expands cluster dependencies across DownloadServers.
 */
void ServerFramework_CollectFarmManagerTopologyNodes(
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
) {
	ServerFramework_FindServerNodeByName("FarmManager");

	if (g_pListServers_DownloadServer) {
		for (CServerNode* pDownload : *g_pListServers_DownloadServer) {
			if (pDownload) {
				ServerFramework_CollectClusterReachableNodes(pDownload, outNodes, visitedNodes, visitedLinks);
			}
		}
	}
}

/**
 * [NATIVE - 0x009583B0]
 * ServerFramework_SerializeDynamicServerNodes
 * Native implementation @ 0x009583B0 (708 bytes)
 *
 * Acquires g_csServerNodeManager and serializes dynamic server node records into packet:
 *   - Emits opening byte (0)
 *   - Iterates g_pListServerNodes: sentinel 1 + 289 bytes per node; terminates with 2, 0
 *   - Iterates g_pListRegisteredNodes: sentinel 1 + 324 bytes per link; terminates with 2, 0
 *   - Iterates g_pListActiveNodes: sentinel 1 + 5 bytes per state; terminates with 2
 */
bool ServerFramework_SerializeDynamicServerNodes(BSLib::CPacket* pPacket) {
	if (!pPacket) return false;

	g_csServerNodeManager.Lock();

	// Native 0x009583C0: Opening stream marker
	uint8_t byStart = 0;
	pPacket->WriteUint8(byStart);

	// 1. Serialize dynamic server nodes (289 bytes / 0x121 each)
	if (g_pListServerNodes != nullptr) {
		for (tagServerNode* pNode : *g_pListServerNodes) {
			if (pNode != nullptr) {
				pPacket->WriteUint8(1); // HasNext sentinel
				pPacket->Write(pNode, 0x121);
			}
		}
	}
	pPacket->WriteUint8(2); // End sentinel
	pPacket->WriteUint8(0); // List terminator

	// 2. Serialize dynamic cluster links (324 bytes / 0x144 each)
	if (g_pListRegisteredNodes != nullptr) {
		for (tagServerNode* pLink : *g_pListRegisteredNodes) {
			if (pLink != nullptr) {
				pPacket->WriteUint8(1); // HasNext sentinel
				pPacket->Write(pLink, 0x144);
			}
		}
	}
	pPacket->WriteUint8(2); // End sentinel
	pPacket->WriteUint8(0); // List terminator

	// 3. Serialize dynamic server states (5 bytes each)
	if (g_pListActiveNodes != nullptr) {
		for (tagServerNode* pState : *g_pListActiveNodes) {
			if (pState != nullptr) {
				pPacket->WriteUint8(1); // HasNext sentinel
				pPacket->Write(pState, 5);
			}
		}
	}
	pPacket->WriteUint8(2); // End sentinel

	g_csServerNodeManager.Unlock();
	return true;
}

/**
 * [RECONSTRUCTED - 0x0093BDE0]
 * ServerFramework_EncodeCertificationTopology
 * Native implementation @ 0x0093BDE0 (5,203 bytes)
 *
 * Serializes the 9 server topology lists and dynamic node registry into the outgoing
 * certification packet (Opcode 0xA003):
 *   1. FarmServer     (node size 0x41 = 65 bytes)
 *   2. GlobalManager  (node size 0x41 = 65 bytes)
 *   3. MachineManager (node size 0x123 = 291 bytes)
 *   4. GatewayServer  (node size 0x122 = 290 bytes)
 *   5. DownloadServer (node size 0x06 = 6 bytes)
 *   6. Certification  (node size 0x22F = 559 bytes)
 *   7. Shard          (node size 0x47 = 71 bytes)
 *   8. GameServer     (node size 0x28 = 40 bytes)
 *   9. ServerLink     (node size 0x11 = 17 bytes)
 *
 * Calling Convention: __fastcall (or __cdecl)
 *   pMsg: Incoming message containing requesting server name & IP
 *   pPacket: Outgoing packet buffer
 *   pContext: Optional caller context
 *
 * Exact Machine Bytes & Instructions:
 *   0093bde0  6aff              push    0xffffffff
 *   0093bde2  686dfeab00        push    0x00ABFE6D              ; SEH handler
 *   0093bde7  64a100000000      mov     eax, fs:[0x0]
 *   0093bded  50                push    eax
 *   0093bdee  81ecbc000000      sub     esp, 0xbc
 */
int32_t ServerFramework_EncodeCertificationTopology(CMassiveMsg* pMsg, BSLib::CPacket* pPacket, void* pContext) {
	(void)pContext;
	if (!pMsg || !pPacket) {
		return 0;
	}

	std::string strServerName = pMsg->ReadString();
	std::string strServerIP = pMsg->ReadString();

	BSLib::Log_Printf(0, "Certification request from : %s(%s)\n", strServerName.c_str(), strServerIP.c_str());

	CServerNode* pTargetNode = ServerFramework_CollectServerNodesByNameOrIP(strServerName.c_str(), strServerIP.c_str());
	if (!pTargetNode) {
		uint8_t byFailure = 2;
		pPacket->WriteUint8(byFailure);
		BSLib::Log_Printf(0x1000000, "cannot certify server body : [%s][%s]\n", strServerName.c_str(), strServerIP.c_str());
		return 0;
	}

	uint8_t bySuccess = 1;
	pPacket->WriteUint8(bySuccess);

	std::set<CServerNode*> visitedNodes;
	std::set<CServerLink*> visitedLinks;
	std::list<CServerNode*> gameServerNodes;

	visitedNodes.insert(pTargetNode);
	if (g_pLocalServerInfo) {
		visitedNodes.insert(g_pLocalServerInfo);
	}

	gameServerNodes.push_back(pTargetNode);
	if (g_pLocalServerInfo) {
		gameServerNodes.push_back(g_pLocalServerInfo);
	}

	ServerFramework_CollectClusterReachableNodes(pTargetNode, gameServerNodes, visitedNodes, visitedLinks);

	// Helper to serialize each list with standard 1-byte delimiters (1 = item, 2 = end, 0 = section separator)
	auto EncodeNodeList = [&](auto* pList, size_t nNodeSize) {
		uint8_t byListHeader = 0;
		pPacket->WriteUint8(byListHeader);

		if (pList) {
			for (auto* pNode : *pList) {
				if (!pNode) continue;
				uint8_t byItemFlag = 1;
				pPacket->WriteUint8(byItemFlag);
				pPacket->Write(pNode, nNodeSize);
			}
		}

		uint8_t byEndSentinel = 2;
		pPacket->WriteUint8(byEndSentinel);
	};

	// 1. FarmServer list (node size 0x41 = 65 bytes)
	EncodeNodeList(g_pListServers_FarmServer, 0x41);

	// 2. GlobalManager list (node size 0x41 = 65 bytes)
	EncodeNodeList(g_pListServers_GlobalManager, 0x41);

	// 3. MachineManager list (node size 0x123 = 291 bytes)
	EncodeNodeList(g_pListServers_MachineManager, 0x123);

	// 4. GatewayServer list (node size 0x122 = 290 bytes)
	EncodeNodeList(g_pListServers_GatewayServer, 0x122);

	// 5. DownloadServer list (node size 0x06 = 6 bytes)
	EncodeNodeList(g_pListServers_DownloadServer, 0x06);

	// 6. Certification list (node size 0x22F = 559 bytes)
	EncodeNodeList(g_pListServers_Certification, 0x22F);

	// 7. Shard list (node size 0x47 = 71 bytes)
	EncodeNodeList(g_pListServers_Shard, 0x47);

	// 8. GameServer list (filtered nodes, node size 0x28 = 40 bytes)
	EncodeNodeList(&gameServerNodes, 0x28);

	// 9. ServerLink list (collected links, node size 0x11 = 17 bytes)
	{
		uint8_t byListHeader = 0;
		pPacket->WriteUint8(byListHeader);

		for (CServerLink* pLink : visitedLinks) {
			if (!pLink) continue;
			uint8_t byItemFlag = 1;
			pPacket->WriteUint8(byItemFlag);
			pPacket->Write(pLink, 0x11);
		}

		uint8_t byEndSentinel = 2;
		pPacket->WriteUint8(byEndSentinel);
	}

	// 10. Dynamic Server Nodes update flag (Native 0x0093C78B)
	uint8_t byDynamicFlag = (g_pLocalServerInfo && g_pLocalServerInfo->wServerType > 0) ? 1 : 0;
	pPacket->WriteUint8(byDynamicFlag);
	if (byDynamicFlag != 0) {
		ServerFramework_SerializeDynamicServerNodes(pPacket);
	}

	return 1;
}

/**
 * [RECONSTRUCTED - 0x0093CCC0]
 * ServerFramework_DecodeCertificationTopology
 * Native implementation @ 0x0093CCC0 (1,664 bytes)
 *
 * Deserializes 9 sequential server and link lists from the certification server response stream:
 *   1. FarmServer     (node size 0x41 = 65 bytes)
 *   2. GlobalManager  (node size 0x41 = 65 bytes)
 *   3. MachineManager (node size 0x123 = 291 bytes)
 *   4. GatewayServer  (node size 0x122 = 290 bytes)
 *   5. DownloadServer (node size 0x06 = 6 bytes)
 *   6. Certification  (node size 0x22F = 559 bytes)
 *   7. Shard          (node size 0x47 = 71 bytes)
 *   8. GameServer     (node size 0x28 = 40 bytes)
 *   9. ServerLink     (node size 0x11 = 17 bytes)
 *
 * Calling Convention: __cdecl (or register ECX = CMassiveMsg*)
 *
 * Exact Machine Bytes & Instructions:
 *   0093ccc0  6aff              push    0xffffffff
 *   0093ccc2  68f3fdab00        push    0x00ABFDF3              ; SEH handler
 *   0093ccc7  64a100000000      mov     eax, fs:[0x0]
 *   0093cccd  50                push    eax
 *   0093ccce  83ec2c            sub     esp, 0x2c
 *   0093ccd1  53                push    ebx
 *   0093ccd2  55                push    ebp
 *   0093ccd3  56                push    esi
 *   0093ccd4  57                push    edi
 *   0093ccd5  a18015c600        mov     eax, [0x00C61580]       ; __security_cookie
 *   0093ccda  33c4              xor     eax, esp
 *   0093ccdc  50                push    eax
 *   0093ccdd  8d442440          lea     eax, [esp+0x40]
 *   0093cce1  64a300000000      mov     fs:[0x0], eax
 *   0093cce7  8bd9              mov     ebx, ecx                ; ebx = CMassiveMsg* pMsg
 *   ... (9 sequential list deserialization loops) ...
 *   0093d2dd  8b4c2420          mov     ecx, [esp+0x20]         ; ecx = pListFarmServer
 *   0093d2e1  e85adfffff        call    0x0093B240              ; ServerFramework_SetFarmServerList
 *   0093d2e6  6a01              push    1
 *   0093d2e8  8d54241f          lea     edx, [esp+0x1f]
 *   0093d2ec  52                push    edx
 *   0093d2ed  e86e8e0100        call    0x00956160              ; Read 1-byte dynamic flag
 *   0093d2f2  807c241b00        cmp     byte [esp+0x1b], 0
 *   0093d2f7  7407              je      0x0093D300
 *   0093d2f9  8bf3              mov     esi, ebx
 *   0093d2fb  e810b30100        call    0x00958610              ; ServerFramework_ProcessDynamicServerNodes
 *   0093d300  ...               push    8 list pointers
 *   0093d324  e857d6ffff        call    0x0093A980              ; ServerFramework_SetServerListsAndBuildRoutes
 *   0093d329  83c420            add     esp, 0x20
 *   0093d33f  c3                retn
 */
int32_t ServerFramework_DecodeCertificationTopology(CMassiveMsg* pMsg) {
	if (!pMsg) {
		return -1;
	}

	auto* pList_FarmServer     = new std::list<CServerNode*>();
	auto* pList_GlobalManager  = new std::list<CServerNode*>();
	auto* pList_MachineManager = new std::list<CServerNode*>();
	auto* pList_GatewayServer  = new std::list<CServerNode*>();
	auto* pList_DownloadServer = new std::list<CServerNode*>();
	auto* pList_Certification  = new std::list<CServerNode*>();
	auto* pList_Shard          = new std::list<CServerNode*>();
	auto* pList_GameServer     = new std::list<CServerNode*>();
	auto* pList_ServerLink     = new std::list<CServerLink*>();

	uint8_t byListHeader = 0;
	uint8_t byItemFlag = 0;

	// Helper lambda for authentic list deserialization loop matching machine instructions
	auto DecodeNodeList = [&](auto* pList, size_t nNodeSize) {
		pMsg->ReadBytes(&byListHeader, 1);
		pMsg->ReadBytes(&byItemFlag, 1);
		if (byItemFlag != 2) {
			do {
				uint8_t* pRawNode = new uint8_t[nNodeSize]();
				pMsg->ReadBytes(pRawNode, nNodeSize);
				pMsg->ReadBytes(&byItemFlag, 1);
				using ItemType = typename std::remove_pointer<typename std::remove_reference<decltype(*pList)>::type::value_type>::type;
				pList->push_back(reinterpret_cast<ItemType*>(pRawNode));
			} while (byItemFlag != 2);
		}
	};

	// 1. FarmServer list (node size 0x41 = 65 bytes)
	DecodeNodeList(pList_FarmServer, 0x41);

	// 2. GlobalManager list (node size 0x41 = 65 bytes)
	DecodeNodeList(pList_GlobalManager, 0x41);

	// 3. MachineManager list (node size 0x123 = 291 bytes)
	DecodeNodeList(pList_MachineManager, 0x123);

	// 4. GatewayServer list (node size 0x122 = 290 bytes)
	DecodeNodeList(pList_GatewayServer, 0x122);

	// 5. DownloadServer list (node size 0x06 = 6 bytes)
	DecodeNodeList(pList_DownloadServer, 0x06);

	// 6. Certification list (node size 0x22F = 559 bytes)
	DecodeNodeList(pList_Certification, 0x22F);

	// 7. Shard list (node size 0x47 = 71 bytes)
	DecodeNodeList(pList_Shard, 0x47);

	// 8. GameServer list (node size 0x28 = 40 bytes)
	DecodeNodeList(pList_GameServer, 0x28);

	// 9. ServerLink list (node size 0x11 = 17 bytes)
	DecodeNodeList(pList_ServerLink, 0x11);

	// Install FarmServer list (Native 0x0093D2E1)
	ServerFramework_SetFarmServerList(pList_FarmServer);

	// Dynamic Server Nodes update flag (Native 0x0093D2F2)
	uint8_t bDynamicUpdateFlag = 0;
	pMsg->ReadBytes(&bDynamicUpdateFlag, 1);
	if (bDynamicUpdateFlag != 0) {
		ServerFramework_ProcessDynamicServerNodes(pMsg);
	}

	// Install remaining lists and calculate cluster routes (Native 0x0093D324)
	return ServerFramework_SetServerListsAndBuildRoutes(
		pList_GlobalManager,
		pList_MachineManager,
		pList_GatewayServer,
		pList_DownloadServer,
		pList_Certification,
		pList_Shard,
		pList_GameServer,
		pList_ServerLink
	);
}

/**
 * [RECONSTRUCTED - 0x0093A350]
 * ServerFramework_GetServerLinks
 * Native implementation @ 0x0093A350 (143 bytes)
 *
 * Filters g_pListServers_AgentServer (0x00C827BC) for all server links where
 * wServer1 == wServerID || wServer2 == wServerID, and appends matching link pointers
 * into outList via outList.push_back(pLink).
 *
 * Calling Convention: __cdecl
 *   [esp+0x28]: wServerID
 *   [esp+0x2C]: &outList (std::list<CServerLink*>&)
 *
 * Exact Machine Bytes & Instructions:
 *   0093a350  83ec14              sub     esp, 0x14
 *   0093a353  a1bc27c800          mov     eax, [0x00C827BC]       ; eax = g_pListServers_AgentServer
 *   0093a358  8b5004              mov     edx, [eax+0x4]          ; edx = list._Myhead
 *   0093a35b  53                  push    ebx
 *   0093a35c  8b1a                mov     ebx, [edx]              ; ebx = list._Myhead->_Next
 *   0093a35e  55                  push    ebp
 *   0093a35f  8be8                mov     ebp, eax
 *   0093a361  85ed                test    ebp, ebp
 *   0093a363  56                  push    esi
 *   0093a364  57                  push    edi
 *   0093a365  89542418            mov     [esp+0x18], edx
 *   0093a369  8944241c            mov     [esp+0x1c], eax
 *   0093a36d  7463                je      0x0093A3D2              ; throw if null list
 *   0093a36f  90                  nop
 *   0093a370  3b6c241c            cmp     ebp, [esp+0x1c]
 *   0093a374  755c                jne     0x0093A3D2
 *   0093a376  3bda                cmp     ebx, edx                ; while (ebx != list._Myhead)
 *   0093a378  745d                je      0x0093A3D7              ; break
 *   0093a37a  3b5d04              cmp     ebx, [ebp+0x4]
 *   0093a37d  744e                je      0x0093A3CD
 *   0093a37f  8b4308              mov     eax, [ebx+0x8]          ; eax = pLink
 *   0093a382  668b4c2428          mov     cx, [esp+0x28]          ; cx = wServerID
 *   0093a387  66394806            cmp     word [eax+0x6], cx      ; pLink->wServer2 == wServerID
 *   0093a38b  89442414            mov     [esp+0x14], eax
 *   0093a38f  7406                je      0x0093A397              ; match -> push_back
 *   0093a391  66394804            cmp     word [eax+0x4], cx      ; pLink->wServer1 == wServerID
 *   0093a395  752d                jne     0x0093A3C4
 *   0093a397  8b44242c            mov     eax, [esp+0x2c]         ; eax = &outList
 *   0093a39b  8b7004              mov     esi, [eax+0x4]          ; esi = outList._Myhead
 *   0093a39e  8b5604              mov     edx, [esi+0x4]          ; edx = outList._Myhead->_Prev
 *   0093a3a1  8d4c2414            lea     ecx, [esp+0x14]         ; ecx = &pLink
 *   0093a3a5  51                  push    ecx
 *   0093a3a6  52                  push    edx
 *   0093a3a7  56                  push    esi
 *   0093a3a8  e8a39e0000          call    0x00944250              ; std_list_buynode_ServerLink
 *   0093a3ad  8b4c242c            mov     ecx, [esp+0x2c]         ; ecx = &outList
 *   0093a3b1  8bf8                mov     edi, eax
 *   0093a3b3  e8489f0000          call    0x00944300              ; std_list_incsize_ServerLink
 *   0093a3b8  8b542418            mov     edx, [esp+0x18]
 *   0093a3bc  897e04              mov     [esi+0x4], edi
 *   0093a3bf  8b4704              mov     eax, [edi+0x4]
 *   0093a3c2  8938                mov     [eax], edi
 *   0093a3c4  3b5d04              cmp     ebx, [ebp+0x4]
 *   0093a3c7  7404                je      0x0093A3CD
 *   0093a3c9  8b1b                mov     ebx, [ebx]              ; ebx = ebx->_Next
 *   0093a3cb  eba3                jmp     0x0093A370
 *   0093a3cd  e9ce9f0000          jmp     0x009443A0              ; throw STL_Throw_InvalidListSubscript
 *   0093a3d2  e939a00000          jmp     0x00944410              ; throw STL_Throw_InvalidListArgument_ServerLinks
 *   0093a3d7  5f                  pop     edi
 *   0093a3d8  5e                  pop     esi
 *   0093a3d9  5d                  pop     ebp
 *   0093a3da  5b                  pop     ebx
 *   0093a3db  83c414              add     esp, 0x14
 *   0093a3de  c3                  retn
 */
void ServerFramework_GetServerLinks(uint16_t wServerID, std::list<CServerLink*>& outList) {
	if (!g_pListServers_AgentServer) {
		return;
	}

	for (CServerLink* pLink : *g_pListServers_AgentServer) {
		if (pLink && (pLink->wServer1 == wServerID || pLink->wServer2 == wServerID)) {
			outList.push_back(pLink);
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0093A3E0]
 * ServerFramework_CountServerLinks
 * Native implementation @ 0x0093A3E0 (91 bytes)
 *
 * Iterates g_pListServers_AgentServer (0x00C827BC) and computes:
 *   1. *pTotalCount: total configured links involving wServerID (wServer1 == wServerID || wServer2 == wServerID)
 *   2. *pActiveCount: links of those that have an established session (m_dwSessionID != 0)
 *
 * Calling Convention:
 *   Register BX = wServerID
 *   Register EDX = pTotalCount
 *   Stack [ebp+0x08] = pActiveCount
 *
 * Exact Machine Bytes:
 *   0093a3e0  55                push    ebp
 *   0093a3e1  8bec              mov     ebp, esp
 *   0093a3e3  83e4f8            and     esp, 0xfffffff8
 *   0093a3e6  8b4508            mov     eax, [ebp+0x8]          ; eax = pActiveCount
 *   0093a3e9  56                push    esi
 *   0093a3ea  8b35bc27c800      mov     esi, [0x00C827BC]       ; esi = g_pListServers_AgentServer
 *   0093a3f0  85f6              test    esi, esi
 *   0093a3f2  57                push    edi
 *   0093a3f3  8b7e04            mov     edi, [esi+0x4]          ; edi = list._Myhead
 *   0093a3f6  8b0f              mov     ecx, [edi]              ; ecx = list._Myhead->_Next
 *   0093a3f8  c70200000000      mov     dword [edx], 0x0        ; *pTotalCount = 0
 *   0093a3fe  c70000000000      mov     dword [eax], 0x0        ; *pActiveCount = 0
 *   0093a404  742a              je      0x0093A430              ; throw invalid_argument if null
 *   0093a406  3bf6              cmp     esi, esi
 *   0093a408  7526              jne     0x0093A430
 *   0093a40a  3bcf              cmp     ecx, edi                ; loop termination check
 *   0093a40c  7427              je      0x0093A435              ; break
 *   0093a40e  8b4108            mov     eax, [ecx+0x8]          ; eax = pLink
 *   0093a411  66395806          cmp     word [eax+0x6], bx      ; pLink->wServer2 == wServerID
 *   0093a415  7406              je      0x0093A41D
 *   0093a417  66395804          cmp     word [eax+0x4], bx      ; pLink->wServer1 == wServerID
 *   0093a41b  750f              jne     0x0093A42C
 *   0093a41d  830201            add     dword [edx], 1          ; (*pTotalCount)++
 *   0093a420  83780d00          cmp     dword [eax+0xd], 0      ; pLink->m_dwSessionID != 0
 *   0093a424  7406              je      0x0093A42C
 *   0093a426  8b4508            mov     eax, [ebp+0x8]
 *   0093a429  830001            add     dword [eax], 1          ; (*pActiveCount)++
 *   0093a42c  8b09              mov     ecx, [ecx]              ; ecx = ecx->_Next
 *   0093a42e  ebd6              jmp     0x0093A406
 *   0093a430  e9db9f0000        jmp     0x00944410              ; throw STL_Throw_InvalidListArgument_ServerLinks
 *   0093a435  5f                pop     edi
 *   0093a436  5e                pop     esi
 *   0093a437  8be5              mov     esp, ebp
 *   0093a439  5d                pop     ebp
 *   0093a43a  c3                retn
 */
void ServerFramework_CountServerLinks(uint16_t wServerID, uint32_t* pTotalCount, uint32_t* pActiveCount) {
	if (pTotalCount) {
		*pTotalCount = 0;
	}
	if (pActiveCount) {
		*pActiveCount = 0;
	}

	if (!g_pListServers_AgentServer) {
		return;
	}

	for (CServerLink* pLink : *g_pListServers_AgentServer) {
		if (!pLink) {
			continue;
		}
		if (pLink->wServer1 == wServerID || pLink->wServer2 == wServerID) {
			if (pTotalCount) {
				(*pTotalCount)++;
			}
			if (pLink->m_dwSessionID != 0) {
				if (pActiveCount) {
					(*pActiveCount)++;
				}
			}
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0093B230]
 * ServerFramework_GetAgentServerList
 * Native implementation @ 0x0093B230 (6 bytes)
 *
 * Machine Bytes:
 *   0093b230  a1bc27c800        mov     eax, [0x00C827BC] ; g_pListServers_AgentServer
 *   0093b235  c3                retn
 */
std::list<CServerLink*>* ServerFramework_GetAgentServerList() {
	return g_pListServers_AgentServer;
}

/**
 * [RECONSTRUCTED - 0x0093B530]
 * ServerFramework_FindServerNodeByName
 * Native implementation @ 0x0093B530 (277 bytes)
 *
 * Looks up a cluster server node by logical name string in g_mapServerNodesByName (0x00D67854).
 * Used by 16 callers across SR_GameServer (e.g. 0x00402AF0: "SR_ShardManager", 0x0093BB60: "FarmManager").
 *
 * Assembly trace & exact operations:
 *   0093b53e  sub   esp, 0x2c
 *   0093b561  mov   eax, [esp+0x4c]         ; szServerName
 *   0093b567  mov   [esp+0x30], 0xf         ; SSO buffer cap = 15
 *   0093b57f  call  0x006B2B00              ; std::string::assign(szServerName)
 *   0093b58c  call  0x0093EC90              ; std_map_string_find(&strName, &iter)
 *   0093b59e  cmp   ecx, 0x00D67854         ; verify map base pointer == g_mapServerNodesByName
 *   0093b5aa  cmp   eax, [0x00D67858]       ; cmp iter.node, map._Myhead (is end()?)
 *   0093b5b5  je    0x0093B603              ; if found, jump to extract value
 *   0093b5e3  xor   eax, eax                ; not found -> return nullptr
 *   0093b60d  mov   esi, [eax+0x28]         ; [PROVEN OFFSET +0x28]: return node->value (CServerNode*)
 *   0093b63c  mov   eax, esi
 *   0093b602  retn
 */
CServerNode* ServerFramework_FindServerNodeByName(const char* szServerName) {
	if (!szServerName || szServerName[0] == '\0') {
		return nullptr;
	}

	auto it = g_mapServerNodesByName.find(szServerName);
	if (it != g_mapServerNodesByName.end()) {
		return it->second;
	}

	// Fallback to local server info if name matches local application name
	if (g_pLocalServerInfo != nullptr && g_pServerApp != nullptr) {
		if (g_pServerApp->GetAppName() == szServerName) {
			return g_pLocalServerInfo;
		}
	}

	return nullptr;
}

void ServerFramework_RegisterServerNodeName(const char* szServerName, CServerNode* pNode) {
	if (szServerName && szServerName[0] != '\0' && pNode != nullptr) {
		g_mapServerNodesByName[szServerName] = pNode;
	}
}

/**
 * [RECONSTRUCTED - 0x0093B650]
 * ServerFramework_NotifyServerStateChange
 * Native implementation @ 0x0093B650 (266 bytes)
 *
 * Dispatches server state updates across:
 *   1. Local server node table (pNode->nState @ +0x10)
 *   2. Server state observer sink (slot 1 @ +0x04)
 *   3. Windows message pump (WM_SERVER_STATE_CHANGED = 0x07EA to CServerFrameWindow)
 *   4. Broadcast Opcode 0x2005 to cluster manager sessions
 */
uint32_t ServerFramework_NotifyServerStateChange(uint32_t nNewState, uint16_t wServerID, void* pExcludeSession) {
	(void)pExcludeSession;
	CServerNode* pNode = ServerFramework_FindServerNode(wServerID);
	if (!pNode) {
		return 0;
	}

	uint32_t dwOldState = pNode->nState;
	pNode->nState       = nNewState;

	if (g_pServerStateObserver != nullptr) {
		g_pServerStateObserver->OnServerStateChanged(pNode, dwOldState);
	}

#ifdef _WIN32
	if (g_hWndServerFrame != nullptr) {
		PostMessageA(static_cast<HWND>(g_hWndServerFrame), 0x07EA, 0, reinterpret_cast<LPARAM>(pNode));
	}
#endif

	const char* pszStateName = "UNKNOWN";
	switch (nNewState) {
		case SERVER_STATE_READY:        pszStateName = "READY"; break;
		case SERVER_STATE_INITIALIZING: pszStateName = "INITIALIZING"; break;
		case SERVER_STATE_CERTIFYING:   pszStateName = "CERTIFYING"; break;
		case SERVER_STATE_RUNNING:      pszStateName = "RUNNING"; break;
		case SERVER_STATE_STOPPING:     pszStateName = "STOPPING"; break;
		case SERVER_STATE_STOPPED:      pszStateName = "STOPPED"; break;
	}
	std::printf("[ServerFramework] Server Node 0x%04X State Change: %u -> %u (%s)\n",
	            wServerID, dwOldState, nNewState, pszStateName);

	// Broadcast Opcode 0x2005 (Type 1: Server Node State) matching Native 0x0093B6B7 - 0x0093B74C
	bool bIsLocalNode = (g_pLocalServerInfo != nullptr && g_pLocalServerInfo->wServerID == wServerID);
	if (dwOldState != nNewState || bIsLocalNode) {
		CPacket* pPacket = CPacket::Allocate(1);
		pPacket->SetOpcode(0x2005);
		pPacket->WriteUint8(1); // Type 1: Server Node
		pPacket->WriteUint8(0);
		pPacket->WriteUint8(1);
		pPacket->WriteUint16(pNode->wServerID);
		pPacket->WriteUint32(pNode->nState);
		pPacket->WriteUint8(2); // Footer

		uint32_t dwExclude = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pExcludeSession));
		ServerFramework_BroadcastPacketToManagers(pPacket, dwExclude);
	}

	return 1;
}

/**
 * [RECONSTRUCTED - 0x0093B760]
 * ServerFramework_NotifyServerLinkStateChange
 * Native implementation @ 0x0093B760 (213 bytes)
 */
bool ServerFramework_NotifyServerLinkStateChange(uint32_t nLinkState, uint32_t dwLinkID, void* pExcludeSession) {
	CServerLink* pLink = ServerFramework_FindServerLink(dwLinkID);
	if (!pLink) {
		return false;
	}

	bool bChanged = (pLink->nLinkState != nLinkState);
	pLink->nLinkState = nLinkState;

#ifdef _WIN32
	if (g_hWndServerFrame != nullptr) {
		PostMessageA(static_cast<HWND>(g_hWndServerFrame), 0x07E9, 0, reinterpret_cast<LPARAM>(pLink));
	}
#endif

	// Broadcast Opcode 0x2005 (Type 2: Server Link State) matching Native 0x0093B79B - 0x0093B826
	if (bChanged) {
		CPacket* pPacket = CPacket::Allocate(1);
		pPacket->SetOpcode(0x2005);
		pPacket->WriteUint8(2); // Type 2: Server Link
		pPacket->WriteUint8(0);
		pPacket->WriteUint8(1);
		pPacket->WriteUint32(pLink->dwLinkID);
		pPacket->WriteUint32(pLink->nLinkState);
		pPacket->WriteUint8(2); // Footer

		uint32_t dwExclude = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pExcludeSession));
		ServerFramework_BroadcastPacketToManagers(pPacket, dwExclude);
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0093B840]
 * ServerFramework_GetServerLinkByServerID
 * Native implementation @ 0x0093B840 (76 bytes)
 *
 * Machine Bytes & Disassembly trace:
 *   0093b840  sub   esp, 0xc
 *   0093b843  push  ebx
 *   0093b844  lea   eax, [esp+0x8]          ; &resultIter
 *   0093b848  push  eax
 *   0093b849  lea   ebx, [esp+0x18]         ; &wServerID (passed on stack by caller)
 *   0093b84d  call  0x00940CA0              ; stdext::hash_map::find(&resultIter, &wServerID)
 *   0093b852  mov   ecx, [esp+0x8]          ; map pointer (0x00D6788C)
 *   0093b856  test  ecx, ecx
 *   0093b858  je    0x0093B887              ; throw if invalid
 *   0093b85a  cmp   ecx, 0x00D6788C         ; verify map base address
 *   0093b860  jne   0x0093B887
 *   0093b862  mov   eax, [esp+0xc]          ; iterator node pointer
 *   0093b866  cmp   eax, [0x00D67890]       ; compare against end()
 *   0093b86c  jne   0x0093B875
 *   0093b86e  xor   eax, eax                ; not found -> return nullptr
 *   0093b870  pop   ebx
 *   0093b871  add   esp, 0xc
 *   0093b874  retn
 *   0093b875  cmp   eax, [ecx+0x4]          ; verify node valid
 *   0093b878  jne   0x0093B87F
 *   0093b87a  jmp   0x00943000              ; throw invalid iterator
 *   0093b87f  mov   eax, [eax+0xc]          ; return pair.second (CServerLink*)
 *   0093b882  pop   ebx
 *   0093b883  add   esp, 0xc
 *   0093b886  retn
 */
CServerLink* ServerFramework_GetServerLinkByServerID(uint16_t wServerID) {
	auto it = g_mapServerLinkIndex.find(wServerID);
	if (it != g_mapServerLinkIndex.end()) {
		return it->second;
	}
	return nullptr;
}

void ServerFramework_AddNotifyServer(uint16_t wServerID) {
	g_setNotifyServerIDs.insert(wServerID);
}

void ServerFramework_RemoveNotifyServer(uint16_t wServerID) {
	g_setNotifyServerIDs.erase(wServerID);
}

void ServerFramework_AddDirectNotifySession(uint32_t dwSessionID) {
	if (dwSessionID != 0) {
		g_setDirectNotifySessions.insert(dwSessionID);
	}
}

void ServerFramework_RemoveDirectNotifySession(uint32_t dwSessionID) {
	g_setDirectNotifySessions.erase(dwSessionID);
}

/**
 * ============================================================================
 * [RECONSTRUCTED - 0x0093DDD0]
 * ServerFramework_BroadcastPacketToManagers
 * Native implementation @ 0x0093DDD0 (335 bytes)
 *
 * Broadcasts an internal cluster IPC packet (e.g. Opcode 0x2005) across all
 * active manager and server-notify targets:
 *   1. Iterates g_setNotifyServerIDs (std::set<uint16_t> @ 0x00D678BC), resolves
 *      each link via ServerFramework_GetServerLinkByServerID, and queues the packet
 *      to pLink->m_dwSessionID (offset +0x0D) if not excluded.
 *   2. Iterates g_setDirectNotifySessions (std::set<uint32_t> @ 0x00D678C8) and
 *      queues the packet to each session if not excluded.
 *   3. Calls CPacket_Flush (0x00956440) to commit queued packet buffers.
 *   4. Calls CPacket_Release (0x00956080) to decrement refcount and return the
 *      packet to the pool via tail call.
 *
 * Assembly trace & exact machine operations:
 *   0093ddd0  sub   esp, 0xc
 *   0093ddd3  mov   edx, [0x00D678C0]       ; edx = g_setNotifyServerIDs head
 *   0093ddd9  mov   ecx, [edx]              ; ecx = first set node
 *   0093dddb  push  ebx
 *   0093dddc  mov   ebx, eax                ; ebx = pPacket (passed in EAX)
 *   0093ddde  mov   eax, 0x00D678BC         ; eax = &g_setNotifyServerIDs
 *   0093dde3  push  esi
 *   0093dde4  mov   [esp+0xc], ecx          ; it._Ptr
 *   0093dde8  mov   [esp+0x8], eax          ; it._Mycont
 *   0093ddec  mov   esi, edx                ; esi = head (end)
 *   ...
 *   Loop 1: Notify Servers by ID (0x0093DDFA - 0x0093DE37):
 *     0093de08  movzx eax, word [ecx+0xc]   ; wServerID = node->value
 *     0093de0c  push  eax
 *     0093de0d  call  0x0093B840            ; ServerFramework_GetServerLinkByServerID
 *     0093de12  add   esp, 4
 *     0093de15  test  eax, eax              ; pLink != nullptr?
 *     0093de17  je    0x0093DE28
 *     0093de19  mov   eax, [eax+0xd]        ; [PROVEN OFFSET +0x0D]: dwSessionID = pLink->m_dwSessionID
 *     0093de1c  cmp   eax, edi              ; dwSessionID != dwExcludeSessionID?
 *     0093de1e  je    0x0093DE28
 *     0093de20  push  1                     ; bKeepPacket = 1
 *     0093de22  push  eax                   ; dwSessionID
 *     0093de23  call  0x009562A0            ; CPacket_SendToSession(pPacket @ ebx, dwSessionID, 1)
 *     ... increment iterator and loop
 *
 *   Loop 2: Direct Notify Sessions (0x0093DE43 - 0x0093DE96):
 *     0093de43  mov   edx, [0x00D678CC]     ; g_setDirectNotifySessions head
 *     ...
 *     0093de78  mov   eax, [ecx+0xc]        ; dwSessionID = node->value
 *     0093de7d  cmp   eax, edi              ; dwSessionID != dwExcludeSessionID?
 *     0093de7f  push  1
 *     0093de81  push  eax
 *     0093de82  call  0x009562A0            ; CPacket_SendToSession(pPacket @ ebx, dwSessionID, 1)
 *     ... increment iterator and loop
 *
 *   Epilogue (0x0093DEA2 - 0x0093DEB0):
 *     0093dea2  mov   eax, ebx              ; eax = pPacket
 *     0093dea4  call  0x00956440            ; CPacket_Flush(pPacket)
 *     0093dea9  mov   eax, ebx              ; eax = pPacket
 *     0093deab  pop   esi
 *     0093deac  pop   ebx
 *     0093dead  add   esp, 0xc
 *     0093deb0  jmp   0x00956080            ; return CPacket_Release(pPacket) __tailcall
 * ============================================================================
 */
int32_t ServerFramework_BroadcastPacketToManagers(CPacket* pPacket, uint32_t dwExcludeSessionID) {
	if (pPacket == nullptr) {
		return 0;
	}

	// Phase 1: Broadcast to all registered ServerNotify server targets (Native 0x0093DDD3 - 0x0093DE37)
	for (uint16_t wServerID : g_setNotifyServerIDs) {
		CServerLink* pLink = ServerFramework_GetServerLinkByServerID(wServerID);
		if (pLink != nullptr && pLink->m_dwSessionID != 0) {
			if (pLink->m_dwSessionID != dwExcludeSessionID) {
				pPacket->Send(pLink->m_dwSessionID, 1);
			}
		}
	}

	// Phase 2: Broadcast to all directly registered manager sessions (Native 0x0093DE43 - 0x0093DE96)
	for (uint32_t dwSessionID : g_setDirectNotifySessions) {
		if (dwSessionID != 0 && dwSessionID != dwExcludeSessionID) {
			pPacket->Send(dwSessionID, 1);
		}
	}

	// Phase 3: Flush packet queue and release packet instance (Native 0x0093DEA2 - 0x0093DEB0)
	pPacket->Flush();
	return pPacket->Release();
}

/**
 * [RECONSTRUCTED - 0x00957C00]
 * ServerFramework_CleanupServerNodesInternal
 * Native implementation @ 0x00957C00 (1276 bytes)
 *
 * Full cluster topology destruction sequence:
 *   1. Acquire critical section lock (g_csServerNodeManager.Lock @ 0x00957C50)
 *   2. Clear type lookup map (0x00957C69: g_mapServerNodesByType.clear())
 *   3. Clear ID lookup map (0x00957C99: g_mapServerNodesByID.clear())
 *   4. Free and erase all allocated server node records (0x00957CE0 - 0x00957D64)
 *   5. Free and reset active connected node list (0x00957DC3 - 0x00957E8D)
 *   6. Free and reset registered child node list (0x00957E95 - 0x00957F69)
 *   7. Free and reset full server node list (0x00957F71 - 0x00958047)
 *   8. Release critical section lock (g_csServerNodeManager.Unlock @ 0x0095806B)
 */
bool ServerFramework_CleanupServerNodesInternal() {
	g_csServerNodeManager.Lock();

	// Step 2 & 3: Clear lookup maps
	g_mapServerNodesByType.clear();
	g_mapServerNodesByID.clear();

	// Step 4: Delete each dynamically allocated node descriptor
	for (auto* pNode : g_listServerNodes) {
		if (pNode != nullptr && pNode != &g_defaultServerNode) {
			delete pNode;
		}
	}
	g_listServerNodes.clear();

	// Step 5: Clean up active node list
	if (g_pListActiveNodes != nullptr) {
		for (auto* pNode : *g_pListActiveNodes) {
			if (pNode != nullptr && pNode != &g_defaultServerNode) {
				delete pNode;
			}
		}
		delete g_pListActiveNodes;
		g_pListActiveNodes = nullptr;
	}

	// Step 6: Clean up registered node list
	if (g_pListRegisteredNodes != nullptr) {
		for (auto* pNode : *g_pListRegisteredNodes) {
			if (pNode != nullptr && pNode != &g_defaultServerNode) {
				delete pNode;
			}
		}
		delete g_pListRegisteredNodes;
		g_pListRegisteredNodes = nullptr;
	}

	// Step 7: Clean up server node list
	if (g_pListServerNodes != nullptr) {
		for (auto* pNode : *g_pListServerNodes) {
			if (pNode != nullptr && pNode != &g_defaultServerNode) {
				delete pNode;
			}
		}
		delete g_pListServerNodes;
		g_pListServerNodes = nullptr;
	}

	g_csServerNodeManager.Unlock();
	return true;
}

/**
 * [RECONSTRUCTED - 0x00957BF0]
 * ServerFramework_CleanupServerNodes
 * Native implementation @ 0x00957BF0 (8 bytes)
 *
 * Machine Bytes:
 *   00957bf0  e80b000000            call    0x957c00  ; ServerFramework_CleanupServerNodesInternal
 *   00957bf5  b001                  mov     al, 0x1   ; return true
 *   00957bf7  c3                    retn
 */
bool ServerFramework_CleanupServerNodes() {
	ServerFramework_CleanupServerNodesInternal();
	return true;
}

} // namespace ServerFramework
