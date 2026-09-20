/**
 * ============================================================================
 * Silkroad Online - Game Server Region
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Region.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CRegion  .?AVCRegion@@ : CBase  vftable 0x00AF8004 (18 slots, 5-7 and 11/14-17 pure),
 *     runtime class 0x00ADE990 ("CRegion", 0x2C)
 *   CRgnTerrain (0x00AF8144): geometry implemented below; admission/loading still partial.
 *   Not ported: CRgnProxyTerrain (0x00AF80BC), CRgnObject (0x00AF805C).
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_REGION_H_
#define _SR_GAMESERVER_REGION_H_

#include <cstdint>
#include <array>
#include "MsgBlock.h"
#include "GlobalPos.h"
#include "GameWorld.h"
#include "../JMX_Library/BSLib/BSObj.h"

class CMsgBlock;

/**
 * [PARTIAL - 0x00537EE0, vftable 0x00AF8004] (Size: 0x2C)
 * CRegion
 * One map region. The terrain kinds split each region into CMsgBlocks.
 */
class CRegion : public CBase {
public:
	// Native 0x00537EE0 (113 bytes, this on the stack, si = wRegionID)
	explicit CRegion(uint16_t wRegionID);

	// vftable[1]: scalar deleting destructor 0x00537F60, body 0x00537F80
	virtual ~CRegion();

	// vftable[0] 0x00537FF0 (6 bytes)
	virtual const CRuntimeClass* GetRuntimeClass() const override;

	// vftable[6]: pure. CRgnTerrain 0x0053AD30 returns its 6x6 CMsgBlockTerrain for the position,
	// CRgnProxyTerrain 0x0053A810 returns NULL.
	virtual CMsgBlock* GetMsgBlock(tagObjLocation pos) = 0;

	// Native 0x005380B0 (78 bytes, retn 0x1C)
	uint16_t GetPCCount(tagRegionContext context, tagObjLocation pos);

public:
	static const CRuntimeClass ms_runtimeClass;

	int32_t  m_nRegionType; // +0x04: -1 CRegion, 0 CRgnTerrain (0x0053A8B7), 1 CRgnProxyTerrain (0x0053A74B), 2 CRgnObject (0x00538420)
	uint16_t m_wRegionID;   // +0x08: constructor argument (0x00537F2E)
	uint16_t pad0A;         // +0x0A
	uint32_t m_dwUnk0C;     // +0x0C: 1 in CRgnTerrain / CRgnObject, 0 in CRgnProxyTerrain; vftable[3] 0x00538010 tests it
	void*    m_pUnk10;      // +0x10: set by vftable[2] 0x00538000, asserted by vftable[4] 0x00538080
	uint32_t m_dwUnk14;     // +0x14
	uint8_t  pad18[0x10];   // +0x18 - +0x27: +0x1C..+0x24 zeroed by the constructor
	uint16_t m_wUnk28;      // +0x28
};

// Terrain geometry reconstructed from 0x53A850, 0x53A9D0, 0x53AD30.
// This is a host C++ representation, not an ABI-compatible x86 layout.
// Region admission, climate loading and inter-server handoff remain separate.
class CRgnTerrain : public CRegion {
public:
    explicit CRgnTerrain(uint16_t regionID, uint16_t layerCount);
    CRgnTerrain(const CRgnTerrain&) = delete;
    CRgnTerrain& operator=(const CRgnTerrain&) = delete;
    const CRuntimeClass* GetRuntimeClass() const override;
    static const CRuntimeClass ms_runtimeClass;
    CMsgBlock* GetMsgBlock(tagObjLocation pos) override;
    CMsgBlockTerrain* GetBlock(uint16_t x, uint16_t z);
    // Supply only regions admitted by the native world/region data loader.
    // Call LinkBlocks after all eight directional links have been installed.
    void SetNeighbour(CRgnTerrain& region);
    void LinkBlocks();
private:
    std::array<CMsgBlockTerrain, 36> m_blocks;
    std::array<CRgnTerrain*, 8> m_neighbours{};
};

#endif // _SR_GAMESERVER_REGION_H_
