#pragma once
#include "EventDispatch.h"
#include "EventArguments.h"
#include <algorithm>
#include <array>
namespace NativeEvents {
inline void Unregister(Handler*);
struct Association { Receiver* receiver; Handler* handler; };
struct Registry {
 std::vector<Association> entries;
 void Detach(Handler*,bool);
 void Clear();
 void RemoveFirstPhaseZeroEvent12();
};
// Portable registration projection, not an ABI/layout claim. Receiver owns
// handler lifetime; Registry stores borrowed associations. Clear's destructor
// callback represents the native scalar deleting destructor and is mandatory.
struct Receiver {
 uint32_t orderKey=0; // explicit native receiver-pointer ordering identity
 std::array<std::vector<Handler*>,3> phases;
 Registry own;
 void Register(uint32_t phase,int32_t key,Handler* h,Registry* owner=nullptr){
  if(phase>=3||!h||h->receiver||orderKey==0)throw std::logic_error("invalid event registration");
  if(!owner)owner=&own;
  h->phase=phase;h->eventKey=key;h->receiver=this;h->registry=owner;
  auto& rows=phases[phase];
  auto pos=std::upper_bound(rows.begin(),rows.end(),key,[](int32_t k,Handler* p){return k<p->eventKey;});
  rows.insert(pos,h);
  auto entry=std::upper_bound(owner->entries.begin(),owner->entries.end(),orderKey,
   [](uint32_t key,const Association& a){return key<a.receiver->orderKey;});
  owner->entries.insert(entry,{this,h});
 }
 std::vector<Handler*> EqualRange(uint32_t phase,int32_t key)const{
  if(phase>=3)throw std::logic_error("invalid event phase");
  std::vector<Handler*> out;for(auto* h:phases[phase])if(h->eventKey==key)out.push_back(h);return out;
 }
 bool IsAbsent(uint32_t phase,int32_t key)const{return EqualRange(phase,key).empty();}
 uint32_t Disable(Handler* h,bool detach){
  if(detach&&h->registry)h->registry->Detach(h,false);
  for(auto& rows:phases)for(auto* entry:rows)if(entry==h){h->enabled=0;return 0;}
  return 2;
 }
 template<class Host> uint32_t Execute(uint32_t phase,int32_t key,Host& host){
  struct Adapter {
   Host& h;
   void ResetArguments(){h.ResetArguments();}
   uint32_t NowMillis(){return h.NowMillis();}
   uint32_t Invoke(Handler& value){return h.Invoke(value);}
   void Unregister(Handler& value){NativeEvents::Unregister(&value);}
  } adapter{host};
  return NativeEvents::Dispatch(EqualRange(phase,key),adapter);
 }
 template<class Clock,class Callback> uint32_t ExecuteArguments(uint32_t phase,int32_t key,Arguments& arguments,Clock now,Callback invoke){
  struct Adapter {
   Arguments& arguments;Clock& now;Callback& invoke;
   void ResetArguments(){arguments.ResetCursor();}
   uint32_t NowMillis(){return now();}
   uint32_t Invoke(Handler& value){return invoke(value,arguments);}
  } adapter{arguments,now,invoke};
  return Execute(phase,key,adapter);
 }
 template<class Destroy> void Clear(Destroy destroy){
  own.Clear();for(auto& rows:phases){for(auto* h:rows)destroy(h);rows.clear();}
 }
};
inline void Registry::Detach(Handler* h,bool disable){
 for(auto i=entries.begin();i!=entries.end();++i)if(i->handler==h){
  if(disable)i->receiver->Disable(h,false);entries.erase(i);return;
 }
}
inline void Registry::Clear(){for(auto a:entries)a.receiver->Disable(a.handler,false);entries.clear();}
inline void Registry::RemoveFirstPhaseZeroEvent12(){
 for(auto a:entries)if(a.handler->phase==0&&a.handler->eventKey==12){Detach(a.handler,true);return;}
}
inline void Unregister(Handler* h){
 if(!h->registry)throw std::logic_error("unregistered event handler");h->registry->Detach(h,true);
}
inline bool IsAbsentEverywhere(const Receiver& global,const Receiver& local,uint32_t phase,int32_t key){
 return global.IsAbsent(phase,key)&&local.IsAbsent(phase,key);
}
}
