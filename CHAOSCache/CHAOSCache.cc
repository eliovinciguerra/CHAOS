#include "mem/cache/CHAOSCache/CHAOSCache.hh"

#include <random>
#include <vector>

#include "debug/CHAOSCache.hh"
#include "mem/cache/base.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/tags/base.hh"

namespace gem5
{
    CHAOSCache::CHAOSCache(const CHAOSCacheParams& p) :
        SimObject(p),
        targetCache(p.target_cache),
        probability(p.probability),
        bits_to_change(p.bitsToChange),
        corruption_size(p.corruptionSize),
        first_clock(p.firstClock),
        last_clock(p.lastClock),
        fault_type_enum(stringToFaultType(p.faultType)),
        fault_mask(static_cast<unsigned char>(std::stoi(p.faultMask, nullptr, 2))),
        tick_to_clock_ratio(p.tickToClockRatio),
        bit_flip_prob(p.bitFlipProb),
        stuck_at_zero_prob(p.stuckAtZeroProb),
        stuck_at_one_prob(p.stuckAtOneProb),
        cycles_permament_fault_check(p.cyclesPermamentFaultCheck),
        write_log(p.writeLog),
        attackEvent([this] { this->injectFault(); }, name()),
        periodicCheck([this] { this->checkPermanent(); }, name() + ".periodicCheck"),
        stats(nullptr)
    {
        if (probability != 0.0) {
            log_stream = simout.create("cache_injections.log", false, true);
            if (!log_stream || !log_stream->stream()) {
                panic("CHAOSCache: Could not open log file");
            }

            if (bits_to_change == -1){
                std::uniform_int_distribution<int> dist(1, 8);
                bits_to_change = dist(rng);
            }

            stats = std::make_unique<CHAOSCacheStats>(this);

            first_tick = first_clock * tick_to_clock_ratio;
            last_tick = last_clock * tick_to_clock_ratio;
            ticks_permament_fault_check = cycles_permament_fault_check * tick_to_clock_ratio;

            rng.seed(rd());
            inter_fault_cycles_dist = std::geometric_distribution<unsigned>(probability);

            scheduleAttack(first_tick + inter_fault_cycles_dist(rng) * tick_to_clock_ratio);

            if ((bit_flip_prob + stuck_at_zero_prob + stuck_at_one_prob) != 1.0){
                warn("Sum of probabilities is not 1, assuming 0.9 for bitFlipProb, 0.05 for stuckAtZeroProb and 0.05 for stuckAtOneProb.\n");
                bit_flip_prob = 0.9;
                stuck_at_zero_prob = 0.05;
                stuck_at_one_prob = 0.05;
            }

            std::vector<double> weights = {bit_flip_prob, stuck_at_zero_prob, stuck_at_one_prob};
            random_fault_distribution = std::discrete_distribution<int>(weights.begin(), weights.end());

            scheduleCheckPermanentFault(first_tick + ticks_permament_fault_check);
        }
    }

    CHAOSCache::CHAOSCacheStats::CHAOSCacheStats(statistics::Group *parent)
    : statistics::Group(parent),
      ADD_STAT(numFaultsInjected, statistics::units::Count::get(),
               "Total number of faults injected"),
      ADD_STAT(numBitFlips, statistics::units::Count::get(),
               "Number of bit flip faults injected"),
      ADD_STAT(numStuckAtZero, statistics::units::Count::get(),
               "Number of stuck-at-0 faults injected"),
      ADD_STAT(numStuckAtOne, statistics::units::Count::get(),
               "Number of stuck-at-1 faults injected"),
      ADD_STAT(numPermanentFaults, statistics::units::Count::get(),
               "Total number of permanent faults injected")
    {
    }

    CHAOSCache::FaultType 
    CHAOSCache::stringToFaultType(const std::string &s) {
        if (s == "bit_flip") return FaultType::BitFlip;
        else if (s == "stuck_at_zero") return FaultType::StuckAtZero;
        else if (s == "stuck_at_one") return FaultType::StuckAtOne;
        return FaultType::Random;
    }

    const char* 
    CHAOSCache::faultTypeToString(CHAOSCache::FaultType f) {
        switch (f) {
            case FaultType::BitFlip: return "bit_flip";
            case FaultType::StuckAtZero: return "stuck_at_zero";
            case FaultType::StuckAtOne: return "stuck_at_one";
        }
        return "random";
    }

    void 
    CHAOSCache::scheduleAttack(Tick time) {
        if (!attackEvent.scheduled()) {
            schedule(attackEvent, time);
        }
    }

    void 
    CHAOSCache::scheduleCheckPermanentFault(Tick time) {
        if (!periodicCheck.scheduled()) {
            schedule(periodicCheck, time);
        }
    }

