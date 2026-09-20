/**
 * ============================================================================
 * Joymax BSLib / BSNet - Network Engine
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\NetEngine.h
 *
 * Implements the asynchronous IOCP network engine and socket management:
 *   - IBSNet interface @ 0x00C80DD4
 *   - CNetEngine class @ 0x00B4239C (RTTI: .?AVCNetEngine@@)
 *   - Native Ctor @ 0x0096B330, Dtor @ 0x0096B520
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_NETENGINE_H_
#define _JMX_LIBRARY_BSLIB_NETENGINE_H_

#include "Synch.h"
#include "NetConfig.h"
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#endif

#include "BSObj.h"

namespace BSLib {
// Core Network Engine Task IDs (Native 0x00C67728, 0x00C6772C, 0x00C67730)
constexpr uint32_t TASK_ID_NET_PROACTOR_1 = 0x10000001; // IOCP or Event Worker Proactor 1
constexpr uint32_t TASK_ID_NET_PROACTOR_2 = 0x10000002; // IOCP or Event Worker Proactor 2
constexpr uint32_t TASK_ID_NET_MAINTAINER = 0x10000003; // Net Maintainer Task
}

#include "Task.h"

namespace BSLib {

/**
 * CProactorIOCP
 * Native VTable @ 0x00B42DCC, Runtime Class @ 0x00ADD89C
 * Native Size: 84 bytes (0x54)
 */
class CProactorIOCP : public CServiceObject {
public:
	CProactorIOCP();
	virtual ~CProactorIOCP() override = default;

	virtual const CRuntimeClass* GetRuntimeClass() const override;
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) override;
	virtual void OnMessage() override;

	static void* CreateObject();

	static const CRuntimeClass ms_classCProactorIOCP;

protected:
	void*    m_hIOCP;
	uint32_t m_dwConcurrentThreads;
};

/**
 * CMaintainer
 * Native VTable @ 0x00B42E3C, Runtime Class @ 0x00ADD874
 * Native Size: 132 bytes (0x84)
 */
class CMaintainer : public CServiceObject {
public:
	CMaintainer();
	virtual ~CMaintainer() override = default;

	virtual const CRuntimeClass* GetRuntimeClass() const override;
	virtual int32_t Process(int32_t* pStopFlag, int32_t nThreadIndex = 0) override;
	virtual void OnMessage() override;

	static void* CreateObject();

	static const CRuntimeClass ms_classCMaintainer;

protected:
	void* m_hEvent;
};

} // namespace BSLib

/**
 * IBSNet Interface
 * Native VTable @ 0x00B4239C
 */
class IBSNet {
public:
	virtual ~IBSNet() = default;

	// [RECONSTRUCTED] Native vfunc slot 2 @ +0x8 (0x0096B6C0)
	virtual bool Stop() = 0;

	// Native vfunc slot 3 @ +0x0C (0x0096B730)
	virtual bool Initialize(const tagNetConfig& config) = 0;

	// [RECONSTRUCTED] Native vfunc slot 4 @ +0x10 (0x0096B9C0)
	virtual int32_t Connect(const sockaddr_in* pRemoteAddr, const sockaddr_in* pLocalAddr = nullptr, int32_t dwParam1 = 0, int32_t bPersistent = 0) = 0;

