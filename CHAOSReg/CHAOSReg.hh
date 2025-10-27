#ifndef __CHAOSReg_HH__
#define __CHAOSReg_HH__

#include <random>
#include <bitset>

#include "params/CHAOSReg.hh"
#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include "cpu/base.hh"

#include <stdexcept>
#include "base/output.hh"

namespace gem5
{
  class CHAOSReg : public SimObject
  {
    public:
      CHAOSReg(const CHAOSRegParams &p);
      ~CHAOSReg();

    private:
      enum class FaultType {
          BitFlip,
          StuckAtZero,
          StuckAtOne,
          Random
      };

      enum class TargetClass {
          Both,
          Integer,
          FloatingPoint
      };
      
      struct PermanentFault {
        FaultType fault_type;
        int mask;
        bool update;
      };

      BaseCPU *cpu;
      float probability;
      int num_bits_to_change;
      Cycles first_clock, last_clock;
      FaultType fault_type_enum;
      std::bitset<32> fault_mask;
      float bit_flip_prob, stuck_at_zero_prob, stuck_at_one_prob;
      Cycles cycles_permament_fault_check;
      TargetClass reg_target_class_enum;
      Addr PC_target;
      bool write_log;

      EventFunctionWrapper attackEvent, periodicCheck;

      int generateRandomMask(std::mt19937 &gen, int bits_to_change, int len);
      void processFault(ThreadID tid);
      void scheduleAttackEvent(Cycles delay);
      void unscheduleAttackEvent();
      void scheduleCheckPermanentFault(Cycles delay);
      void checkPermanent();
      void attackCheck();
      const char* faultTypeToString(CHAOSReg::FaultType f);
      static FaultType stringToFaultType(const std::string &s);
      static TargetClass stringToTargetClass(const std::string &s);

      std::geometric_distribution<unsigned> inter_fault_cycles_dist;
      std::discrete_distribution<int> random_fault_distribution;

      std::mt19937 rng;
      std::random_device rd;
      std::map<std::pair<ThreadID, gem5::RegId>, PermanentFault> permanent_faults;
      OutputStream *log_stream;

      struct CHAOSRegStats : public statistics::Group
      {
        statistics::Scalar numFaultsInjected;
        statistics::Scalar numBitFlips;
        statistics::Scalar numStuckAtZero;
        statistics::Scalar numStuckAtOne;
        statistics::Scalar numPermanentFaults;
        
        CHAOSRegStats(statistics::Group *parent);
      };
      
      std::unique_ptr<CHAOSRegStats> stats;
  };

} // namespace gem5
#endif // __CHAOSReg_HH__