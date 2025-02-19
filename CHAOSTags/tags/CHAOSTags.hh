/**
 * @file
 * Declaration of CHAOSTag class for cache tagstore objects.
 */

#ifndef __MEM_CACHE_TAGS_CHAOS_HH__
#define __MEM_CACHE_TAGS_CHAOS_HH__

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <random>

#include "base/callback.hh"
#include "base/logging.hh"
#include "base/statistics.hh"
#include "base/types.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/packet.hh"
#include "params/CHAOSTags.hh"
#include "sim/clocked_object.hh"
#include "mem/cache/tags/base.hh"

namespace gem5
{
/**
 * A CHAOSTag class of Cache tagstore objects.
 */
class CHAOSTags : public BaseTags
{
//   protected:

  public:
    PARAMS(CHAOSTags);
    CHAOSTags(const Params &p);

    /**
     * Destructor.
     */
    virtual ~CHAOSTags();

    /**
     * Initialize blocks. Must be overriden by every subclass that uses
     * a block type different from its parent's, as the current Python
     * code generation does not allow templates.
     */
    virtual void tagsInit() = 0;

    /**
     * Average in the reference count for valid blocks when the simulation
     * exits.
     */
    void cleanupRefs();

    /**
     * Computes stats just prior to dump event
     */
    void computeStats();

    /**
     * Print all tags used
     */
    std::string print();

    /**
     * Finds the block in the cache without touching it.
     *
     * @param addr The address to look for.
     * @param is_secure True if the target memory space is secure.
     * @return Pointer to the cache block.
     */
    virtual CacheBlk *findBlock(const CacheBlk::KeyType &key) const;

    /**
     * Find a block given set and way.
     *
     * @param set The set of the block.
     * @param way The way of the block.
     * @return The block.
     */
    virtual ReplaceableEntry* findBlockBySetAndWay(int set, int way) const;

    /**
     * Align an address to the block size.
     * @param addr the address to align.
     * @return The block address.
     */
    Addr blkAlign(Addr addr) const
    {
        return addr & ~blkMask;
    }

    /**
     * Calculate the block offset of an address.
     * @param addr the address to get the offset of.
     * @return the block offset.
     */
    int extractBlkOffset(Addr addr) const
    {
        return (addr & blkMask);
    }

    /**
     * Limit the allocation for the cache ways.
     * @param ways The maximum number of ways available for replacement.
     */
    virtual void setWayAllocationMax(int ways)
    {
        panic("This tag class does not implement way allocation limit!\n");
    }

    /**
     * Get the way allocation mask limit.
     * @return The maximum number of ways available for replacement.
     */
    virtual int getWayAllocationMax() const
    {
        panic("This tag class does not implement way allocation limit!\n");
        return -1;
    }

    /**
     * This function updates the tags when a block is invalidated
     *
     * @param blk A valid block to invalidate.
     */
    virtual void invalidate(CacheBlk *blk)
    {
        assert(blk);
        assert(blk->isValid());

        stats.occupancies[blk->getSrcRequestorId()]--;
        stats.totalRefs += blk->getRefCount();
        stats.sampledRefs++;

        blk->invalidate();
    }

    /**
     * Find replacement victim based on address. If the address requires
     * blocks to be evicted, their locations are listed for eviction. If a
     * conventional cache is being used, the list only contains the victim.
     * However, if using sector or compressed caches, the victim is one of
     * the blocks to be evicted, but its location is the only one that will
     * be assigned to the newly allocated block associated to this address.
     * @sa insertBlock
     *
     * @param addr Address to find a victim for.
     * @param is_secure True if the target memory space is secure.
     * @param size Size, in bits, of new block to allocate.
     * @param evict_blks Cache blocks to be evicted.
     * @param partition_id Partition ID for resource management.
     * @return Cache block to be replaced.
     */
    virtual CacheBlk* findVictim(const CacheBlk::KeyType &key,
                                 const std::size_t size,
                                 std::vector<CacheBlk*>& evict_blks,
                                 const uint64_t partition_id=0) = 0;

