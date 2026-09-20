/**
 * ============================================================================
 * Silkroad Online - Game Server Message Block Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\MsgBlock.cpp
 *
 * Implements:
 *   - tagMsgBlockLayerTable::GetLayer      @ 0x00533340
 *   - tagMsgBlockLayer::AppendRecipients   @ 0x005342A0
 *   - CMsgBlock::CMsgBlock                 @ 0x00533520
 *   - CMsgBlock::~CMsgBlock                @ 0x00533610
 *   - CMsgBlock::IsBroadcastEnabled        @ 0x00533900
 *   - CMsgBlock::AppendLayerRecipients     @ 0x005341E0
 *   - CMsgBlock::SendPacketToLayer         @ 0x00534150
 *   - CMsgBlock::AddObjectToLayer          @ 0x005340E0
 *   - CMsgBlock::RemoveObjectFromLayer     (port bookkeeping only)
 *   - CMsgBlock::OnObjectEnter             @ 0x00534340
 *   - CMsgBlock::OnObjectLeave             @ 0x005344B0
 *   - CMsgBlock::GetPCCount                @ 0x005347C0
 *   - CMsgBlockTerrain::IsInside           @ 0x005376C0
 * ============================================================================
 */

#include "MsgBlock.h"
#include "Region.h"
#include "GObj.h"
#include "GObjChar.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/Packet.h"

#include <cmath>
#include <stdexcept>

/**
 * [RECONSTRUCTED - 0x00533340] (125 bytes)
 * tagMsgBlockLayerTable::GetLayer
 * Native reports an invalid layer then its VC8 checked subscript throws.
 * The host representation throws directly; it must never substitute layer zero.
 */
tagMsgBlockLayer* tagMsgBlockLayerTable::GetLayer(uint16_t wLayerID) {
	if (wLayerID == 0) {
		return &m_Layer0;
	}

	if (wLayerID > m_vecLayer.size()) {
		throw std::out_of_range("message block layer is not initialized");
	}
	return &m_vecLayer[wLayerID - 1];
}

/**
 * [PARTIAL - 0x005342A0] (144 bytes)
 * tagMsgBlockLayer::AppendRecipients
 *
 * Every player of this layer is added to the packet's recipient list. The native walks its map straight into
 * the CAgentMsg target buffer at 0x00CD7E18 and then hands the whole buffer to the agent servers as grouped
 * 0x220A / 0x220E / 0x220F envelopes (CAgentMsg::DispatchToTargets 0x004054D0); that inter-server transport
 * is not ported. The current CPacket::Send is logging-only, and an object GID is
 * not a verified session identity. This path does NOT establish network delivery.
 */
void tagMsgBlockLayer::AppendRecipients(CPacket* pPacket) {
	for (std::map<uint32_t, CGObj*>::const_iterator it = m_mapPC.begin(); it != m_mapPC.end(); ++it) {
		CGObj* pObj = it->second;
		if (pObj == nullptr) {
			continue;
		}

		ASSERT(pObj->IsPlayer()); // 0x00534302

		// 0x00534309 / 0x00534314: the native takes the player's client context and gives it to
		// CAgentMsg::AddTargetClient. TODO: replace this GID/logging-only substitution
		// with verified client-context routing before enabling live visibility.
		pPacket->Send(pObj->GetGlobalID(), 1);
	}
}

/**
 * [PARTIAL - 0x00533520] (126 bytes)
 * CMsgBlock::CMsgBlock
 * The object set at +0x1C is kept as padding.
 */
CMsgBlock::CMsgBlock()
	: m_pRegion(nullptr)
	, m_wBlockZ(0)
	, m_wBlockX(0) {
	std::memset(pad1C, 0, sizeof(pad1C));
}

/**
 * [STUB - 0x00533610]
 * CMsgBlock::~CMsgBlock
 * The native body is not reconstructed.
 */
CMsgBlock::~CMsgBlock() {
}