    BaseTags*
    CHAOSCache::getTags() const
    {
        struct CacheAccessor : public Cache {
            BaseTags* getTagsPublic() { return tags; }
        };
        
        return static_cast<CacheAccessor*>(targetCache)->getTagsPublic();
    }

    uint8_t 
    CHAOSCache::generateRandomMask(std::mt19937 &rng, int bits_to_change, unsigned size) {
        uint8_t mask = 0;
        std::uniform_int_distribution<int> bit_dist(0, size - 1);
        for (int i = 0; i < bits_to_change; i++) {
            mask |= (1ULL << bit_dist(rng));
        }
        return mask;
    }

    void
    CHAOSCache::injectFault()
    {   
        BaseTags* tags = getTags();
        unsigned blockSize = targetCache->getBlockSize();
        
        std::vector<CacheBlk*> validBlocks;
        
        tags->forEachBlk([&validBlocks](CacheBlk &blk) {
            if (blk.isValid()) {
                validBlocks.push_back(&blk);
            }
        });
        
        if (validBlocks.empty()) {
            warn("No valid block found\n");
        } else{
        
            std::uniform_int_distribution<int> blockDist(0, validBlocks.size() - 1);
            int randomIdx = blockDist(rng);
            CacheBlk* targetBlk = validBlocks[randomIdx];

            Addr blockAddr = tags->regenerateBlkAddr(targetBlk);

            uint8_t* data = targetBlk->data;

            std::uniform_int_distribution<int> byteDist(0, blockSize - 1);

            FaultType chosen_fault_type_enum = fault_type_enum;
            if (fault_type_enum == FaultType::Random) {
                int faultIdx = random_fault_distribution(rng);
                chosen_fault_type_enum = static_cast<FaultType>(faultIdx);
            }
            
            for (int i = 0; i < corruption_size; i++) {
                unsigned char mask = (fault_mask != 0) ? fault_mask : generateRandomMask(rng, bits_to_change, 8);
                int byteOffset = byteDist(rng);

                if (mask == 0) {
                    warn("Mask is 0.");
                    continue;
                }

                // uint8_t oldValue = data[byteOffset];

                switch (chosen_fault_type_enum) {
                    case FaultType::StuckAtZero:
                        data[byteOffset] &= ~mask;
                        stats->numStuckAtZero++;
                        stats->numPermanentFaults++;
                        permanent_faults[std::make_pair(blockAddr, byteOffset)] = {chosen_fault_type_enum, mask, true};
                        break;
                    case FaultType::StuckAtOne:
                        data[byteOffset] |= mask;
                        stats->numStuckAtOne++;
                        stats->numPermanentFaults++;
                        permanent_faults[std::make_pair(blockAddr, byteOffset)] = {chosen_fault_type_enum, mask, true};
                        break;
                    case FaultType::BitFlip:
                        data[byteOffset] ^= mask;
                        stats->numBitFlips++;
                        break;
                    default:
                        break;
                }

                // uint8_t newValue = data[byteOffset];
                stats->numFaultsInjected++;

                if (write_log){
                    *(log_stream->stream())  << "Tick: " << curTick()
                        << ", Cache Block Addr: " << blockAddr
                        << ", Byte Offset: " << byteOffset
                        << ", FaultType: " << faultTypeToString(chosen_fault_type_enum)
                        << ", Mask: " << std::bitset<8>(mask)
                        << std::endl;
                }
            }

            targetBlk->setCoherenceBits(CacheBlk::DirtyBit);
        }

        Tick next_injection = curTick() + inter_fault_cycles_dist(rng) * tick_to_clock_ratio;
        if (next_injection <= last_tick || last_tick == 0) {
            scheduleAttack(next_injection);
        }
    }

    void
    CHAOSCache::checkPermanent()
    {
        BaseTags* tags = getTags();

        for (auto& entry : permanent_faults) {
            if(entry.second.update){
                const std::pair<Addr, int>& key = entry.first;
                const PermanentFault& fault = entry.second;

                Addr blockAddr = key.first;
                int byteOffset = key.second;
                FaultType faultType = fault.fault_type;
                uint64_t mask = fault.mask;

                CacheBlk* blk = nullptr;
                tags->forEachBlk([&](CacheBlk &b) {
                    Addr blkAddr = tags->regenerateBlkAddr(&b);
                    if (blkAddr == blockAddr && b.isValid()) {
                        blk = &b;
                    }
                });
                if (!blk) {
                    continue;
                }

                uint8_t* data = blk->data;
                // uint8_t oldValue = data[byteOffset];

                switch (faultType) {
                    case FaultType::StuckAtZero:
                        data[byteOffset] &= ~((uint8_t)mask);
                        break;
                    case FaultType::StuckAtOne:
                        data[byteOffset] |= ((uint8_t)mask);
                        break;
                    default:
                        break;
                }
            }
        }
    }
} // namespace gem5