	// [RECONSTRUCTED] Native vfunc slot 5 @ +0x14 (0x0096B8F0)
	virtual bool Connect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1 = 0, int32_t dwParam2 = 0) = 0;

	// [RECONSTRUCTED] Native vfunc slot 6 @ +0x18 (0x0096BB10)
	virtual bool CreateListener(const struct sockaddr* pSockAddr) = 0;

	// [RECONSTRUCTED] Native vfunc slot 7 @ +0x1C (0x0096BA80)
	virtual bool CreateListener(const char* szIP, uint16_t wPort) = 0;

	// [RECONSTRUCTED] Native vfunc slot 8 @ +0x20 (0x0096BC10)
	// Calling convention: __stdcall (retn 0x14 - 5 dwords: this, dwTaskID, pTask, pRuntimeClass, dwParam)
	virtual bool RegisterTask(uint32_t dwTaskID, BSLib::CTask* pTask, const CRuntimeClass* pRuntimeClass, uint32_t dwParam) = 0;

	// Convenience overload matching callers that omit pTask
	bool RegisterTask(uint32_t dwTaskID, const CRuntimeClass* pRuntimeClass, uint32_t dwParam = 0);

	// [RECONSTRUCTED] Native vfunc slot 10 @ +0x28 (0x0096BCF0)
	virtual bool ActivateTask(uint32_t dwTaskID, uint32_t dwThreadCount = 0, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwParam3 = 0, const int32_t* pAffinityConfig = nullptr) = 0;

	// [RECONSTRUCTED] Native vfunc slot 11 @ +0x2C (0x0096BD80)
	virtual bool DeactivateTask(uint32_t dwTaskID) = 0;

	// [RECONSTRUCTED] Native vfunc slot 12 @ +0x30 (0x0096BDB0)
	virtual bool PauseTask(uint32_t dwTaskID) = 0;

	// [RECONSTRUCTED] Native vfunc slot 13 @ +0x34 (0x0096BDE0)
	virtual bool UnregisterTask(uint32_t dwTaskID) = 0;

	// [RECONSTRUCTED] Native vfunc slot 14 @ +0x38 (0x0096C310)
	virtual bool GetStatistics(uint32_t dwFlags, void* pStats) = 0;

	// [RECONSTRUCTED] Native vfunc slot 16 @ +0x40 (0x0096B2F0)
	virtual void* GetTask(uint32_t dwTaskID) = 0;

	// [RECONSTRUCTED] Native vfunc slot 18 @ +0x48 (0x0096BF10)
	virtual void* AllocateBuffer(uint32_t bEncrypted = 0) = 0;

	// [RECONSTRUCTED] Native vfunc slot 19 @ +0x4C (0x0096C030)
	// (Previously mislabeled as ProcessIocpPacket - proven via LLIL / machine bytes)
	virtual int32_t ReleaseBuffer(void* pBuffer) = 0;

	// [RECONSTRUCTED] Native vfunc slot 20 @ +0x50 (0x0096BE80)
	// Transmits packet buffer over the given session socket
	virtual int32_t Send(void* pSession, void* pBuffer) = 0;
	virtual int32_t Send(uint32_t dwSessionID, void* pBuffer) = 0;

	// [RECONSTRUCTED] Native vfunc slot 28 @ +0x70 (0x0096C780)
	virtual bool ConnectDirect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1 = 0, int32_t dwParam2 = 0) = 0;

	// [RECONSTRUCTED] Native vfunc slot 30 @ +0x78 (0x0096C4C0)
	virtual void DumpMessageUsage(uint32_t dwType, uint32_t dwThreshold) = 0;
};

// Global network engine pointer matching native 0x00C8258C
extern IBSNet* g_pNetEngine;

// Internal global network engine instance pointer matching native 0x00C83840
extern class CNetEngine* g_pNetEngineInstance;

// OS platform detection flag matching native 0x00C63B5C (0 = WinNT, 1 = Win9x)
extern uint32_t g_bIsWin9xPlatform;

// Forward declarations of socket & session pointers
class CSockStream;
class CSockDatagram;
class CSession;