/**
 * [RECONSTRUCTED - 0x00533900] (12 bytes)
 * CMsgBlock::IsBroadcastEnabled
 * The flag the region carries at +0x0C: 1 on a terrain or object region, 0 on a proxy one, which is how a
 * proxy region stays out of everybody's view.
 */
int32_t CMsgBlock::IsBroadcastEnabled() const {
	if (m_pRegion == nullptr) {
		return 0; // 0x00533905
	}
	return static_cast<int32_t>(m_pRegion->m_dwUnk0C);
}

/**
 * [RECONSTRUCTED - 0x005341E0] (183 bytes)
 * CMsgBlock::AppendLayerRecipients
 * 0xFFFF collects every layer the table holds, anything else just that one. A layer nobody is standing in
 * is skipped before the recipient walk.
 */
void CMsgBlock::AppendLayerRecipients(uint16_t wLayerID, CPacket* pPacket) {
	if (wLayerID == 0xFFFF) {
		// 0x005341F4: layers 1 .. count, layer 0 is not broadcast to
		for (uint32_t i = 1; i < m_LayerTable.m_vecLayer.size() + 1; ++i) {
			tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(static_cast<uint16_t>(i));
			if (!pLayer->m_mapPC.empty()) {                 // 0x00534255
				pLayer->AppendRecipients(pPacket);           // 0x00534268
			}
		}
		return;
	}

	tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(wLayerID); // 0x00534272
	if (!pLayer->m_mapPC.empty()) {                              // 0x00534279
		pLayer->AppendRecipients(pPacket);                        // 0x0053428C
	}
}

/**
 * [PARTIAL - 0x00534150] (135 bytes)
 * CMsgBlock::SendPacketToLayer
 * Slot 6. A packet reaches this block and every neighbour of it that is broadcasting - that pair is what
 * "nearby" means for the whole server.
 */
void CMsgBlock::SendPacketToLayer(uint16_t wLayerID, CPacket* pPacket) {
	AppendLayerRecipients(wLayerID, pPacket); // 0x00534162

	for (std::vector<CMsgBlock*>::const_iterator it = m_vecNeighbour.begin();
		it != m_vecNeighbour.end(); ++it) {
		CMsgBlock* pNeighbour = *it;
		if (pNeighbour != nullptr && pNeighbour->IsBroadcastEnabled() == 1) { // 0x0053418C - 0x00534199
			pNeighbour->AppendLayerRecipients(wLayerID, pPacket);              // 0x005341A1
		}
	}

	pPacket->ResetReadCursors(); // TODO: native 0x5341BB commits WRITE cursors; this is not equivalent.
}

/**
 * [RECONSTRUCTED - 0x005340E0] (105 bytes)
 * CMsgBlock::AddObjectToLayer
 * A player and anything else are kept in separate maps of the layer, both keyed by the object's game id.
 */
void CMsgBlock::AddObjectToLayer(CGObj* pObj) {
	tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(pObj->GetLayerID()); // 0x005340EC

	if (pObj->IsPlayer()) {                                    // 0x005340FA
		ASSERT(pLayer->m_mapPC.find(pObj->GetGlobalID()) == pLayer->m_mapPC.end()); // 0x00534113
		pLayer->m_mapPC[pObj->GetGlobalID()] = pObj;            // 0x0053411E
	} else {
		ASSERT(pLayer->m_mapNonPC.find(pObj->GetGlobalID()) == pLayer->m_mapNonPC.end()); // 0x00534135
		pLayer->m_mapNonPC[pObj->GetGlobalID()] = pObj;         // 0x00534140
	}
}

/**
 * [PORT BOOKKEEPING - no standalone native equivalent]
 * CMsgBlock::RemoveObjectFromLayer
 * Native 0x00534050 broadcasts 0x3016; map erasure occurs inside 0x005344B0.
 */
void CMsgBlock::RemoveObjectFromLayer(CGObj* pObj) {
	tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(pObj->GetLayerID()); // 0x00534054

	if (pObj->IsPlayer()) {
		pLayer->m_mapPC.erase(pObj->GetGlobalID());
	} else {
		pLayer->m_mapNonPC.erase(pObj->GetGlobalID());
	}
}