    /**
     * Access block and update replacement data. May not succeed, in which case
     * nullptr is returned. This has all the implications of a cache access and
     * should only be used as such. Returns the tag lookup latency as a side
     * effect.
     *
     * @param pkt The packet holding the address to find.
     * @param lat The latency of the tag lookup.
     * @return Pointer to the cache block if found.
     */
    virtual CacheBlk* accessBlock(const PacketPtr pkt, Cycles &lat) = 0;

    /**
     * Generate the tag from the given address.
     *
     * @param addr The address to get the tag from.
     * @return The tag of the address.
     */
    virtual Addr extractTag(const Addr addr) const;

    /**
     * Insert the new block into the cache and update stats.
     *
     * @param pkt Packet holding the address to update
     * @param blk The block to update.
     */
    virtual void insertBlock(const PacketPtr pkt, CacheBlk *blk);

    /**
     * Move a block's metadata to another location decided by the replacement
     * policy. It behaves as a swap, however, since the destination block
     * should be invalid, the result is a move.
     *
     * @param src_blk The source block.
     * @param dest_blk The destination block. Must be invalid.
     */
    virtual void moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk);

    /**
     * Regenerate the block address.
     *
     * @param block The block.
     * @return the block address.
     */
    virtual Addr regenerateBlkAddr(const CacheBlk* blk) const = 0;

    /**
     * Visit each block in the tags and apply a visitor
     *
     * The visitor should be a std::function that takes a cache block
     * reference as its parameter.
     *
     * @param visitor Visitor to call on each block.
     */
    void forEachBlk(std::function<void(CacheBlk &)> visitor);

    /**
     * Find if any of the blocks satisfies a condition
     *
     * The visitor should be a std::function that takes a cache block
     * reference as its parameter. The visitor will terminate the
     * traversal early if the condition is satisfied.
     *
     * @param visitor Visitor to call on each block.
     */
    virtual bool anyBlk(std::function<bool(CacheBlk &)> visitor) = 0;

  private:
    std::random_device rd;
    std::mt19937 rng;
    void injectFault(PacketPtr);
    unsigned char generateRandomMask(std::mt19937 &rng, int numBitsToChangePerByte);
    std::ofstream logFile;          // Log file to track events
    std::geometric_distribution<unsigned> inter_fault_cycle_dist;
    unsigned fault_tick;
    int tickToClockRatio;

    float probability;            /* Probability of injecting a fault in a given cycle
                                        Allowed values -> float between 0 and 1
                                        Default value -> 0 */

    int numBytesToChange, numBitsToChangePerByte;          /* Number of bits to modify for fault injection  
                                                            If "faultMask" is not "0", this parameter has no effect on the simulation.  
                                                            Allowed values -> integers between 1 and 32  
                                                            Default value -> 32 */

    int firstClock, lastClock;    /* Range of ticks during which fault injection is enabled  
                                        Default value for firstClock -> 0  
                                        Default value for lastClock -> last simulation tick */

    unsigned char faultMask;    /* Bitmask to apply to the target register's bits  
                                    Allowed values -> A string containing bits ranging from "0" to "11111111111111111111111111111111"  
                                    Default value -> "0" (if set to "0", a random bitmask of numBitsToChange bits is generated) */

    std::string faultType;        /* Type of modification to apply to the target register's bits  
                                    Allowed values -> "bit_flip",  
                                                        "stuck_at_zero",  
                                                        "stuck_at_one",  
                                                        "random" (randomly selects one of the above methods)  
                                    Default value -> "random" */
//     /**
//      * Update the reference stats using data from the input block
//      *
//      * @param blk The input block
//      */
//     void cleanupRefsVisitor(CacheBlk &blk);

//     /**
//      * Update the occupancy and age stats using data from the input block
//      *
//      * @param blk The input block
//      */
//     void computeStatsVisitor(CacheBlk &blk);
};

} // namespace gem5

#endif //__MEM_CACHE_TAGS_CHAOS_HH__
