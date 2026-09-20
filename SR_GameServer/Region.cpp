/**
 * ============================================================================
 * Silkroad Online - Game Server Region Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Region.cpp
 *
 * Implements:
 *   - CRegion::CRegion @ 0x00537EE0
 *   - CRegion::~CRegion @ 0x00537F80
 *   - CRegion::GetRuntimeClass (vftable[0]) @ 0x00537FF0
 *   - CRegion::GetPCCount @ 0x005380B0
 * ============================================================================
 */

#include "Region.h"
#include "MsgBlock.h"
#include <cstring>

// Native runtime descriptor @ 0x00ADE990: "CRegion" (0x00AF7FF4), 0x2C, no factory, no base
const CRuntimeClass CRegion::ms_runtimeClass = {
	"CRegion",
	0x2C,
	nullptr,
	nullptr,
	nullptr
};

/**
 * [RECONSTRUCTED - 0x00537EE0] (113 bytes)
 * CRegion::CRegion
 */
CRegion::CRegion(uint16_t wRegionID)
	: m_nRegionType(-1)
	, m_wRegionID(wRegionID)
	, pad0A(0)
	, m_dwUnk0C(0)
	, m_pUnk10(nullptr)
	, m_dwUnk14(0)
	, m_wUnk28(0) {
	std::memset(pad18, 0, sizeof(pad18));
}

/**
 * [STUB - 0x00537F80]
 * CRegion::~CRegion
 * The native body is not reconstructed.
 */
CRegion::~CRegion() {
}

/**
 * [RECONSTRUCTED - 0x00537FF0] (6 bytes)
 * CRegion::GetRuntimeClass (vftable[0])
 */
const CRuntimeClass* CRegion::GetRuntimeClass() const {
	return &ms_runtimeClass;
}

/**
 * [RECONSTRUCTED - 0x005380B0] (78 bytes)
 * CRegion::GetPCCount
 * Players on the context's layer inside the CMsgBlock that contains pos.
 */
uint16_t CRegion::GetPCCount(tagRegionContext context, tagObjLocation pos) {
	CMsgBlock* pMsgBlock = GetMsgBlock(pos);
	if (pMsgBlock == nullptr) {
		return 0;
	}
	return pMsgBlock->GetPCCount(&context);
}


#include <cmath>
#include <stdexcept>

namespace {
// Native signed (dx,dz) table at 0x00C64090.
constexpr int terrainDirections[8][2] = {
    {-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}
};
}

CRgnTerrain::CRgnTerrain(uint16_t regionID, uint16_t layerCount)
    : CRegion(regionID) {
    if (regionID & 0x8000)
        throw std::invalid_argument("terrain region cannot use an object-region ID");
    m_nRegionType = 0;
    m_dwUnk0C = 1;
    // 0x53A9D0 -> slot 1 -> 0x537B60 -> 0x5336A0.
    for (uint16_t z = 0; z < 6; ++z) {
        for (uint16_t x = 0; x < 6; ++x) {
            auto& block = m_blocks[z * 6 + x];
            block.m_pRegion = this;
            block.m_wBlockX = x;
            block.m_wBlockZ = z;
            block.m_LayerTable.m_vecLayer.resize(layerCount);
        }
    }
    LinkBlocks();
}

CMsgBlockTerrain* CRgnTerrain::GetBlock(uint16_t x, uint16_t z) {
    if (x >= 6 || z >= 6)
        throw std::out_of_range("terrain block coordinates outside 6x6 grid");
    return &m_blocks[z * 6 + x];
}

CMsgBlock* CRgnTerrain::GetMsgBlock(tagObjLocation pos) {
    // 0x53AD30: validate before conversion; 1920 belongs to the next region.
    // Exceptions replace native assertion/dump paths, avoiding host float-to-int UB.
    if (!std::isfinite(pos.fPosX) || !std::isfinite(pos.fPosZ) ||
        pos.fPosX < 0 || pos.fPosX >= SRO_REGION_WIDTH ||
        pos.fPosZ < 0 || pos.fPosZ >= SRO_REGION_WIDTH)
        throw std::out_of_range("terrain position outside region");
    return GetBlock(static_cast<uint16_t>(double(pos.fPosX) / kMsgBlockSize),
                    static_cast<uint16_t>(double(pos.fPosZ) / kMsgBlockSize));
}

void CRgnTerrain::SetNeighbour(CRgnTerrain& region) {
    // 0x53AAB0: signed sector differences, without byte wrapping.
    const int dx = int(region.m_wRegionID & 255) - int(m_wRegionID & 255);
    const int dz = int(region.m_wRegionID >> 8) - int(m_wRegionID >> 8);
    for (size_t i = 0; i < m_neighbours.size(); ++i) {
        if (dx != terrainDirections[i][0] || dz != terrainDirections[i][1]) continue;
        if (m_neighbours[i] && m_neighbours[i] != &region)
            throw std::logic_error("duplicate terrain neighbour");
        m_neighbours[i] = &region;
        return;
    }
    throw std::invalid_argument("regions are not adjacent");
}

void CRgnTerrain::LinkBlocks() {
    // 0x53AB90 / 0x53AC20: eight neighbours, including adjacent regions;
    // absent/proxy regions contribute no block. Region links are non-owning.
    for (auto& block : m_blocks) {
        block.m_vecNeighbour.clear();
        for (const auto& direction : terrainDirections) {
            const int x = int(block.m_wBlockX) + direction[0];
            const int z = int(block.m_wBlockZ) + direction[1];
            const int dx = (x + 6) / 6 - 1;
            const int dz = (z + 6) / 6 - 1;
            CRgnTerrain* region = this;
            if (dx || dz) {
                region = nullptr;
                for (size_t i = 0; i < m_neighbours.size(); ++i)
                    if (terrainDirections[i][0] == dx && terrainDirections[i][1] == dz)
                        region = m_neighbours[i];
            }
            if (region && region->m_dwUnk0C)
                block.m_vecNeighbour.push_back(region->GetBlock((x + 6) % 6, (z + 6) % 6));
        }
    }
}

// 0xADE954 / 0x53A840: native descriptor, not sizeof(host class).
const CRuntimeClass CRgnTerrain::ms_runtimeClass = {
    "CRgnTerrain", 0x158C, nullptr, nullptr, &CRegion::ms_runtimeClass
};
const CRuntimeClass* CRgnTerrain::GetRuntimeClass() const { return &ms_runtimeClass; }
