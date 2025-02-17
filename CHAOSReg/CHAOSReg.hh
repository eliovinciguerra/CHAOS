#ifndef __CHAOSReg_HH__
#define __CHAOSReg_HH__

#include <iostream>
#include <random>
#include <bitset>
#include <fstream>

#include "params/CHAOSReg.hh"

#include "sim/sim_object.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"
#include "cpu/o3/cpu.hh"

namespace gem5
{
    /**
    * The CHAOSReg class is responsible for injecting faults into the registers 
    * of the simulated CPU. Faults are introduced with a configurable probability, 
    * enabling the evaluation of CPU resilience under hardware error conditions. 
    * Fault injection is performed at the thread level, with each injection occurring 
    * on a per-clock-cycle basis.
    */

    class CHAOSReg : public SimObject
    {
     private:
        o3::CPU *cpu;               // Pointer to the O3 (Out-of-Order) CPU

        float probability;            /* Probability of injecting a fault in a given cycle
                                        Allowed values -> float between 0 and 1
                                        Default value -> 0 */

        int numBitsToChange;          /* Number of bits to modify for fault injection  
                                        If "faultMask" is not "0", this parameter has no effect on the simulation.  
                                        Allowed values -> integers between 1 and 32  
                                        Default value -> 32 */

        int firstClock, lastClock;    /* Range of clock cycles during which fault injection is enabled  
                                        Default value for firstClock -> 0  
                                        Default value for lastClock -> last simulation cycle */

        std::string faultType;        /* Type of modification to apply to the target register's bits  
                                        Allowed values -> "bit_flip",  
                                                            "stuck_at_zero",  
                                                            "stuck_at_one",  
                                                            "random" (randomly selects one of the above methods)  
                                        Default value -> "random" */

        std::bitset<32> faultMask;    /* Bitmask to apply to the target register's bits  
                                        Allowed values -> A string containing bits ranging from "0" to "11111111111111111111111111111111"  
                                        Default value -> "0" (if set to "0", a random bitmask of numBitsToChange bits is generated) */

        std::string instTarget;       /* Type of instruction that triggers fault injection  
                                        Allowed values ->  
                                            "all" (enables injection for any instruction)  
                                            "nop"  
                                            "load"  
                                            "store"  
                                            "atomic"  
                                            "store_conditional"  
                                            "inst_prefetch"  
                                            "data_prefetch"  
                                            "integer"  
                                            "floating"  
                                            "vector"  
                                            "control"  
                                            "call"  
                                            "return"  
                                            "direct_ctrl"  
                                            "indirect_ctrl"  
                                            "serializing"  
                                            "serialize_before"  
                                            "serialize_after"  
                                            "squash_after"  
                                            "full_mem_barrier"  
                                            "read_barrier"  
                                            "write_barrier"  
                                            "non_speculative"  
                                            "unverifiable"  
                                            "syscall"  
                                            "macroop"  
                                            "microop"  
                                            "delayed_commit"  
                                            "last_microop"  
                                            "first_microop"  
                                            "htm_start"  
                                            "htm_stop"  
                                            "htm_cancel"  
                                            "htm_cmd"  
                                        Default value -> "all" */

        std::string regTargetClass;   /* Register class from which the target for fault injection is selected  
                                        Allowed values ->  
                                            "both" (randomly selects a class)  
                                            "integer"  
                                            "floating_point"  
                                        Default value -> "both" */

        Addr PCTarget;                /* Program Counter (PC) address at which to enable fault injection  
                                        Default value -> 0 */ 

        EventFunctionWrapper tickEvent; // Event to execute the tick() function periodically
        std::ofstream logFile;          // Log file to track events

        
        /**
        * Function that generates a random bitmask to be used for fault injection.
        * The mask is used to change "numBits" random bits in the register value.
        * 
        * @param rng Random number generator.
        * @param numBits Number of bits to modify (bits in the mask set to "1").
        * @return The generated bitmask.
        */
        int generateRandomMask(std::mt19937 &gen, int numBits);

        /*
        * Destructor that closes the log file.
        */
        ~CHAOSReg();

        /*
         * About mt19937 random generator
         */
        std::random_device rd;
        std::mt19937 rng;
        std::geometric_distribution<unsigned> inter_fault_cycles_dist;

    public:
        /*
        * Function that injects a fault into the registers of a specified thread.
        * It randomly selects a register and injects the fault by modifying its bits.
        * 
        * @param tid Thread identifier to inject the fault into.
        */
        CHAOSReg(const CHAOSRegParams &p);

        // Static function to create a CHAOSReg object
        static CHAOSReg *create(const CHAOSRegParams &params);

        /**
        * Function that injects a fault into the registers of a specified thread.
        * It randomly selects a register and injects the fault by modifying its bits.
        * 
        * @param tid Thread identifier to which the fault is injected.
        */
        void processFault(ThreadID tid);

        /**
        * Tick of the SimObject: introduces a fault and schedules the next fault.
        */
        void tick();

        // Function to schedule fault injection at specific cycle intervals
        void scheduleTickEvent(Cycles delay);

        // Function to cancel the scheduling of the tick event
        void unscheduleTickEvent();

        /**
        * Checks if the current instruction or the PC matches the target for fault injection.
        * 
        * @return true if the current instruction is the target or if the PC is the target, otherwise false.
        */
        bool checkInst(); 
    };

} // namespace gem5

#endif // __CHAOSReg_HH__