/**
 * [RECONSTRUCTED - 0x0096D8E0 / 0x0096D940]
 * CSocketPool
 * Native constructor @ 0x0096D8E0, destructor @ 0x0096D940
 * Size: 152 bytes (0x98)
 *
 * Implements pool for active/passive stream sockets (CSockStream).
 * Struct layout proven against machine bytes (0x0096D8E0, 0x0096EE60, 0x0096EF30):
 *   +0x00: uint32_t m_dwParam0
 *   +0x04: uint32_t m_dwParam4
 *   +0x08: uint32_t m_dwCapacity (500 / 0x1F4)
 *   +0x0C: uint8_t  m_queSockets[28] (CQue<CSockStream*>)
 *   +0x28: std::list<void*> m_listSockets (12 bytes)
 *   +0x34: uint32_t m_dwAllocatedCount
 *   +0x38: uint32_t m_dwFreeCount
 *   +0x3C: uint32_t m_dwTotalCount
 *   +0x40: std::string m_poolName (28 bytes)
 *   +0x5C: CCriticalSectionBS m_csPool (60 bytes / 0x3C)
 */
class CSocketPool {
public:
	explicit CSocketPool(const char* pszPoolName = "SocketPool");
	~CSocketPool();

	void* AcquireSocket();
	void ReleaseSocket(void* pSocket);
	void GetStatistics(uint32_t* pAllocated, uint32_t* pFree, uint32_t* pActive);
	void Clear();
	const std::string& GetPoolName() const;

private:
	uint32_t            m_dwParam0 = 0;          // +0x00
	uint32_t            m_dwParam4 = 0;          // +0x04
	uint32_t            m_dwCapacity = 500;      // +0x08 (0x1F4)
	uint8_t             m_queSockets[28] = {0};  // +0x0C (CQue)
	std::list<void*>    m_listSockets;           // +0x28 (std::list tracking all sockets)
	uint32_t            m_dwAllocatedCount = 0;  // +0x34
	uint32_t            m_dwFreeCount = 0;       // +0x38
	uint32_t            m_dwTotalCount = 0;      // +0x3C
	std::string         m_poolName;              // +0x40 (28 bytes)
	CCriticalSectionBS  m_csPool;                // +0x5C (60 bytes / 0x3C)
};

/**
 * [RECONSTRUCTED - 0x0096DB90 / 0x0096DBF0]
 * CSockDatagramPool (formerly mislabeled CIOCPWorkerPool)
 * Native constructor @ 0x0096DB90, destructor @ 0x0096DBF0
 * Size: 152 bytes (0x98)
 *
 * Implements pool for UDP datagram sockets (CSockDatagram).
 */
class CSockDatagramPool {
public:
	CSockDatagramPool();
	~CSockDatagramPool();

	void ReleaseSocket(void* pSocket);
	void GetStatistics(uint32_t* pAllocated, uint32_t* pFree, uint32_t* pActive);

private:
	uint32_t            m_dwParam0 = 0;          // +0x00
	uint32_t            m_dwParam4 = 0;          // +0x04
	uint32_t            m_dwCapacity = 500;      // +0x08 (0x1F4)
	uint8_t             m_queSockets[28] = {0};  // +0x0C (CQue)
	std::list<void*>    m_listSockets;           // +0x28
	uint32_t            m_dwAllocatedCount = 0;  // +0x34
	uint32_t            m_dwFreeCount = 0;       // +0x38
	uint32_t            m_dwTotalCount = 0;      // +0x3C
	std::string         m_poolName;              // +0x40 (28 bytes: "Unknown")
	CCriticalSectionBS  m_csPool;                // +0x5C (60 bytes / 0x3C)
};

using CIOCPWorkerPool = CSockDatagramPool;

/**
 * [RECONSTRUCTED - 0x0096AF40 / 0x0096AFC0]
 * CSessionManager (formerly mislabeled CNetworkThreadPool)
 * Native constructor @ 0x0096AF40, destructor @ 0x0096AFC0
 * Size: 200 bytes (0xC8)
 *
 * Owns session ID pool, session instances, and active connection table.
 */
class CSessionManager {
public:
	CSessionManager();
	~CSessionManager();

