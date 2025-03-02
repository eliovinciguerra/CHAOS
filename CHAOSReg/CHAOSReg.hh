#ifndef __CHAOSReg_HH__
#define __CHAOSReg_HH__

#include <random>
#include <bitset>

#include "params/CHAOSReg.hh"
#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include "cpu/base.hh"

namespace gem5
{
  class CHAOSReg : public SimObject
  {
    private:
      BaseCPU *cpu;
      float probability;
      int numBitsToChange;
      int firstClock, lastClock;
      std::string faultType;
      std::bitset<32> faultMask;
      std::string regTargetClass;
      Addr PCTarget;
      EventFunctionWrapper tickEvent;
      std::ofstream logFile;

      std::random_device rd;
      std::mt19937 rng;
      std::geometric_distribution<unsigned> inter_fault_cycles_dist;

      int generateRandomMask(std::mt19937 &gen, int numBits);
      ~CHAOSReg();

    public:
      CHAOSReg(const CHAOSRegParams &p);
      static CHAOSReg *create(const CHAOSRegParams &p);

      void processFault(ThreadID tid);
      void tick();
      void scheduleTickEvent(Cycles delay);
      void unscheduleTickEvent();
      bool checkInst();
  };

} // namespace gem5
#endif // __CHAOSReg_HH__