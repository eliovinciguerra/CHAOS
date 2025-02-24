#include "CHAOSReg/CHAOSReg.hh"
#include "params/CHAOSReg.hh"

#include <cassert>

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <bitset>
#include <unordered_map>
#include <functional>
#include <string>     

#include "sim/system.hh"
#include "sim/cur_tick.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst.hh" 
#include "cpu/thread_context.hh"
#include "arch/generic/isa.hh"


namespace gem5
{
/**
 * Constructor of the CHAOSReg class.
 * Initializes the parameters passed from the configuration file and sets up the environment for fault injection.
 * The CPU validity is checked, and a log file is opened to track the injected faults.
 */
CHAOSReg::CHAOSReg(const CHAOSRegParams &params)
    : SimObject(params),
      probability(params.probability),
      numBitsToChange(params.numBitsToChange),
      firstClock(params.firstClock),
      lastClock(params.lastClock),
      faultType(params.faultType),
      faultMask(std::bitset<32>(params.faultMask)),
      instTarget(params.instTarget),
      regTargetClass(params.regTargetClass),
      PCTarget(params.PCTarget),
      cpu(dynamic_cast<o3::CPU *>(params.cpu)),
      tickEvent([this] { this->tick(); }, name())
{
    // Verifies that the pointer to the CPU is valid
    if (!cpu) {
        throw std::runtime_error("CHAOSReg: Invalid CPU pointer");
    }

    // Opens the log file to save details about the injected faults
    logFile.open("fault_injections.log", std::ios::out);
    if (!logFile.is_open()) {
        throw std::runtime_error("CHAOSReg: Could not open log file for writing");
    }

    // Initializes the random number generator
    auto seed = rd(); // TODO - si potrebbe mettere seed tra i parametri per riproducibilità
    rng.seed(seed);
    inter_fault_cycles_dist = std::geometric_distribution<unsigned>(probability);

    // Schedules the first fault
    unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
    scheduleTickEvent(Cycles(next_fault_cycle_distance)); 
}

/**
 * Static method to create a CHAOSReg with the specified parameters.
 */
CHAOSReg *CHAOSReg::create(const CHAOSRegParams &params) 
{
    return new CHAOSReg(params);
}

gem5::CHAOSReg *
gem5::CHAOSRegParams::create() const
{
    return new gem5::CHAOSReg(*this);
}

/**
 * Destructor method: closes the log file.
 */
CHAOSReg::~CHAOSReg()
{
    if (logFile.is_open()) {
        logFile.close();
    }
}

/**
 * Schedules the tick event starting from a certain delay in clock cycles.
 */
void CHAOSReg::scheduleTickEvent(Cycles delay)
{
    if (!tickEvent.scheduled())
        schedule(tickEvent, cpu->clockEdge(delay));
}

/**
 * Cancels the tick event if it has been scheduled.
 */
void CHAOSReg::unscheduleTickEvent()
{
    if (tickEvent.scheduled())
        tickEvent.squash();
}

/**
 * Generates a random mask to modify a specified number of bits.
 * 
 * @param gen Random number generator.
 * @param numBits Number of bits to modify.
 * @return The generated mask.
 */
int CHAOSReg::generateRandomMask(std::mt19937 &gen, int numBits)
{
    int mask = 0;
    std::uniform_int_distribution<int> bitDist(0, 31);

    while (numBits--) {
        mask |= (1 << bitDist(gen));
    }
    return mask;
}

/**
 * Applies a fault to a random register of the thread.
 * The register class, fault type, and bit mask applied to the register value are passed via configuration.
 * If not specified, they will be chosen randomly.
 * 
 * @param tid Target thread ID.
 */
void CHAOSReg::processFault(ThreadID tid)
{
    // Retrieves the thread context with the specified tid code
    auto *threadContext = cpu->tcBase(tid);
    if (!threadContext)
        return;

    // Retrieves the pointer to the ISA, allowing access to the registers afterward
    BaseISA *isa = threadContext->getIsaPtr();
    if (!isa)
        return;

    // Retrieves the available register classes for that ISA
    const auto &regClasses = isa->regClasses();
    const RegClass *regClass = nullptr;

    // Retrieves the register class specified by the configuration parameter
    if (regTargetClass == "both") { // One of the two available register classes is selected randomly
        regClass = (rand() % 2 == 0) ? regClasses[IntRegClass] : regClasses[FloatRegClass];
    } else if (regTargetClass == "integer") {
        regClass = regClasses[IntRegClass];
    } else if (regTargetClass == "floating_point") {
        regClass = regClasses[FloatRegClass];
    }

    if (!regClass || regClass->numRegs() == 0)
        return;

    // A random register is selected from the previously obtained register class
    std::mt19937 gen(std::random_device{}());
    int randomReg = std::uniform_int_distribution<>(0, regClass->numRegs() - 1)(gen);

    // If not provided in the configuration parameters, a random bit mask is generated
    int mask = faultMask.any() ? faultMask.to_ulong() : generateRandomMask(gen, numBitsToChange);

    RegId regId(*regClass, randomReg);

    try {
        // Attempts to access the register and reads its value
        RegVal regVal = threadContext->getReg(regId);
 
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
            regVal &= ~mask;
        } else if (chosenFaultType == "stuck_at_one") {
            regVal |= mask;
        } else if (chosenFaultType == "bit_flip") {
            regVal ^= mask;
        }

        // Updates the register with the new modified value obtained
        threadContext->setReg(regId, regVal);

        // Logs the operations just performed in the log file
        logFile << "Cycle: " << cpu->curCycle()
        << ", Register " << regClass->name() 
        << ": " << randomReg
        << ", Mask: " << std::bitset<32>(mask)
        << ", FaultType: " << chosenFaultType
        << std::dec << std::endl;

        logFile.flush();
    } catch (const std::exception &e) {
        // Catches the error and logs it
        logFile << "Error: Exception caught during fault injection. "
                << "ThreadID: " << tid
                << ", Register: " << randomReg
                << ", Error: " << e.what() << std::endl;

        logFile.flush();
    } catch (...) {
        // Catches general error
        logFile << "Error: Unknown exception during fault injection. "
                << "ThreadID: " << tid
                << ", Register: " << randomReg << std::endl;

        logFile.flush();
    }
}