	int32_t RegisterSession(void* pSocket, int32_t dwParam);
	void* FindSession(uint32_t hSession);
	void GetStatistics(uint32_t* pActive, uint32_t* pAvailable);

private:
	CCriticalSectionBS  m_csPool;                // +0x00: Critical section (60 bytes / 0x3C)
	uint8_t             m_idPool[32] = {0};      // +0x3C: CIDPool (32 bytes)
	uint8_t             m_sessionPool[92] = {0}; // +0x5C: CQue<CSession*> "NetEngine::SessionPool" (92 bytes)
	std::vector<void*>  m_vecSessions;           // +0xB8: std::vector<CSession*> (12 bytes)
	uint32_t            m_dwReserved = 0;        // +0xC4: 4 bytes
};

using CNetworkThreadPool = CSessionManager;

/**
 * CSocketOption
 * Native constructor @ 0x00981270, cleanup @ 0x00981390
 * Size: 8 bytes (0x08)
 */
struct CSocketOption {
	void* pOpt1 = nullptr;              // +0x00
	void* pOpt2 = nullptr;              // +0x04
};

/**
 * [RECONSTRUCTED - 0x0096DD80 / 0x0096B470]
 * CNetBufferManager
 * Native constructor @ 0x0096DD80, destructor @ 0x0096B470
 * Size: 164 bytes (0xA4)
 *
 * Implements packet buffer management and opcode usage tracking.
 */
class CNetBufferManager {
public:
	CNetBufferManager();
	~CNetBufferManager();

	void* AllocateBuffer(uint32_t bEncrypted = 0);
	int32_t ReleaseBuffer(void* pBuffer);
	void DumpMessageUsage(uint32_t dwThreshold);

private:
	uint8_t                      m_msgPool[152] = {0}; // +0x00: CMsgPool "NetEngine::MsgPool" (152 bytes / 0x98)
	std::map<uint32_t, uint32_t> m_mapActiveMessages;  // +0x98: Active message tracking map (12 bytes)
};

/**
 * CSockListener
 * Native constructor @ 0x00967850, destructor @ 0x009678C0
 * Size: 156 bytes (0x9C)
 */
class CSockListener {
public:
	CSockListener();
	virtual ~CSockListener();

	bool BindAndListen(const struct sockaddr* pSockAddr);
	bool AttachToTask(BSLib::CTask* pTask);

private:
	intptr_t    m_socket;                   // +0x04: Native SOCKET handle
	uint16_t    m_nPort;                    // +0x08: Port
	uint8_t     m_pad[0x90] = { 0 };        // Internal socket buffer & IOCP context
};

/**
 * [RECONSTRUCTED - 0x0096B280]
 * tagConnectRequest
 * Native size: 44 bytes (0x2C)
 *
 * Stored in std::list<tagConnectRequest> inside CNetEngine (+0x4D4 / +0x4D8).
 * Node size in heap = 52 bytes (0x34) = 8 bytes pointers + 44 bytes payload.
 */
struct tagConnectRequest {
	sockaddr_in m_remoteAddr = {};      // +0x00: 16 bytes (remote endpoint sockaddr_in)
	sockaddr_in m_localAddr  = {};      // +0x10: 16 bytes (local bind sockaddr_in)
	uint32_t    m_dwParam1   = 0;       // +0x20: 4 bytes (user param 1)
	uint32_t    m_hSession   = 0;       // +0x24: 4 bytes (active session handle or 0)
	int32_t     m_nRetryCount= 0;       // +0x28: 4 bytes (failed retry count, resets to 0 on connect)
};

/**
 * [RECONSTRUCTED - 0x00978780]
 * BSNet_ResolveHostOrIP
 * Native implementation @ 0x00978780 (73 bytes)
 */
uint32_t BSNet_ResolveHostOrIP(const char* pszHostOrIP);

/**
 * [RECONSTRUCTED - 0x0096B280]
 * CConnectRequest_Initialize
 * Native implementation @ 0x0096B280 (97 bytes)
 */
void CConnectRequest_Initialize(tagConnectRequest* pReq, const sockaddr_in* pRemote, const sockaddr_in* pLocal, uint32_t dwParam1, uint32_t dwParam2);

