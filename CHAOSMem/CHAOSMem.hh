#ifndef __MEM_CHAOSMem_HH__
#define __MEM_CHAOSMem_HH__

#include <iostream>
#include <random>
#include <bitset>
#include <fstream>
#include <string>
#include <random>
#include <cassert>
#include <cstdint>
#include <functional>

#include "sim/sim_object.hh"
#include "mem/abstract_mem.hh"
#include "sim/eventq.hh"
#include "base/types.hh"
#include "params/CHAOSMem.hh"
#include "mem/packet.hh"
#include "base/callback.hh"
#include "base/logging.hh"
#include "base/statistics.hh"

namespace gem5 {

  class CHAOSMem : public SimObject {
    private:
      memory::AbstractMemory* memory;
      std::random_device rd;
      std::mt19937 rng;
      unsigned char generateRandomMask(std::mt19937 &rng, int numBitsToChangePerByte);
      std::ofstream logFile;          // Log file to track events
      std::geometric_distribution<unsigned> inter_fault_tick_dist;
      float probability;            /* Probability of injecting a fault in a given cycle
                                    Allowed values -> float between 0 and 1
                                    Default value -> 0 */

      int bitsToChange;          /* Number of bits to modify for fault injection  
                                  If "faultMask" is not "0", this parameter has no effect on the simulation.  
                                  Allowed values -> integers between 1 and 32  
                                  Default value -> 32 */

      int firstClock, lastClock;    /* Range of clock cycles during which fault injection is enabled  
                                  Default value for firstClock -> 0  
                                  Default value for lastClock -> last simulation clock cycle */

      unsigned char faultMask;    /* Bitmask to apply to the target register's bits  
                                  Allowed values -> A string containing bits ranging from "0" to "11111111111111111111111111111111"  
                                  Default value -> "0" (if set to "0", a random bitmask of numBitsToChange bits is generated) */

      int tickToClockRatio;

      EventFunctionWrapper attackEvent;
  
    public:
      CHAOSMem(const CHAOSMemParams& params);
      ~CHAOSMem();
      void attackMemory();
      void scheduleAttack(Tick time);
  };

} // namespace gem5

#endif // __MEM_CHAOSMem_HH__