/**
 * At each tick, checks if the conditions specified for enabling fault injection are met.
 */
void CHAOSReg::tick()
{
    // If the injection probability is zero, stops the fault injector
    if (!probability) {
        return;
    }

    // Checks the status of the CPU threads
    bool allThreadsHalted = true;
    for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
        auto *threadContext = cpu->tcBase(tid);
        if (!threadContext) {
            std::cerr << "ThreadContext not found for tid " << tid << std::endl;
            continue;
        }

        // Check thread status
        if (threadContext->status() != ThreadContext::Halted) {
            allThreadsHalted = false;
        }
    }

    // Check CPU status
    bool cpuDrained = (cpu->drainState() == DrainState::Drained);

    // If the CPU and thread have finished, or if the clock cycle limit has been exceeded, stops the fault injector
    if (allThreadsHalted || cpuDrained || (lastClock != -1 && cpu->curCycle() > lastClock)) {
        unscheduleTickEvent();
        return;
    }

    // Schedules the next fault injection function call
    unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
    scheduleTickEvent(Cycles(next_fault_cycle_distance));

    // Verifies that there are valid decoded instructions and that the minimum clock value has been exceeded to activate the fault injector
    if (cpu->curCycle() < firstClock || cpu->instList.empty()) {
        return;
    }

    // Checks the conditions for fault injection
    if (checkInst()) {  // If the current instruction or PC matches the specified ones
        // Enables fault injection for this cycle for all threads
        for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
            processFault(tid);
        }
    }
}


