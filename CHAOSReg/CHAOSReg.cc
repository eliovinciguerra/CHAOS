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

namespace gem5{

    CHAOSReg::CHAOSReg(const CHAOSRegParams &p)
        : SimObject(p),
        cpu(dynamic_cast<BaseCPU *>(p.cpu)),
        probability(p.probability),
        num_bits_to_change(p.bitsToChange),
        first_clock(Cycles(p.firstClock)),
        last_clock(Cycles(p.lastClock)),
        fault_type_enum(stringToFaultType(p.faultType)),
        fault_mask(std::bitset<32>(p.faultMask)),
        bit_flip_prob(p.bitFlipProb),
        stuck_at_zero_prob(p.stuckAtZeroProb),
        stuck_at_one_prob(p.stuckAtOneProb),
        cycles_permament_fault_check(Cycles(p.cyclesPermamentFaultCheck)),
        reg_target_class_enum(stringToTargetClass(p.regTargetClass)),
        PC_target(p.PCTarget),
        write_log(p.writeLog),
        attackEvent([this] { this->attackCheck(); }, name()),
        periodicCheck([this] { this->checkPermanent(); }, name() + ".periodicCheck"),
        stats(nullptr)
    {
        if (probability > 0.0){
            if (!cpu) {
                throw std::runtime_error("CHAOSReg: Invalid CPU pointer.\n");
            }

            log_stream = simout.create("fault_injections.log", false, true);
            if (!log_stream || !log_stream->stream()) {
                panic("CHAOSReg: Could not open log file");
            }

            stats = std::make_unique<CHAOSRegStats>(this);

            rng.seed(rd());
            if (PC_target != 0){
                // In this case, it is necessary to check at every cycle if the PC matches, so it is better to set the probability to 1.
                probability = 1;
            }

            if (num_bits_to_change == -1){
                std::uniform_int_distribution<int> dist(1, 32);
                num_bits_to_change = dist(rng);
            }

            inter_fault_cycles_dist = std::geometric_distribution<unsigned>(probability);
            
            unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
            scheduleAttackEvent(first_clock + Cycles(next_fault_cycle_distance));
            
            if ((bit_flip_prob + stuck_at_zero_prob + stuck_at_one_prob) != 1.0){
                warn("Sum of probabilities is not 1, assuming 0.9 for bitFlipProb, 0.05 for stuckAtZeroProb and 0.05 for stuckAtOneProb.\n");
                bit_flip_prob = 0.9;
                stuck_at_zero_prob = 0.05;
                stuck_at_one_prob = 0.05;
            }

            std::vector<double> weights = {bit_flip_prob, stuck_at_zero_prob, stuck_at_one_prob};
            random_fault_distribution = std::discrete_distribution<int>(weights.begin(), weights.end());

            scheduleCheckPermanentFault(first_clock + cycles_permament_fault_check);
        }
    }

    CHAOSReg::CHAOSRegStats::CHAOSRegStats(statistics::Group *parent)
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

    CHAOSReg::~CHAOSReg(){}

    CHAOSReg::FaultType 
    CHAOSReg::stringToFaultType(const std::string &s) {
        if (s == "bit_flip") return FaultType::BitFlip;
        else if (s == "stuck_at_zero") return FaultType::StuckAtZero;
        else if (s == "stuck_at_one") return FaultType::StuckAtOne;
        return FaultType::Random;
    }

    const char* 
    CHAOSReg::faultTypeToString(CHAOSReg::FaultType f) {
        switch (f) {
            case FaultType::BitFlip: return "bit_flip";
            case FaultType::StuckAtZero: return "stuck_at_zero";
            case FaultType::StuckAtOne: return "stuck_at_one";
        }
        return "random";
    }
    
    CHAOSReg::TargetClass 
    CHAOSReg::stringToTargetClass(const std::string &s) {
        if (s == "integer") return TargetClass::Integer;
        else if (s == "floating_point") return TargetClass::FloatingPoint;
        return TargetClass::Both;
    }

    void 
    CHAOSReg::scheduleAttackEvent(Cycles delay)
    {
        if (!attackEvent.scheduled())
            schedule(attackEvent, cpu->clockEdge(delay));
    }

    void 
    CHAOSReg::scheduleCheckPermanentFault(Cycles delay)
    {
        if (!periodicCheck.scheduled())
            schedule(periodicCheck, cpu->clockEdge(delay));
    }

    void 
    CHAOSReg::unscheduleAttackEvent()
    {
        if (attackEvent.scheduled())
            attackEvent.squash();

        if (periodicCheck.scheduled())
            periodicCheck.squash();
    }

    int 
    CHAOSReg::generateRandomMask(std::mt19937 &gen, int bits_to_change, int len)
    {
        int mask = 0;
        std::uniform_int_distribution<int> bitDist(0, len-1);

        while (bits_to_change-- > 0) {
            mask |= (1 << bitDist(gen));
        }
        return mask;
    }

