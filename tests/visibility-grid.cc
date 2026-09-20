#include "SR_GameServer/Region.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <iostream>

template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
}
int main() {
    CRgnTerrain a(0x6464, 2), east(0x6465, 2), northeast(0x6565, 2);
    tagObjLocation p{};
    for (unsigned z=0; z<6; ++z) for (unsigned x=0; x<6; ++x) {
        p.fPosX=x*320.f; p.fPosZ=z*320.f;
        auto* b=a.GetMsgBlock(p);
        assert(b==a.GetBlock(x,z));
        assert(b->m_pRegion==&a && b->m_wBlockX==x && b->m_wBlockZ==z);
        for (unsigned layer=0; layer<=2; ++layer) assert(b->m_LayerTable.GetLayer(layer)->wPCCount==0);
        assert(b->m_LayerTable.GetLayer(1)!=b->m_LayerTable.GetLayer(2));
        p.fPosX=std::nextafter((x+1)*320.f, 0.f);
        p.fPosZ=std::nextafter((z+1)*320.f, 0.f);
        assert(a.GetMsgBlock(p)==b);
    }
    for(float v : {-1.f,1920.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        p.fPosX=v;p.fPosZ=0;rejects([&]{a.GetMsgBlock(p);});
        p.fPosX=0;p.fPosZ=v;rejects([&]{a.GetMsgBlock(p);});
    }
    rejects([&]{a.GetBlock(6,0);});
    rejects([&]{a.GetBlock(0,0)->m_LayerTable.GetLayer(3);});
    rejects([&]{a.GetBlock(0,0)->m_LayerTable.GetLayer(0xffff);});
    p.fPosX=std::numeric_limits<float>::quiet_NaN();p.fPosZ=0;
    assert(!a.GetBlock(0,0)->IsInside(p));
    assert(a.GetRuntimeClass()==&CRgnTerrain::ms_runtimeClass);
    assert(a.GetBlock(2,2)->m_vecNeighbour.size()==8);
    assert(a.GetBlock(0,0)->m_vecNeighbour.size()==3);
    a.SetNeighbour(east);a.SetNeighbour(northeast);a.LinkBlocks();
    auto& n=a.GetBlock(5,5)->m_vecNeighbour;
    assert(std::find(n.begin(),n.end(),east.GetBlock(0,5))!=n.end());
    assert(std::find(n.begin(),n.end(),northeast.GetBlock(0,0))!=n.end());
    auto before=n; a.LinkBlocks(); assert(before==n);
    assert(std::find(n.begin(),n.end(),a.GetBlock(5,5))==n.end());
    rejects([&]{a.SetNeighbour(a);});
    std::cout << "terrain grid boundaries, layer initialization and region links passed\n";
}