/**
 * Method to check the current instruction and the current PC
 * Returns true if the current instruction or the current PC matches the target, false otherwise
 */
bool CHAOSReg::checkInst()
{
    // Retrieves the last decoded instruction
    auto inst = cpu->instList.back();
    auto staticInst = inst->staticInst;

    if (!staticInst) {
        return false;
    }

    // If the Program Counter matches the target, enable fault injection
    if (PCTarget > 0 && PCTarget == inst->pcState().instAddr()) {
        return true;
    }

    // Defines a map that links possible values of the 'instTarget' parameter to the characteristics of the current instruction
    static const std::unordered_map<std::string, std::function<bool()>> instCheckMap = {
        {"all", []() { return true; }},
        {"nop", [&]() { return staticInst->isNop(); }},
        {"load", [&]() { return staticInst->isLoad(); }},
        {"store", [&]() { return staticInst->isStore(); }},
        {"atomic", [&]() { return staticInst->isAtomic(); }},
        {"store_conditional", [&]() { return staticInst->isStoreConditional(); }},
        {"inst_prefetch", [&]() { return staticInst->isInstPrefetch(); }},
        {"data_prefetch", [&]() { return staticInst->isDataPrefetch(); }},
        {"integer", [&]() { return staticInst->isInteger(); }},
        {"floating", [&]() { return staticInst->isFloating(); }},
        {"vector", [&]() { return staticInst->isVector(); }},
        {"control", [&]() { return staticInst->isControl(); }},
        {"call", [&]() { return staticInst->isCall(); }},
        {"return", [&]() { return staticInst->isReturn(); }},
        {"direct_ctrl", [&]() { return staticInst->isDirectCtrl(); }},
        {"indirect_ctrl", [&]() { return staticInst->isIndirectCtrl(); }},
        {"cond_ctrl", [&]() { return staticInst->isCondCtrl(); }},
        {"uncond_ctrl", [&]() { return staticInst->isUncondCtrl(); }},
        {"serializing", [&]() { return staticInst->isSerializing(); }},
        {"serialize_before", [&]() { return staticInst->isSerializeBefore(); }},
        {"serialize_after", [&]() { return staticInst->isSerializeAfter(); }},
        {"squash_after", [&]() { return staticInst->isSquashAfter(); }},
        {"full_mem_barrier", [&]() { return staticInst->isFullMemBarrier(); }},
        {"read_barrier", [&]() { return staticInst->isReadBarrier(); }},
        {"write_barrier", [&]() { return staticInst->isWriteBarrier(); }},
        {"non_speculative", [&]() { return staticInst->isNonSpeculative(); }},
        {"unverifiable", [&]() { return staticInst->isUnverifiable(); }},
        {"syscall", [&]() { return staticInst->isSyscall(); }},
        {"macroop", [&]() { return staticInst->isMacroop(); }},
        {"microop", [&]() { return staticInst->isMicroop(); }},
        {"delayed_commit", [&]() { return staticInst->isDelayedCommit(); }},
        {"last_microop", [&]() { return staticInst->isLastMicroop(); }},
        {"first_microop", [&]() { return staticInst->isFirstMicroop(); }},
        {"htm_start", [&]() { return staticInst->isHtmStart(); }},
        {"htm_stop", [&]() { return staticInst->isHtmStop(); }},
        {"htm_cancel", [&]() { return staticInst->isHtmCancel(); }},
        {"htm_cmd", [&]() { return staticInst->isHtmCmd(); }}
    };

    // Searches for the current instruction in 'instCheckMap'. If found, fault injection is enabled.
    auto it = instCheckMap.find(instTarget);
    if (it != instCheckMap.end()) {
        return it->second(); // Executes the associated function.
    }

    return false; // Returns false if 'instTarget' is not in the map.
}

} // namespace gem5
