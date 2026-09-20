/**
 * ============================================================================
 * Silkroad Online - Server Topology & Routing Management
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerFramework\ServerTopology.h
 *
 * Manages the cluster server topology, node map, link states, and network routing:
 *   - Native ServerNode Map @ 0x00D67830 (g_mapServerNodes)
 *   - Native ServerLink Map @ 0x00D6783C (g_mapServerLinks)
 *   - 10 Dynamic Server Lists @ 0x00C827A0 - 0x00C827C8
 *   - 13 Static Topology Containers @ 0x00D67818 - 0x00D678C8
 *   - Native Methods:
 *       ServerFramework_SetStateObserver            @ 0x009396A0
 *       ServerFramework_CleanupServerRouting        @ 0x009396B0
 *       ServerFramework_FindServerNode              @ 0x0093B0F0
 *       ServerFramework_GetOtherServerNode          @ 0x0093B140
 *       ServerFramework_GetPeerServerNode           @ 0x0093B170
 *       ServerFramework_FindServerLink              @ 0x0093B180
 *       ServerFramework_FindServerNodeByName        @ 0x0093B530
 *       ServerFramework_NotifyServerStateChange     @ 0x0093B650
 *       ServerFramework_NotifyServerLinkStateChange @ 0x0093B760
 *       ServerFramework_CleanupProxyRoutes          @ 0x0093DAE0
 *       ServerFramework_BroadcastPacketToManagers   @ 0x0093DDD0
 * ============================================================================
 */

#ifndef _JMX_SERVERFRAMEWORK_SERVERTOPOLOGY_H_
#define _JMX_SERVERFRAMEWORK_SERVERTOPOLOGY_H_

#include "ServerConfig.h"
#include "../../Common/Framework/MassiveMsg.h"
#include "../../JMX_Library/BSLib/Synch.h"
#include "../../JMX_Library/BSLib/Packet.h"
#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ServerFramework {

#pragma pack(push, 1)
/**
 * CServerLink
 * Native Link descriptor between two cluster server nodes
 * Proven offsets:
 *   +0x00: dwLinkID (4 bytes)
 *   +0x04: wServer1 (2 bytes)
 *   +0x06: wServer2 (2 bytes)
 *   +0x08: byPadding (1 byte)
 *   +0x09: nLinkState (4 bytes) - Native 0x0093B78F
 *   +0x0D: pSession (4 bytes)   - Native 0x0093DE19 (mov eax, [eax+0x0d])
 */
struct tagServerLink {
	uint32_t dwLinkID       = 0; // +0x00: Composite Link ID
	uint16_t wServer1       = 0; // +0x04: First Server Node ID
	uint16_t wServer2       = 0; // +0x06: Second Server Node ID
	uint8_t  byPadding      = 0; // +0x08
	uint32_t nLinkState     = 0; // +0x09: Link State (Connected, Closed, etc.)
	union {
		uint32_t dwSessionID   = 0;
		uint32_t m_dwSessionID;          // +0x0D: Active Network Session ID (Native 0x0093DE19)
		void*    pSession;
	};
};
#pragma pack(pop)
typedef tagServerLink CServerLink;

/**
 * tagServerEndpoint / CServerEndpoint
 * Native struct layout proven from:
 *   - Native 0x0093F3F0: tagServerEndpoint_constructor (std::string @ +0x00, dwServerID @ +0x1C)
 *   - Native 0x0093B460: tagServerEndpoint_destructor (inlined std::string dtor @ +0x00)
 *   - Native 0x0093B0A0: ServerFramework_FindServerEndpoint (looks up in g_mapServerEndpoints @ 0x00D67848)
 */
struct tagServerEndpoint {
	std::string strEndpoint;    // +0x00: IP string, hostname, or connection endpoint
	uint32_t    dwServerID = 0; // +0x1C: Bound server / target ID
};
typedef tagServerEndpoint CServerEndpoint;

