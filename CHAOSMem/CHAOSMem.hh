#ifndef __MEM_CHAOSMem_HH__
#define __MEM_CHAOSMem_HH__

#include <random>
#include <bitset>
#include <string>
#include <cstdint>
#include <functional>

#include "sim/sim_object.hh"
#include "mem/abstract_mem.hh"
#include "sim/eventq.hh"
#include "base/types.hh"
#include "params/CHAOSMem.hh"
#include "mem/packet.hh"
#include <stdexcept>
#include "base/output.hh"

namespace gem5 {

  class CHAOSMem : public SimObject {
    private:
      float probability;  /* Probability of injecting a fault in a given cycle
                          Allowed values -> float between 0 and 1
                          Default value -> 0 */
      int bitsToChange;   /* Number of bits to modify for fault injection  
                          If "faultMask" is not "0", this parameter has no effect on the simulation.  
                          Allowed values -> integers between 1 and 32  
                          Default value -> 32 */
      uint64_t firstClock, lastClock;/* Range of clock cycles during which fault injection is enabled  
                                Default value for firstClock -> 0  
                                Default value for lastClock -> last simulation clock cycle */
      int tickToClockRatio;
      unsigned char faultMask;/* Bitmask to apply to the target register's bits  
                              Allowed values -> A string containing bits ranging from "0" to "11111111111111111111111111111111"  
                              Default value -> "0" (if set to "0", a random bitmask of numBitsToChange bits is generated) */

      std::string faultType;/* Type of modification to apply to the target register's bits  
                            Allowed values -> "bit_flip",  
                                              "stuck_at_zero",  
                                              "stuck_at_one",  
                                              "random" (randomly selects one of the above methods)  
                            Default value -> "random" */
                            
      memory::AbstractMemory* memory;
      Addr target_start, target_end, target_size;
      std::random_device rd;
      std::mt19937 rng;
      unsigned char generateRandomMask(std::mt19937 &rng, int numBitsToChangePerByte);
      OutputStream *logStream;         // Log file to track events
      OutputStream *logTest;
      std::geometric_distribution<unsigned> inter_fault_tick_dist;

      EventFunctionWrapper attackEvent;
  
    public:
      CHAOSMem(const CHAOSMemParams& p);
      ~CHAOSMem();
      void attackMemory();
      void scheduleAttack(Tick time);
  };

} // namespace gem5

#endif // __MEM_CHAOSMem_HH__