    void 
    CHAOSReg::processFault(ThreadID tid)
    {
        gem5::ThreadContext *thread_context = cpu->getContext(tid);
        if (!thread_context)
            return;
    
        gem5::BaseISA *isa = thread_context->getIsaPtr();
        if (!isa)
            return;
    
        const auto &reg_classes = isa->regClasses();
        const gem5::RegClass *reg_class = nullptr;
    
        if (reg_target_class_enum == TargetClass::Both) {
            int intRegs = reg_classes[gem5::IntRegClass]->numRegs();
            int floatRegs = reg_classes[gem5::FloatRegClass]->numRegs();

            if (intRegs == 0 && floatRegs == 0) {
                warn("processFault: No registers found\n");
                return;
            } 
            if (intRegs > 0 && floatRegs == 0) {
                reg_class = reg_classes[gem5::IntRegClass];
            } else if (intRegs == 0 && floatRegs > 0) {
                reg_class = reg_classes[gem5::FloatRegClass];
            } else {
                reg_class = (rand() % 2 == 0) ? reg_classes[gem5::IntRegClass] : reg_classes[gem5::FloatRegClass];
            }
        } else if (reg_target_class_enum == TargetClass::Integer) {
            reg_class = reg_classes[gem5::IntRegClass];
            if (reg_class->numRegs() == 0) return;
        } else if (reg_target_class_enum == TargetClass::FloatingPoint) {
            reg_class = reg_classes[gem5::FloatRegClass];
            if (reg_class->numRegs() == 0) return;
        }
    
        if (!reg_class || reg_class->numRegs() == 0)
            return;
    
        int random_reg = std::uniform_int_distribution<>(0, reg_class->numRegs() - 1)(rng);
        gem5::RegId reg_id(*reg_class, random_reg);
        
        try {
            gem5::RegVal reg_val = thread_context->getReg(reg_id);

            int mask = fault_mask.any() ? fault_mask.to_ulong() : generateRandomMask(rng, num_bits_to_change, sizeof(reg_val) << 3);
    
            FaultType chosen_fault_type_enum = fault_type_enum;
            if (fault_type_enum == FaultType::Random) {
                int faultIdx = random_fault_distribution(rng);
                chosen_fault_type_enum = static_cast<FaultType>(faultIdx);
            }
    
            switch (chosen_fault_type_enum) {
                case FaultType::StuckAtZero:
                    reg_val &= ~mask;
                    stats->numStuckAtZero++;
                    stats->numPermanentFaults++;
                    permanent_faults[std::make_pair(tid, reg_id)] = {chosen_fault_type_enum, mask, true};
                    break;
                case FaultType::StuckAtOne:
                    reg_val |= mask;
                    stats->numStuckAtOne++;
                    stats->numPermanentFaults++;
                    permanent_faults[std::make_pair(tid, reg_id)] = {chosen_fault_type_enum, mask, true};
                    break;
                case FaultType::BitFlip:
                    reg_val ^= mask;
                    stats->numBitFlips++;
                    break;
                default:
                    break;
            }
    
            thread_context->setReg(reg_id, reg_val);
            stats->numFaultsInjected++;
            
            if (write_log){
                *(log_stream->stream())  << "Cycle: " << cpu->curCycle()
                    << ", CPU: " << cpu->name()
                    << ", Thread: " << tid
                    << ", Register: " << reg_class->name() << "[" << random_reg << "]"
                    << ", FaultType: " << faultTypeToString(chosen_fault_type_enum)
                    << ", Mask: " << std::bitset<32>(mask)
                    << std::endl;
            }

        } catch (const std::exception &e) {
            *(log_stream->stream())  << "Error: Exception during fault injection. "
                    << "ThreadID: " << tid
                    << ", Error: " << e.what() << std::endl;
        } catch (...) {
            *(log_stream->stream())  << "Error: Unknown exception during fault injection. "
                    << "ThreadID: " << tid << std::endl;
        }
    }

    void 
    CHAOSReg::attackCheck()
    {
        if (!probability)
            return;

        for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
            ThreadContext *thread_context = cpu->getContext(tid);
            if (!thread_context || thread_context->status() == ThreadContext::Halted) {
                continue;
            }

            if (PC_target == 0 || (PC_target == thread_context->pcState().instAddr())) {
                processFault(tid);
            }
        }

        bool any_active = false;
        for (ThreadID tid = 0; tid < cpu->numThreads; ++tid) {
            ThreadContext *thread_context = cpu->getContext(tid);
            if (thread_context && thread_context->status() != ThreadContext::Halted) {
                any_active = true;
                break;
            }
        }
        if (any_active) {
            Cycles next_injection = Cycles(inter_fault_cycles_dist(rng));
            if (last_clock == 0 || (next_injection + cpu->curCycle()) <= last_clock){
                scheduleAttackEvent(next_injection);
            }
        } else {
            unscheduleAttackEvent();
        }
    }

    void 
    CHAOSReg::checkPermanent()
    {
        for (auto &entry : permanent_faults) {
            if (!entry.second.update)
                continue;

            ThreadID tid = entry.first.first;
            gem5::RegId reg_id = entry.first.second;
            const PermanentFault &fault = entry.second;

            try {
                gem5::ThreadContext *thread_context = cpu->getContext(tid);
                if (!thread_context)
                    continue;

                gem5::RegVal reg_val = thread_context->getReg(reg_id);

                switch (fault.fault_type) {
                    case FaultType::StuckAtZero:
                        reg_val &= ~fault.mask;
                        break;
                    case FaultType::StuckAtOne:
                        reg_val |= fault.mask;
                        break;
                    default:
                        break;
                }

                thread_context->setReg(reg_id, reg_val);
                entry.second.update = false;
            } catch (const std::exception &e) {
                *(log_stream->stream())  << "Error: Exception during fault injection. "
                        << "ThreadID: " << tid
                        << ", Error: " << e.what() << std::endl;
            } catch (...) {
                *(log_stream->stream())  << "Error: Unknown exception during fault injection. "
                        << "ThreadID: " << tid << std::endl;
            }

            scheduleCheckPermanentFault(cycles_permament_fault_check);
        }
    }
} // namespace gem5