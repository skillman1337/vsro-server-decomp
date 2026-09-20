#include "SR_GameServer/GObjPC.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

struct Player : CGObjPC {
    uint8_t mode = 0;
    unsigned sends = 0;
    CPacket packet;
    uint8_t GetBodyMode() const override { return mode; }
    CPacket* AllocMsgForPeer(uint16_t opcode) override { packet.m_streamBuffer.clear(); packet.SetOpcode(opcode); return &packet; }
    int32_t SendMsgToPeer(CPacket* p) override {
        if (p->GetOpcode()!=0x304E || p->m_streamBuffer.size()!=3 ||
            p->m_streamBuffer[0]!=4 || p->m_streamBuffer[2]!=7)
            throw std::runtime_error("berserk wire mismatch");
        ++sends; return 1;
    }
};
int main(int argc, char** argv) {
    if (argc!=2) return 2;
    std::ifstream input(argv[1]);
    unsigned mode, initial, expected, dirty, notifications, count=0;
    int delta;
    while (input>>mode>>initial>>delta>>expected>>dirty>>notifications) {
        Player player; CInstancePC record;
        player.m_pDataPermanent=&record; player.mode=mode;
        record.m_byBerserkPoints=initial; record.m_dwStateFlags=0x200;
        record.m_dwRemainSkillPoint=7654321;
        player.ModifyBerserkPoints(delta,7);
        if (record.m_byBerserkPoints!=expected || record.m_dwStateFlags!=dirty ||
            player.sends!=notifications || record.m_dwRemainSkillPoint!=7654321) return 1;
        if (notifications && player.packet.m_streamBuffer[1]!=expected) return 4;
        player.m_pDataPermanent=nullptr;
        ++count;
    }
    if (!input.eof() || count!=126) return 3;
    std::cout<<count<<" native berserk point mutation sequences passed\n";
}