/**
 * CNetEngine
 * Native VTable @ 0x00B4239C, RTTI: .?AVCNetEngine@@
 * Native Constructor @ 0x0096B330, Destructor @ 0x0096B520
 *
 * Struct layout proven against machine bytes (0x0096B330 - 0x0096B448):
 *   +0x00: vptr (0x00B4239C for IBSNet)
 *   +0x04: m_dwReserved (initialized to 0 @ 0x0096B412)
 *   +0x08: m_netConfig (tagNetConfig, 276 bytes / 0x114 @ 0x0096B35F)
 *   +0x11C: m_csConnectQueue (CCriticalSectionBS @ 0x0096B36B)
 *   +0x158: m_csSendQueue (CCriticalSectionBS @ 0x0096B37D)
 *   +0x194: m_pSockListener (CSockListener* @ 0x0096B415 / 0x0096BB8B)
 *   +0x198: m_activeSocketPool (CSocketPool "NetEngine::ActiveSocketPool" @ 0x0096B393)
 *   +0x230: m_passiveSocketPool (CSocketPool "NetEngine::PassiveSocketPool" @ 0x0096B3A9)
 *   +0x2C8: m_iocpWorkerPool (CIOCPWorkerPool @ 0x0096B3BA)
 *   +0x360: m_networkThreadPool (CNetworkThreadPool @ 0x0096B3CB)
 *   +0x428: m_socketOption (CSocketOption @ 0x0096B3DB)
 *   +0x430: m_netBufferManager (CNetBufferManager @ 0x0096B3EC)
 *   +0x4D4: m_listConnectRequests (std::list<tagConnectRequest> @ 0x0096B3FB)
 */
class CNetEngine : public IBSNet {
public:
	CNetEngine();
	virtual ~CNetEngine() override;

	// [RECONSTRUCTED] Native vfunc slot 2 @ +0x8 (0x0096B6C0)
	virtual bool Stop() override;

	// Native vfunc slot 3 @ +0x0C (0x0096B730)
	virtual bool Initialize(const tagNetConfig& config) override;

	// [RECONSTRUCTED] Native vfunc slot 4 @ +0x10 (0x0096B9C0)
	virtual int32_t Connect(const sockaddr_in* pRemoteAddr, const sockaddr_in* pLocalAddr = nullptr, int32_t dwParam1 = 0, int32_t bPersistent = 0) override;

