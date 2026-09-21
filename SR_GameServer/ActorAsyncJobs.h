#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <stdexcept>

// Portable ownership projection, not a native ABI layout. Callback destruction
// represents virtual release (including subtype-owned payload cleanup).
class ActorAsyncJob {
public:
 uint32_t flags=0;
 virtual ~ActorAsyncJob()=default;
 virtual uint32_t Advance(float deltaSeconds)=0;
 virtual uint8_t Category()const=0;
 virtual uint32_t Matches(uint32_t kind,uint32_t value)const=0;
};
class ActorAsyncJobs {
 std::list<std::unique_ptr<ActorAsyncJob>> jobs;
 std::function<uint32_t()> now;
 uint32_t lastTick;
public:
 explicit ActorAsyncJobs(std::function<uint32_t()> clock):now(std::move(clock)),lastTick(0){
  if(!now)throw std::invalid_argument("missing actor job clock");lastTick=now();
 }
 ActorAsyncJobs(const ActorAsyncJobs&)=delete;
 ActorAsyncJobs& operator=(const ActorAsyncJobs&)=delete;
 ~ActorAsyncJobs(){Clear();}
 size_t Size()const{return jobs.size();}
 void ResetClock(){lastTick=now();}
 void Append(std::unique_ptr<ActorAsyncJob> job){
  if(!job)throw std::invalid_argument("null actor job");
  if(jobs.empty())ResetClock();jobs.push_back(std::move(job));
 }
 void Advance(){
  if(jobs.empty())return;
  const uint32_t tick=now();
  const float delta=float(double(uint32_t(tick-lastTick))/1000.0);
  lastTick=tick;
  for(auto it=jobs.begin();it!=jobs.end();){
   if((*it)->Advance(delta)==0){it->reset();it=jobs.erase(it);}else ++it;
  }
 }
 void Clear(){
  // Release every job before clearing any nodes, preserving callback order.
  for(auto& job:jobs)job.reset();jobs.clear();lastTick=0;
 }
 bool RemoveFirstMatching(uint8_t category,uint32_t kind,uint32_t value){
  for(auto it=jobs.begin();it!=jobs.end();++it){
   auto& job=*it;
   if(job->flags!=0||job->Category()!=category||job->Matches(kind,value)!=1)continue;
   job.reset();jobs.erase(it);return true;
  }
  return false;
 }
};
