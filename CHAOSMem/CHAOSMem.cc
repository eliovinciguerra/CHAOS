#include "mem/CHAOSMem/CHAOSMem.hh"
#include "params/CHAOSMem.hh"

#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <bitset>
#include <unordered_map>
#include <functional>
#include <string>     

#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include "base/random.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5 {
    CHAOSMem::CHAOSMem(const CHAOSMemParams& params)
    : SimObject(params), probability(params.probability), bitsToChange(params.bitsToChange), 
    firstClock(params.firstClock), lastClock(params.lastClock), tickToClockRatio(params.tickToClockRatio), 
    faultType(params.faultType), faultMask(static_cast<unsigned char>(std::stoi(params.faultMask, nullptr, 2))),
    attackEvent([this]{ this->attackMemory(); }, name()) 
    {
        if (! (probability == 0.0)){
            if (params.mem) {
                memory = params.mem;
            }

            // Opens the log file to save details about the injected faults
            logFile.open("main_mem_injections.log", std::ios::out);
            if (!logFile.is_open()) {
                throw std::runtime_error("CHAOSMem: Could not open log file for writing");
            }

            // Initializes the random number generator
            auto seed = rd();
            rng.seed(seed);
            inter_fault_tick_dist = std::geometric_distribution<unsigned>(probability);

            // Schedules the first fault
            scheduleAttack(curTick() + inter_fault_tick_dist(rng) * tickToClockRatio);
        }
    }

    CHAOSMem::~CHAOSMem() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    /**
    * Generates a random mask to modify a specified number of bits.
    * 
    * @param gen Random number generator.
    * @param numBits Number of bits to modify.
    * @return The generated mask.
    */
    unsigned char 
    CHAOSMem::generateRandomMask(std::mt19937 &rng, int bitsToChange)
    {
        unsigned char mask = 0;
        std::uniform_int_distribution<int> bitDist(0, 7);
    
        for (int i = 0; i < bitsToChange; i++) {
            mask |= (1 << bitDist(rng));
        }
        return mask;
    }

    void CHAOSMem::scheduleAttack(Tick time) {
        if (!attackEvent.scheduled()) {
            schedule(attackEvent, time);
        }
    }

    void CHAOSMem::attackMemory() {
        int cycle = curTick() / tickToClockRatio;
        if (!(cycle >= firstClock && (cycle <= lastClock || lastClock == -1))) {
            return;
        }
        
        if (!memory) {
            warn("CHAOSMem: Memory not available.\n");
            return;
        }

        Addr mem_start = memory->getAddrRange().start();
        Addr mem_size = memory->getAddrRange().size();
        Addr target_addr = mem_start + (random() % mem_size);

        uint8_t data;
        RequestPtr req = std::make_shared<Request>(target_addr, sizeof(uint8_t), 0, 0);
        PacketPtr read_pkt = new Packet(req, MemCmd::ReadReq);
        read_pkt->dataStatic(&data);

        memory->access(read_pkt);

        unsigned char mask = (faultMask != 0) ? faultMask : generateRandomMask(rng, bitsToChange);

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
            data &= ~mask;
        } else if (chosenFaultType == "stuck_at_one") {
            data |= mask;
        } else if (chosenFaultType == "bit_flip") {
            data ^= mask;
        }

        PacketPtr write_pkt = new Packet(req, MemCmd::WriteReq);
        write_pkt->dataStatic(&data);

        memory->access(write_pkt);

        delete read_pkt;
        delete write_pkt;

        // Logs the operations just performed in the log file
        logFile << "Tick: " << curTick() 
        << ", target addr: " << target_addr
        << ", Mask: " << std::bitset<8>(mask)
        << ", Fault Type: " << chosenFaultType
        << std::dec << std::endl;

        logFile.flush();

        // Schedules the next fault
        scheduleAttack(curTick() + inter_fault_tick_dist(rng) * tickToClockRatio);
    }

} // namespace gem5