// 10 Global Server Lists allocated dynamically during certification (Native 0x00C827A0 - 0x00C827C8)
extern std::list<CServerNode*>* g_pListServers_Shard;           // 0x00C827A0
extern std::list<CServerNode*>* g_pListServers_GlobalManager;   // 0x00C827A4
extern std::list<CServerNode*>* g_pListServers_MachineManager;  // 0x00C827A8
extern std::list<CServerNode*>* g_pListServers_GatewayServer;   // 0x00C827AC
extern std::list<CServerNode*>* g_pListServers_DownloadServer;  // 0x00C827B0
extern std::list<CServerNode*>* g_pListServers_Certification;   // 0x00C827B4
extern std::list<CServerNode*>* g_pListServers_GameServer;      // 0x00C827B8
extern std::list<CServerLink*>* g_pListServers_AgentServer;     // 0x00C827BC (stores tagServerLink* for agent/cluster links)
extern std::list<CServerNode*>* g_pListServers_FarmServer;      // 0x00C827C0
extern std::list<CServerNode*>* g_pListServers_Proxy;           // 0x00C827C8
extern std::list<CServerNode*>* g_pListServers_ProxyAux;        // 0x00C827CC

// 13 Global Static Topology & Routing Containers (Native 0x00D67818 - 0x00D678C8)
extern std::map<uint16_t, void*>              g_mapServersByType;         // 0x00D67818
extern std::map<uint8_t, CServerNode*>        g_mapServersByCluster;      // 0x00D67824
extern std::map<uint16_t, CServerNode*>       g_mapServerNodes;           // 0x00D67830
extern std::map<uint32_t, CServerLink*>       g_mapServerLinks;           // 0x00D6783C
extern std::map<uint32_t, tagServerEndpoint*> g_mapServerEndpoints;       // 0x00D67848
extern std::map<std::string, CServerNode*> g_mapServerNodesByName;  // 0x00D67854
extern std::list<void*>                 g_listManagerConnections;   // 0x00D67864
extern std::vector<void*>               g_vecPeerConnections;       // 0x00D67870
extern std::list<void*>                 g_listClusterPaths;         // 0x00D6788C / 0x00D67890
extern std::vector<void*>               g_vecTopologyLinks;         // 0x00D67898
extern std::map<uint32_t, CServerLink*> g_mapServerLinksBySession;  // 0x00D678B0

// Download & Gateway File Transfer Routing Containers (Native 0x00D678D4, 0x00D678E0)
extern std::map<uint32_t, void*>        g_mapDownloadTransferEntries; // 0x00D678D4
extern std::map<uint32_t, void*>        g_mapDownloadFileLinks;       // 0x00D678E0 (stdext::hash_map)

// [RECONSTRUCTED - 0x009396A0]
void ServerFramework_SetStateObserver(IServerStateObserver* pObserver);

// Cluster Server Node Manager Synchronization Guard (Native 0x00D67B40)
extern CCriticalSectionBS g_csServerNodeManager;

// Cluster Server Node Registry Collections (Native 0x00D67B04, 0x00D67B28, 0x00D67B34)
extern std::list<tagServerNode*>                     g_listServerNodes;      // 0x00D67B04
extern std::map<uint32_t, std::list<tagServerNode*>> g_mapServerNodesByType; // 0x00D67B28
extern std::map<uint16_t, tagServerNode*>            g_mapServerNodesByID;   // 0x00D67B34

// Active Cluster Dynamic Server Node Lists (Native 0x00C827E4, 0x00C827E8, 0x00C827EC)
extern std::list<tagServerNode*>*                    g_pListServerNodes;     // 0x00C827E4
extern std::list<tagServerNode*>*                    g_pListRegisteredNodes; // 0x00C827E8
extern std::list<tagServerNode*>*                    g_pListActiveNodes;     // 0x00C827EC

// Cached Local Server Node Type Code (Native 0x00C82798)
extern uint32_t                                      g_dwLocalServerType;

/**
 * ServerFramework_CacheLocalServerType
 * Native implementation @ 0x00939690
 *
 * Caches g_pLocalServerInfo->wServerType into g_dwLocalServerType @ 0x00C82798.
 */
void ServerFramework_CacheLocalServerType();

// [RECONSTRUCTED - 0x00957BF0]
// Cleans up all registered cluster server nodes and resets topology registries
bool ServerFramework_CleanupServerNodes();

// [RECONSTRUCTED - 0x00957C00]
// Core thread-safe node registry destruction routine
bool ServerFramework_CleanupServerNodesInternal();

// [RECONSTRUCTED - 0x009396B0]
// Cleans up all cluster routing lists, dynamic node allocations, and static topology maps on shutdown
void ServerFramework_CleanupServerRouting();

// [RECONSTRUCTED - 0x0093DAE0]
// Cleans up proxy server routing lists and download file transfer maps
bool ServerFramework_CleanupProxyRoutes();

