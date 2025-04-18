#include "CHAOSReg/CHAOSReg.hh"
#include "params/CHAOSReg.hh"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <bitset>

#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "arch/generic/isa.hh"

namespace gem5
{
    CHAOSReg::CHAOSReg(const CHAOSRegParams &p)
        : SimObject(p),
        cpu(dynamic_cast<BaseCPU *>(p.cpu)),
        probability(p.probability),
        numBitsToChange(p.numBitsToChange),
        firstClock(p.firstClock),
        lastClock(p.lastClock),
        faultType(p.faultType),
        faultMask(std::bitset<32>(p.faultMask)),
        regTargetClass(p.regTargetClass),
        PCTarget(p.PCTarget),
        tickEvent([this] { this->tick(); }, name())
    {
        if (! (probability == 0.0)){
            if (!cpu) {
                throw std::runtime_error("CHAOSReg: Invalid CPU pointer");
            }

            logStream = simout.create("fault_injections.log", false, true);
            if (!logStream || !logStream->stream()) {
                panic("CHAOSReg: Could not open log file");
            }

            rng.seed(rd());
            inter_fault_cycles_dist = std::geometric_distribution<unsigned>(probability);

            unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
            scheduleTickEvent(Cycles(next_fault_cycle_distance));
        }
    }

    CHAOSReg::~CHAOSReg()
    {}

    gem5::CHAOSReg *
    gem5::CHAOSRegParams::create() const
    {
        return new gem5::CHAOSReg(*this);
    }

    CHAOSReg *CHAOSReg::create(const CHAOSRegParams &p)
    {
        return new CHAOSReg(p);
    }

    void 
    CHAOSReg::scheduleTickEvent(Cycles delay)
    {
        if (!tickEvent.scheduled())
            schedule(tickEvent, cpu->clockEdge(delay));
    }

    void 
    CHAOSReg::unscheduleTickEvent()
    {
        if (tickEvent.scheduled())
            tickEvent.squash();
    }

    int 
    CHAOSReg::generateRandomMask(std::mt19937 &gen, int numBits)
    {
        int mask = 0;
        std::uniform_int_distribution<int> bitDist(0, 31);

        while (numBits--) {
            mask |= (1 << bitDist(gen));
        }
        return mask;
    }

    void 
    CHAOSReg::processFault(ThreadID tid)
    {
        gem5::ThreadContext *threadContext = cpu->getContext(tid);
        if (!threadContext)
            return;
    
        gem5::BaseISA *isa = threadContext->getIsaPtr();
        if (!isa)
            return;
    
        const auto &regClasses = isa->regClasses();
        const gem5::RegClass *regClass = nullptr;
    
        if (regTargetClass == "both") {
            regClass = (rand() % 2 == 0) ? regClasses[gem5::IntRegClass] : regClasses[gem5::FloatRegClass];
        } else if (regTargetClass == "integer") {
            regClass = regClasses[gem5::IntRegClass];
        } else if (regTargetClass == "floating_point") {
            regClass = regClasses[gem5::FloatRegClass];
        }
    
        if (!regClass || regClass->numRegs() == 0)
            return;
    
        int randomReg = std::uniform_int_distribution<>(0, regClass->numRegs() - 1)(rng);
        int mask = faultMask.any() ? faultMask.to_ulong() : generateRandomMask(rng, numBitsToChange);
        gem5::RegId regId(*regClass, randomReg);
    
        try {
            gem5::RegVal regVal = threadContext->getReg(regId);
    
            std::string chosenFaultType = faultType;
            if (faultType == "random") {
                static std::uniform_int_distribution<int> dis(0, 2);
                const char *faultTypes[] = {"bit_flip", "stuck_at_zero", "stuck_at_one"};
                chosenFaultType = faultTypes[dis(rng)];
            }
    
            if (chosenFaultType == "stuck_at_zero") {
                regVal &= ~mask;
            } else if (chosenFaultType == "stuck_at_one") {
                regVal |= mask;
            } else if (chosenFaultType == "bit_flip") {
                regVal ^= mask;
            }
    
            threadContext->setReg(regId, regVal);
    
            *(logStream->stream())  << "Cycle: " << cpu->curCycle()
                    << ", CPU: " << cpu->name()
                    << ", Thread: " << tid
                    << ", Register: " << regClass->name() << "[" << randomReg << "]"
                    << ", FaultType: " << chosenFaultType
                    << ", Mask: " << std::bitset<32>(mask)
                    << std::endl;
        } catch (const std::exception &e) {
            *(logStream->stream())  << "Error: Exception during fault injection. "
                    << "ThreadID: " << tid
                    << ", Error: " << e.what() << std::endl;
        } catch (...) {
            *(logStream->stream())  << "Error: Unknown exception during fault injection. "
                    << "ThreadID: " << tid << std::endl;
        }
    }

    void 
    CHAOSReg::tick()
    {
        if (!probability)
            return;

        bool allThreadsHalted = true;
        for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
            ThreadContext *threadContext = cpu->getContext(tid);

            if (threadContext && threadContext->status() != ThreadContext::Halted) {
                allThreadsHalted = false;
            }
        }

        if (allThreadsHalted || (lastClock != -1 && cpu->curCycle() > lastClock)) {
            unscheduleTickEvent();
            return;
        }

        unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
        scheduleTickEvent(Cycles(next_fault_cycle_distance));

        if (cpu->curCycle() < firstClock)
            return;

        if (checkInst()) {
            for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
                processFault(tid);
            }
        }
    }

    bool 
    CHAOSReg::checkInst()
    {
        for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
            ThreadContext *threadContext = cpu->getContext(tid);

            if (!threadContext)
                continue;

            Addr pc = threadContext->pcState().instAddr();

            if (PCTarget > 0 && PCTarget == pc)
                return true;

            return true;
        }

        return false;
    }

} // namespace gem5