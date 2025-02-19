/**
 * @file
 * Definitions of CHAOSTags.
 */

#include "mem/cache/tags/CHAOSTags.hh"
#include <iostream>
#include <cassert>
#include <random>

namespace gem5
{

    CHAOSTags::CHAOSTags(const Params &p)
    : BaseTags(p), probability(p.probability), numBytesToChange(p.numBytesToChange), 
    numBitsToChangePerByte(p.numBitsToChangePerByte), firstClock(p.firstClock), faultType(p.faultType),
    lastClock(p.lastClock), tickToClockRatio(p.tickToClockRatio), faultMask(static_cast<unsigned char>(std::stoi(p.faultMask, nullptr, 2)))
{
    // Opens the log file to save details about the injected faults
    logFile.open("cache_injections.log", std::ios::out);
    if (!logFile.is_open()) {
        throw std::runtime_error("CHAOS: Could not open log file for writing");
    }

    // Initializes the random number generator
    auto seed = rd();
    rng.seed(seed);
    inter_fault_cycle_dist = std::geometric_distribution<unsigned>(probability);

    // Schedules the first fault
    fault_tick = curTick() + inter_fault_cycle_dist(rng)*tickToClockRatio;
}

/**
 * Destructor method: closes the log file.
 */
 CHAOSTags::~CHAOSTags()
 {
     if (logFile.is_open()) {
         logFile.close();
     }
 }

ReplaceableEntry*
CHAOSTags::findBlockBySetAndWay(int set, int way) const
{
    return BaseTags::findBlockBySetAndWay(set, way);
}

CacheBlk*
CHAOSTags::findBlock(const CacheBlk::KeyType &key) const
{
    return BaseTags::findBlock(key);
}

/**
 * Generates a random mask to modify a specified number of bits.
 * 
 * @param gen Random number generator.
 * @param numBits Number of bits to modify.
 * @return The generated mask.
 */
 unsigned char 
 CHAOSTags::generateRandomMask(std::mt19937 &rng, int numBitsToChangePerByte)
 {
    unsigned char mask = 0;
     std::uniform_int_distribution<int> bitDist(0, 7);
 
    for (int i = 0; i < numBitsToChangePerByte; i++) {
         mask |= (1 << bitDist(rng));
     }
     return mask;
 }

void 
CHAOSTags::injectFault(PacketPtr pkt) {
    if (curTick() >= firstClock && (curTick() <= lastClock || lastClock == -1)) {
        unsigned char mask = (faultMask != 0) ? faultMask : generateRandomMask(rng, numBitsToChangePerByte);
        if(mask != 0){
            uint8_t *x = pkt->getPtr<uint8_t>();

            std::uniform_int_distribution<int> dist(0, pkt->getSize() - 1);
            int byte = dist(rng);

             // Determines the type of fault to apply to the obtained value
            std::string chosenFaultType = faultType;
            if (faultType == "random") { // Random selection of the fault type
                static std::mt19937 gen(std::random_device{}());
                static std::uniform_int_distribution<int> dis(0, 2);

                // Randomly maps the number to a fault type
                const char* faultTypes[] = {"bit_flip", "stuck_at_zero", "stuck_at_one"};
                chosenFaultType = faultTypes[dis(gen)];
            }
            // Applies the fault to the register value
            if (chosenFaultType == "stuck_at_zero") {
                x[byte] &= ~mask;
            } else if (chosenFaultType == "stuck_at_one") {
                x[byte] |= mask;
            } else if (chosenFaultType == "bit_flip") {
                x[byte] ^= mask;
            }

            // Logs the operations just performed in the log file
            logFile << "Tick: " << curTick() 
            << ", Packet Byte: " << byte
            << ", Mask: " << std::bitset<8>(mask)
            << ", Fault Type: " << chosenFaultType
            << std::dec << std::endl;

            logFile.flush();

            // Schedules the next fault
            fault_tick = curTick() + inter_fault_cycle_dist(rng)*tickToClockRatio;
        }
    }
}

void
CHAOSTags::insertBlock(const PacketPtr pkt, CacheBlk *blk)
{
    if(probability > 0.0){
        if(curTick() >= fault_tick){
            assert(!blk->isValid());
            CHAOSTags::injectFault(pkt);
        }
    }

    BaseTags::insertBlock(pkt, blk);
}

void
CHAOSTags::moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk)
{
    BaseTags::moveBlock(src_blk, dest_blk);
}

Addr
CHAOSTags::extractTag(const Addr addr) const
{
    return BaseTags::extractTag(addr);
}

// void
// CHAOSTags::cleanupRefsVisitor(CacheBlk &blk)
// {
//     BaseTags::cleanupRefsVisitor(blk);
// }

void
CHAOSTags::cleanupRefs()
{
    BaseTags::cleanupRefs();
}

// void
// CHAOSTags::computeStatsVisitor(CacheBlk &blk)
// {
//     BaseTags::computeStatsVisitor(blk);
// }

void
CHAOSTags::computeStats()
{
    BaseTags::computeStats();
}

std::string
CHAOSTags::print()
{
    return BaseTags::print();
}

void
CHAOSTags::forEachBlk(std::function<void(CacheBlk &)> visitor)
{
    BaseTags::forEachBlk(visitor);
}

} // namespace gem5