// [RECONSTRUCTED - 0x0093DD70]
// Resolves download file transfer link entry by file ID
void* ServerFramework_FindDownloadFileLink(int32_t nFileID);

// [RECONSTRUCTED - 0x0093B0A0]
tagServerEndpoint* ServerFramework_FindServerEndpoint(uint32_t dwEndpointID);

// [RECONSTRUCTED - 0x0093B0F0]
CServerNode* ServerFramework_FindServerNode(uint16_t wServerID);

// [RECONSTRUCTED - 0x0093B140]
CServerNode* ServerFramework_GetOtherServerNode(CServerLink* pLink, CServerNode* pNode);
CServerNode* ServerFramework_GetOtherServerNode(CServerLink* pLink, uint16_t wServerID);

// [RECONSTRUCTED - 0x0093B170]
CServerNode* ServerFramework_GetPeerServerNode(CServerLink* pLink);

// [RECONSTRUCTED - 0x0093B4D0]
CServerNode* ServerFramework_FindClusterServer(uint8_t byClusterID);

// [RECONSTRUCTED - 0x0093B180]
CServerLink* ServerFramework_FindServerLink(uint32_t dwLinkID);

// [RECONSTRUCTED - 0x0093B1D0]
CServerLink* ServerFramework_FindServerLinkBySession(uint32_t dwSessionID);

// [RECONSTRUCTED - 0x0093A350]
// Collects all server links connecting to wServerID (wServer1 == wServerID || wServer2 == wServerID)
void ServerFramework_GetServerLinks(uint16_t wServerID, std::list<CServerLink*>& outList);

// [RECONSTRUCTED - 0x0093A3E0]
// Counts total configured and active/connected links involving wServerID from g_pListServers_AgentServer (0x00C827BC)
void ServerFramework_CountServerLinks(uint16_t wServerID, uint32_t* pTotalCount, uint32_t* pActiveCount);

// [RECONSTRUCTED - 0x0093B220]
std::list<CServerNode*>* ServerFramework_GetGameServerList();

// [RECONSTRUCTED - 0x0093B230]
std::list<CServerLink*>* ServerFramework_GetAgentServerList();

// [RECONSTRUCTED - 0x0093B530]
CServerNode* ServerFramework_FindServerNodeByName(const char* szServerName);
void ServerFramework_RegisterServerNodeName(const char* szServerName, CServerNode* pNode);

// [RECONSTRUCTED - 0x0093B650]
// Updates node state, invokes observer trigger, notifies GUI window, broadcasts opcode 0x2005
uint32_t ServerFramework_NotifyServerStateChange(uint32_t nNewState, uint16_t wServerID, void* pExcludeSession = nullptr);

// [RECONSTRUCTED - 0x0093B760]
// Updates link state, notifies GUI window, broadcasts opcode 0x2005
bool ServerFramework_NotifyServerLinkStateChange(uint32_t nLinkState, uint32_t dwLinkID, void* pExcludeSession = nullptr);

// Cluster Manager Broadcast Sets (Native 0x00D6788C, 0x00D678BC, 0x00D678C8)
extern std::set<uint16_t>                g_setNotifyServerIDs;       // 0x00D678BC
extern std::set<uint32_t>                g_setDirectNotifySessions;  // 0x00D678C8
extern std::map<uint16_t, CServerLink*>  g_mapServerLinkIndex;       // 0x00D6788C

// Architecture View HWND @ 0x00C827FC
extern HWND g_hWndServerArchitectureView;

// [RECONSTRUCTED - 0x0093A440]
// Recursive cluster route path expansion traversing linked server graph
int32_t ServerFramework_BuildClusterRouteRecursive(
	uint16_t wCurrentNodeID,
	uint16_t wTargetNodeID,
	int32_t nMaxHops,
	void* pRouteMap,
	std::list<CServerLink*>* pVisitedLinks
);

// [RECONSTRUCTED - 0x0093A600]
// Generates cluster routing graph and direct/indirect link paths across all cluster server nodes
int32_t ServerFramework_BuildClusterRoutes();

// [RECONSTRUCTED - 0x0093B840]
// Returns the CServerLink connecting to wServerID (from hash_map / index @ 0x00D6788C)
CServerLink* ServerFramework_GetServerLinkByServerID(uint16_t wServerID);
inline CServerLink* ServerFramework_FindServerLinkByServerID(uint16_t wServerID) {
	return ServerFramework_GetServerLinkByServerID(wServerID);
}