/**
 * [PARTIAL - 0x00534340] (356 bytes)
 * CMsgBlock::OnObjectEnter
 * Slot 2, reached from CGObj::SetCellNode (0x00485BE8). The layer's player count follows the object and the
 * object joins the layer's map, which is what makes it a recipient of everything broadcast here afterwards.
 *
 * Not ported: the whole enter-view protocol. The native tells this block and each neighbour that the object
 * became visible - CMsgBlock::SendObjectSpawnToPeer (0x00533E70, opcodes 0x3017 / 0x3018 / 0x3019) and
 * CMsgBlock::BroadcastObjectSpawn (0x00533F70, opcode 0x3015) - and when the object merely moved between two
 * blocks that already saw each other it only notifies the blocks newly in view (0x00534420). Those need the
 * object serializer at slot 294, which is not ported, so nothing is announced and only the bookkeeping runs.
 */
void CMsgBlock::OnObjectEnter(CGObj* pObj, CMsgBlock* /*pOldBlock*/, int32_t /*nMode*/) {
	tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(pObj->GetLayerID()); // 0x0053435C

	if (pObj->IsPlayer()) {
		pLayer->wPCCount += 1; // 0x0053448C
	}

	AddObjectToLayer(pObj); // 0x00534496
}

/**
 * [PARTIAL - 0x005344B0] (343 bytes)
 * CMsgBlock::OnObjectLeave
 * Slot 3. The mirror of OnObjectEnter; the leave-view packets the native sends are not ported for the same
 * reason.
 */
void CMsgBlock::OnObjectLeave(CGObj* pObj, CMsgBlock* /*pNewBlock*/) {
	tagMsgBlockLayer* pLayer = m_LayerTable.GetLayer(pObj->GetLayerID());

	if (pObj->IsPlayer() && pLayer->wPCCount > 0) {
		pLayer->wPCCount -= 1;
	}

	RemoveObjectFromLayer(pObj);
}

/**
 * [RECONSTRUCTED - 0x005347C0] (20 bytes)
 * CMsgBlock::GetPCCount
 */
uint16_t CMsgBlock::GetPCCount(const tagRegionContext* pContext) {
	return m_LayerTable.GetLayer(pContext->wLayerID)->wPCCount;
}

// ============================================================================
// CMsgBlockTerrain
// ============================================================================

CMsgBlockTerrain::CMsgBlockTerrain() {
}

CMsgBlockTerrain::~CMsgBlockTerrain() {
}

/**
 * [RECONSTRUCTED - 0x005376C0] (64 bytes)
 * CMsgBlockTerrain::IsInside
 * Slot 4. The position's grid cell has to be this block's own: a region is 1920 units across and each block
 * covers 320 of them, so six by six of them tile it.
 */
int32_t CMsgBlockTerrain::IsInside(tagObjLocation pos) const {
	// Reject non-finite/out-of-int-range input before the host float-to-int conversion.
	if (!std::isfinite(pos.fPosX) || !std::isfinite(pos.fPosZ) ||
	    std::abs(double(pos.fPosX) / kMsgBlockSize) > 2147483647.0 ||
	    std::abs(double(pos.fPosZ) / kMsgBlockSize) > 2147483647.0) return 0;
	// 0x005376C5: both divisions happen at double width and are truncated toward zero by CRT_ftol
	const int32_t nX = static_cast<int32_t>(static_cast<double>(pos.fPosX) / kMsgBlockSize);
	if (nX != static_cast<int32_t>(m_wBlockX)) {
		return 0; // 0x005376DC
	}

	const int32_t nZ = static_cast<int32_t>(static_cast<double>(pos.fPosZ) / kMsgBlockSize);
	if (nZ != static_cast<int32_t>(m_wBlockZ)) {
		return 0; // 0x005376ED
	}
	return 1; // 0x005376EF
}
