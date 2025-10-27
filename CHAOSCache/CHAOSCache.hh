#ifndef __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__
#define __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__

#include <random>

#include "mem/cache/cache.hh"
#include "params/CHAOSCache.hh"
#include "sim/sim_object.hh"

#include "mem/cache/cache.hh"
#include "params/CHAOSCache.hh"
#include <bitset>
#include <string>
#include <iostream>
#include <random>
#include <stdexcept>
#include "base/output.hh"
#include <map>

namespace gem5
{

class CHAOSCache : public SimObject
{
  public:
    CHAOSCache(const CHAOSCacheParams& params);
    virtual ~CHAOSCache() {}

  private:
    enum class FaultType {
      BitFlip,
      StuckAtZero,
      StuckAtOne,
      Random
    };

    struct PermanentFault {
      FaultType fault_type;
      uint64_t mask;
      bool update;
    };

    Cache* targetCache;
    double probability;
    int bits_to_change;
    int corruption_size;
    uint64_t first_clock, last_clock;
    FaultType fault_type_enum;
    unsigned char fault_mask;
    int tick_to_clock_ratio;
    float bit_flip_prob, stuck_at_zero_prob, stuck_at_one_prob;
    int cycles_permament_fault_check;
    bool write_log;

    EventFunctionWrapper attackEvent, periodicCheck;
    Tick first_tick, last_tick, ticks_permament_fault_check;
    std::map<std::pair<Addr, int>, PermanentFault> permanent_faults;
    std::geometric_distribution<unsigned> inter_fault_cycles_dist;
    std::discrete_distribution<int> random_fault_distribution;
    
    std::mt19937 rng;
    std::random_device rd;
    OutputStream *log_stream;
    
    static FaultType stringToFaultType(const std::string &s);
    const char* faultTypeToString(CHAOSCache::FaultType f);
    void scheduleAttack(Tick tick);
    void scheduleCheckPermanentFault(Tick time);
    BaseTags* getTags() const;
    uint8_t generateRandomMask(std::mt19937 &rng, int bits_to_change, unsigned size);
    void injectFault();
    void checkPermanent();

    struct CHAOSCacheStats : public statistics::Group
    {
      statistics::Scalar numFaultsInjected;
      statistics::Scalar numBitFlips;
      statistics::Scalar numStuckAtZero;
      statistics::Scalar numStuckAtOne;
      statistics::Scalar numPermanentFaults;
      
      CHAOSCacheStats(statistics::Group *parent);
    };

    std::unique_ptr<CHAOSCacheStats> stats;
};

} // namespace gem5

#endif // __MEM_CACHE_CHAOSCACHE_CHAOSCACHE_HH__