	// [RECONSTRUCTED] Native vfunc slot 5 @ +0x14 (0x0096B8F0)
	virtual bool Connect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1 = 0, int32_t dwParam2 = 0) override;

	// [RECONSTRUCTED] Native vfunc slot 6 @ +0x18 (0x0096BB10)
	virtual bool CreateListener(const struct sockaddr* pSockAddr) override;

	// [RECONSTRUCTED] Native vfunc slot 7 @ +0x1C (0x0096BA80)
	virtual bool CreateListener(const char* szIP, uint16_t wPort) override;

	// [RECONSTRUCTED] Native vfunc slot 8 @ +0x20 (0x0096BC10)
	virtual bool RegisterTask(uint32_t dwTaskID, BSLib::CTask* pTask, const CRuntimeClass* pRuntimeClass, uint32_t dwParam) override;

	// [RECONSTRUCTED] Native vfunc slot 10 @ +0x28 (0x0096BCF0)
	virtual bool ActivateTask(uint32_t dwTaskID, uint32_t dwThreadCount = 0, uint32_t dwParam1 = 0, uint32_t dwParam2 = 0, uint32_t dwParam3 = 0, const int32_t* pAffinityConfig = nullptr) override;

	// [RECONSTRUCTED] Native vfunc slot 11 @ +0x2C (0x0096BD80)
	virtual bool DeactivateTask(uint32_t dwTaskID) override;

	// [RECONSTRUCTED] Native vfunc slot 12 @ +0x30 (0x0096BDB0)
	virtual bool PauseTask(uint32_t dwTaskID) override;

	// [RECONSTRUCTED] Native vfunc slot 13 @ +0x34 (0x0096BDE0)
	virtual bool UnregisterTask(uint32_t dwTaskID) override;

	// [RECONSTRUCTED] Native vfunc slot 14 @ +0x38 (0x0096C310)
	virtual bool GetStatistics(uint32_t dwFlags, void* pStats) override;

	// [RECONSTRUCTED] Native vfunc slot 16 @ +0x40 (0x0096B2F0)
	virtual void* GetTask(uint32_t dwTaskID) override;

	// [RECONSTRUCTED] Native vfunc slot 18 @ +0x48 (0x0096BF10)
	virtual void* AllocateBuffer(uint32_t bEncrypted = 0) override;

	// [RECONSTRUCTED] Native vfunc slot 19 @ +0x4C (0x0096C030)
	virtual int32_t ReleaseBuffer(void* pBuffer) override;

	// [RECONSTRUCTED] Native vfunc slot 20 @ +0x50 (0x0096BE80)
	virtual int32_t Send(void* pSession, void* pBuffer) override;
	virtual int32_t Send(uint32_t dwSessionID, void* pBuffer) override;

	// [RECONSTRUCTED - 0x0096CA20]
	int32_t EnqueueConnectRequest(const tagConnectRequest& req);

	// [RECONSTRUCTED - 0x0096C780]
	virtual bool ConnectDirect(const char* szRemoteIP, uint16_t wPort, const char* szLocalIP, int32_t dwParam1 = 0, int32_t dwParam2 = 0) override;

	// [RECONSTRUCTED] Native vfunc slot 30 @ +0x78 (0x0096C4C0)
	virtual void DumpMessageUsage(uint32_t dwType, uint32_t dwThreshold) override;

	// [RECONSTRUCTED - 0x0096CB00]
	int32_t ProcessKeepAliveConnections();

	// [RECONSTRUCTED - 0x0096CC90]
	void OnConnectSessionClosed(uint32_t hSession);

	// [RECONSTRUCTED - 0x0096C840]
	int32_t PostAcceptEx();

	// [RECONSTRUCTED - 0x0096C8D0]
	void ReleaseActiveSocket(void* pSocket);

	// [RECONSTRUCTED - 0x0096C900]
	void ReleasePassiveSocket(void* pSocket);

	// [PARTIAL] Native CRC32 table initialization @ 0x00978070
	static void InitializeCrc32Tables();

	const CNetConfig& GetNetConfig() const;

protected:
	// Exact struct layout matching native binary bytes:
	uint32_t                    m_dwReserved;               // +0x04
	CNetConfig                  m_netConfig;                // +0x08: 276 bytes (0x114)
	CCriticalSectionBS          m_csConnectQueue;           // +0x11C: 60 bytes (0x3C)
	CCriticalSectionBS          m_csSendQueue;              // +0x158: 60 bytes (0x3C)
	CSockListener*              m_pSockListener;            // +0x194: 4 bytes (CSockListener* @ 0x0096BB8B)
	CSocketPool                 m_activeSocketPool;         // +0x198: 152 bytes (0x98)
	CSocketPool                 m_passiveSocketPool;        // +0x230: 152 bytes (0x98)
	CIOCPWorkerPool             m_iocpWorkerPool;           // +0x2C8: 152 bytes (0x98)
	CNetworkThreadPool          m_networkThreadPool;        // +0x360: 200 bytes (0xC8)
	CSocketOption               m_socketOption;             // +0x428: 8 bytes (0x08)
	CNetBufferManager           m_netBufferManager;         // +0x430: 164 bytes (0xA4)
	std::list<tagConnectRequest> m_listConnectRequests;      // +0x4D4: 12 bytes (0x0C)
};

#endif // _JMX_LIBRARY_BSLIB_NETENGINE_H_
