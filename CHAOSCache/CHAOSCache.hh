#ifndef __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__
#define __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__

#include "mem/cache/cache.hh"
#include "params/CHAOSCache.hh"
#include <bitset>
#include <string>
#include <iostream>
#include <fstream>
#include <random>

namespace gem5 {
  class CHAOSCache : public Cache {
    private:
      std::mt19937 rng;
      double probability;
      bool injectOnRead;
      bool injectOnWrite;
      int numBitsToChange;
      std::uniform_real_distribution<double> probDist;
      int firstClock, lastClock;
      std::string faultType;
      std::ofstream logFile;
      int tickToClockRatio;
      uint64_t generateRandomMask(std::mt19937 &rng, int numBitsToChange, unsigned size);
      void injectFault(PacketPtr pkt);

    public:
      CHAOSCache(const CHAOSCacheParams &p);
      Tick recvAtomic(PacketPtr pkt) override;
      void recvTimingReq(PacketPtr pkt) override;
      ~CHAOSCache();
  };
} // namespace gem5

#endif // __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__