// [RECONSTRUCTED - 0x0093B240]
// Configures g_pListServers_FarmServer, binds cluster maps and endpoint records
bool ServerFramework_SetFarmServerList(std::list<CServerNode*>* pList);

// [RECONSTRUCTED - 0x0093A980]
// Binds 8 server lists to global pointers, indexes nodes into maps, and builds routing graph
int32_t ServerFramework_SetServerListsAndBuildRoutes(
	std::list<CServerNode*>* pGlobalManager,
	std::list<CServerNode*>* pMachineManager,
	std::list<CServerNode*>* pGatewayServer,
	std::list<CServerNode*>* pDownloadServer,
	std::list<CServerNode*>* pCertification,
	std::list<CServerNode*>* pShard,
	std::list<CServerNode*>* pGameServer,
	std::list<CServerLink*>* pAgentServer
);

// [NATIVE - 0x00958610]
// Acquires g_csServerNodeManager and processes dynamic server node synchronization
bool ServerFramework_ProcessDynamicServerNodes(CMassiveMsg* pMsg = nullptr);

// [NATIVE - 0x009583B0]
// Acquires g_csServerNodeManager and serializes dynamic server node registry into packet
bool ServerFramework_SerializeDynamicServerNodes(BSLib::CPacket* pPacket);

// [RECONSTRUCTED - 0x0093BCC0]
// Searches g_pListServers_GameServer for matching server node by cluster name and endpoint IP
CServerNode* ServerFramework_CollectServerNodesByNameOrIP(const char* szServerName, const char* szServerIP);

// [RECONSTRUCTED - 0x0093B890]
// Collects all peer server nodes and link edges connected via CServerLink to pNode
void ServerFramework_CollectConnectedNodesByLinks(
	CServerNode* pNode,
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
);

// [RECONSTRUCTED - 0x0093BA40]
// Recursively collects all cluster-reachable server nodes and link edges
void ServerFramework_CollectClusterReachableNodes(
	CServerNode* pNode,
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
);

// [RECONSTRUCTED - 0x0093BB60]
// Collects FarmManager topology nodes and download server dependencies
void ServerFramework_CollectFarmManagerTopologyNodes(
	std::list<CServerNode*>& outNodes,
	std::set<CServerNode*>& visitedNodes,
	std::set<CServerLink*>& visitedLinks
);

// [RECONSTRUCTED - 0x0093BDE0]
// Serializes 9 server topology lists and dynamic node registry into outgoing certification packet (Opcode 0xA003)
int32_t ServerFramework_EncodeCertificationTopology(CMassiveMsg* pMsg, BSLib::CPacket* pPacket, void* pContext = nullptr);

// [RECONSTRUCTED - 0x0093CCC0]
// Decodes certification server response payload and instantiates all 9 cluster server and link lists
int32_t ServerFramework_DecodeCertificationTopology(CMassiveMsg* pMsg);

// [RECONSTRUCTED - 0x0093DA40]
// Resolves server entry from g_mapServersByType (0x00D67818)
CServerNode* ServerFramework_FindServerByType(uint16_t wServerType);
void* ServerFramework_FindServerByType(uint32_t dwServerType);

// [RECONSTRUCTED - 0x0093DA90]
// Resolves link session for wServerID and transmits packet buffer via CNetEngine
bool ServerFramework_SendPacketToServer(uint16_t wServerID, void* pBuffer);
bool ServerFramework_SendPacketToServer(uint16_t wServerID, BSLib::CPacket* pPacket);

// Management Notification Registration APIs:
void ServerFramework_AddNotifyServer(uint16_t wServerID);
void ServerFramework_RemoveNotifyServer(uint16_t wServerID);
void ServerFramework_AddDirectNotifySession(uint32_t dwSessionID);
void ServerFramework_RemoveDirectNotifySession(uint32_t dwSessionID);

// [RECONSTRUCTED - 0x0093DDD0]
// Broadcasts an internal cluster management packet to all active manager sessions
int32_t ServerFramework_BroadcastPacketToManagers(CPacket* pPacket, uint32_t dwExcludeSessionID = 0);

} // namespace ServerFramework

#endif // _JMX_SERVERFRAMEWORK_SERVERTOPOLOGY_H_
