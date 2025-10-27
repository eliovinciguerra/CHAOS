#include "mem/CHAOSMem/CHAOSMem.hh"
#include "params/CHAOSMem.hh"

#include <fstream>
#include <random>
#include <bitset>
#include <functional>
#include <string>     

#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5 {

    CHAOSMem::CHAOSMem(const CHAOSMemParams& p)
    : SimObject(p), 
    probability(p.probability), 
    num_bits_to_change(p.bitsToChange),
    first_clock(p.firstClock), 
    last_clock(p.lastClock),
    fault_type_enum(stringToFaultType(p.faultType)),
    fault_mask(static_cast<unsigned char>(std::stoi(p.faultMask, nullptr, 2))), 
    tick_to_clock_ratio(p.tickToClockRatio), 
    bit_flip_prob(p.bitFlipProb),
    stuck_at_zero_prob(p.stuckAtZeroProb),
    stuck_at_one_prob(p.stuckAtOneProb),
    cycles_permament_fault_check(p.cyclesPermamentFaultCheck),
    write_log(p.writeLog),
    target_start(p.addr_start), 
    target_end(p.addr_end),
    attackEvent([this]{ this->attackMemory(); }, name()),
    periodicCheck([this] { this->checkPermanent(); }, name() + ".periodicCheck"),
    stats(nullptr)
    {
        if (probability > 0.0) {
            if (p.mem) {
                memory = p.mem;
            }

            if (!memory) {
                warn("CHAOSMem: Memory not available. Disabling fault injection.\n");
                return;
            }
            
            log_stream = simout.create("main_mem_injections.log", false, true);
            if (!log_stream || !log_stream->stream()) {
                panic("CHAOSMem: Could not open log file");
            }
            
            if (num_bits_to_change == -1){
                std::uniform_int_distribution<int> dist(1, 8);
                num_bits_to_change = dist(rng);
            }

            Addr mem_start = memory->getAddrRange().start();
            Addr mem_size = memory->getAddrRange().size();
            
            if (target_start < mem_start) {
                target_start = mem_start;
                warn("CHAOSMem: target_start adjusted to memory start\n");
            }
            
            if (target_end == 0 || target_end < target_start) {
                target_end = mem_start + mem_size - 1;
                warn("CHAOSMem: target_end set to memory end\n");
            }

            stats = std::make_unique<CHAOSMemStats>(this);

            target_size = target_end - target_start + 1;

            first_tick = first_clock * tick_to_clock_ratio;
            last_tick = last_clock * tick_to_clock_ratio;

            ticks_permament_fault_check = cycles_permament_fault_check * tick_to_clock_ratio;

            rng.seed(rd());
            inter_fault_tick_dist = std::geometric_distribution<unsigned>(probability);
            
            scheduleAttack(first_tick + inter_fault_tick_dist(rng) * tick_to_clock_ratio);

            if ((bit_flip_prob + stuck_at_zero_prob + stuck_at_one_prob) != 1.0){
                warn("Sum of probabilities is not 1, assuming 0.9 for bitFlipProb, 0.05 for stuckAtZeroProb and 0.05 for stuckAtOneProb.\n");
                bit_flip_prob = 0.9;
                stuck_at_zero_prob = 0.05;
                stuck_at_one_prob = 0.05;
            }

            std::vector<double> weights = {bit_flip_prob, bit_flip_prob, stuck_at_one_prob};
            random_fault_distribution = std::discrete_distribution<int>(weights.begin(), weights.end());

            scheduleCheckPermanentFault(first_tick + ticks_permament_fault_check);
        }
    }

    CHAOSMem::CHAOSMemStats::CHAOSMemStats(statistics::Group *parent)
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

    CHAOSMem::~CHAOSMem() {}

    CHAOSMem::FaultType 
    CHAOSMem::stringToFaultType(const std::string &s) {
        if (s == "bit_flip") return FaultType::BitFlip;
        else if (s == "stuck_at_zero") return FaultType::StuckAtZero;
        else if (s == "stuck_at_one") return FaultType::StuckAtOne;
        return FaultType::Random;
    }

    const char* 
    CHAOSMem::faultTypeToString(CHAOSMem::FaultType f) {
        switch (f) {
            case FaultType::BitFlip: return "bit_flip";
            case FaultType::StuckAtZero: return "stuck_at_zero";
            case FaultType::StuckAtOne: return "stuck_at_one";
        }
        return "random";
    }

    void 
    CHAOSMem::scheduleAttack(Tick time) {
        if (!attackEvent.scheduled()) {
            schedule(attackEvent, time);
        }
    }

    void 
    CHAOSMem::scheduleCheckPermanentFault(Tick time)
    {
        if (!periodicCheck.scheduled()) {
            schedule(periodicCheck, time);
        }
    }

    unsigned char 
    CHAOSMem::generateRandomMask(std::mt19937 &rng, int bits_to_change, int len)
    {
        unsigned char mask = 0;
        std::uniform_int_distribution<int> bitDist(0, len-1);
    
        for (int i = 0; i < bits_to_change; i++) {
            mask |= (1 << bitDist(rng));
        }
        return mask;
    }

    void 
    CHAOSMem::attackMemory() {
        if (!memory) {
            warn("CHAOSMem: Memory not available.\n");
            scheduleAttack(curTick() + inter_fault_tick_dist(rng) * tick_to_clock_ratio);
            return;
        }

        std::uniform_int_distribution<Addr> dist(target_start, target_end - 1);
        Addr target_addr = dist(rng);

        try {
            // Attack a single byte
            uint8_t data;
            RequestPtr req = std::make_shared<Request>(target_addr, sizeof(data), 0, 0);
            PacketPtr read_pkt = new Packet(req, MemCmd::ReadReq);
            read_pkt->dataStatic(&data);

            memory->access(read_pkt);

            unsigned char mask = (fault_mask != 0) ? fault_mask : generateRandomMask(rng, num_bits_to_change, sizeof(data) << 3);

            FaultType chosen_fault_type_enum = fault_type_enum;
            if (fault_type_enum == FaultType::Random) {
                int faultIdx = random_fault_distribution(rng);
                chosen_fault_type_enum = static_cast<FaultType>(faultIdx);
            }

            switch (chosen_fault_type_enum) {
                case FaultType::StuckAtZero:
                    data &= ~mask;
                    stats->numStuckAtZero++;
                    stats->numPermanentFaults++;
                    permanent_faults[target_addr] = {chosen_fault_type_enum, mask, true};
                    break;
                case FaultType::StuckAtOne:
                    data |= mask;
                    stats->numStuckAtOne++;
                    stats->numPermanentFaults++;
                    permanent_faults[target_addr] = {chosen_fault_type_enum, mask, true};
                    break;
                case FaultType::BitFlip:
                    data ^= mask;
                    stats->numBitFlips++;
                    break;
                default:
                    break;
            }

            PacketPtr write_pkt = new Packet(req, MemCmd::WriteReq);
            write_pkt->dataStatic(&data);

            memory->access(write_pkt);
            stats->numFaultsInjected++;

            delete read_pkt;
            delete write_pkt;

            if (write_log){
                *(log_stream->stream()) << "Tick: " << curTick() 
                    << ", target addr: " << target_addr
                    << ", Mask: " << std::bitset<8>(mask)
                    << ", Fault Type: " << faultTypeToString(chosen_fault_type_enum)
                    << std::dec << std::endl;
            }

        } catch (const std::exception &e) {
            *(log_stream->stream())  << "Error: Exception during fault injection. "
                    << "Target Addr: " << target_addr
                    << ", Error: " << e.what() << std::endl;
        } catch (...) {
            *(log_stream->stream())  << "Error: Unknown exception during fault injection. "
                    << "Target Addr: " << target_addr << std::endl;
        }
        
        Tick next_injection = curTick() + inter_fault_tick_dist(rng) * tick_to_clock_ratio;

        if (next_injection <= last_tick || last_tick == 0) {
            scheduleAttack(next_injection);
        }
    }

    void CHAOSMem::checkPermanent()
    {
        for (auto &entry : permanent_faults) {
            if (!entry.second.update)
                continue;

            Addr target_addr = entry.first;
            const PermanentFault &fault = entry.second;

            try {
                uint8_t data;
                RequestPtr req = std::make_shared<Request>(target_addr, sizeof(uint8_t), 0, 0);
                PacketPtr read_pkt = new Packet(req, MemCmd::ReadReq);
                read_pkt->dataStatic(&data);

                memory->access(read_pkt);

                switch (fault.fault_type) {
                    case FaultType::StuckAtZero:
                        data &= ~fault.mask;
                        break;
                    case FaultType::StuckAtOne:
                        data |= fault.mask;
                        break;
                    default:
                        break;
                }

                PacketPtr write_pkt = new Packet(req, MemCmd::WriteReq);
                write_pkt->dataStatic(&data);

                memory->access(write_pkt);
                entry.second.update = false;

                delete read_pkt;
                delete write_pkt;
            } catch (const std::exception &e) {
                *(log_stream->stream())  << "Error: Exception during fault injection. "
                        << "Target Addr: " << target_addr
                        << ", Error: " << e.what() << std::endl;
            } catch (...) {
                *(log_stream->stream())  << "Error: Unknown exception during fault injection. "
                        << "Target Addr: " << target_addr << std::endl;
            }
        }
        scheduleCheckPermanentFault(curTick() + ticks_permament_fault_check);
    }

} // namespace gem5