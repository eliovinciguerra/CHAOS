#include "mem/cache/CHAOSCache/CHAOSCache.hh"
#include <random>

namespace gem5 {
    static std::random_device rd;
    static std::mt19937 rng(rd());

    CHAOSCache::CHAOSCache(const CHAOSCacheParams &p)
        : Cache(p),
        probability(p.probability),
        injectOnRead(p.injectOnRead),
        injectOnWrite(p.injectOnWrite),
        numBitsToChange(p.numBitsToChange),
        firstClock(p.firstClock),
        lastClock(p.lastClock),
        faultType(p.faultType),
        tickToClockRatio(p.tickToClockRatio)
    {
        if (probability != 0.0){
            logStream = simout.create("cache_injections.log", false, true);
            if (!logStream || !logStream->stream()) {
                panic("CHAOSCache: Could not open log file");
            }

            auto seed = rd();
            rng.seed(seed);
            probDist = std::uniform_real_distribution<double> (0.0, probability);
        }else{
            injectOnRead = false;
            injectOnWrite = false;
        }
    }

    void 
    CHAOSCache::recvTimingReq(PacketPtr pkt) 
    {
        if (pkt->isWrite() && injectOnWrite) {
            Tick cycle = curTick() / tickToClockRatio;
            if (probDist(rng) < probability && (cycle >= firstClock && (cycle <= lastClock || lastClock == 0))) {
                injectFault(pkt);
            }
        }
    
        Cache::recvTimingReq(pkt);

        if (pkt->isResponse() && injectOnRead) {
            Tick cycle = curTick() / tickToClockRatio;
            if (probDist(rng) < probability && (cycle >= firstClock && (cycle <= lastClock || lastClock == 0))) {
                injectFault(pkt);
            }
        }
    }

    Tick 
    CHAOSCache::recvAtomic(PacketPtr pkt) 
    {
        if (pkt->isWrite() && injectOnWrite) {
            Tick cycle = curTick() / tickToClockRatio;
            if (probDist(rng) < probability && (cycle >= firstClock && (cycle <= lastClock || lastClock == 0))) {
                injectFault(pkt);
            }
        }

        Tick latency = Cache::recvAtomic(pkt);

        if (pkt->isResponse() && injectOnRead) {
            Tick cycle = curTick() / tickToClockRatio;
            if (probDist(rng) < probability && (cycle >= firstClock && (cycle <= lastClock || lastClock == 0))) {
                injectFault(pkt);
            }
        }
        return latency;
    }

    uint64_t 
    CHAOSCache::generateRandomMask(std::mt19937 &rng, int bitsToChange, unsigned size)
    {
        uint64_t mask = 0;
        std::uniform_int_distribution<int> bitDist(0, size - 1);

        for (int i = 0; i < bitsToChange; i++) {
            mask |= (1ULL << bitDist(rng));
        }
        return mask;
    }

    void 
    CHAOSCache::injectFault(PacketPtr pkt) 
    {   
        uint8_t *data = pkt->getPtr<uint8_t>();
        uint64_t mask;
        unsigned size = pkt->getSize();
        if (numBitsToChange == -1){
            mask = generateRandomMask(rng, size, size);    
        }else{
            mask = generateRandomMask(rng, numBitsToChange, size);
        }

        std::string chosenFaultType = faultType;
        if (faultType == "random") {
            static std::mt19937 gen(std::random_device{}());
            static std::uniform_int_distribution<int> dis(0, 2);

            const char* faultTypes[] = {"bit_flip", "stuck_at_zero", "stuck_at_one"};
            chosenFaultType = faultTypes[dis(gen)];
        }
        if (chosenFaultType == "stuck_at_zero") {
            *data &= ~mask;
        } else if (chosenFaultType == "stuck_at_one") {
            *data |= mask;
        } else if (chosenFaultType == "bit_flip") {
            *data ^= mask;
        }

        *(logStream->stream()) << "Tick: " << curTick() 
            << ", Mask (decimal representation): " << mask
            << ", Fault Type: " << chosenFaultType
            << ", Pkt Size: " << size
            << std::dec << std::endl;
    }

    CHAOSCache::~CHAOSCache() 
    {}
} // namespace gem5